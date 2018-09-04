#include "secu.h"
#include "bsp.h"
#include "app_cfg.h"
#include "../htod.h"

// -----------------------------
// global declares
// -----------------------------
#define TaskPrio_Main                (20)  // the application-wide lowest priority
#define TaskStkSz_Main               (configMINIMAL_STACK_SIZE) //96
static OS_STK TaskStk_Main[TaskStkSz_Main];

#define SubTaskPrio_Base		     4

#define TaskPrio_Timer               (SubTaskPrio_Base)
#define TaskStkSz_Timer              (0x40)
static OS_STK TaskStk_Timer[TaskStkSz_Timer];

#define TaskPrio_Communication        (SubTaskPrio_Base +2)
#define TaskStkSz_Communication       (0x100)	//96
static OS_STK TaskStk_Communication[TaskStkSz_Communication];

/*
#define TaskPrio_CO              (SubTaskPrio_Base +1)
#define TaskStkSz_CO            (0xa0)	// 160
static OS_STK TaskStk_CO[TaskStkSz_CO];
*/

// -----------------------------
// main()
// Description : This is the standard entry point for C code.  It is assumed that your code will call
//               main() once you have performed all necessary initialization.
// Argument: none.
// Return   : integer code
// Note    : none.
// -----------------------------
int main2(void)
{
	CPU_INT08U os_err;

	BSP_IntDisAll();    // disable all ints until we are ready to accept them.
	OSInit();           // initialize "uC/OS-II, The Real-Time Kernel".
	BSP_Init();         // initialize BSP functions.

	MsgLine_init();

	// create Task_Main
	trace("*** ECU starts\r\n");
	os_err = OSTaskCreate((void (*) (void *)) Task_Main, (void *) 0,
		(OS_STK*) &TaskStk_Main[TaskStkSz_Main - 1], (INT8U) TaskPrio_Main);

#if (OS_TASK_NAME_SIZE >= 11)
	OSTaskNameSet(TaskPrio_Main, (CPU_INT08U*) Task_Main, &os_err);
#endif

	trace("  * starting multi-tasks\r\n");
	OSTimeSet(0);
	OSStart();          // start multitasking (i.e. give control to uC/OS-II)

	trace("*** ECU ends\r\n");
	return 0;
}

void InitProcedure(void)
{
	int i;

	// play the start-up song
	trace("start up song\r\n");
	for (i =0; 0 != startSong[i].pitchReload && 0 != startSong[i].lenByQuarter; i++)
		DQBoard_beep(startSong[i].pitchReload, startSong[i].lenByQuarter *80);

	// blicking the leds
	for (i =0; i<8; i++)
	{
		DQBoard_setLed(0, 1); delayXusec(80000); DQBoard_setLed(0, 0); 
		DQBoard_setLed(1, 1); delayXusec(80000); DQBoard_setLed(1, 0); 
		DQBoard_setLed(2, 1); delayXusec(80000); DQBoard_setLed(2, 0); 
		DQBoard_setLed(3, 1); delayXusec(80000); DQBoard_setLed(3, 0);
	}

	// test to operate the relays
	for (i =0; i < CHANNEL_SZ_Relay*2; i++)
	{
		ECU_setRelay(i, 1);  delayXusec(300000);
		ECU_setRelay(i, 0);  delayXusec(300000);
	}
}

//  OS_EVENT* lkUART=NULL;

// -----------------------------
// Task_Main()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
// -----------------------------
// Task_Main()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
static void Task_Main(void* p_arg)
{
	uint16_t nextSleep; // *pU16;
	CPU_INT08U os_err;
	(void) p_arg;
	
	_deviceState = _gState =0;
	State_init(TRUE);
//	InitCanOpen();

#define NEXT_SLEEP_MIN  (50)
#define NEXT_SLEEP_MAX  (NEXT_SLEEP_MIN*20)

	trace("  * Task_Main starts at %s mode\r\n", (_deviceState & dsf_Debug)?"DEBUG":"NORMAL");
	OSTimeSet(0);
	OS_CPU_SysTickInit();       // initialize the SysTick

#if (OS_TASK_STAT_EN > 0)
	OSStatInit();               // Determine CPU capacity.
#endif

	// play the start-up song
	InitProcedure();

	trace("  * creating sub tasks\r\n");

 	// create the Task_Communication()
	os_err = OSTaskCreate((void (*) (void *)) Task_Communication, NULL,
		(OS_STK *) &TaskStk_Communication[TaskStkSz_Communication -1], (INT8U)TaskPrio_Communication);

/*
	// create the Task_CO()
	os_err = OSTaskCreate((void (*) (void *)) Task_CO, NULL,
		(OS_STK *) &TaskStk_CO[TaskStkSz_CO -1], (INT8U)TaskPrio_CO);
*/

	os_err = OSTaskCreate((void (*) (void *)) Task_Timer, NULL,
		(OS_STK *) &TaskStk_Timer[TaskStkSz_Timer -1], (INT8U)TaskPrio_Timer);

	ThreadStep_doMainLoop();
}

