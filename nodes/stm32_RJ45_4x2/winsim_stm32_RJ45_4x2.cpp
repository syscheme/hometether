#include "ZQ_common_conf.h"
#include "getopt.h"
#include "NativeThread.h"
#include "Locks.h"
#include "TimeUtil.h"
#include "FileLog.h"

typedef unsigned short   uint16_t;
typedef          short   int16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#ifndef __NOW__
#  define __NOW__ (int32_t) (ZQ::common::TimeUtil::now() & 0xffffffff)
#  define __YIELD__  Sleep(1) // yield cpu for a short while
#endif // __NOW__

void printLine(const char *fmt, ...);


/*
typedef struct _Pulses
{
	uint16 short_durXusec;
	uint8 seqlen;
	uint8* seq;
} Pulses;

bool PulseDecode_PT2262(Pulses* in, uint32* pCode);
void PulseEncode_PT2262(Pulses* out, uint32 code);
bool PulseDecode_EV1527(Pulses* in, uint32* pCode);
void PulseEncode_EV1527(Pulses* out, uint32 code);

// #define __HomeTether_pulses_H__
*/
/*
// -----------------------------
// BlinkTick
// -----------------------------
// about blink an output
#define uint8_t uint8
typedef struct _BlinkTick
{
	uint8_t tick, reloadOn, reloadOff;
} BlinkTick;

bool Blink_isActive(BlinkTick* pPT);
bool Blink_tick(BlinkTick* pBT);
void Blink_config(BlinkTick* pPT, uint8_t reloadOn, uint8_t reloadOff);
void Blink_configByUri(BlinkTick* pPT, char* msg);
*/

extern "C"
{
#include "secu.h"
#include "../htcan.h"
}

#define strchar strchr

static bool bQuit =false;

// ------------------------------------------
// Windows Simulator serial ports (threads)
// ------------------------------------------
// take local named pipe to simulate a COM
class WinSim_COM : public ZQ::common::NativeThread
{
	HANDLE _hPipe;
	std::string _pipePath;
	uint8 _flagEcho;
	static uint8 _configEcho;

public:
	WinSim_COM(const char* pipeName, uint8 chId): _hPipe(NULL), buf_i(0), _chId(chId)
	{
		static uint8 _id=0;
		_flagEcho = (1<<(_id++));

		_pipePath = "\\\\.\\pipe\\";
		_pipePath += (pipeName ? pipeName : "vmcom_1");
	}

	virtual ~WinSim_COM()
	{
		if (NULL!=_hPipe && INVALID_HANDLE_VALUE!=_hPipe)
			::CloseHandle(_hPipe); 
		_hPipe = NULL;
	}

	void echo(char* msg, uint8 maxLen)
	{
		if (NULL ==msg || 0==(_configEcho & _flagEcho))
			return;

		for (int i =0; msg && i < maxLen; i++)
		{
			send(&msg[i], 1);
			if ('\r' == msg[i])
				send("\n",1);
		}
	}

	void enableEcho(bool enable)
	{
		if (enable)
			_configEcho |= _flagEcho;
		else _configEcho &= ~_flagEcho;
	}

	int send(const char* buf, int len)
	{
		if (NULL == _hPipe || INVALID_HANDLE_VALUE ==_hPipe)
			return -2;
		// Send a message to the pipe server. 
		DWORD bytesend = len;
		// printf("sending %d byte message: %s\n", bytesend, buf); 
		if (!::WriteFile(_hPipe, buf, bytesend, &bytesend, NULL)) 
		{
			printf("WriteFile to pipe failed. err:%d\n", GetLastError()); 
			return -1;
		}

		::FlushFileBuffers(_hPipe);
		return bytesend;
	}

	char buf[1024];
	int buf_i;
	uint _chId;
	OVERLAPPED _overlap;
// 	FILE* _fif;

protected:

