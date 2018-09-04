#include "includes.h"

#define MIN_ANN_INTERVAL (10*1000) // 10sec

// -----------------------------
// global declares
// -----------------------------
static uint32_t nodeId = 0x01010102;
#define RF_NODEID  (0x50150000) // the RF id of this node

// task priorities
#define TaskPrio_SongHongIS               2
#define TaskPrio_LED_Base                 5

//task stack sizes
#define TaskStackSize_SongHongIS          40
#define TaskStackSize_LED                 20
#define TaskStackSize_Poll                20

//task stacks
static OS_STK TaskStack_SongHongIS[TaskStackSize_SongHongIS];
static OS_STK TaskStack_Poll[TaskStackSize_Poll];

typedef struct _TaskEnv_LED
{
	uint8_t id;
	OS_STK taskStk[TaskStackSize_LED];
} TaskEnv_LED;

static TaskEnv_LED taskEnv_Leds[4];

//tasks
static  void Task_SongHongIS(void* p_arg);
static  void Task_LED(void* p_arg);
static  void Task_Poll(void* arg);

nRF24L01 nrf24l01 = { RF_NODEID, NULL, SPI2, {GPIOB, GPIO_Pin_1}, {GPIOB, GPIO_Pin_0}, EXTI_Line8, NULL};
void sendWirelessTxtMsg(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg);

uint16_t ADC_DMAbuf[ADC_CHS];

uint32_t RF_masterId   = 0x12345678;
uint8_t  RF_masterPort = 0x00;

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

	BSP_IntDisAll();    // disable all ints until we are ready to accept them.
	OSInit();           // initialize "uC/OS-II, The Real-Time Kernel".
	BSP_Init();         // initialize BSP functions.
	BSP_IntInit();      // initialize BSP interrupt ISRs

	os_err = OSTaskCreate((void(*) (void *)) Task_SongHongIS,   // create MyTask.
		NULL, (OS_STK *) &TaskStack_SongHongIS[TaskStackSize_SongHongIS -1], (INT8U) TaskPrio_SongHongIS);

#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(TaskPrio_SongHongIS, (CPU_INT08U*) Task_SongHongIS, &os_err);
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
// Task_SongHongIS()
// Description : The startup task. The uC/OS-II ticker should only be initialize once multitasking starts.
// Argument: p_arg argument passed to 'Task_SongHongIS()' by 'OSTaskCreate()'.
// Return  : none.
// Caller  : This is a task.
// Note    : none.
// -----------------------------
static void Task_SongHongIS(void* p_arg)
{
	uint16_t temperature, temperatureChip;
	CPU_INT08U os_err;
	INT8U i;
	uint32_t nextSleep  = 500, stampLastAnn =0, tmp; // 500msec
	uint8_t  lastMotion = 0;
	bool bAnnNeeded = FALSE;
	char txBuf[80];
	(void) p_arg;

	OS_CPU_SysTickInit();       // initialize the SysTick

#if (OS_TASK_STAT_EN > 0)
	OSStatInit();               // Determine CPU capacity.
#endif

	for (i=3; i < 4; i++)
	{
		taskEnv_Leds[i].id = i;

		os_err = OSTaskCreate((void (*) (void *)) Task_LED, (void *) &taskEnv_Leds[i].id,
	               (OS_STK *) &taskEnv_Leds[i].taskStk[TaskStackSize_LED - 1],
	               (INT8U) TaskPrio_LED_Base +i);
	
		OSTimeDlyHMSM(0, 0, 0, 500);
	}

	nRF24L01_init(&nrf24l01, RF_NODEID, NULL, TRUE, TRUE);

	while (1)
	{
		OSTimeDlyHMSM(0, 0, nextSleep /1000, nextSleep %1000);

		nextSleep = 500; // init with 500msec
		bAnnNeeded = FALSE;

		// step 1. test if the motion state has been changed
		tmp = motionState();
		if (tmp != lastMotion)
			bAnnNeeded = TRUE;
		
		lastMotion = (uint8_t) tmp;

		tmp = OSTimeGet();
		if (tmp - stampLastAnn > MIN_ANN_INTERVAL)
			bAnnNeeded = TRUE;
			
		if (!bAnnNeeded)
			continue;

		ISBoard_setLed(0, 1);
		temperature   = DS18B20_read(&ds18b20);
		// tempadc = (1.43 - tempadc*3.3/4096)*1000/4.35 +25; // convert adc value to temperature (in °Ê) = {(V25 - VSENSE) / Avg_Slope} + 25, where V25µ‰–Õ=1.43V, Avg_Slopeµ‰–Õ=4.3mV/°Ê
		// tempadc = (143 - tempadc*330/4096)*10000/435 +250; // in 0.1C
		temperatureChip = (1718- (int32_t) chipTemperature) * 330/4096*10000/435 + 140; // in 0.1C: measure data: adc=1718 when the temp is 14C
		//		adc  = ADC_GetConversionValue(ADC1);

		sprintf(txBuf, "POST %x/status?temp=%d&tempChip=%d&lumin=%d&motion=%d\r\n",
				nodeId, temperature, temperatureChip, luminVal, (lastMotion?1:0));

		sendWirelessTxtMsg(&nrf24l01, RF_masterId, RF_masterPort, txBuf);
		ISBoard_setLed(0, 0);

		stampLastAnn = OSTimeGet();
	}
}

