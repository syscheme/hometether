#include "includes.h"
#include "pulses.h"

#define RECVR
// #define NRF2401_POLL

#ifdef RECVR
#   define RF_NODEID_THIS (0x12345678)
#   define RF_NODEID_PEER (0xaabbccdd)
#else
#   define RF_NODEID_THIS (0xaabbccdd)
#   define RF_NODEID_PEER (0x12345678)
#endif

// #   define RF_NODEID_THIS (0x12345678)
// #   define RF_NODEID_PEER (0x12345678)

#define RF_PORTNUM (0)
// -----------------------------
// global declares
// -----------------------------
// task priorities
#define TaskPrio_Start                    10
#define TaskPrio_LED_Base                 11

//task stack sizes
#define TaskStackSize_Start               40
#define TaskStackSize_LED                 20
#define TaskStackSize_Poll                20

//task stacks
static OS_STK TaskStack_Start[TaskStackSize_Start];

typedef struct _TaskEnv_LED
{
	uint8_t id;
	OS_STK taskStk[TaskStackSize_LED];
} TaskEnv_LED;

static TaskEnv_LED taskEnv_Leds[4];



//tasks
static  void Task_Start(void* p_arg);
static  void Task_LED(void* p_arg);

#define TaskPrio_Timer          (2)
#define TaskStkSz_Timer       (40)
static OS_STK TaskStk_Timer[TaskStkSz_Timer];
static void Task_Timer(void* p_arg);

nRF24L01 nrf24l01 = { RF_NODEID_THIS, NULL, SPI2, {GPIOB, GPIO_Pin_1}, {GPIOB, GPIO_Pin_0}, EXTI_Line8, NULL};
void sendWirelessTxtMsg(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg);

uint16_t ADC_DMAbuf[ADC_CHS];

// -----------------------------
// main()
// Description : This is the standard entry point for C code.  It is assumed that your code will call
//               main() once you have performed all necessary initialization.
// Argument: none.
// Return   : integer code
// Note    : none.
// -----------------------------
static uint32_t nodeId = 0x01010102;
int main(void)
{
	CPU_INT08U os_err;

	BSP_IntDisAll();    // disable all ints until we are ready to accept them.
	OSInit();           // initialize "uC/OS-II, The Real-Time Kernel".
	BSP_Init();         // initialize BSP functions.
	BSP_IntInit();      // initialize BSP interrupt ISRs

	os_err = OSTaskCreate((void(*) (void *)) Task_Start,   // create MyTask.
		NULL, (OS_STK *) &TaskStack_Start[TaskStackSize_Start -1], (INT8U) TaskPrio_Start);

#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(TaskPrio_Start, (CPU_INT08U*) Task_Start, &os_err);
#endif

	OSTimeSet(0);
	OSStart();          // start multitasking (i.e. give control to uC/OS-II)

	return 0;
}

uint16_t temp, adc, tempadc;
char txBuf[256];


// -----------------------------
// Task_Start()
// Description : The startup task.  The uC/OS-II ticker should only be initialize once multitasking starts.
// Argument: p_arg       Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Caller   : This is a task.
// Note    : none.
// -----------------------------
PulseCode pulseCode;
void Task_Start(void* p_arg)
{
	INT8U i;
	uint32_t tickLast =0, tickNow=0;
	int32_t msecDiff;
	char *p =NULL;
	CPU_INT08U os_err;

	bool bIrRecvd = FALSE;
	(void) p_arg;

	OS_CPU_SysTickInit();       // initialize the SysTick

#if (OS_TASK_STAT_EN > 0)
	OSStatInit();               // Determine CPU capacity.
#endif

	// create the Task_Timer()
	os_err = OSTaskCreate((void (*) (void *)) Task_Timer, NULL,
		(OS_STK *) &TaskStk_Timer[TaskStkSz_Timer -1], (INT8U)TaskPrio_Timer);

	for (i=3; i <4; i++)
	{
		taskEnv_Leds[i].id = i;

		os_err = OSTaskCreate((void (*) (void *)) Task_LED, (void *) &taskEnv_Leds[i].id,
	               (OS_STK *) &taskEnv_Leds[i].taskStk[TaskStackSize_LED - 1],
	               (INT8U) TaskPrio_LED_Base +i);
	
		OSTimeDlyHMSM(0, 0, 0, 500);
	}

	BSP_Start();

	nRF24L01_init(&nrf24l01, RF_NODEID_THIS, NULL, TRUE);

#ifdef RECVR
	nRF24L01_setModeRX(&nrf24l01);
	OSTimeDlyHMSM(0, 0, 2, 0);
	nRF24L01_OnPacket(&nrf24l01, 2, NULL, 1);
#else
// 	memcpy(nrf24l01.tx, &nodeId, nRF24L01_ADDR_LEN);
	nRF24L01_setModeTX(&nrf24l01, RF_NODEID_PEER, RF_PORTNUM);
#endif

	tickLast = OSTimeGet();

	while (1)
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		tickNow = OSTimeGet(); 	msecDiff = tickNow - tickLast;
		msecDiff *= ((OS_TICKS_PER_SEC +999) /1000); // round up for diff in msec
		bIrRecvd = FALSE; 

		if (pendingPulseSeq.timeout >0)
		{
			if (Pulses_decodeEx(&pendingPulseSeq.ps, &pulseCode))
			{
				sprintf(txBuf, "POST %x/pulse?en=%s&value=%d\r\n", nodeId, Pulse_profName(pulseCode.profId), pulseCode.code);
				sendWirelessTxtMsg(&nrf24l01, RF_NODEID_PEER, RF_PORTNUM, txBuf);
			}

			pendingPulseSeq.timeout =0;
		}

/*
		if (bIrRecvd)
		{
			p = txBuf;
			sprintf(p, "POST %x/irrecv?bintv=%x&sdur=%x&seqlen=%x&seq=", nodeId, pulses.short_durXusec, pulses.seqlen);
			p += strlen(p);
			for(i=0;i<pulses.seqlen;i++)
			{
				sprintf(p, "%02x.", pulses.seq[i]); p+=strlen(p);
			}
			sprintf(--p, "\r\n");

 			sendWirelessTxtMsg(&nrf24l01, RF_NODEID_PEER, RF_PORTNUM, txBuf);
		}
 */

#ifdef RECVR
		continue;
#endif
		if (msecDiff <5000)
			continue;
 		tickLast = tickNow;

		temp = DS18B20_read(&ds18b20);
		tempadc = ADC_DMAbuf[0];
		// tempadc = (1.43 - tempadc*3.3/4096)*1000/4.35 +25; // convert adc value to temperature (in ¡æ) = {(V25 - VSENSE) / Avg_Slope} + 25, where V25µäÐÍ=1.43V, Avg_SlopeµäÐÍ=4.3mV/¡æ
		tempadc = (143 - tempadc*330/4096)*10000/435 +250; // in 0.1C
		tempadc = (1718- (int32_t) chipTemperature) * 330/4096*10000/435 + 140; // in 0.1C: measure data: adc=1718 when the temp is 14C
		//		adc  = ADC_GetConversionValue(ADC1);

		sprintf(txBuf, "POST %x/status?temp=%d&lumin=%d&itemp=%d\r\n", nodeId, temp, luminVal, chipTemperature);

 		// sprintf(txBuf, "0123456789a123456789b123456789\r\n");

//		ISBoard_setLed(0, 1);

//		nRF24L01_setModeTX(&nrf24l01);
		// nRF24L01_TxPacket(&nrf24l01, txBuf);
		sendWirelessTxtMsg(&nrf24l01, RF_NODEID_PEER, RF_PORTNUM, txBuf);
		// OSTimeDlyHMSM(0, 0, 1, 0);
//		nRF24L01_setModeRX(&nrf24l01, TRUE);

		// blink led0
//		ISBoard_setLed(0, 0);

	}
}