	virtual int run(void)
	{
		while (!bQuit)
		{
			do {
				_hPipe = INVALID_HANDLE_VALUE;
				_hPipe = CreateFile(_pipePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
				if (INVALID_HANDLE_VALUE != _hPipe) 
					break; // quit the open loop if open succeeded
				
				// exit if an error other than ERROR_PIPE_BUSY occurs. 
				if (GetLastError() != ERROR_PIPE_BUSY) 
				{
					printf("Could not open pipe. err=%d\n", GetLastError()); 
					return -1;
				}

				// All pipe instances are busy, so wait for 20 seconds. 
				if (!::WaitNamedPipe(_pipePath.c_str(), 20000)) 
				{ 
					printf("Could not open pipe: 20 second wait timed out."); 
					return -1;
				} 
			} while (!bQuit);

			// pipe has been opened successfully here

			// The pipe connected; change to message-read mode. 
			DWORD dwMode = PIPE_READMODE_MESSAGE; 
//			if (!::SetNamedPipeHandleState(_hPipe, &dwMode, NULL, NULL)) 
//			{
//				printf("SetNamedPipeHandleState failed. err=%d\n", GetLastError() ); 
//				return -1;
//			}

			char buf[1024];
			uint8_t buf_start=0;

			while (!bQuit)
			{
#pragma message ( __MSGLOC__ __FUNCTION__ "() maps to ISR_USART1() ")

				DWORD byteread = sizeof(buf)-buf_start;

				if (!::ReadFile(_hPipe, buf+buf_start, byteread, &byteread, NULL) && ERROR_MORE_DATA != GetLastError())
					break;

				if (byteread>0)
				{
					echo(buf +buf_start, (uint8)byteread);
					ThreadStep_doRecvTTY(_chId, buf, byteread, &buf_start);
				}
			}

			if (bQuit)
				break;

			::CloseHandle(_hPipe);
			_hPipe = NULL;
		}

		return 0;
	}

};

uint8 WinSim_COM::_configEcho =1;
WinSim_COM  com_admin("vmcom_1", MSG_CH_RS232_Received);

typedef struct _ENC28J60
{
	const uint8_t macaddr[6];

	void* pCtx;

// private workctx of ENC28J60
	uint8_t currentBank;
	uint16_t ptrNextPacket;
} ENC28J60;

const uint8_t  MyIP[4]            = {192,168,0, NODEID_TO_IP_B4(HT_NODEID)};
const uint16_t MyServerPort       = 80;
const uint8_t  GroupIP[4]         = {239,0,0,55};

ENC28J60 nic= {
			{0x00,0x1A,0x6B,0xCE,0x92,0x66},
			NULL,
			};

#define BYTES2WORD( _BYTE_HIGH, _BYTE_LOW) ((((uint16_t)_BYTE_HIGH) <<8) | (_BYTE_LOW &0xff))
uint16_t ENC28J60_recvPacket(ENC28J60* nic, uint16_t maxlen, uint8_t* packet);

extern "C"
{
#include  <conio.h>
#include "../htcan.c"
#include "../htod.c"
#include "../../stm32/common/pulses.c"
#include "../../stm32/common/common.c"
#include "./state_secu.c"
#include "./gw_secu.c"
#include "../../stm32/common/ip_arp_udp_tcp.c"
}

/*
typedef struct _MsgLine
{
	uint8_t chId, offset, timeout;
	char msg[MAX_MSGLEN];
} MsgLine;
*/

#include <queue>
#define strchar strchr
void usage();

// utility funcs
// ------------------------------------------
#define OSTimeDlyHMSM(HH,MM,SS,mm) Sleep(((HH*60 + MM)*60+SS)*1000+mm)

ZQ::common::FileLog flogger(".\\secu.log", ZQ::common::Log::L_DEBUG, 2, 1024*1024*10, 512);
bool bfileLog=false;

static ZQ::common::Mutex scrLock;
void printLine(const char *fmt, ...)
{
	char msg[2048];
	va_list args;
	va_start(args, fmt);
	int nCount = _vsnprintf(msg, sizeof(msg)-3, fmt, args);
	va_end(args);
	if(nCount <0)
		msg[0] = '\0';
	else if (nCount > sizeof(msg)-3)
		nCount = sizeof(msg)-3;
	
	msg[nCount] = '\0';
	if (bfileLog)
	{
		flogger(ZQ::common::Log::L_INFO, msg);
	}
	else
	{
		if (NULL == strchr(msg, '\n'))
		{
			msg[nCount++] = '\r';
			msg[nCount++] = '\n';
			msg[nCount++] = '\0';
		}

		ZQ::common::MutexGuard g(scrLock);
		printf(msg);
	}
}

void printMsg(const HTCanMessage *m, const char* prefix)
{
	char buf[30];
	HtCan_toCompactText(buf, sizeof(buf)-2, m);
	trace("%s %s", prefix, buf);
}

// Windows Simulator portal data
// ------------------------------------------
uint8_t fdadm = 3;
uint8_t fdext = 4;
uint8_t fdcan = 5;
uint8_t fdeth = 6;

uint8_t   Ecu_globalState        = soc_Easy;
uint8_t   Ecu_masterNodeId       = 0x43;

// local state
uint16_t  Ecu_localState      =0x00;     // =0;
uint16_t  Ecu_motionState     =0xfc;    // =0;

// ECU configurations
uint8_t   EcuConf_temperatureMask =0xff; // =0xff;
uint8_t   EcuConf_temperatureIntv =5; // =0xff;
uint16_t  EcuConf_motionDiffMask =0xff;  // =0xff;
uint16_t  EcuConf_motionOuterMask = 0xf0;
uint16_t  EcuConf_motionBedMask = 0x02;
uint8_t   EcuConf_motionDiffIntv =10;  // =0xff;
uint8_t   EcuConf_lumineDiffMask =0xff;  // =0xff;
uint8_t   EcuConf_lumineDiffIntv= 4;  // =0xff;
uint16_t  EcuConf_lumineDiffTshd = 500;

// ECU spec
const uint8_t   HtCan_thisNodeId           =HT_NODEID;
const uint8_t   EcuSpec_versionMajor    =0;
const uint8_t   EcuSpec_versionMinor    =1;

// temperature values
uint16_t Ecu_temperature[CHANNEL_SZ_Temperature];

// lumine ADC values
uint16_t Ecu_adc[CHANNEL_SZ_ADC];

// Control of relay

uint8_t EcuCtrl_relays;
BlinkTick EcuCtrl_relayTicks[4] = {
	{0,0,0}, {0,0,0}, {0,0,0}, {0,0,0},
};

// About pulse receiving
uint8_t EcuCtrl_pulseSendChId            =0;
uint8_t EcuCtrl_pulseSendCode            =0;
uint8_t EcuCtrl_pulseSendPflId           =0xff;
uint8_t EcuCtrl_pulseSendBaseIntvX10usec =0;

// About pulse receiving
uint8_t EcuCtrl_pulseRecvChId            =0;
uint8_t EcuCtrl_pulseRecvTimeout         =0; // 0x00 means the receiving buffer would be overwritten if new pulse seq is captured 
uint8_t Ecu_pulseRecvBuf[PULSE_RECV_MAX];
Pulses  Ecu_pulseRecvSeq                 ={ 0, PULSE_RECV_MAX, Ecu_pulseRecvBuf };

/////////dummy portal for sensors //////////////
uint16 MotionState() { return Ecu_motionState; }
int16 DS18B20_read(DS18B20* chip) { return 0x1123; }
void ECU_setRelay(uint8 id, uint8 on) {}
DS18B20 ds18b20s[CHANNEL_SZ_DS18B20];

// Windows Simulator portal implementations
// ------------------------------------------
// Events that portal may needs to hook
// ------------------------------------

void OnClock(uint8_t fdCAN, uint32_t secSinc198411)
{
	int32_t diff = (int32_t) (ZQ::common::TimeUtil::now() & 0xffffffff);
	diff = (diff & 0xffffff) - (secSinc198411 %60*60*24);
	if (diff < 0) // abs the diff
 		diff =-diff;

	if (diff > 5)
	{	// necessary do clock adjusting if the clock has more than 5sec diff
	
		// 1/1/1984 was a sunday, convert the inputed secSinc198411 to our format per DQ
		secSinc198411 = (secSinc198411 %60*60*24) + ((secSinc198411 /60*60*24) %7) << 24;
		// DQBoard_updateClock(secSinc198411);
	}
}

void OnSdoUpdated(const ODObj* odEntry, uint8_t subIdx, uint8_t updatedCount)
{
	ZQ::common::MutexGuard g(scrLock);
	trace("local obj[%04x]#%d updated: c[%d]", odEntry->objIndex, subIdx, updatedCount);
}

void OnSdoClientDone(uint8_t fdCAN, uint8_t errCode, uint8_t peerNodeId, uint16_t objIdx, uint8_t subIdx, uint8_t* data, uint8_t dataLen, void* pCtx)
{
	ZQ::common::MutexGuard g(scrLock);
	if (SDO_CLIENT_ERR_OK == errCode)
		trace("peer[%02x] ack: obj[%04x]#%d c[%d]", peerNodeId, objIdx, subIdx, dataLen);
	else if (SDO_CLIENT_ERR_TIMEOUT == errCode)
		trace("peer[%02x] timeout: obj[%04x]#%d c[%d]", peerNodeId, objIdx, subIdx, dataLen);
	else if (SDO_PEER_ERR == errCode)
		trace("peer[%02x] err: obj[%04x]#%d c[%d]", peerNodeId, objIdx, subIdx, dataLen);
}

void doResetNode()
{
	Sleep(2000);
	bQuit = true;
	Sleep(2000);
	exit(0);
}

std::queue <HTCanMessage> bus;
ZQ::common::Mutex busLock;

uint8_t doCanSend(uint8_t fdCAN, const HTCanMessage *m)
{
	{
		ZQ::common::MutexGuard g(busLock);
		bus.push(*m);
	}

	printMsg(m, "S>>");
	GW_dispatchCANbyFd(fdCAN, m);

	HtCan_flushOutgoing(fdCAN);
	return 1;
}

void procTxtMessage(volatile MsgLine* pLine)
{
	if (NULL == pLine)
		return;

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

		return;
	}

