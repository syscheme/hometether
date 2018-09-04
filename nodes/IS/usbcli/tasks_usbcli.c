// --------  includes  ------------
#include  "../includes.h"
#include "pulses.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"

// --------  Private typedef  ------------
// --------  Private define  ------------
// --------  Private macro  ------------
// --------  Private variables  ------------
// --------  Extern variables  ------------

//#ifdef DEBUG
#  define MIN_ANN_INTERVAL   (15*1000) // 15sec
#  define HEARTBEAT_INTERVAL (2*1000) // 2sec
//#else
//#  define MIN_ANN_INTERVAL   (60*1000)  // 1min
//#  define HEARTBEAT_INTERVAL (10*1000)  // 10sec
//#endif
// -----------------------------
// global declares
// -----------------------------
static uint32_t nodeId = 0x01010102;
#define RF_NODEID                      (0x50150000) // the RF id of this node

// task priorities
#define TaskPrio_UsbRecv                2
#define TaskPrio_LocalStatus           10
#define TaskPrio_MsgProc                3

//task stack sizes
#define TaskStackSize_UsbRecv          40
#define TaskStackSize_LocalStatus      100
#define TaskStackSize_MsgProc          100

//task stacks
static OS_STK TaskStack_UsbRecv[TaskStackSize_UsbRecv];
static OS_STK TaskStack_LocalStatus[TaskStackSize_LocalStatus];
static OS_STK TaskStack_MsgProc[TaskStackSize_MsgProc];

//tasks
static  void Task_UsbRecv(void* p_arg);
static  void Task_LocalStatus(void* arg);
static  void Task_MsgProc(void* arg);

uint16_t ADC_DMAbuf[ADC_CHS];

uint32_t RF_masterId   = 0x12345678;
uint8_t  RF_masterPort = 0x00;

extern u32 count_out;
extern u8 buffer_out[VIRTUAL_COM_PORT_DATA_SIZE];

nRF24L01 nrf24l01 = { RF_NODEID, NULL, SPI2, {GPIOB, GPIO_Pin_1}, {GPIOB, GPIO_Pin_0}, EXTI_Line8, NULL};
void sendTxtMsgToNRF(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg);
void sendMsgToUsb(const char* msg, uint8_t len);

#define MAX_MSGLEN    100
#define MAX_PENDMSGS (5)

typedef struct _PendTxtMsg
{
	uint8_t chId, offset, timeout;
	char msg[MAX_MSGLEN];
} PendTxtMsg;

PendTxtMsg _pendTxtMsgs[MAX_PENDMSGS]; //  = { {} };
void OnMessageReceived(uint8_t pipeId, uint8_t* inBuf, uint8_t maxLen);

#define RecvChId_USB_BASE (0xf0)

uint8_t irseq[50];
Pulses pulses = {50, 0, sizeof(irseq), irseq};
char txBuf[80];

// --------  Private function prototypes  ------------
// --------  Private functions  ------------
void InitProcedure(void);
void DataToUSB_Send(void);

#ifdef  DEBUG
// -----------------------------
// assert_failed()
// Description : Reports the name of the source file and the source line number
//               where the assert_param error has occurred.
// Argument    : - file: pointer to the source file name
//               - line: assert_param error line source number
// Return      : none
// Note        : none.
// -----------------------------
void assert_failed(u8* file, u32 line)
{
  // User can add his own implementation to report the file name and line number,
  //  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line)

  // Infinite loop
  while(1)
  {
  }
}
#endif // DEBUG

// -----------------------------
// main()
// Description : This is the standard entry point for C code.  It is assumed that your code will call
//               main() once you have performed all necessary initialization.
// Argument: none.
// Return   : integer code
// Note    : none.
// -----------------------------
int main(void)
{
	CPU_INT08U os_err;

#ifdef DEBUG
	debug();
#endif // DEBUG

	BSP_IntDisAll();    // disable all ints until we are ready to accept them.
	OSInit();           // initialize "uC/OS-II, The Real-Time Kernel".
	BSP_Init();         // initialize BSP functions.
	BSP_IntInit();      // initialize BSP interrupt ISRs

	InitProcedure();

	os_err = OSTaskCreate((void(*) (void *)) Task_UsbRecv,   // create MyTask.
		NULL, (OS_STK *) &TaskStack_UsbRecv[TaskStackSize_UsbRecv -1], (INT8U) TaskPrio_UsbRecv);

#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(TaskPrio_UsbRecv, (CPU_INT08U*) Task_UsbRecv, &os_err);
#endif

	OSTimeSet(0);
	OSStart(); // start multitasking (i.e. give control to uC/OS-II)

	return 0;
}

