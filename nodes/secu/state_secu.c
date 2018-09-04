#include "bsp.h"
#include "app_cfg.h"
#include "../htod.h"

#ifndef __HomeTether_pulses_H__
bool PulseDecode_PT2262(Pulses* in, uint32_t* pCode);
void PulseEncode_PT2262(Pulses* out, uint32_t code);
bool PulseDecode_EV1527(Pulses* in, uint32_t* pCode);
void PulseEncode_EV1527(Pulses* out, uint32_t code);
#endif // __HomeTether_pulses_H__

// -----------------------------
// global declares
// -----------------------------
void ASKDevice_scan(uint32_t msecNow);
void ASKDevice_OnEvent(uint32_t deviceId);

// -----------------------------
// mapping to OD variables
// -----------------------------
uint16_t* ADC_DMAbuf = Ecu_adc; // the DMA buf
static uint16_t* const ChannelVals_Lumin         = &Ecu_adc[0];
static uint16_t* const ChannelVals_Temperature   = &Ecu_temperature[0];

uint32_t Timeout_MasterHeartbeat = 30*1000;
uint32_t Timeout_State           = 30*1000;

uint16_t LuminBaks[CHANNEL_SZ_LUMIN];

#define StateHB(_GSTATE) (_GSTATE & gsf_StateMask)
#define GSTATE           StateHB(_gState)

void State_init(bool debugMode)
{
	if (debugMode)
		_deviceState |= dsf_Debug; 
	Timeout_MasterHeartbeat = DEFAULT_MasterHeartbeatTimeoutReload;
	Timeout_State = STATE_TIMEOUT_Easy;
}

void State_enter(StateOrdinalCode newState)
{
	// about the procedure when entering a new state
	trace("State_enter() leaving state[%d] for state[%d]", StateHB(_gState), newState);
	if (newState == StateHB(_gState))
	{
		trace("State_enter() ignore no state change");
		_gState = (_gState & 0xf0) | newState;
		return;
	}

	// procedure when leaving the current state
	switch(GSTATE)
	{
	case soc_Easy:
		break;

	case soc_Guard:
		break;

	case soc_PreGuard:
		break;

	case soc_PreAlarm:
#ifdef __CC_ARM
#  warning TODO: turn off beep
#endif // __CC_ARM
		break;

	case soc_Alarm:
#ifdef __CC_ARM
#  warning TODO: turn off alarm
#endif // __CC_ARM
		break;

	default:
		break;
	}

	_gState = (_gState & 0xf0) | newState;
	triggerPdo(HTPDO_GlobalStateChanged);

	// about the procedure when entering a new state
	switch(GSTATE)
	{
	case soc_Easy: // ease state
		Timeout_State = STATE_TIMEOUT_Easy;

#ifdef __CC_ARM
#  warning TODO: turn off all beep/alarms
#endif // __CC_ARM
		break;

	case soc_SilentHours:
		Timeout_State = STATE_TIMEOUT_SilentHours;
		break;

	case soc_PreGuard:
		Timeout_State = STATE_TIMEOUT_PreAlarm;
		break;

	case soc_Guard:
		Timeout_State = STATE_TIMEOUT_Guard;
		break;

	case soc_PreAlarm:
		Timeout_State = STATE_TIMEOUT_PreAlarm;
		break;

	case soc_Alarm:
		Timeout_State = STATE_TIMEOUT_Alarm;
		break;

	default:
		Timeout_State = STATE_TIMEOUT_Guard;
		break;
	}
}

