#include "bsp.h"
#include "app_cfg.h"
#include "htcluster.h"

// -------------------------------------------------------------------------------
// The byte of state
// -------------------------------------------------------------------------------
// local state
uint16_t  Ecu_localState           =0x0000;     // =0;
uint16_t  Ecu_motionState          =0x00fc;    // =0;

BlinkPair Ecu_relays[4];

// -------------------------------------------------------------------------------
// Configurations
// -------------------------------------------------------------------------------
#define CONST(_DEBUGVAL, _NORMALVAL) ((dsf_Debug & Ecu_localState) ? (_DEBUGVAL)  : (_NORMALVAL)) 

// #define DEFAULT_Timeout_Pump      CONST(   10, 3*60)  // 3min
uint8_t   CONFIG_temperatureMask  =0xff;
uint8_t   CONFIG_temperatureIntv  =20;	// 20sec in sec

uint16_t  CONFIG_motionDiffMask   = 0xff;
uint8_t   CONFIG_motionDiffIntv   = 20;    // 2sec in 100msec
uint16_t  CONFIG_motionOuterMask  = 0xf0;  // mask of out-doors
uint16_t  CONFIG_motionBedMask    = 0x02;  // mask of bedrooms

uint8_t   CONFIG_lumineDiffMask   =0xff;
uint8_t   CONFIG_lumineDiffIntv   =10;	 // 10sec
uint16_t  CONFIG_lumineDiffTshd;  


// -------------------------------------------------------------------------------
// Runtime varaibles
// -------------------------------------------------------------------------------
int16_t  temperatures[CHANNEL_SZ_DS18B20]={0,0,};
// lumine ADC values
uint16_t ADC_DMAbuf[(1+ CHANNEL_SZ_ADC)*2];

// uint16_t Ecu_localState =0;

// -------------------------------------------------------------------------------
// Dictionary expose for HtNode
// -------------------------------------------------------------------------------
enum {
	ODID_lstatus     = ODID_USER_MIN,      // the Ecu_localState
	ODID_mstatus,      // the Ecu_motionState
	ODID_temps,  ODID_lumins, ODID_adc,

	// configurations
	ODID_CFGtMask, ODID_CFGtIntv,
	ODID_CFGmMask, ODID_CFGmIntv, ODID_CFGmOuter, ODID_CFGmBR,
	ODID_CFGlMask, ODID_CFGlIntv, ODID_CFGlDiff,

};

// OD declaration:
USR_OD_DECLARE_START()
  OD_DECLARE(lstatus,     ODTID_BYTES, ODF_Readable, &Ecu_localState,  sizeof(Ecu_localState))
  OD_DECLARE(mstatus,     ODTID_BYTES, ODF_Readable, &Ecu_motionState, sizeof(Ecu_motionState))
  OD_DECLARE(temps,       ODTID_BYTES, ODF_Readable, temperatures,     sizeof(temperatures))
  OD_DECLARE(lumins,      ODTID_BYTES, ODF_Readable, &ADC_DMAbuf[1],   sizeof(ADC_DMAbuf)-sizeof(uint16_t))
  OD_DECLARE(adc,         ODTID_BYTES, ODF_Readable, ADC_DMAbuf,       sizeof(ADC_DMAbuf))

  // configuations
  OD_DECLARE(CFGtMask,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_temperatureMask,  sizeof(CONFIG_temperatureMask))
  OD_DECLARE(CFGtIntv,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_temperatureIntv,  sizeof(CONFIG_temperatureIntv))
  OD_DECLARE(CFGmMask,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionDiffMask,   sizeof(CONFIG_motionDiffMask))
  OD_DECLARE(CFGmIntv,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionDiffIntv,   sizeof(CONFIG_motionDiffIntv))
  OD_DECLARE(CFGmOuter,   ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionOuterMask,  sizeof(CONFIG_motionOuterMask))
  OD_DECLARE(CFGmBR,      ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionBedMask,    sizeof(CONFIG_motionBedMask))
  OD_DECLARE(CFGlMask,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffMask,   sizeof(CONFIG_lumineDiffMask))
  OD_DECLARE(CFGlIntv,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffIntv,   sizeof(CONFIG_lumineDiffIntv))
  OD_DECLARE(CFGlDiff,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffTshd,   sizeof(CONFIG_lumineDiffTshd))

USR_OD_DECLARE_END()

// -------------------------------------------------------------------------------
// Definition of HtNode Events
// -------------------------------------------------------------------------------
// TODO
enum {
	ODEvent_StateChanged = ODEvent_USER_MIN, 
};
//
static const uint8_t eventOD_StateChanged[] = { ODID_lstatus, ODID_mstatus, ODID_NONE};
USR_ODEVENT_DECLARE_START()
 	ODEVENT_DECLARE(StateChanged, eventOD_StateChanged)