void AdminExt_sendMsg(uint8_t fdout, char* msg, uint8_t len);

// -----------------------------
// Task_Communication()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
static void Task_Communication(void* p_arg)
{
	trace("  * Task_Communication starts\r\n");
	// dispatch on states and monitoring
	while (1)
	{
#ifndef EN28J60_INT
		ThreadStep_doNicProc();
#endif // EN28J60_INT

		ThreadStep_doMsgProc();
		ThreadStep_doCanMsgIO();

		OSTimeDlyHMSM(0, 0, 0, 1);

/*
		OSTimeDlyHMSM(0, 0, 0, 10);

		// step 1. scan if there are any pending text msgs need to be processed
		for (i =0, c=0; i < MAX_PENDMSGS; i++)
		{
			if (0 == _pendingLines[i].timeout || _pendingLines[i].offset >0)
				continue;  // idle or incomplete message

			// process a completed pending message
			//step 1.1. reserve timeout to hold this message during processing
			_pendingLines[i].timeout = 0xff;	c++;
		
			tmp = strlen((const char*)_pendingLines[i].msg);
			if (tmp<=0) { _pendingLines[i].timeout =0; continue; }

			// step 1.2 process the text messages per chId
 			switch(_pendingLines[i].chId)
			{
			case MSG_CH_RS232_Received: 
				GW_dispatchTextMessage(fadm, (char*)_pendingLines[i].msg);
				break;

			case MSG_CH_RS232_SendTo:
				AdminExt_sendMsg(fadm, (char*)_pendingLines[i].msg, strlen((const char*)_pendingLines[i].msg));
				break;

			case MSG_CH_RS485_Received:
				GW_dispatchTextMessage(fext, (char*)_pendingLines[i].msg);
				break;

			case MSG_CH_RS485_SendTo:
				AdminExt_sendMsg(fext, (char*)_pendingLines[i].msg, strlen((const char*)_pendingLines[i].msg));
				break;

			default:
				break;
			}
		
			//step 1.End. reset timeout to be idle after processing
			_pendingLines[i].timeout = 0;

			OSTimeDlyHMSM(0, 0, 0, 1); // yield cpu for a short while
		}

#warning TODO: step 2. see if there is any captured IR pulses need to process
/*
		// step 3. see if there is any captured pulses need to process
		if (pulseCapBuf.timeout >0)
		{
			pulses.seqlen = sizeof(seq) -2;
			bIrRecvd = Pulse_compress(&pulseCapBuf, &Ecu_pulseRecvSeq);
			pulseCapBuf.timeout = 0;
		}

		if (bIrRecvd)
		{
			EcuCtrl_pulseRecvTimeout = 5000; // 5sec
			triggerPdo(HTPDO_PulsesReceived);

			p = txBuf;
			sprintf(p, "POST %x/irrecv?bintv=%x&sdur=%x&seqlen=%x&seq=", nodeId, pulses.baseIntvX10usec, pulses.short_dur, pulses.seqlen);
			p += strlen(p);
			for(i=0;i<pulses.seqlen;i++)
			{
				sprintf(p, "%02x.", pulses.seq[i]); p+=strlen(p);
			}
			sprintf(--p, "\r\n");

 			sendWirelessTxtMsg(&nrf24l01, RF_NODEID_PEER, RF_PORTNUM, txBuf);
		}

		// step 2. send the pending pulses signals
		if (0xff != EcuCtrl_pulseSendPflId)
		{
			// there is a pending pulse code, send the pulses
			PulseSend_byProfileId(EcuCtrl_pulseSendPflId, EcuCtrl_pulseSendCode, &IrLEDs[EcuCtrl_pulseSendChId % CHANNEL_SZ_IrLED], FALSE, EcuCtrl_pulseSendBaseIntvX10usec*10);
			EcuCtrl_pulseSendPflId =0xff; // reset to idle
		}
 */

	}
	
	trace("  * Task_Communication quits\r\n");
}