	switch(pLine->chId)
	{
	case MSG_CH_RS232_SendTo:
		AdminExt_sendMsg(fdadm, (char*)pLine->msg, strlen((const char*)pLine->msg));
		return;
	case MSG_CH_RS485_SendTo:
		AdminExt_sendMsg(fdext, (char*)pLine->msg, strlen((const char*)pLine->msg));
		return;
	case MSG_CH_RS232_Received:
		GW_dispatchTextMessage(fdadm, (char*)pLine->msg);
		return;
	case MSG_CH_RS485_Received:
		GW_dispatchTextMessage(fdext, (char*)pLine->msg);
		return;
	}
}

class WinSim_MsgProc : public ZQ::common::NativeThread
{
public:
	WinSim_MsgProc() {}

	virtual ~WinSim_MsgProc()
	{
	}


protected:

	virtual int run(void)
	{
		// dispatch on states and monitoring
		while (!bQuit)
		{
#pragma message ( __MSGLOC__ __FUNCTION__ "() maps to void Task_MsgProc(void* p_arg) ")

			if (ThreadStep_doMsgProc() <=0)
				OSTimeDlyHMSM(0,0,0,200);
		}

		return 0;
	}

};

WinSim_MsgProc msgProcer;

void WinSim_sendMsg(uint8_t fout, const char* msg, uint8_t len)
{
	if (fout==fdadm)
	{
		com_admin.send(msg, len); com_admin.send("\r\n", 2);
	}

	printLine("%d>> %s", (int)(((uint32_t)fout)&0xff), msg);
}


