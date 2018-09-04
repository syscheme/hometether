#include "includes.h"
#include "htcluster.h"
#include "textmsg.h"

// -----------------------------
// global declares
// -----------------------------
#if 0
#define TaskPrio_Main                (20)  // the application-wide lowest priority
#define TaskStkSz_Main               (configMINIMAL_STACK_SIZE) //96
static OS_STK TaskStk_Main[TaskStkSz_Main];

#define SubTaskPrio_Base		     4

#define TaskPrio_Exec               (SubTaskPrio_Base)
#define TaskStkSz_Exec              (0x40)
static OS_STK TaskStk_Timer[TaskStkSz_Exec];

#define TaskPrio_Communication        (SubTaskPrio_Base +2)
#define TaskStkSz_Communication       (0x100)	//96
static OS_STK TaskStk_Communication[TaskStkSz_Communication];
#endif // 0

// -----------------------------
// Task_Communication()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
TextLineCollector tlcCOM1 = {NULL, 0,};
TextLineCollector tlcRF = {NULL, 0,};
FIFO rxRF;

portTASK_FUNCTION(Task_Communication, p_arg)
{
	char buf[16];
	uint16_t i;
	pbuf* packet;
	trace("Task_Communication starts");
	TextLineCollector_init(&tlcCOM1, TextMsg_procLine, &COM1);
	TextLineCollector_init(&tlcRF, TextMsg_procLine, &si4432);

#ifdef USART_BY_FILE
	USART_open(&COM1, "COM1", 1152);
#else
	USART_open(&COM1, 1152);
#endif // USART_BY_FILE

	if (ERR_SUCCESS == 	FIFO_init(&rxRF, 10, sizeof(pbuf*), 0))
		SI4432_init(&si4432);
	// dispatch on states and monitoring
	while (1)
	{
		i = USART_receive(&COM1, (uint8_t*)buf, sizeof(buf));
		if (i>0)
		{
			TextLineCollector_push(&tlcCOM1, buf, i);
			continue;
		}

		packet = NULL;
		if (FIFO_ready(rxRF) && ERR_SUCCESS == FIFO_pop(&rxRF, &packet) && packet)
		{
			for (; packet; packet= packet->next)
			{
				TextLineCollector_push(&tlcRF, (char*)packet->payload, packet->len);
			}
			continue;
		}

#ifndef EN28J60_INT
//		ThreadStep_doNicProc();
#endif // EN28J60_INT

//		ThreadStep_doMsgProc();
//		ThreadStep_doCanMsgIO();

		// idle when reached here
		sleep(2);
		// HtDDL_readUSART(&COM1);
	}
	USART_close(&COM1);

	trace("Task_Communication quits");
}


// -----------------------------
// Task_Exec()
// Description: the task as the 1 msec timer to drive the primary job
// Argument: none.
// Return: none.
// -----------------------------
portTASK_FUNCTION(Task_Exec, p_arg)
{
	uint16_t nextSleep =0;

	trace("Task_Exec() starts");

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

		nextSleep = Water_do1round();
		ADJUST_BY_RANGE(nextSleep, NEXT_SLEEP_MIN, NEXT_SLEEP_MAX);
	}

	trace("Task_Exec() quits");
}

// -----------------------------
// Task_Capture()
// Description: the task handles the captured pulses
// Argument: none.
// Return: none.
// -----------------------------
void Water_Config_PulseCapture(void);

portTASK_FUNCTION(Task_Capture, p_arg)
{
	uint8_t cCap =0;
	trace("Task_Capture() starts");

	// capture should be the last step because its EXTI may interrupt others
	Water_Config_PulseCapture();

	while (1)
	{
		if ((PulseCapture_doParse(&pulsechs[0]) >0) && (cCap++ <5))
			continue;

		sleep(5);
	}

	trace("Task_Capture() quits");
}

// -----------------------------
// Task_Main()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
#define HEARTBEAT_INTERVAL_SEC   (12)
extern __IO__ uint8_t byteStatus;
char buf[128];

// extern uint32_t Timeout_MasterHeartbeat;
// hterr HtNode_doSetObject(void* nic, uint8_t objectId, uint8_t subIdx, const uint8_t* value, uint8_t vlen);
// static const NodeId_t dummyUnlink = 0x87654321;
// extern HtNetIf netifUplink;
extern __IO__ uint32_t msecWaterLast;

