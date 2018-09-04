#include "bsp.h"
#include "app_cfg.h"
#include "htcluster.h"
#include "esp8266.h"

ESP8266 esp8266;

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
#define DEFAULT_CONF_VAL(_DEBUGVAL, _NORMALVAL) ((dsf_Debug & Ecu_localState) ? (_DEBUGVAL)  : (_NORMALVAL)) 

// #define DEFAULT_Timeout_Pump      CONST(   10, 3*60)  // 3min
uint16_t  CONFIG_temperatureMask  =0xffff;
uint8_t   CONFIG_temperatureIntv  =20;	// 20sec in sec

uint16_t  CONFIG_motionDiffMask   = 0xff;
uint8_t   CONFIG_motionDiffIntv   = 20;    // 2sec in 100msec
uint16_t  CONFIG_motionOuterMask  = 0xf0;  // mask of out-doors
uint16_t  CONFIG_motionBedMask    = 0x02;  // mask of bedrooms

uint8_t   CONFIG_lumineDiffMask   =0xff;  // initialized in PECU_init() 
uint8_t   CONFIG_lumineDiffIntv   =10;	 // 10sec, initialized in PECU_init() 
uint16_t  CONFIG_lumineDiffTshd;         //initialized in PECU_init() 

uint16_t  CONFIG_notifyIntv_env   = 10;  // in sec, initialized in PECU_init() 

// -------------------------------------------------------------------------------
// Runtime varaibles
// -------------------------------------------------------------------------------
int16_t  temperatures[CHANNEL_SZ_Temperature]={0,0,};
uint8_t  humidities[CHANNEL_SZ_Humidity]={0,0,};
// lumine ADC values
uint16_t ADC_DMAbuf[(1+ CHANNEL_SZ_ADC)*2];
uint16_t lumins[CHANNEL_SZ_LUMIN];

// uint16_t Ecu_localState =0;

// -------------------------------------------------------------------------------
// Dictionary expose for HtNode
// -------------------------------------------------------------------------------
// OD declaration:
USR_OD_DECLARE_START()
  OD_DECLARE(lstatus,     ODTID_WORD, ODF_Readable, &Ecu_localState,  sizeof(Ecu_localState))
  OD_DECLARE(mstatus,     ODTID_WORD, ODF_Readable, &Ecu_motionState, sizeof(Ecu_motionState))
  OD_DECLARE(temps,       ODTID_WORD, ODF_Readable, temperatures,     sizeof(temperatures))
  OD_DECLARE(humds,       ODTID_BYTE, ODF_Readable, humidities,       sizeof(humidities))
  OD_DECLARE(lumns,      ODTID_WORD, ODF_Readable, lumins,           sizeof(lumins))
  OD_DECLARE(adc,         ODTID_WORD, ODF_Readable, ADC_DMAbuf,       sizeof(ADC_DMAbuf[1]) *CHANNEL_SZ_ADC)

  // configuations
  OD_DECLARE(CFGtMask,    ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_temperatureMask,  sizeof(CONFIG_temperatureMask))
  OD_DECLARE(CFGtIntv,    ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_temperatureIntv,  sizeof(CONFIG_temperatureIntv))

  OD_DECLARE(CFGmMask,    ODTID_WORD, ODF_Readable|ODF_Writeable, &CONFIG_motionDiffMask,   sizeof(CONFIG_motionDiffMask))
  OD_DECLARE(CFGmIntv,    ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_motionDiffIntv,   sizeof(CONFIG_motionDiffIntv))
  OD_DECLARE(CFGmOuter,   ODTID_WORD, ODF_Readable|ODF_Writeable, &CONFIG_motionOuterMask,  sizeof(CONFIG_motionOuterMask))
  OD_DECLARE(CFGmBR,      ODTID_WORD, ODF_Readable|ODF_Writeable, &CONFIG_motionBedMask,    sizeof(CONFIG_motionBedMask))

  OD_DECLARE(CFGlMask,    ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffMask,   sizeof(CONFIG_lumineDiffMask))
  OD_DECLARE(CFGlIntv,    ODTID_BYTE, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffIntv,   sizeof(CONFIG_lumineDiffIntv))
  OD_DECLARE(CFGlDiff,    ODTID_WORD, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffTshd,   sizeof(CONFIG_lumineDiffTshd))

USR_OD_DECLARE_END()

// -------------------------------------------------------------------------------
// Definition of HtNode Events
// -------------------------------------------------------------------------------
// TODO
//
static const uint8_t eventOD_StateChanged[] = { ODID_lstatus, ODID_mstatus, ODID_NONE };
static const uint8_t eventOD_env[]          = { ODID_mstatus, ODID_temps, ODID_humds, ODID_lumns, ODID_NONE };

USR_ODEVENT_DECLARE_START()
 	ODEVENT_DECLARE(StateChanged, eventOD_StateChanged)
 	ODEVENT_DECLARE(env,          eventOD_env)
USR_ODEVENT_DECLARE_END()