// ------------------------------------------
// Windows Simulator background tasks(threads)
// ------------------------------------------
class Task_CO : public ZQ::common::NativeThread
{
public:
	Task_CO() {}
	virtual ~Task_CO() {}

protected:
	virtual int run(void)
	{
#pragma message ( __MSGLOC__ __FUNCTION__ "() maps to void Task_CO(void* p_arg) ")

		while (!bQuit)
		{
#ifdef _DEBUG
			{
				ZQ::common::MutexGuard g(busLock);
				if (!bus.empty())
					HtCan_flushOutgoing(MSG_CH_CAN1);
			}

#endif // _DEBUG
			Sleep(1);
			HTCanMessage m;
			bool bNewMsg= false;
			do {
				ZQ::common::MutexGuard g(busLock);
				if (bQuit || bus.empty())
					break;
				m = bus.front();
				bus.pop();
				bNewMsg = true;
			} while (0);

			if (bNewMsg)
			{
				printMsg(&m, "R<<");
				HtCan_processReceived(MSG_CH_CAN1, &m);
			}

			HtCan_doScan(MSG_CH_CAN1);
		}

		return 0;
	}

};

class Task_Main : public ZQ::common::NativeThread
{
public:
	Task_Main() {}
	virtual ~Task_Main() {}

public:
	virtual int run(void)
	{
		_deviceState =0;
		_gState =gsf_Autonomy;
		while (!bQuit)
		{
#pragma message ( __MSGLOC__ __FUNCTION__ "() maps to void Task_Main(void* p_arg) ")

			int nextSleep = 5000;  // initialize with a maximal sleep time

			nextSleep = ThreadStep_doStateScan(nextSleep);

			// sleep for next round
			Sleep(nextSleep);
		}

		return 0;
	}

};

#define DEC_TIMEOUT(_TO, _BY)	if (_TO >_BY) _TO -= _BY; else _TO =0;
#define OS_TICKS_PER_SEC  (72*1000)
class Task_Timer1msec : public ZQ::common::NativeThread
{
public:
	Task_Timer1msec() {}
	virtual ~Task_Timer1msec() {}

protected:
	virtual int run(void)
	{
#pragma message ( __MSGLOC__ __FUNCTION__ "() maps to void Task_Timer(void* p_arg) ")

		while (!bQuit)
		{
			Sleep(1);
			ThreadStep_do1msecTimerProc();
		}

		return 0;
	}

};