static void State_OnTimeout(void)
{
	if (Ecu_masterNodeId != HtCan_thisNodeId)
	{
		// if this is not the master, no need to dispatch on timeout, just refresh and listen to what master says
		Timeout_State = STATE_TIMEOUT_Easy;
		return;
	}

	switch(GSTATE)
	{
	case soc_Easy:
		// timeout at ease state means there are no inner motions for long time, enter either soc_Guard or soc_SilentHours per RTC hours
		if (EcuConf_motionDiffMask & Ecu_motionState)
			Timeout_State = STATE_TIMEOUT_Easy;
		else
		{
#ifdef __CC_ARM
#  warning TODO: test RTC hours and lumin values, enter soc_SilentHours if RTC hour in range of [21:00-5:00] and all lumin indicates dark
#endif // __CC_ARM

			State_enter(soc_Guard);
		}
		break;

	case soc_SilentHours:
#ifdef __CC_ARM
#  warning TODO: if RTC hour gets out of [21:00 - 9:00], enter soc_Guard, else simply renew the timeout
#endif // __CC_ARM
		Timeout_State = STATE_TIMEOUT_SilentHours;
		break;

	case soc_PreGuard:
		if (EcuConf_motionDiffMask & (~EcuConf_motionOuterMask) & Ecu_motionState)
			State_enter(soc_PreAlarm);
		else State_enter(soc_Guard);
		break;

	case soc_Guard:
		// never timeout, simply renew
		Timeout_State = STATE_TIMEOUT_Guard; // 1hr
		break;

	case soc_PreAlarm:
		// timeout but no manual action to release the alarm, simply enter soc_Alarm
		State_enter(soc_Alarm);
		break;

	case soc_Alarm:
		// timeout but no manual action to release the alarm, enter soc_Guard again if inner motions still there
		if (EcuConf_motionDiffMask & (~EcuConf_motionOuterMask) & Ecu_motionState)
			State_enter(soc_PreAlarm);
		else State_enter(soc_Guard);
		break;

	default:
		State_enter(soc_Guard); // by default
		break;
	}
}

static void State_OnDangerousSource(uint16_t flags)
{
	switch(GSTATE)
	{
	case soc_Easy:
		break;

	case soc_SilentHours:
		break;

	case soc_Guard:
		break;

	case soc_PreAlarm:
		break;

	case soc_Alarm:
		break;

	default:
		break;
	}
}

static void State_OnOuterMotion(uint16_t flags)
{
	switch(GSTATE)
	{
	case soc_Easy:
	case soc_SilentHours:
		trace("blink: stay away this wall");
		break;

	case soc_Guard:
	case soc_PreAlarm:
		trace("warning: leave this wall");
		break;

	case soc_Alarm:
		trace("!!WAA WOO GET AWAY THIS WALL!!");
		break;

	default:
		break;
	}
}

static void State_OnInnerMotion(uint16_t flags)
{
	switch(GSTATE)
	{
	case soc_Easy:
		// in easy state, refresh the Timeout_State if there are any motion detected in the inner area
		Timeout_State = STATE_TIMEOUT_Easy;
		break;

	case soc_SilentHours:
		// if motion is firstly occurs out of the bedroom or officeroom, enter soc_PreAlarm
		if ((~EcuConf_motionBedMask) & flags)
			State_enter(soc_PreAlarm);
		else State_enter(soc_Easy);

		break;

	case soc_Guard:
			State_enter(soc_PreAlarm);
		break;

	case soc_PreAlarm:
	case soc_Alarm:
		if (_deviceState & dsf_MotionChanged)
		{
#ifdef __CC_ARM
#  warning TODO: send sms
#endif // __CC_ARM
		}
		break;

	default:
		break;
	}
}

extern const uint8_t HtCan_thisNodeId;

uint16_t lastDeviceState=0;
uint8_t  askStateYield =0;