void InitProcedure(void)
{
	int i;

	// blicking the leds
	for (i =0; i<8; i++)
	{
		ISBoard_setLed(0, 1); delayXusec(100000); ISBoard_setLed(0, 0); 
		ISBoard_setLed(1, 1); delayXusec(100000); ISBoard_setLed(1, 0); 
		ISBoard_setLed(2, 1); delayXusec(100000); ISBoard_setLed(2, 0); 
		ISBoard_setLed(3, 1); delayXusec(100000); ISBoard_setLed(3, 0);
	}
}

// -----------------------------
// Task_UsbRecv()
// Description : The startup task. The uC/OS-II ticker should only be initialize once multitasking starts.
// Argument: p_arg argument passed to 'Task_UsbRecv()' by 'OSTaskCreate()'.
// Return  : none.
// Caller  : This is a task.
// Note    : none.
// -----------------------------
static void Task_UsbRecv(void* p_arg)
{
	CPU_INT08U os_err;
	(void) p_arg;

	OS_CPU_SysTickInit();       // initialize the SysTick

#if (OS_TASK_STAT_EN > 0)
	OSStatInit();               // Determine CPU capacity.
#endif

	os_err = OSTaskCreate((void (*) (void *)) Task_LocalStatus, (void *) NULL,
			(OS_STK *) &TaskStack_LocalStatus[TaskStackSize_LocalStatus -1], (INT8U) TaskPrio_LocalStatus);

	os_err = OSTaskCreate((void (*) (void *)) Task_MsgProc, (void *) NULL,
			(OS_STK *) &TaskStack_MsgProc[TaskStackSize_MsgProc -1], (INT8U) TaskPrio_MsgProc);

	//!!USB: Set_System();
	Set_USBClock();
	//!!USB: USB_Interrupts_Config();
	USB_Init();

	while (1)
	{
    	if (0 == count_out)
		{
			OSTimeDlyHMSM(0, 0, 0, 50);
			continue;
		}

		OnMessageReceived(RecvChId_USB_BASE, buffer_out, count_out);
		count_out = 0;
		OSTimeDlyHMSM(0, 0, 0, 1);
	}
}

// -----------------------------
// Task_MsgProc()
// Description : the task to process incoming messages
// Argument: p_arg argument from call-up task Task_UsbRecv()
// Return  : none.
// Caller  : Task_UsbRecv()
// Note    : none.
// -----------------------------
static void Task_MsgProc(void* p_arg)
{
	int i;
	uint32_t val=0;
	uint8_t  c=0;
	const char* p =NULL, *q=NULL;

	(void) p_arg;

	while (1)
	{
	// scan if there are any pending wireless msgs need to be forwarded or processed
	for (i =0, c=0; i < MAX_PENDMSGS; i++)
	{
		if (0 == _pendTxtMsgs[i].timeout || _pendTxtMsgs[i].offset >0)
			continue;  // idle or incomplete message

		// process a completed pending message
		//step 0. reserve timeout to hold this message during processing
		_pendTxtMsgs[i].timeout = 0xff;	c++;
		
		val = strlen(_pendTxtMsgs[i].msg);
		if (val<=0) { _pendTxtMsgs[i].timeout =0; continue; }
		
		//step 1. forward msg to USB if it was not originally from USB itself
		if (_pendTxtMsgs[i].chId != RecvChId_USB_BASE)
			sendMsgToUsb(_pendTxtMsgs[i].msg, val);

		// step 2. check if it is about the local node 
		p =	strstr(_pendTxtMsgs[i].msg, "POST "); if (NULL == p) { _pendTxtMsgs[i].timeout =0; continue; }
		p = strstr(p, "/RF_master?");
		if (NULL != p)
		{ 
			// message to configure the RF_master
			p += sizeof("/RF_master?")-1;
			q = strstr(p, "node=");
			if (NULL != q)
			{
				q+= sizeof("node=")-1;
				hex2int(q, &val);  RF_masterId = val;
			}

			q = strstr(p, "port=");
			if (NULL != q)
			{
				q+= sizeof("port=")-1;
				hex2int(q, &val);  RF_masterPort = val %5;
			}
		}

		//step End. reset timeout to be idle after processing
		_pendTxtMsgs[i].timeout = 0;
	}

	ISBoard_setLed(1, c?1:0);// blink LED1
	OSTimeDlyHMSM(0, 0, 0, c?8:100);
	}
}