// ------------------------------------------
// Windows Simulator test programs
// ------------------------------------------
void sdoTest()
{
	uint8_t v[] = {0x11, 0x11, 0x11, 0x11};
	setSDO_async(MSG_CH_CAN1, HT_NODEID, 0x2004, 5, v, 4, NULL);
	getSDO_async(MSG_CH_CAN1, 7, 0x2004, 5, NULL);

	//	for (int j=0; j< 20; j++)
	//		testBuf[j] = j;

	static const HTCanMessage testMsgs[] = {
		{ fcSDOtx<<7 | 7, 0, 8, {SDO_RESP_GET, 0x04, 0x20, 0x05, 0x12, 0x34, 0x56, 0x78} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x01, 0x20, 0x01} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 6, {SDO_CMD_SET, 0x00, 0x20, 0x01, 0x55, 0x77} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x00, 0x20, 0x01} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x01} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x08} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 8, {SDO_CMD_SET, 0x04, 0x20, 0x03, 0x12, 0x34, 0x56, 0x78} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x02} },
		{ fcSDOtx<<7 | HT_NODEID, 0, 4, {SDO_CMD_GET, 0x04, 0x20, 0x04} },

		{ 0xffff, 0, 4, ""},
	};

	for (int i =0; 0xffff != testMsgs[i].cob_id; i++)
	{
		Sleep(100);
		// printMsg(&testMsgs[i], "R<<");
		// HtCan_processReceived(&testMsgs[i]);
		doCanSend(MSG_CH_CAN1, &testMsgs[i]);
	}
}

void pdoTest()
{
	sendHeartbeat(MSG_CH_CAN1);

	triggerPdo(HTPDO_DeviceStateChanged);
	HtCan_doScan(MSG_CH_CAN1);

	static const HTCanMessage testMsgs[] = {
		{ HTPDO_DeviceStateChanged<<7 | 17, 0, 4, {0x12, 0x34, 0x56, 0x78} },
		{ fcPDO3tx<<7 | 17, 0, 4, {0x12, 0x34, 0x56, 0x78} },

		{ 0xffff, 0, 4, ""},
	};

	for (int i =0; 0xffff != testMsgs[i].cob_id; i++)
	{
		// printMsg(&testMsgs[i], "R<<");
		// HTCan_processReceived(&testMsgs[i]);
		doCanSend(MSG_CH_CAN1, &testMsgs[i]);
	}

}

// ------------------------------------------
// Windows Simulator main program
// ------------------------------------------
void nRF24L01_OnPacket(void* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen);
void sendWirelessTxtMsg(char* msg)
{
#define PAYLOAD_LEN 10
	static char buf[PAYLOAD_LEN+2];
	static uint8_t buf_i;

	for (; *msg; msg++)
	{
		buf[buf_i] = *msg;
		if ('\r' != buf[buf_i] && '\n' != buf[buf_i])
		{
			buf_i = ++buf_i % PAYLOAD_LEN;

			if (0 == buf_i)
			{
				// playload reached, send it immediately
				nRF24L01_OnPacket(NULL, 5, (uint8_t*)buf, PAYLOAD_LEN);
			}

			continue;
		}

		if (0 == buf_i) // empty line, continue
			continue;

		// a line just ends, send it immediately
		buf[buf_i++] = '\r'; buf[buf_i] = '\n'; // NULL terminate the string
		buf_i =0;
		nRF24L01_OnPacket(NULL, 5, (uint8_t*)buf, PAYLOAD_LEN);
		break;
	}
}


uint16_t _signalbuf[] = { 198, 99, 11, 22, 11, 44, 10, 23, 11, 50, 999, -1};

PulseCaptureBuf _signal = {
	100, 100, 12,
	_signalbuf
};

typedef void (*signal_f) (void* ctx, uint8_t val);

void signalPulses(Pulses* in, signal_f signaler, void* ctx)
{
	int i=0;
	uint8_t sig=0;

	if (NULL ==in || NULL == in->seq || NULL == signaler)
		return;

	for(; i< in->seqlen && in->seq[i] !=0xff; i++)
	{
		if (in->seq[i] >0x80)
		{
			signaler(ctx, sig); sig = sig ? 0:1;
			delayXusec((int32_t) in->short_durXusec * (0x7f & in->seq[i]));
		}
		else
		{
			signaler(ctx, sig); sig = sig ? 0:1;
			delayXusec((int32_t) in->short_durXusec * (in->seq[i] >>4));

			signaler(ctx, sig); sig = sig ? 0:1;
			delayXusec((int32_t) in->short_durXusec * (in->seq[i] & 0x0f));
		}
	}
}

#include "TimeWindowCounter.h"

uint8_t sample[] = {0x81, 0x80 +33, 
//			0x41, 0x41, 0x14, 0x14, 0x14, 0x14, 0x41, 0x41,  //1001=>9, 11000011=>c3
			0x14, 0x41, 0x14, 0x14, 0x14, 0x14, 0x41, 0x41,  //f001=>9, 01000011=>43
			0x14, 0x14, 0x41, 0x41, 0x14, 0x14, 0x41, 0x41,  //0101=>5, 00110011=>33
			0x14, 0x14, 0x14, 0x14, 0x41, 0x41, 0x14, 0x14,  //0010=>2, 00001100=>0c
			999, -1};

Pulses puls = {560, 26, sample};

void ASKDevice_processMessage(Pulses* captured);