// -----------------------------
// Task_Poll()
// Description : the task to poll on board resources
// Argument: none.
// Return   : none.
// Caller   : Task_SongHongIS().
// Note    : none.
// -----------------------------
static void Task_Poll(void* arg)
{
    while (1)
	{
       // TODO:

		OSTimeDlyHMSM(0, 0, 10, 0); // every 10 sec
	}
}

// -----------------------------
// Task_LED()
// Description : Create the application tasks.
// Argument: none.
// Return   : none.
// Caller   : Task_SongHongIS().
// Note    : none.
// -----------------------------
static void Task_LED(void* arg)
{
	uint8_t* id = (void*) arg;
	uint16_t PinMask = GPIO_Pin_0;

	if (*id<4)
		PinMask <<= *id +1;
	else PinMask = GPIO_Pin_1;

    while (1)
	{
		GPIO_WriteBit(GPIOA, PinMask, (BitAction) 0);	  // turn on LED
		OSTimeDlyHMSM(0, 0, 0,   3);
    	GPIO_WriteBit(GPIOA, PinMask, (BitAction) 1);	  // turn off LED
		OSTimeDlyHMSM(0, 0, 1, 997);
	}
}

void sendWirelessTxtMsg(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg)
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
		nRF24L01_setModeRX(chip, FALSE); // ensure to close the current send-to connection, will to be TX when next tx
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

	nRF24L01_setModeRX(chip, FALSE);
}

void nRF24L01_OnPacket(uint32_t localNodeId, uint8_t portNum, const char* txtMsg)
{
	const char* p =	strstr(txtMsg, "POST "), *q=NULL;
	uint32_t val;

	if (NULL == p) return; p += sizeof("POST ")-1;

	p = strstr(p, "/RF_master?"); if (NULL == p) return;  p += sizeof("/RF_master?")-1;
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

void nRF24L01_OnPacket(nRF24L01* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen)
{
#define MAX_MSGLEN 100
	static char wtxtMsg[5][MAX_MSGLEN];
	static uint8_t wtxtMsg_i[5]={0,0,0,0,0};

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
			buf_i = (++buf_i) % (MAX_MSGLEN-2);
			continue;
		}

		// a line just received
		buf[buf_i] = '\0'; // NULL terminate the string
		buf_i =0; // reset for next receiving

		if ('\0' == buf[0]) // empty line, ignore
			continue;

		// a valid line of message here
		nRF24L01_OnPacket(chip->nodeId, pipeId-1, (char*)buf);
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