#define tick2msec(_TICKS) ((_TICKS)*(OS_TICKS_PER_SEC +999) /1000)

// -----------------------------
// Task_LocalStatus()
// Description : the task to poll on board status, report if necessary
// Argument: none.
// Return   : none.
// Caller   : Task_UsbRecv().
// Note    : none.
// -----------------------------
static void Task_LocalStatus(void* arg)
{
	uint16_t temperature, temperatureChip;
	int i=0;
	uint32_t stampNow=0, stampLastAnn =0, stampBlink =0; // 500msec
	uint8_t  lastMotion = 0, tmpU8;
	bool bAnnNeeded = FALSE, bIrRecvd = FALSE;
	char *p=NULL;

	memset(_pendTxtMsgs, 0x00, sizeof(_pendTxtMsgs));
	nRF24L01_init(&nrf24l01, RF_NODEID, NULL, TRUE);

	while (1)
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		stampNow = OSTimeGet();

		bIrRecvd = FALSE;
		if (pulseCapBuf.timeout >0)
		{
			pulses.seqlen = sizeof(irseq) -2;
			bIrRecvd = Pulse_compress(&pulseCapBuf, &pulses);
			pulseCapBuf.timeout = 0;
		}

		if (bIrRecvd)
		{
			p = txBuf;
			sprintf(p, "POST %x/irrecv?bintv=%x&sdur=%x&seqlen=%x&seq=", nodeId, pulses.baseIntvX10usec, pulses.short_dur, pulses.seqlen);
			p += strlen(p);
			for(i=0;i<pulses.seqlen;i++)
			{
				sprintf(p, "%02x.", pulses.seq[i]); p+=strlen(p);
			}
			sprintf(--p, "\r\n");

			sendMsgToUsb(txBuf, strlen(txBuf));
		}

		bAnnNeeded = FALSE;

		// step 1. test if the motion state has been changed
		tmpU8 = motionState();
		if (tmpU8 != lastMotion)
			bAnnNeeded = TRUE;
		
		lastMotion = tmpU8;

		stampNow = OSTimeGet();
		if (tick2msec((int32_t)stampNow - stampLastAnn) > MIN_ANN_INTERVAL)
			bAnnNeeded = TRUE;
			
		if (tick2msec((int32_t)stampNow - stampBlink) > HEARTBEAT_INTERVAL) //blink led3
		{ ISBoard_setLed(3, 1); OSTimeDlyHMSM(0, 0, 0, 10); ISBoard_setLed(3, 0); stampBlink =stampNow; }
					
		if (!bAnnNeeded)
			continue;

		ISBoard_setLed(0, 1);
		temperature = DS18B20_read(&ds18b20);
		// tempadc = (1.43 - tempadc*3.3/4096)*1000/4.35 +25; // convert adc value to temperature (in ¡æ) = {(V25 - VSENSE) / Avg_Slope} + 25, where V25µäÐÍ=1.43V, Avg_SlopeµäÐÍ=4.3mV/¡æ
		// tempadc = (143 - tempadc*330/4096)*10000/435 +250; // in 0.1C
		temperatureChip = (1718- (int32_t) chipTemperature) * 330/4096*10000/435 + 140; // in 0.1C: measure data: adc=1718 when the temp is 14C
		//		adc  = ADC_GetConversionValue(ADC1);

		sprintf(txBuf, "POST %x/status?temp=%d&tempChip=%d&lumin=%d&motion=%d\r\n",
				nodeId, temperature, temperatureChip, luminVal, (lastMotion?1:0));

		sendMsgToUsb(txBuf, strlen(txBuf));
		sendTxtMsgToNRF(&nrf24l01, RF_masterId, RF_masterPort, txBuf);
		ISBoard_setLed(0, 0);

		stampLastAnn = stampNow;
	}
}