void Pulse_test()
{
	uint32_t code;
	PulseDecode_PT2262(&puls, &code);
//	PulseEncode_PT2262(&puls, code);
//	PulseDecode_PT2262(&puls, &code);

	PulseDecode_EV1527(&puls, &code);
	PulseEncode_EV1527(&puls, code);
	PulseDecode_EV1527(&puls, &code);

//	ASKDevice_processMessage(&puls);

	ZQ::common::TimeWindowCounter twc(1000);
	for(int k=0; k < 99999999; k++)
	{
		printf("%d> c=%d\n", k, twc.count()); Sleep(1+k%20);
	}

	uint8_t sig[100];
	Pulses pulses; pulses.seq = sig;
	Pulse_parseSignals(&_signal, &pulses);


	char* hexstrs[] = {
		"0123", "11223344", "122\0123", "11aabbcc44", "22ccffgg", NULL
	};

	uint32_t val;
	for (int i =0; hexstrs[i]; i++)
	{
		const char* p = hex2int(hexstrs[i], &val);
		int len = p-hexstrs[i];
		printf("val[0x%x], len[%d]\n", val, len);
	}
}

const char* sdoparams[] = { "tempIntv", "lumineDiffIntv", "lumineDiffMask"};
char testv1[20] = "01", testv2[20] = "02", testv3[20] = "03";
const char* testvalues[] = { testv1, testv2, testv3 };
const char* pdoparams[] = { "gState", "masterId"};

static void WinSim_OnFillCanMsgParsed(HTCanMessage* pMsg, void* pCtx)
{
	HtCan_send((uint8_t)pCtx, pMsg);
}

static void test_dispatchTextMsg()
{
	char testmsg1[] = "POST can:590N5V2000200103",  // SDO_PUT by can
		testmsg2[] = "PUT ht://20/global?gstate=3", // SDO_PUT by ht
		testmsg3[] = "GET ht://10/global?gstate=", // SDO_GET by ht
		testmsg4[] = "";
	char* testmsgs[] = { testmsg1, testmsg2, testmsg3, NULL };

	// HtCan_parseRichURI(VERB_GET,  0x10, "ecuConfig",  3, sdoparams, testvalues, WinSim_OnFillCanMsgParsed, (void*) MSG_CH_CAN1);
	// HtCan_parseRichURI(VERB_PUT,  0x10, "ecuConfig",  3, sdoparams, testvalues, WinSim_OnFillCanMsgParsed, (void*) MSG_CH_CAN1);
	// HtCan_parseRichURI(VERB_POST, 0x10, "ecuConfig",  3, sdoparams, testvalues, WinSim_OnFillCanMsgParsed, (void*) MSG_CH_CAN1);
	// HtCan_parseRichURI(VERB_POST, 0x10, "gstateChgd", 2, pdoparams, testvalues, WinSim_OnFillCanMsgParsed, (void*) MSG_CH_CAN1);

	for (int i=0; testmsgs[i]; i++)
		GW_dispatchTextMessage(NULL, testmsgs[i]);
}


static const char* stateStr(StateOrdinalCode state)
{
#define CASE_STATE(_ST) case soc_##_ST:  return #_ST
	switch(GSTATE)
	{
//		CASE_STATE(Slave);
		CASE_STATE(Easy);
		CASE_STATE(PreGuard);
		CASE_STATE(Guard);
		CASE_STATE(SilentHours);
		CASE_STATE(PreAlarm);
		CASE_STATE(Alarm);
	}
#undef CASE_STATE

	return "UNKNOWN";
}