USR_ODEVENT_DECLARE_END()

// -------------------------------------------------------------------------------
// SECU_init()
// -------------------------------------------------------------------------------
void SECU_init(bool debugMode)
{
	if (debugMode)
		Ecu_localState |= dsf_Debug; 
}

// -------------------------------------------------------------------------------
// SECU_scanDevice()
// -------------------------------------------------------------------------------
static uint16_t	lastMotionState = 0;
static uint16_t lastLocalState =0;
static uint16_t LuminBaks[CHANNEL_SZ_ADC];
void SECU_scanDevice(void)
{
	static uint32_t stampLastTemp =0, stampLastMotion=0, stampLastLumin=0;
	uint16_t tmpU16=0, flag=0, i;

	// collecting informations and determine the flags of Ecu_localState
	// ----------------------------
	
	// part 1. motion status
	if (CONFIG_motionDiffIntv >0 && gClock_getElapsed(stampLastMotion) > (CONFIG_motionDiffIntv*100)) // config in 100msec
	{
		stampLastMotion = gClock_msec;
		Ecu_motionState = MotionState();

		if ((CONFIG_motionDiffMask & Ecu_motionState) != (CONFIG_motionDiffMask & lastMotionState))
		{
			// the motion state was just changed
			Ecu_localState |= dsf_MotionChanged;  // the per-state dispatch should issue TPDO1 by this flag
		}
		else Ecu_localState &= ~dsf_MotionChanged;

		lastMotionState = Ecu_motionState;
	}

	// part 2. reading the temperatures
	if (CONFIG_temperatureIntv >0 && gClock_getElapsed(stampLastTemp) > (CONFIG_temperatureIntv*1000)) // config in 1sec
	{
		stampLastTemp = gClock_msec;

		flag = (1<<0);
		for (i=0; i < min(CHANNEL_SZ_DS18B20, 8); i++, flag<<=1)
		{
			if (0 == (CONFIG_temperatureMask & flag))
			{
				temperatures[i] = DS18B20_INVALID_VALUE;
				continue;
			}

			temperatures[i] = DS18B20_read(&ds18b20s[i]);
		}
	}

	// step 3. about the lumin ADC values that have been DMA to Ecu_lumine[0]~7
	if (CONFIG_lumineDiffIntv >0 && gClock_getElapsed(stampLastLumin) > (CONFIG_lumineDiffIntv*1000)) // config in 1sec
	{
		stampLastLumin = gClock_msec;

		flag = (1<<0);
		tmpU16 =0;
		for (i=0; CONFIG_lumineDiffTshd >0 && i < min(CHANNEL_SZ_LUMIN, 8); i++, flag<<=1)
		{
			if ((CONFIG_lumineDiffMask & flag) && abs(LuminBaks[i] - ADC_DMAbuf[i+1]) >= CONFIG_lumineDiffTshd)
				tmpU16 |= flag;

			LuminBaks[i] = ADC_DMAbuf[i+1];
		}

		if (0 != tmpU16)
			Ecu_localState |= dsf_LuminChanged; // the per-state dispatch should issue TPDO1 by this flag
		else Ecu_localState &= ~dsf_LuminChanged;
	}


	// step End. trigger the event(s)
	if (lastLocalState != Ecu_localState)
		HtNode_triggerEvent(ODEvent_StateChanged);

	lastLocalState = Ecu_localState;
}

// -------------------------------------------------------------------------------
// SECU_doMainLoop()
// -------------------------------------------------------------------------------
// the main loop
void SECU_doMainLoop(void)
{
	uint16_t nextSleep =0;

	SECU_init(1);

	// dispatch on states and monitoring
	while (1)
	{
		sleep(1);

		// timer-ed in milliseconds
		// ---------------------------------------
		HtNode_do1msecScan();

		// exec the water loop
		// ---------------------------------------
		if (nextSleep--)
			continue;

		// nextSleep = NEXT_SLEEP_MIN;  // initialize with a maximal sleep time
		// scan and update the device status
		SECU_scanDevice();
		// ADJUST_BY_RANGE(nextSleep, NEXT_SLEEP_MIN, NEXT_SLEEP_MAX);
		nextSleep = NEXT_SLEEP_DEFAULT;
	}
}

#if 0
void NetIf_transmitPacket(void* netIf, pbuf* packet)
{
	if (NULL == netIf|| NULL == packet)
		return;

	for (;packet; packet=packet->next)
	{
		if (netIf == &COM1)
		{
			USART_transmit(&COM1, packet->payload, packet->len);
			continue;
		}

		if (netIf == &si4432)
		{
			SI4432_transmit(&si4432, packet);
			continue;
		}
	}
}