// -----------------------------
// Task_Timer()
// Description : the task as the 1 msec timer to drive CanFestival's timer portal
// Argument: none.
// Return: none.
// -----------------------------
#define DEC_TIMEOUT(_TO, _BY)	if (_TO >_BY) _TO -= _BY; else _TO =0;

void Task_Timer(void* arg)
{
	uint32_t tickLast =0, tickNow=0;
	int32_t msecDiff;

	tickLast = OSTimeGet();

	while (1)
	{
		OSTimeDlyHMSM(0, 0, 0, 10);

		tickNow = OSTimeGet(); 	msecDiff = tickNow - tickLast; 
		msecDiff *= ((OS_TICKS_PER_SEC +999) /1000); // round up for diff in msec

		tickLast = tickNow;
		if (msecDiff <=0)
			continue;

		DEC_TIMEOUT(pendingPulseSeq.timeout,    msecDiff);
	}

	tickNow = OSTimeGet();
}

// -----------------------------
// Task_LED()
// Description : Create the application tasks.
// Argument: none.
// Return   : none.
// Caller   : Task_Start().
// Note    : none.
// -----------------------------
void Task_LED(void* arg)
{
	uint8_t id = *((uint8_t*) arg);

    while (1)
	{
		ISBoard_setLed(id, 1); // turn on LED
		OSTimeDlyHMSM(0, 0, 0,   3);
		ISBoard_setLed(id, 0); // turn on LED
		OSTimeDlyHMSM(0, 0, 1, 997);
	}
}

void sendWirelessTxtMsg(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg)
{
	static char buf[nRF24L01_MAX_PLOAD +2];
	static uint8_t buf_i;
	bool   bEOL = FALSE;

	ISBoard_setLed(0, 1);
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
	ISBoard_setLed(0, 0);
}

void OnWirelessTextMsg(uint32_t localNodeId, uint8_t portNum, const char* txtMsg)
{
	ISBoard_setLed(1, 1);
	delayXusec(50*1000);
	ISBoard_setLed(1, 0);
}

void nRF24L01_OnPacket(nRF24L01* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen)
{
#define MAX_MSGLEN 100
	static char wtxtMsg[nRF_RX_CH][MAX_MSGLEN];
	static uint8_t wtxtMsg_i[nRF_RX_CH]; // ={0,0,0,0,0};

#define buf_i (wtxtMsg_i[pipeId-1])
	char* buf = wtxtMsg[pipeId-1];

	for (; playLoadLen>0 && *bufPlayLoad; playLoadLen--)
	{
		buf[buf_i] = *bufPlayLoad++;

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
		if (buf_i < MAX_MSGLEN -3) // re-fill in the line-end, any one of the charactors is enough
		{ buf[buf_i++] = '\r'; buf[buf_i++] = '\n'; }
		buf[buf_i] = '\0'; // NULL terminate the string
		buf_i =0; // reset for next receiving

		if ('\0' == buf[0]) // empty line, ignore
			continue;

		// a valid line of message here
		OnWirelessTextMsg(chip->nodeId, pipeId-1, (char*)buf);
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