// -------------------------------------------------------------------------------
// PECU_init()
// -------------------------------------------------------------------------------
void PECU_init(bool debugMode)
{
	if (debugMode)
		Ecu_localState |= dsf_Debug; 

	CONFIG_temperatureMask  =0xffff;
	CONFIG_temperatureIntv  = DEFAULT_CONF_VAL(5, 20);	// 20sec in sec

	CONFIG_motionDiffMask   = 0xff;
	CONFIG_motionDiffIntv   = DEFAULT_CONF_VAL(10,   20);    // 2sec in 100msec
	CONFIG_motionOuterMask  = DEFAULT_CONF_VAL(0xf0, 0xf0);  // mask of out-doors
	CONFIG_motionBedMask    = DEFAULT_CONF_VAL(0x02, 0x02);;  // mask of bedrooms

	CONFIG_lumineDiffMask   = 0xff;
	CONFIG_lumineDiffIntv   = DEFAULT_CONF_VAL(10, 10);	 // 10sec
	// CONFIG_lumineDiffTshd;  

	CONFIG_notifyIntv_env   = DEFAULT_CONF_VAL(10, 20); // 300);  // in sec  
}

// -------------------------------------------------------------------------------
// PECU_scanDevice()
// -------------------------------------------------------------------------------
static uint16_t	lastMotionState = 0;
static uint16_t lastLocalState =0;
void PECU_scanDevice(void)
{
	static uint32_t stampLastTemp =0, stampLastMotion=0, stampLastLumin=0;
	uint16_t tmpU16=0, flag=0, i, vU16;

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

		CRITICAL_ENTER();
		if (CONFIG_temperatureMask & (1<<0))
			temperatures[0] = STM32TemperatureADC2dC((ADC_DMAbuf[0] + ADC_DMAbuf[CHANNEL_SZ_ADC])/2);

		tmpU16 = 1; // DS18B20 fill since temperatures[1]
		flag = (1<<1);
		for (i=0; i < CHANNEL_SZ_DS18B20 && (tmpU16+i) < CHANNEL_SZ_Temperature; i++, flag<<=1)
		{
			if (0 == (CONFIG_temperatureMask & flag))
				continue;

			temperatures[tmpU16+i] = DS18B20_read(&ds18b20s[i]);
		}

		tmpU16 = 1+CHANNEL_SZ_DS18B20; // DHT11 fill since temperatures[1+CHANNEL_SZ_DS18B20]
		for (i=0; i < CHANNEL_SZ_DHT11 && (tmpU16+i) < CHANNEL_SZ_Temperature; i++, flag<<=1)
		{
			uint16_t t;
			uint8_t h;
			if (0 == (CONFIG_temperatureMask & flag) || ERR_SUCCESS != DHT11_read(&dht11s[i], &t, &h))
				continue;

			temperatures[tmpU16+i] = t;
			humidities[i] = h;
		}
		CRITICAL_LEAVE();
	}

	// step 3. about the lumin ADC values that have been DMA to Ecu_lumine[0]~7
	if (CONFIG_lumineDiffIntv >0 && gClock_getElapsed(stampLastLumin) > (CONFIG_lumineDiffIntv*1000)) // config in 1sec
	{
		stampLastLumin = gClock_msec;

		flag = (1<<0);
		tmpU16 =0;

		for (i=0; i < min(CHANNEL_SZ_LUMIN, 8); i++, flag<<=1)
		{
			vU16 = (ADC_DMAbuf[i+1] + ADC_DMAbuf[CHANNEL_SZ_ADC +1 +i])/2;
			if (CONFIG_lumineDiffTshd >0 && (CONFIG_lumineDiffMask & flag) && abs(lumins[i] - vU16) >= CONFIG_lumineDiffTshd)
				tmpU16 |= flag;

			lumins[i] = vU16;
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
// PECU_doMainLoop()
// -------------------------------------------------------------------------------
// the main loop
void PECU_doMainLoop(void)
{
	uint16_t nextSleep =0;

	PECU_init(1);

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
		PECU_scanDevice();
		// ADJUST_BY_RANGE(nextSleep, NEXT_SLEEP_MIN, NEXT_SLEEP_MAX);
		nextSleep = NEXT_SLEEP_DEFAULT;
	}
}


void ECU_sendMsg(void* netIf, pbuf* pmsg)
{
	if (NULL == pmsg || NULL == netIf)
		return;
	
	if(((void*)&esp8266) == netIf)
		ESP8266_transmit(&esp8266, esp8266.activeConn &0x0f, pmsg->payload, pmsg->len);
}

// portal impl of TextMsg
void TextMsg_OnResponse(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
}

void TextMsg_OnSetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
	pbuf* pmsg=NULL;
	klvc = HtNode_accessByKLVs(1, klvc, klvs);
	pmsg = TextMsg_composeResponse(cseq, klvc, klvs);

	ECU_sendMsg(netIf, pmsg);
	pbuf_free(pmsg);
}

void TextMsg_OnGetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
	pbuf* pmsg=NULL;
	klvc = HtNode_accessByKLVs(0, klvc, klvs);
	pmsg = TextMsg_composeResponse(cseq, klvc, klvs);

	ECU_sendMsg(netIf, pmsg);
	pbuf_free(pmsg);
}

void TextMsg_OnPostRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
}

void HtNode_OnEventIssued(uint8_t klvc, const KLV eklvs[])
{
	CRITICAL_ENTER();
	ESP8266_jsonPost2Coap(&esp8266, ALIYUN_ADDR, ALIYUN_PORT, BSP_nodeId, klvc, eklvs);

//	pbuf* pmsg = TextMsg_composeRequest(TEXTMSG_VERBCH_OTH, klvc, eklvs);
//
//	ECU_sendMsg(&TTY1, pmsg);
//	pbuf_free(pmsg);

	CRITICAL_LEAVE();

	sleep(1000);
}

void ESP8266_OnReceived(ESP8266* chip, char* msg, int len, bool bEnd)
{
	if (!bEnd || len <=0)
		return;

	TextMsg_procLine(chip, msg);
}