void TextMsg_OnRequest(void* netIf, uint16_t cseq, uint8_t* msg, uint16_t len)
{
	char obuf[64], *p=obuf;
	pbuf* pResp;
	uint8_t i;
	uint32_t w;
	do {
		// CMD>> query status: ws=?
		//          resp: ws=<statusByte>,<timerPump>,<timerValve2to1>,<timerYield>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(ws=?))) // list interfaces
		{
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "ws=%02x,p%d,v%d,y%d", byteStatus, timerPump, timerValve2to1, timerYield);
			break;
		}

		// CMD>> user force to state: ws=<statusByte>
		//          resp: ws=<statusByte>,<timerPump>,<timerValve2to1>,<timerYield>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(pt=)))
		{
			if (hex2byte((const char*)msg+3, &i)<2)
				return; // invalid
			byteStatus = i;
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "ws=%x,p%d,v%d,y%d\n", byteStatus, timerPump, timerValve2to1, timerYield);
			break;
		}

		// CMD>> temperature status: ts=?
		//          resp: ts=<temp1>,<temp2>,<temp3>...
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(ts=?))) // list interfaces
		{
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "ts=");
			for (i =0 ; i< cTemperatures; i++)
			{
				p += snprintf(p, obuf + sizeof(obuf) -p -2, "%d,", temperatures[i]);
			}
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "\n");
			break;
		}

		// CMD>> user request to pump: pt=<sec>[,0|1]
		//          resp: pt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(pt=)))
		{
			w = atoi((const char*)msg+3);
			if (w > CONFIG_TimeoutPump *10)
				w = CONFIG_TimeoutPump *10;
			if (strstr((const char*) msg+3, ",1"))
			{
				// full loop
				timerValve2to1 = CONFIG_TimeoutValve2to1;
				byteStatus |= FLAG_FullLoop;
			}

			timerPump = w;
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "pt=%d\n", timerPump);
			break;
		}

		// CMD>> user config pump timeout: cpt=<sec>
		//          resp: cpt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cpt=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cpt=));
			ADJUST_BY_RANGE(w, MIN_Timeout_Pump, MAX_Timeout_Pump);
			CONFIG_TimeoutPump = w;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cpt=%d\n", CONFIG_TimeoutPump);
			break;
		}

		// CMD>> user config valve timeout: cvt=<sec>
		//          resp: cvt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cvt=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cvt=));
			ADJUST_BY_RANGE(w, 0, MAX_Timeout_Valve2to1);

			// treat to disable valve if input is less than CONFIG_TimeoutValve2to1
			if (w > MIN_Timeout_Valve2to1)
				CONFIG_TimeoutValve2to1 = w;
			else CONFIG_TimeoutValve2to1 = 0;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cvt=%d\n", CONFIG_TimeoutValve2to1);
			break;
		}

		// CMD>> user config valve timeout: cyt=<sec>
		//          resp: cyt=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cyt=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cyt=));
			ADJUST_BY_RANGE(w, MIN_Timeout_Yield, MAX_Timeout_Yield);
			CONFIG_TimeoutYield = w;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cyt=%d\n", CONFIG_TimeoutYield);
			break;
		}

		// CMD>> user config valve timeout: cit=<sec>
		//          resp: cit=<sec>
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(cit=)))
		{
			w = atoi((const char*)msg+CONST_STR_LEN(cit=));
			ADJUST_BY_RANGE(w, MIN_Timeout_Idle, 36000);
			CONFIG_TimeoutIdle = w;

			p += snprintf(p, obuf + sizeof(obuf) -p -2, "cit=%d\n", CONFIG_TimeoutIdle);
			break;
		}

		// CMD>> user request to reboot: reboot=<nodeId>
		//          no resp if rebooted
		if (0 == strncmp((const char*)msg, CONST_STR_AND_LEN(reboot=)))
		{
			w =0;
			hex2int((const char*)msg+CONST_STR_LEN(reboot=), &w);
			if (w == nidThis)
				RNode_reset();
			p += snprintf(p, obuf + sizeof(obuf) -p -2, "nid=%08x\n", nidThis);
			break;
		}

	} while(0);

	pResp = TextMsg_composeResponse(cseq, (uint8_t*)obuf, p - obuf);
	NetIf_transmitPacket(netIf, pResp);
	pbuf_free(pResp);
}

void TextMsg_OnResponse(void* netIf, uint8_t cseq, uint8_t* msg, uint16_t len)
{
	// do nothing
}

#endif // 0