/*
// -----------------------------
// Task_CO()
// Description : The task to dispatch CO stack
// Argument: p_arg  Argument passed to 'Task_Main()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
static void Task_CO(void* p_arg)
{
    OS_CPU_SR  cpu_sr = 0;
	trace("  * Task_CO starts\r\n");
	while(1)
	{
        OS_ENTER_CRITICAL();
		HtCan_flushOutgoing();
		HtCan_doScan();
        OS_EXIT_CRITICAL();

		OSTimeDlyHMSM(0, 0, 0, 1);
	}

	trace("  * Task_CO quits\r\n");
}
*/

// -----------------------------
// Task_Timer()
// Description : the task as the 1 msec timer to drive CanFestival's timer portal
// Argument: none.
// Return: none.
// -----------------------------
#define DEC_TIMEOUT(_TO, _BY)	if (_TO >_BY) _TO -= _BY; else _TO =0;
extern uint32_t Timeout_MasterHeartbeat;

static void Task_Timer(void* arg)
{
	trace("  * Task_Timer starts\r\n");

	while (1)
	{
		OSTimeDlyHMSM(0, 0, 0, 1);
 		ThreadStep_do1msecTimerProc();
	}

	trace("  * Task_Timer quits\r\n");
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

// --- capture test
void CAP_Config()
{
  // configuring GPIO pin as input floating, PA7 maps to TIM3_CH2
  GPIO_InitTypeDef GPIO_InitStruct;

// http://xiaozhekobe.blog.163.com/blog/static/175646098201192221820218/
	RCC_APP2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

  GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_7;    //GPIO配置 
  GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING; 
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_Init(GPIOA, &GPIO_InitStruct); 
 
  // NVIC for TIM3
  NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStruct.NVIC_IRQChannePreemptionPriority = 0; 
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1; 
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; 
  NVIC_Init(&NVIC_InitStruct); 
 
  // TIM input channel
TIM_ICInitStruct.TIM_ICMode      = TIM_ICMode_ICAP; // TIM_ICMode_ICAP:使用输入捕获模式, TIM_ICMode_PWM1:使用输入PWM模式
TIM_ICInitStruct.TIM_Channel     = TIM_Channel_2; 
TIM_ICInitStruct.TIM_ICPolarity  = TIM_ICPolarity_Rising;    //输入活动沿: TIM_ICPolarity_Rising:上升沿触发, TIM_ICPolarity_Falling:捕获下降沿
TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI; //管脚与寄存器对应关系 
TIM_ICInitStruct.TIM_ICPSC_DIV1; // 输入预分频，意思是控制在多少个输入周期做一个捕获，如果输入的信号频率没有变，测得的周期也不会变，比如选择四分频，则每四个输入周期做一次捕获，这样在输入信号不频繁的情况下，可以减少软件被不断中断的次数 
TIM_ICInitStruct.TIM_ICFiter = 0x0; //滤波设置，经历几个周期跳变认定波形稳定0x0-0xf; 
TIM_PWMIConfig(TIM3, &TIM_ICInitStruct); //根据参数配置TIM外设信息
 
// select TIM modes
TIM_SelectInputTrigger(TIM3, TIM_TS_TI2FP2); //选择TIM的输入触发器为 TIM_TS_TI2FP2
TIM_SelectMasterSlaveMode(TIM3, TIM_MasterSlaveMode_Enable); 

// start TIM
TIM_Cmd(TIM3, ENABLE); 
TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE); 
}
 
void TIM3_IRQHandler(void)
{ 
    TIM_CLearITPendingBit(TIM3, TIM_IT_CC3); 
    IC2Value = TIM_GetCapture2(TIM3); 
    if(IC2Value != 0) 
    { 
        DutyCyle = (TIM_GetCapture1(TIM3) * 100) / IC2Value; 
        Frequency = 72000000 / IC2Value; 
    } 
    else 
    { 
        DutyCycle = 0; 
        Frequency = 0; 
    } 
} 
 
注（一）：若想改变测量的PWM频率范围，可将TIM时钟频率做分频处理 
 
TIM_TimeBaseStructure.TIM_Period = 0xFFFF;     //周期0～FFFF 
TIM_TimeBaseStructure.TIM_Prescaler = 5;       //时钟分频，分频数为5+1即6分频 
TIM_TimeBaseStructure.TIM_ClockDivision = 0;   //时钟分割 
TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//模式 
TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);//基本初始化 
注（二）：定时器TIM的倍频器X1或X2。在APB分频为1时，倍频值为1，否则为2 
// */