void State_process(void)
{
	uint16_t flag2Test = ~0; // dsf_MotionChanged | dsf_LuminChanged | dsf_IRSent | dsf_IRReceived;

	uint16_t askDevState =0, i;
	uint32_t askMask;

/*
	if (0 == askStateYield--)
	{
		// determ flags dsf_Window, dsf_GasLeak, dsf_Smoke and dsf_OtherDangers by checking check if any dangerous source has activities
		for (i =0, askMask=1; i < ASK_DEVICES_MAX; i++, askMask<<=1)
		{
			if (0 == (EcuCtrl_askDevState & askMask))
				continue;
			askDevState |= 1 << (EcuCtrl_askDevTypes[i] -1);
		}

		askDevState &= 0x0f;
		_deviceState |= askDevState << ASK_DEVICE_BASEFLG;

		// reset the EcuCtrl_askDevState and yield a number of scans
		EcuCtrl_askDevState = 0; askStateYield = ASK_DEVICES_TIMEOUT_REFILL; 
	}

#ifdef __CC_ARM
#  warning TODO:	determ flags dsf_GasLeak, dsf_Smoke and dsf_OtherDangers by checking check if any dangerous source has activities
#endif // __CC_ARM

	if ((flag2Test & _deviceState) != (flag2Test & lastDeviceState)) // test if there is any interested state changed
	{
		trace("sending DeviceStateChanged [%04x]=>[%04x] ...", lastDeviceState, _deviceState);
		triggerPdo(HTPDO_DeviceStateChanged);
	}
*/

	lastDeviceState = _deviceState;  // refresh the backup of local device state

	if (Timeout_MasterHeartbeat >0 || 0 == (_gState & gsf_Autonomy))
		return; // done here as a slave

	// if timeout of master heartbeat has been reached, set local ECU as the master
	if (Ecu_masterNodeId != HtCan_thisNodeId)
	{
		trace("timeout at waiting heartbeats from master node, entering master mode: latest remote master[%02x], timeout[%d]msec", Ecu_masterNodeId, DEFAULT_MasterHeartbeatTimeoutReload);

		// if it was at a slave state, start with soc_Easy here
//		if (soc_Slave == _gState)
//			_gState = soc_Easy;

		// take over the masterId and issue a PDO to others
		Ecu_masterNodeId = HtCan_thisNodeId;
		triggerPdo(HTPDO_GlobalStateChanged);
	}

//	_deviceState |= dsf_LocalMaster;

	flag2Test = dsf_GasLeak |dsf_Smoke |dsf_OtherDangers;
	flag2Test &= _deviceState;
	if (flag2Test)
		State_OnDangerousSource(flag2Test);

	flag2Test = Ecu_motionState & EcuConf_motionOuterMask;
	if (flag2Test) // there are some motion occur within the outer area
		State_OnOuterMotion(flag2Test);

	flag2Test = Ecu_motionState & ~((uint16_t) EcuConf_motionOuterMask) & EcuConf_motionDiffMask;
	if (flag2Test) // there are some motion occur within the inner area
		State_OnInnerMotion(flag2Test);

	if (Timeout_State <=0)
		State_OnTimeout();
}

uint16_t State_scanDevice(uint16_t hintedSleep)
{
	uint16_t tmpU16, flag;
	static uint16_t lastMotionState =0;
	static uint32_t msecLastTemp =0, msecLastMotion=0, msecLastLumin=0;
	uint32_t msecNow=0, msecDiff;
	uint8_t i;	

	// collecting informations and determine the flags of _deviceState
	// ----------------------------

	msecNow = getTickXmsec();

	// step 1.1 about the ASK device states
	// ASKDevice_scan(msecNow);

	// step 1.2 about the motion states and ask device states
	msecDiff = msecNow - msecLastMotion;
	if (EcuConf_motionDiffIntv >0 && (msecDiff/100) > EcuConf_motionDiffIntv)  // motionInterval is defined in 100msec, in the range of [0.1, 25.5] sec
	{
		Ecu_motionState = MotionState();

		if ((EcuConf_motionDiffMask & Ecu_motionState) != (EcuConf_motionDiffMask & lastMotionState))
		{
			// the motion state was just changed
			_deviceState |= dsf_MotionChanged;  // the per-state dispatch should issue TPDO1 by this flag
			hintedSleep = 1;
		}
		else _deviceState &= ~dsf_MotionChanged;

		lastMotionState = Ecu_motionState;
		msecLastMotion  = msecNow;
	}

	// step 2. reading the temperatures
	msecDiff = msecNow - msecLastTemp;
	if (EcuConf_temperatureIntv >0 && (msecDiff/1000) > EcuConf_temperatureIntv)
	{
		msecLastTemp = msecNow;
		flag = (1<<0);
		for (i=0; i < min(CHANNEL_SZ_DS18B20, 8); i++, flag<<=1)
		{
			if (0 == (EcuConf_temperatureMask & flag))
			{
				ChannelVals_Temperature[i] = 0x7fff;
				continue;
			}

			ChannelVals_Temperature[i] = DS18B20_read(&ds18b20s[i]);
		}
	}

	// step 3. about the lumin ADC values that have been DMA to Ecu_lumine[0]~7
	msecDiff = msecNow - msecLastLumin;
	if (EcuConf_lumineDiffIntv >0 && (msecDiff/200) > EcuConf_lumineDiffIntv)
	{
		msecLastLumin = msecNow;
		flag = (1<<0);
		tmpU16 =0;
		for (i=0; i < min(CHANNEL_SZ_LUMIN, 8); i++, flag<<=1)
		{
			if (EcuConf_lumineDiffTshd >0 && EcuConf_lumineDiffMask & flag && abs(LuminBaks[i] - ChannelVals_Lumin[i]) >= EcuConf_lumineDiffTshd)
				tmpU16 |= flag;

			LuminBaks[i] = ChannelVals_Lumin[i];
		}

		if (0 != tmpU16)
		{
			_deviceState |= dsf_LuminChanged; // the per-state dispatch should issue TPDO1 by this flag
			hintedSleep = 1;
		}
		else _deviceState &= ~dsf_LuminChanged;
	}

	// step 4. drive the relays: IO=CtrlBytes_relays
	// leave the onTick to drive the IO
	// for (i=0; i <CHANNEL_SZ_Relay; i++)
	// ECU_setRelay(i, (EcuCtrl_relayFlags & (uint8_t)(1<<i)) ? 1:0);

	return hintedSleep;
}

