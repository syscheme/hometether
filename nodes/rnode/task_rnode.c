#include "includes.h"
#include "htcluster.h"

// -----------------------------
// global declares
// -----------------------------
#define NEXT_SLEEP_MIN               (20)
#define NEXT_SLEEP_MAX               (1000*60)

#if 0
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
#endif // 0

void InitProcedure(void)
{
}

void ThreadStep_doMainLoop(void)
{
	uint16_t nextSleep =0; // *pU16;
	trace("Task_main loop start");
	// dispatch on states and monitoring
	while (1)
	{
		nextSleep = NEXT_SLEEP_MAX;  // initialize with a maximal sleep time

		// dispatch by states

		// ----------------------------
		// State_process();

		// scan and update the device status
		// nextSleep = State_scanDevice(nextSleep);
		if (nextSleep < NEXT_SLEEP_MIN)
			nextSleep = NEXT_SLEEP_MIN;

		// sleep for next round
		sleep(nextSleep);
	}

	trace("Task_Main() quits");
//	canClose(&OD_secu_Data);
}

#include "usart.h"
// -----------------------------
// Task_Communication()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argume nt passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
USART COM1;
void Task_Communication(void* p_arg)
{
	trace("Task_Communication starts");
	
#ifdef USART_BY_FILE
	USART_open(&COM1, "COM1", 96);
#else
	USART_open(&COM1, 96);
#endif // USART_BY_FILE
	// dispatch on states and monitoring
	while (1)
	{

#ifndef EN28J60_INT
//		ThreadStep_doNicProc();
#endif // EN28J60_INT

//		ThreadStep_doMsgProc();
//		ThreadStep_doCanMsgIO();

		sleep(1);
		// HtDDL_readUSART(&COM1);
	}
	USART_close(&COM1);
	
	trace("Task_Communication quits");
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
	trace("Task_Timer starts");

	while (1)
	{
		HtNode_do1msecScan();
		sleep(1);
	}

	trace("Task_Timer quits");
}

// -----------------------------
// Task_Main()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
hterr HtNode_doSetObject(void* nic, uint8_t objectId, uint8_t subIdx, const uint8_t* value, uint8_t vlen);
static const NodeId_t dummyUnlink = 0x87654321;
extern HtNetIf netifUplink;

void Task_Main(void* p_arg)
{
//	trace("Task_Main starts at %s mode\r\n", (_deviceState & dsf_Debug)?"DEBUG":"NORMAL");

	// play the start-up song
	InitProcedure();
	HtNode_init(&netifUplink);

	trace("creating sub tasks");

 	// create the Task_Communication()
	createTask(Communication, NULL);

	// create the Task_CO()
	// createTask(CO, NULL);

	// create the Task_Timer()
	createTask(Timer, NULL);

	while (1)
	{
		sleep(3000);

		htnThis.eNid =5;
		HtNode_doSetObject(NULL, ODID_uplink, 0, (const uint8_t*)&dummyUnlink, sizeof(NodeId_t));
		HtNode_sendGETOBJ(5, ODID_nodeId, 0);

		sleep(3000);
		HtNode_sendGETOBJ(5, ODID_objectList, 0);
		HtNode_sendGETOBJ(5, ODID_nodeId, ODSubIdx_ObjectUri);
		HtNode_sendGETOBJ(5, ODID_nodeId, ODSubIdx_ObjectType);

		HtNode_sendSETOBJ(5, ODID_data1, 0, (const uint8_t*)&dummyUnlink, 2);
		HtNode_sendSETOBJ(5, ODID_data1, 0, ((const uint8_t*)&dummyUnlink)+2, 2);
		htnThis.timeoutUnlink =0;

	}
	// do the main loop
	ThreadStep_doMainLoop();

	// htnThis.uplink.nextNid=0x76543210;
}