portTASK_FUNCTION(Task_Main, p_arg)
{
	static uint32_t msecLastSec=0;
	uint16_t i, clock_sec =0, secLastHeartbeat =0;
	char *p=buf;
	pbuf* pmsg = NULL;

	uint8_t penaltySi4432 =0, gPenalty=0;
	uint8_t cExceptionsInRound =0;

	trace("Task_Main() starts at %s mode\r\n", (_deviceState & dsf_Debug)?"DEBUG":"NORMAL");

	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 22, 22, 0); // @(250+250msec)
	Water_init();
	//	HtNode_init(&netifUplink);

	delayXmsec(1500);
	SI4432_init(&si4432);
	delayXmsec(500);

	// create the Task_Exec()
	createTask(Exec, NULL);

	// create the Task_Communication()
	createTask(Communication, NULL);

	// create the Task_Capture()
	createTask(Capture, NULL);

	while (1)
	{
		sleep(100); // at least yield 100msec

		// filter timer by seconds
		// ---------------------------------------
		i = gClock_getElapsed(msecLastSec) /1000;
		if (i <= 0)
			continue;
		msecLastSec = gClock_msec; 
		clock_sec += i;

		// check the healthy of each compnent every sec
		// ---------------------------------------
		cExceptionsInRound =0;

		// part 1. for Si4432: a) its stampLastIO is more than HEARTBEAT_INTERVAL_SEC ago
		if (0 == (si4432.ITstatus & (ITSTATUS_POR|ITSTATUS_CHIPRDY)))
			penaltySi4432++;

		if (si4432.ITstatus & (ITSTATUS_CRC_ERR | ITSTATUS_FIFO_ERR | ITSTATUS_RX_FULL | ITSTATUS_TX_FULL))
			penaltySi4432++;

		if (gClock_getElapsed(si4432.stampLastIO)/1000 > HEARTBEAT_INTERVAL_SEC)
			penaltySi4432 +=10;
		else if (FIFO_awaitSize(&si4432.txFIFO) <= (si4432.txFIFO.count >>2))
			penaltySi4432 =0;

		if (penaltySi4432 >0)
		{
			cExceptionsInRound++;
			if (penaltySi4432 > 50)
			{
				// Si4432 looks like stuck, try to recover by reset it individually
				gPenalty++;
				SI4432_reset(&si4432);
				penaltySi4432 =0;
			}
		}

		// part 2. for water: at least its msecWaterLast should be stepped
		if (gClock_getElapsed(msecWaterLast)/1000 > HEARTBEAT_INTERVAL_SEC)
		{
			cExceptionsInRound++;
			gPenalty++;
		}

		// TODO: more health-check

		// end of health-check, see if gPenalty exceeded max
		// ---------------------------------------
		if (0 == cExceptionsInRound) // no exceptions in this round, reset gPenalty
			gPenalty = 0;

		if (gPenalty > 30) // no freq than every 0.5hr
		{
			trace("Task_Main() encountered too many gPenalty[%d], force to reboot\n", gPenalty);
			RNode_reset();
		}
		
		// sending the RF heartbeat very HEARTBEAT_INTERVAL_SEC
		if (UINT_SUB(uint16_t, clock_sec, secLastHeartbeat) >HEARTBEAT_INTERVAL_SEC)
		{
			secLastHeartbeat = clock_sec;

			p = buf;
			i = STM32TemperatureADC2dC(ADC_DMAbuf[0]);	// read the in-chip temperature
			p += snprintf(p, buf +sizeof(buf) -p-2, "+FFws%02xi%d@%d,%dt%d", byteStatus, i, ADC_DMAbuf[0], ADC_DMAbuf[1],cTemperatures);
			for (i=0; i<cTemperatures; i++)
				p += snprintf(p, buf +sizeof(buf) -p-2, ",%d", temperatures[i]);

			*p++= '\n', *p++= '\0';
			// p += snprintf(p, buf +sizeof(buf) -p-2, "stmp%d\n", gClock_msec);

			pmsg = pbuf_dup(buf, p -buf);
			SI4432_transmit(&si4432, pmsg);
			pbuf_free(pmsg);

			USART_transmit(&COM1, (uint8_t*)buf, p -buf);
		}
	}
}