/*
#define PT2262Devices_Max               (10)
#define PT2262Devices_TimeoutReload     (5*1000) // 5sec
typedef struct _PT2262Device
{
	uint32_t id;
	uint16_t timeout;	
} PT2262Device;

// PT2262Device PT2262s[PT2262Devices_Max];
volatile bool bPT2262sBusy = FALSE;

void ASKDevice_OnEvent(uint32_t code)
{
	int i =0, k=PT2262Devices_Max;
	if (FALSE != bPT2262sBusy)
		return; // simply drop the signal this time because PT2262 is supposed to repeat several times
	bPT2262sBusy = TRUE;

	// step 1. queue this message into buff PT2262s
	for (i=0; i< PT2262Devices_Max; i++)
	{
		if (PT2262s[i].id == code)
			k=i;

		if (PT2262Devices_Max == k && 0 ==PT2262s[i].timeout)
			k =i;
	}

	if (k < PT2262Devices_Max)
	{
		PT2262s[k].id      = code;
		PT2262s[k].timeout = PT2262Devices_TimeoutReload;
	}

	// step 2. set the flag if this is a known device registered in local node
	for (i=0; i <ASK_DEVICES_MAX; i++)
	{
		if (code == (EcuCtrl_askDevTable[i] & 0xffffff))
		{
			EcuCtrl_askDevState |= (1<<i);
			break;
		}
	}

	//TODO: step 3. forward the message to the bus: POST ask:<thisNodeId>/askId=<code>
	bPT2262sBusy = FALSE;
}

void ASKDevice_scan(uint32_t msecNow)
{
	int i =0, j=0;
	int32_t newTimeout;
	static uint32_t _lastScan =0;
	int32_t msecToDecrease = msecNow - _lastScan;
	if (msecToDecrease<=0 || FALSE != bPT2262sBusy)
		return; // simply ignore this scan

	bPT2262sBusy = TRUE;
	_lastScan = msecNow;

	for (i=0, j=0; i< PT2262Devices_Max; i++)
	{
		newTimeout = (int32_t)PT2262s[i].timeout - msecToDecrease;
		if (newTimeout>0)
		{
			PT2262s[j].timeout = newTimeout;
			PT2262s[j].id = PT2262s[i].id;
			j++;
			continue;
		}

		PT2262s[i].timeout =0;
	}

	for (; j< PT2262Devices_Max; j++)
		PT2262s[j].id =0;

	bPT2262sBusy = FALSE;
}

void ASKDevice_processMessage(Pulses* captured)
{
	uint32_t code =0;

	if (!PulseDecode_PT2262(captured, &code) && !PulseDecode_EV1527(captured, &code))
		return;

	ASKDevice_OnEvent(code);
}
*/