int main0(int argc, char* argv[])
{
	Task_Main       th_main;
	Task_Timer1msec th_1msec;
	Task_CO         th_co;

	trace("=============== WinSim_secu starts ============");

	msgProcer.start();
	com_admin.start();

	test_dispatchTextMsg();

	Sleep(1000);
	com_admin.send("\r\n*** start\r\n", 14); 

	char buf[1024];

	strcpy(buf, "POST 10/2000?gstate=1");
	GW_dispatchTextMessage(fdadm, buf);
	strcpy(buf, "POST can:590R02V112233"); // set SDO
	GW_dispatchTextMessage(fdadm, buf);

	sendWirelessTxtMsg("abcdefghij");
	sendWirelessTxtMsg("klmn\r\n");
	sendWirelessTxtMsg("1234");
	sendWirelessTxtMsg("567\r\n");

	th_1msec.start();

	HtCan_init(MSG_CH_CAN1);
	th_co.start();

	trace("*** th_main starts ");
	th_main.start();

	char c =0;
	while (!bQuit && 'q' != (c=getch())) // =getche())) // getc(stdin)))
	{
		switch(c)
		{
		case 'm': 
			printf("motion cleaned\n");
			Ecu_motionState = 0x0000;
			break;

		case 'i': 
			printf("inner motion issued\n");
			Ecu_motionState |= 0x0001;
			break;

		case 'b': 
			printf("bedroom motion issued\n");
			Ecu_motionState |= 0x0002;
			break;

		case 'o': 
			printf("outer motion issued\n");
			Ecu_motionState |= 0x0010;
			break;

		case 'l':
			printf("lumin changed issued\n");
			ChannelVals_Lumin[0] ^= 0x0fff;
			break;

		case 'e':
			printf("force to soc_Easy\n");
			//strcpy(buf, "PUT 10/2000?gstate=1"); 
			//GW_dispatchTextMessage(fdadm, buf); 
			State_enter(soc_Easy);
			break;

		case 'g':
			printf("force to soc_PreGuard\n");
			State_enter(soc_PreGuard);
			break;

		case 's':
			printf("force to soc_SilentHours\n");
			State_enter(soc_SilentHours);
			break;

		case 'n':
			printf("master's heartbeat to push into slave mode\n");
			strcpy(buf, "POST can:680N3V054000");
			GW_dispatchTextMessage(fdadm, buf);
			break;

		case 'p':
			printf("gstate[%s(%02x)] master[%02x] lstate[%04x/%04x] timeout[%d]\nlumin: ", stateStr((StateOrdinalCode)Ecu_globalState), Ecu_globalState, Ecu_masterNodeId, Ecu_localState, Ecu_motionState, Timeout_State);
			for (int i=0; i < CHANNEL_SZ_LUMIN; i++) printf("%d ", ChannelVals_Lumin[i]); printf("\ntemp: ");
			for (int i=0; i < CHANNEL_SZ_DS18B20; i++) printf("%d ", ChannelVals_Temperature[i]); printf("\n");
			break;
		}
	}

	bQuit = true;
	HtCan_stop(MSG_CH_CAN1);
	Sleep(1000);

	return 0;
}

int main1(int argc, char* argv[])
{
	char htmsg[][200] = {
		"POST /10/can?5a0N04V40022001",
		"POST /10/can?590N04V40022001",
		"POST /10/can?590N04V40022001",

		"GET /10/rBuf?b00=&b04=&b08=&b0c=&b10=&b14=&b18=&b1c=&b20=&b24=&b28=&b2c= HTTP/1.1",
		"GET ecuConfig?tempMask=&tempIntv=&motionDiffMask=&motionOuterMask=&motionBedMask=&motionDiffIntv=&lumineDiffMask=&lumineDiffIntv=&lumineDiffTshd=&fwdAdm=&fwdExt=; HTTP/1.1",
		"GET /11/ecuConfig?tempMask=&tempIntv=&motionDiffMask=&motionOuterMask=&motionBedMask=&motionDiffIntv=&lumineDiffMask=&lumineDiffIntv=&lumineDiffTshd=&fwdAdm=&fwdExt= HTTP/1.1",
		"POST can:590N04V40022001",
		"POST 10/2001?rqwrq=sdfsaf&safsf=we",
		"PUT 10/2001?rqwrq=sdfsaf&safsf=we",
		"GET 10/2001?rqwrq=sdfsaf&safsf=we",
		""
	};

	char* p, * q;
	uint16_t plen;
	uint32_t nodeId;

	for (int i=0; htmsg[i] && htmsg[i][0]; i++)
	{
		strcpy((char *)&(packetEth[TCP_PAYLOAD_P]), htmsg[i]);
		////////////////////////

					// terminate the URI string
					p = strstr((char *)&(packetEth[TCP_PAYLOAD_P]), " HTTP/1.");
					if (NULL !=p)
						*p= '\0';

					// fixup the URI
					p = strchr((char *)&(packetEth[TCP_PAYLOAD_P]), ' '); // the space after HTTP verb
					if (NULL ==p)
					{
						plen=fill_tcp_data_str(packetEth, 0, "HTTP/1.0 300 Bad Request\r\nContent-Type: text/html\r\n\r\n<h1>300 Bad Request</h1>");
						break;
					}

					if ('/' != *(++p))
					{
						// the uri is a relative URI, complete it with prefix ht:<thisNodeId>/
						for (q= p +strlen(p) +1; q >= p; q--)
							*(q+6) = *q;

						q++;
						*q++='h'; *q++='t'; *q++=':';
						*q++ = hexchar(HtCan_thisNodeId>>4);  *q++ = hexchar(HtCan_thisNodeId&0xf);
						*q++='/';

					}
					else
					{
						// absolute URI
						q = p;
						q = (char*)hex2int(++q, &nodeId); nodeId &=0x7f;
						if (nodeId == 0)
						{
							plen=fill_tcp_data_str(packetEth,0, "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Node Not Found</h1>");
							break;
						}

						if ((SUBNET_SEGMENT_MASK & HtCan_thisNodeId) != (SUBNET_SEGMENT_MASK & nodeId))
						{
							// this should redirect to another ECU, send the response "HTTP 1.0 302 Object Moved"
							p = (char *)&(packetEth[TCP_PAYLOAD_P + TCP_DATA_BODY_START]);
							sprintf(p, "http://%d.%d.%d.%d:%d/%02x%s", MyIP[0], MyIP[1], MyIP[2], NODEID_TO_IP_B4(nodeId), MyServerPort, nodeId, q);  
							plen=fill_tcp_data_str(packetEth,0, "HTTP/1.0 302 Object Moved\r\nLocation: ");
							plen=fill_tcp_data_str(packetEth,plen, p);
							plen=fill_tcp_data_str(packetEth,plen, "\r\n\r\n");
							break;
						}

						// fixup the URI with nodeId already presented
						for (q= p +strlen(p) +1; q > p; q--)
							*(q+2) = *q;

						*q++='h'; *q++='t'; *q++=':';
					}



		//////////////////////////
		packetDataPos = TCP_DATA_BODY_START;
		p = (char *)&(packetEth[TCP_PAYLOAD_P]);
		GW_dispatchTextMessage(fdeth, p);
		plen=fill_tcp_data_str(packetEth, 0, "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n{ ");
		p = (char *)&(packetEth[TCP_PAYLOAD_P + TCP_DATA_BODY_START]);
		while (' '==*p || '\t'==*p || ','==*p) p++;
		plen=fill_tcp_data_str(packetEth, plen, p);
		plen=fill_tcp_data_str(packetEth, plen, " }");
	}


	HtCan_init(MSG_CH_CAN1);

	Task_CO proc;
	proc.start();


	char c =0;
	while ('q' != (c=getc(stdin)))
	{
		switch(c)
		{
		case 's': sdoTest(); break;
		case 'p': pdoTest(); break;
		case 'q': bQuit =true; return 1;
		}
	}

	HtCan_stop(MSG_CH_CAN1);
	Sleep(1000);

	//	pdoTest();

	// recvTest();
	return 0;
}