void sendTxtMsgToNRF(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg)
{
	static char buf[nRF24L01_MAX_PLOAD +2];
	static uint8_t buf_i;
	bool   bEOL = FALSE;

	if (chip->peerNodeId != peerNodeId || chip->peerPortNum != peerPortNum)
	{
		if (buf_i >0)
		{
			// flush the pending message to the peer right the way
			buf[buf_i++] = '\0';
			nRF24L01_setModeTX(chip, peerNodeId, peerPortNum);
			nRF24L01_TxPacket(chip, (uint8_t*)buf);
		}

		buf_i = 0;
		chip->peerNodeId  = peerNodeId;
		chip->peerPortNum = peerPortNum;
		nRF24L01_setModeRX(chip); // ensure to close the current send-to connection, will to be TX when next tx
	}

	for (; *txtMsg; txtMsg++)
	{
		buf[buf_i] = *txtMsg;
		bEOL = ('\r' == buf[buf_i] || '\n' == buf[buf_i]);

		if (!bEOL)
		{
			buf_i = (++buf_i) % nRF24L01_MAX_PLOAD;

			if (0 == buf_i)
			{
				// playload reached, send it immediately
				buf[nRF24L01_MAX_PLOAD] = 0;
				nRF24L01_setModeTX(chip, peerNodeId, peerPortNum);
				nRF24L01_TxPacket(chip, (uint8_t*) buf);
			}

			continue;
		}

		if (0 == buf_i && !bEOL) // empty line, continue
			continue;

		// a line just ends, send it
		if (buf_i < nRF24L01_MAX_PLOAD -3) // re-fill in the line-end, any one of the charactors is enough
		{ buf[buf_i++] = '\r'; buf[buf_i++] = '\n'; }
		buf[buf_i] = '\0';

		buf_i =0;
		nRF24L01_setModeTX(chip, peerNodeId, peerPortNum);
		nRF24L01_TxPacket(chip, (uint8_t*) buf);
		break;
	}

	nRF24L01_setModeRX(chip);
}

void sendMsgToUsb(const char* msg, uint8_t len)
{
	if (NULL ==msg || len<=0)
		return;

	UserToPMABufferCopy((u8*)msg, ENDP1_TXADDR, len);
	SetEPTxCount(ENDP1, len);
	SetEPTxValid(ENDP1);
}

void nRF24L01_OnPacket(nRF24L01* chip, uint8_t pipeId, uint8_t* inBuf, uint8_t maxLen)
{
	OnMessageReceived(pipeId, inBuf, maxLen);
}