#define DEC_TIMEOUT(_TO, _BY)	if (_TO >_BY) _TO -= _BY; else _TO =0;

void ThreadStep_do1msecTimerProc(void)
{
	static uint32_t tickLast =0, tickNow=0, tickLastBlink=0;
	static int32_t i;

#ifdef WITH_UCOSII
    OS_CPU_SR  cpu_sr = 0;
	OS_ENTER_CRITICAL();
#endif // WITH_UCOSII

	do {
		tickNow = getTickXmsec();
		i = tickNow - tickLast;
		tickLast = tickNow;

		if (i <=0)
			break;

//		DEC_TIMEOUT(EcuCtrl_pulseRecvTimeout, i);
		DEC_TIMEOUT(Timeout_MasterHeartbeat,  i);
		DEC_TIMEOUT(Timeout_State,            i);

		// last blink is 0.1sec based
		if (tickNow - tickLastBlink <100)
			break;
		tickLastBlink = tickNow;

		// tick the blinks, and apply on to the relays
		for (i=0; i < CHANNEL_SZ_Relay; i++)
			BlinkPair_OnTick(&EcuCtrl_relays[i]);

	} while(0);

#ifdef WITH_UCOSII
	OS_EXIT_CRITICAL();
#endif // WITH_UCOSII
}

int ThreadStep_doStateScan(int nextSleep)
{
	int ret =0;
#ifdef WITH_UCOSII
    OS_CPU_SR  cpu_sr = 0;
	OS_ENTER_CRITICAL();
#endif // WITH_UCOSII

	// dispatch by states
	// ----------------------------
	State_process();

	// scan and update the device status
	ret = State_scanDevice(nextSleep);

#ifdef WITH_UCOSII
	OS_EXIT_CRITICAL();
#endif // WITH_UCOSII

	return ret;

}

void ThreadStep_doRecvTTY(uint8_t chId, char* buf, uint8_t byteRead, uint8_t* pStartOffset)
{
	int c =-1;
#ifdef WITH_UCOSII
    OS_CPU_SR  cpu_sr = 0;
	OS_ENTER_CRITICAL();
#endif // WITH_UCOSII

	do {
		c =MsgLine_recv(chId, (uint8_t*)buf, (*pStartOffset) +byteRead);
		if (c<0 || c >=byteRead)
		{
			*pStartOffset =0;
			break;
		}

		for (*pStartOffset=0; *pStartOffset < (byteRead -c); *pStartOffset++)
			buf[*pStartOffset] = buf[c + *pStartOffset];

	} while(0);
 #ifdef WITH_UCOSII
	OS_EXIT_CRITICAL();
#endif // WITH_UCOSII

}

#ifdef _WIN32
#endif // _WIN32

