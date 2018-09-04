#ifndef __ECU_SECU_H__
#define __ECU_SECU_H__

#include "htcomm.h"
#include "../ht.h"
#include "../htod.h"
#include "ds18b20.h"
#include "usart.h"

#include "FreeRTOS.h"
#include "htcluster.h"

#define NEXT_SLEEP_DEFAULT   (200) // 200msec

#define MAX_NIC_SEND_FAIL    (5)	  // the continuous NIC send fail before trigger NIC reset

#define PORT_WWW              80
#define GROUP_PORT_BASE       5204
#define GROUP_PORT_Adm        (GROUP_PORT_BASE)
#define GROUP_PORT_CanOverIP  (GROUP_PORT_BASE+1)
#define THIS_IP               {192,168,0,NODEID_TO_IP_B4(HT_NODEID)}
#define GROUP_IP              {239,0,0,55}
#define HLK_TCP_SERVE_PORT    (2168)

// NodeId is 7bit, its higher 3bit defines the ID of ECU, and the lower
// 4bit defines the extension nodes connected to this ECU thru interface fdext
// b7             0 
// +-+-+-+-+-+-+-+
// | ecu |  ext  |
// +-+-+-+-+-+-+-+
#define SUBNET_SEGMENT_MASK (0x70)
#define SUBNET_SEGMENT_BITS (4)
#define NODEID_TO_IP_B4(_NODEID)           (0xa0 | (_NODEID>>SUBNET_SEGMENT_BITS)) // start since decimal 160

enum _TxtMsg_ChId
{
	// yield 0~5 for nRF channels
	MSG_CH_CAN1            =6,
	MSG_CH_RS232_Received  =8, 
	MSG_CH_RS232_SendTo,
	MSG_CH_RS485_Received,
	MSG_CH_RS485_SendTo,
	BASE_IP_SOCKET         =0x20, // the socketId
};

extern uint8_t fdadm;
extern uint8_t fdext;
extern uint8_t fdcan;

extern const uint8_t  MyIP[4];  // = THIS_IP;
extern const uint16_t MyServerPort; // PORT_WWW
extern const uint8_t  GroupIP[4]; // = GRUP_IP

// about global state word flags
#define _gState         Ecu_globalState

#define DEBUG_CONST  (dsf_Debug & _deviceState)

// about local state flags
#define _deviceState    Ecu_localState
#define _gState         Ecu_globalState

#define MotionMask_Outer    0x03

// -------------------------------------------------------------------------------
// SECU functions
// -------------------------------------------------------------------------------
void SECU_init(bool debugMode);
void SECU_scanDevice(void);
void SECU_setRelay(uint8_t id, uint8_t on);

// -------------------------------------------------------------------------------
// SECU task confs
// -------------------------------------------------------------------------------
#define TaskPrio_Base		         (tskIDLE_PRIORITY + 2) // the application-wide lowest priority
#define TaskPrio_Main                (TaskPrio_Base +0)
#define TaskPrio_Timer               (TaskPrio_Base +1)
#define TaskPrio_Comm                (TaskPrio_Base +3)


#define TaskStkSz_Main               (configMINIMAL_STACK_SIZE) //96
#define TaskStkSz_Timer              (configMINIMAL_STACK_SIZE)
#define TaskStkSz_Comm               (configMINIMAL_STACK_SIZE+100)

// -------------------------------------------------------------------------------
// SECU task steps
// -------------------------------------------------------------------------------
// Steps
void InitProcedure(void);
void ThreadStep_doNicProc(void);
int  ThreadStep_doMsgProc(void);
void ThreadStep_doMainLoop(void);

// APIs about message gateway
void GW_dispatchTextMessage(uint8_t fdin, char* msg);
void AdminExt_sendMsg(uint8_t fdout, char* msg, uint8_t len);

// =========================================================================
// External IO Resources
// =========================================================================
#define CHANNEL_SZ_DS18B20     (8)
#define CHANNEL_SZ_MOTION     (10)
#define CHANNEL_SZ_IrLED       (4)
#define CHANNEL_SZ_IrRecv      (4)
#define CHANNEL_SZ_Relay       (4)
#define CHANNEL_SZ_LUMIN       (8)

#ifndef CHANNEL_SZ_ADC
#  define CHANNEL_SZ_ADC          CHANNEL_SZ_LUMIN	+1
#  define CHANNEL_SZ_Temperature  (8)
#endif // CHANNEL_SZ_ADC

extern DS18B20 ds18b20s[CHANNEL_SZ_DS18B20];

uint16_t MotionState(void);

// about the ENC18J60 if adapted
#ifdef ECU_WITH_ENC28J60 
#  include "enc28j60.h"
#  include "ip_arp_udp_tcp.h"
#  include "net.h"

extern uint8_t packetEth[ENC28J60_MTU_SIZE+1];
extern ENC28J60 nic;
#endif // ECU_WITH_ENC28J60

// per profile IrSend()s
void IrSend_PT2262(uint8_t chId, uint32_t code);
void IrSend_uPD6121G(uint8_t chId, uint16_t customCode, uint8_t data);

void ThreadStep_do1msecTimerProc(void);
int  ThreadStep_doStateScan(int nextSleep);
void ThreadStep_doRecvTTY(uint8_t chId, char* buf, uint8_t byteRead, uint8_t* pStartOffset);
int  ThreadStep_doMsgProc(void);
void ThreadStep_doCanMsgIO(void);

// -----------------------------
// task declares
// -----------------------------
void Task_Main(void* p_arg);
void Task_Timer(void* p_arg);
void Task_CO(void* p_arg);
void Task_Communication(void* p_arg);

#endif // __ECU_SECU_H__