void usage()
{
	printf("Usage: secu [-h]\n");
	printf("options:\n");
	printf("\t-h   display this help\n");
}


void OnWirelessTextMsg(uint32_t localNodeId, uint8_t portNum, const char* txtMsg)
{
	trace("wirelessmsg[%s]", txtMsg);
}

#define nRF24L01_RX_PIPES 6 

void nRF24L01_OnPacket(void* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen)
{
	static char wtxtMsg[nRF24L01_RX_PIPES-1][MAX_MSGLEN];
	static uint8_t wtxtMsg_i[nRF24L01_RX_PIPES-1]={0,0,0,0,0};

#define buf_i (wtxtMsg_i[pipeId-1])
	char* buf = wtxtMsg[pipeId-1];

	for (; playLoadLen>0 && *bufPlayLoad; playLoadLen--)
	{
		buf[buf_i] = *bufPlayLoad++;
		if ('\r' != buf[buf_i] && '\n' != buf[buf_i])
		{
			buf_i = ++buf_i % (MAX_MSGLEN-2);
			continue;
		}

		if (0 == buf_i) // empty line
			continue;

		// a line just received
		buf[buf_i++] = '\0'; buf[buf_i] = '\1'; // NULL terminate the string
		buf_i =0;		
		break;
	}

	if ('\0' != buf[0] && 0 == buf_i) // a valid message with complete line
		OnWirelessTextMsg(0x12345678, pipeId -1, (char*)buf);
}

uint32_t EcuCtrl_askDevState;
uint32_t EcuCtrl_askDevTable[ASK_DEVICES_MAX]={0x123, 0x0043330c,}; // the code table of known devices
uint8_t EcuCtrl_askDevTypes[ASK_DEVICES_MAX]={3, 1,};

uint16_t ENC28J60_recvPacket(ENC28J60* nic, uint16_t maxlen, uint8_t* packet)
{
	return 0;
}

void NIC_sendPacket(void* nic, uint8_t* packet, uint16_t packetLen)
{
}


#include "Counter.h"
int main(int argc, char* argv[])
{
	ZQ::common::FloatWindowCounters fwc(10000, 4);
	for (int i=999999; i>0; i--)
	{
		fwc.addCount((i %4));

		printf("sum=%d: z=%d 1=%d 2=%d 3=%d\n", fwc.getCounter(), fwc.getCounter(0), fwc.getCounter(1), fwc.getCounter(2), fwc.getCounter(3));
		Sleep(100);
	}

}