void OnMessageReceived(uint8_t chId, uint8_t* inBuf, uint8_t maxLen)
{
	int i =0;
	PendTxtMsg* thisMsg = NULL;

	for (i =0; i < MAX_PENDMSGS; i++)
	{
		if (0 != _pendTxtMsgs[i].timeout)
		{
			 _pendTxtMsgs[i].timeout--;
			 if (_pendTxtMsgs[i].chId == chId && _pendTxtMsgs[i].offset >0)
			 	thisMsg = &_pendTxtMsgs[i]; // continue with the recent incomplete receving;
		}
		else if (NULL == thisMsg) // take the first idle pend msg
			thisMsg = &_pendTxtMsgs[i];
	}

	// ISBoard_setLed(1, bHasPending?1:0);// blink LED1

	if (NULL == thisMsg) // failed to find an available idle pend buffer, simply give up this receiving
		return;

 #define buf_i (thisMsg->offset)
 #define buf   (thisMsg->msg)

 	thisMsg->chId  = chId;
	thisMsg->timeout = (MAX_PENDMSGS<<2)+3; // refill the timeout as 4 times of the buffer

	for (; maxLen>0 && *inBuf; maxLen--)
	{
		buf[buf_i] = *inBuf++;

		// a \0 is known as the data end of this received frame
		if ('\0' == buf[buf_i])
			break;

		// an EOL would be known as a message end
		if ('\r' != buf[buf_i] && '\n' != buf[buf_i])
		{
			buf_i = (++buf_i) % (MAX_MSGLEN-3);
			continue;
		}

		// a line just received
		if (buf_i < MAX_MSGLEN -3) // re-fill in the line-end if the buffer is enough
		{ buf[buf_i++] = '\r'; buf[buf_i++] = '\n'; }
		
		buf[buf_i] = '\0'; // NULL terminate the string
		buf_i =0; // reset for next receiving

		if ('\0' == buf[0]) // empty line, ignore
			continue;

		// a valid line of message here
		// OnWirelessTextMsg(chip->nodeId, pipeId-1, (char*)buf);
		break;
	}
}


#if (OS_APP_HOOKS_EN > 0)
// -----------------------------
// uC/OS-II APP HOOKS
// -----------------------------

// -----------------------------
// App_TaskCreateHook()
// Description : This function is called when a task is created.
// Argument: ptcb   is a pointer to the task control block of the task being created.
// Note    : (1) Interrupts are disabled during this call.
// -----------------------------
void App_TaskCreateHook(OS_TCB* ptcb)
{
}

// -----------------------------
// App_TaskDelHook()
// Description : This function is called when a task is deleted.
// Argument: ptcb   is a pointer to the task control block of the task being deleted.
// Note    : (1) Interrupts are disabled during this call.
// -----------------------------
void App_TaskDelHook(OS_TCB* ptcb)
{
	(void) ptcb;
}

// -----------------------------
// App_TaskStatHook()
// Description : This function is called by OSTaskStatHook(), which is called every second by uC/OS-II's
//               statistics task.  This allows your application to add functionality to the statistics task.
// -----------------------------
void App_TaskStatHook(void)
{
}

#if OS_TASK_SW_HOOK_EN > 0
// -----------------------------
// App_TaskSwHook()
// Description : This function is called when a task switch is performed.  This allows you to perform other
//               operations during a context switch.
// Note    : (1) Interrupts are disabled during this call.
//           (2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
//               will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
//               task being switched out (i.e. the preempted task).
// -----------------------------
void App_TaskSwHook(void)
{
}
#endif


#if OS_VERSION >= 251
// -----------------------------
// App_TaskIdleHook()
// Description : This function is called by OSTaskIdleHook(), which is called by the idle task.  This hook
//               has been added to allow you to do such things as STOP the CPU to conserve power.
// Note    : (1) Interrupts are disabled during this call.
// -----------------------------
void App_TaskIdleHook(void)
{
}
#endif

#if OS_VERSION >= 204
// -----------------------------
// App_TCBInitHook()
// Description : This function is called by OSTCBInitHook(), which is called by OS_TCBInit() after setting
//               up most of the TCB.
// Argument : ptcb    is a pointer to the TCB of the task being created.
// Note    : (1) Interrupts are disabled during this call.
// -----------------------------
void App_TCBInitHook(OS_TCB* ptcb)
{
	(void) ptcb;
}
#endif

#endif


typedef enum _DeviceStateFlag
{
	// the lowerest 4bits is the StateOrdinalCode
	dsf_MotionChanged   = (1<<0),
	dsf_LuminChanged    = (1<<1),
	dsf_IrTxEmpty       = (1<<2),
	dsf_IrRxAvail       = (1<<3),
	dsf_StateTimer      = (1<<4),
	dsf_GasLeak         = (1<<5),
	dsf_Smoke           = (1<<6),
	dsf_OtherDangers    = (1<<7),

	dsf_LocalMaster     = (1<<15) // the local ECU work as the master according to election
} DeviceStateFlag;

uint32_t _deviceState =0;
