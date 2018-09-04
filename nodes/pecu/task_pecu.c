#include "pecu.h"
#include "bsp.h"
#include "app_cfg.h"
#include "../htod.h"

// -----------------------------
// global declares
// -----------------------------

// -----------------------------
// utility functions
// -----------------------------
static void InitProcedure(void)
{
	int i;

	// play the start-up song
	trace("start up song\r\n");

	// test to operate the relays
	for (i =0; i < CHANNEL_SZ_Relay*2; i++)
	{
		PECU_setRelay(i, 1);  delayXmsec(300);
		PECU_setRelay(i, 0);  delayXmsec(300);
	}
}

void ThreadStep_doMainLoop(void)
{
	uint32_t stampLastReportEnv =0;

	PECU_init(1);

	trace("  * Task_main loop start\r\n");

	// dispatch on states and monitoring
	while (1)
	{
		sleep(1);

		// timer-ed in milliseconds
		// ---------------------------------------
		HtNode_do1msecScan();

		// scan and update the device status
		PECU_scanDevice();

		// notifications by timer
		// ---------------------------------------
		// NT.1 ODEvent_env
		if (CONFIG_notifyIntv_env>0 && gClock_getElapsed(stampLastReportEnv) >= (CONFIG_notifyIntv_env*1000))
		{
			stampLastReportEnv = gClock_msec; 
			HtNode_triggerEvent(ODEvent_env);
		}
	}

	trace("  * Task_Main quits\r\n");
}

void AdminExt_sendMsg(uint8_t fdout, char* msg, uint8_t len);

// -----------------------------
// Task_Comm()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
void Task_Comm(void* p_arg)
{
	uint32_t stampLastCheck =0;
	uint8_t cdReset =0;

	trace("  * Task_Comm() starts\r\n");

	ESP8266_init(&esp8266, &TTY1);

	// dispatch on states and monitoring
	while (1)
	{
		if (ESP8266_loopStep(&esp8266) >0)
			continue;

		sleep(1);
		if (gClock_getElapsed(stampLastCheck) <15000)
			continue;

		stampLastCheck = gClock_msec;
		ESP8266_check(&esp8266);

		if (cdReset++ < 40)
			continue;
		cdReset =0;
		ESP8266_init(&esp8266, &TTY1);
	}

	USART_close(&TTY1);
	
	trace("  * Task_Comm() quits\r\n");
}

// -----------------------------
// Task_Timer()
// Description : the task as the 1 msec timer to drive CanFestival's timer portal
// Argument: none.
// Return: none.
// -----------------------------
#define DEC_TIMEOUT(_TO, _BY)	if (_TO >_BY) _TO -= _BY; else _TO =0;
extern uint32_t Timeout_MasterHeartbeat;

void Task_Timer(void* arg)
{
	trace("  * Task_Timer starts\r\n");

	while (1)
	{
		sleep(1);
#ifdef WIN32
		gClock_msec++;
#endif
	}

	trace("  * Task_Timer quits\r\n");
}

// -----------------------------
// Task_Main()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
void Task_Main(void* p_arg)
{
	trace("  * Task_Main starts at %s mode\r\n", (_deviceState & dsf_Debug)?"DEBUG":"NORMAL");

	// play the start-up song
	InitProcedure();

	trace("  * creating sub tasks\r\n");

 	// create the Task_Comm()
	createTask(Comm, NULL);

	// create the Task_Timer()
	createTask(Timer, NULL);

	// do the main loop
	ThreadStep_doMainLoop();
}