int ThreadStep_doMsgProc(void)
{
	static int i=0, c=0, tmp=0;
	volatile MsgLine* pLine = NULL;

#ifdef WITH_UCOSII
    OS_CPU_SR  cpu_sr = 0;
	OS_ENTER_CRITICAL();
#endif // WITH_UCOSII
	// step 1. scan if there are any pending text msgs need to be processed
	for (i =0, c=0; i < MAX_PENDMSGS; i++)
	{
		if (0 == _pendingLines[i].timeout || _pendingLines[i].offset >0)
			continue;  // idle or incomplete message

		// NEVER print_tty here!!!!=>dead loop trace("processing msg@%d for ch[%d]: %s", i, _pendingLines[i].chId, _pendingLines[i].msg);

		// process a completed pending message
		//step 1.1. reserve timeout to hold this message during processing
		_pendingLines[i].timeout = 0xff;	c++;

		tmp = strlen((const char*)_pendingLines[i].msg);
		if (tmp<=0) { _pendingLines[i].timeout =0; continue; }

		do {
			pLine = &_pendingLines[i];

			// step 1.2 process the text messages per chId
#ifdef _WIN32
			char* cmdEcho = strstr((char*)pLine->msg, "PUT echo="); 
			if (cmdEcho)
			{
				cmdEcho += sizeof("PUT echo=")-1;
				bool bEcho = ('0' != *cmdEcho);

				switch(pLine->chId)
				{
				case MSG_CH_RS232_Received:
					com_admin.enableEcho(bEcho);
					break;
				case MSG_CH_RS232_SendTo:
				case MSG_CH_RS485_Received:
				case MSG_CH_RS485_SendTo:
				default:
					break;
				}

				break;
			}
#endif // _WIN32

			switch(pLine->chId)
			{
			case MSG_CH_RS232_SendTo:
				AdminExt_sendMsg(fdadm, (char*)pLine->msg, strlen((const char*)pLine->msg));
				break;
			case MSG_CH_RS485_SendTo:
				AdminExt_sendMsg(fdext, (char*)pLine->msg, strlen((const char*)pLine->msg));
				break;
			case MSG_CH_RS232_Received:
				GW_dispatchTextMessage(fdadm, (char*)pLine->msg);
				break;
			case MSG_CH_RS485_Received:
				GW_dispatchTextMessage(fdext, (char*)pLine->msg);
				break;
			}

		} while(0);

		//step 1.End. reset timeout to be idle after processing
		MsgLine_free(&_pendingLines[i]);
	}

#ifdef WITH_UCOSII
	OS_EXIT_CRITICAL();
#endif // WITH_UCOSII

	/*
	// step 2. send the pending pulses signals
	if (0xff != EcuCtrl_pulseSendPflId)
	{
		// there is a pending pulse code, send the pulses
		PulseSend_byProfileId(EcuCtrl_pulseSendPflId, EcuCtrl_pulseSendCode, &IrLEDs[EcuCtrl_pulseSendChId % CHANNEL_SZ_IrLED], FALSE, EcuCtrl_pulseSendBaseIntvX10usec*10);
		EcuCtrl_pulseSendPflId =0xff; // reset to idle
	}
	//	*/

	return c;
}

void ThreadStep_doCanMsgIO(void)
{
#ifdef WITH_UCOSII
    OS_CPU_SR  cpu_sr = 0;
	OS_ENTER_CRITICAL();
#endif // WITH_UCOSII
	HtCan_flushOutgoing(MSG_CH_CAN1);
	HtCan_doScan(MSG_CH_CAN1);

#ifdef WITH_UCOSII
	OS_EXIT_CRITICAL();
#endif // WITH_UCOSII
}

extern void GW_nicReceiveLoop(ENC28J60* nic);

void ThreadStep_doNicProc(void)
{
#ifdef WITH_UCOSII
    OS_CPU_SR  cpu_sr = 0;
	OS_ENTER_CRITICAL();
#endif // WITH_UCOSII

	GW_nicReceiveLoop(&nic);

#ifdef WITH_UCOSII
	OS_EXIT_CRITICAL();
#endif // WITH_UCOSII
}

void OnHeartbeat(uint8_t fdCAN, uint8_t fromWhom, uint8_t canState, uint8_t gState, uint8_t masterId)
{
	bool bChanged = false;
	char msg[40];

	if (fromWhom >0)
	{
		snprintf(msg, sizeof(msg)-2, "POST ht:%x/%shb?l=%x&canState=%x\r\n", fromWhom, (ecs_Operational != canState)?"EVT_":"", (HtCan_thisNodeId==fromWhom)?1:0, canState);
		AdminExt_sendMsg(fdadm, msg, strlen(msg));
	}

	if (((fromWhom & 0xf0) >= HtCan_thisNodeId) || (ecs_Operational != canState))
	{
		// a lower priority node or masternode advertizes, ignore
		return;
	}

	Timeout_MasterHeartbeat = DEFAULT_MasterHeartbeatTimeoutReload;
//	Ecu_localState &= ~((uint16_t)dsf_LocalMaster);
	if (Ecu_masterNodeId != masterId || _gState != gState)
	{
		_gState = _gState & 0x0f | gState & 0xf0; // takes the higher flags directly
		State_enter((StateOrdinalCode)(gState & 0x0f));
		Ecu_masterNodeId = masterId;
		bChanged = true;
	}

	if (bChanged)
		triggerPdo(HTPDO_GlobalStateChanged);
}
