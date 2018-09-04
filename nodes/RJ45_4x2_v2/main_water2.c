#include "includes.h"
#include "pulses.h"
#include "htcluster.h"

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

static portTASK_FUNCTION( vSuicidalTask, pvParameters )
{
volatile long l1, l2;
TaskHandle_t xTaskToKill;
const TickType_t xDelay = ( TickType_t ) 200 / portTICK_PERIOD_MS;

	if( pvParameters != NULL )
	{
		/* This task is periodically created four times.  Two created tasks are
		passed a handle to the other task so it can kill it before killing itself.
		The other task is passed in null. */
		xTaskToKill = *( TaskHandle_t* )pvParameters;
	}
	else
	{
		xTaskToKill = NULL;
	}

	for( ;; )
	{
		/* Do something random just to use some stack and registers. */
		l1 = 2;
		l2 = 89;
		l2 *= l1;
		vTaskDelay( xDelay );

		if( xTaskToKill != NULL )
		{
			/* Make sure the other task has a go before we delete it. */
			vTaskDelay( ( TickType_t ) 0 );

			/* Kill the other task that was created by vCreateTasks(). */
			vTaskDelete( xTaskToKill );

			/* Kill ourselves. */
			vTaskDelete( NULL );
		}
	}
}

// -----------------------------
// main()
// Description : This is the standard entry point for C code.  It is assumed that your code will call
//               main() once you have performed all necessary initialization.
// Argument: none.
// Return   : integer code
// Note    : none.
// -----------------------------
// #define TEST_RX

int main(void)
{
	// const char msg[] = "hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n";
	// pbuf* packet = pbuf_mmap((void*)"hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n", 60);

	BSP_Init();         // initialize        BSP functions.

	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 10, 10, 0);
	delayXmsec(2000);
//	SI4432_init(&si4432);

	SI4432_setMode_RX(&si4432);
	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 8, 2, 2);

	// create the main task.
	createTask(Main, NULL);

	// The suicide tasks must be created last as they need to know how many
	// tasks were running prior to their creation in order to ascertain whether
	// or not the correct/expected number of tasks are running at any given time.
    // vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );
	
	// run the scheduler
	runTaskScheduler();

	return 0;
}





////////////////////////////////////////////////////////////////////////////////////////////
uint16_t temp, adc, tempadc;
char txBuf[256];

void PulseCapture_OnFrame(PulseCapture* ch)
{
	int i=0;
	for (i=0; i< 1000; i++);
}

void HtFrame_OnReceived(PulseCapture* ch, uint8_t* msg, uint8_t len)
{
}

void SI4432_OnReceived(SI4432* chip, pbuf* packet)
{
	uint8_t blinks=1;
	if (60 == packet->tot_len && 'h'== packet->payload[0] && 'i'== packet->payload[1])
		blinks =3;
	RNode_blink(BlinkPair_FLAG_A_ENABLE, 1, 1, blinks);

	if (!FIFO_ready(rxRF))
		return;

	if (ERR_SUCCESS == FIFO_push(&rxRF, &packet))
		pbuf_ref(packet);
}

void SI4432_OnSent(SI4432* chip, pbuf* packet)
{
	RNode_blink(BlinkPair_FLAG_B_ENABLE, 1, 1,2);
}


////////////////////////////////////////////////////////////////////////////////////////////
// ==================================
// TEST: loopback pluse-capture
// ==================================
uint16_t htddl_doTX(void*pIF, const hwaddr_t dest, const uint8_t* data, uint16_t len)
{
	return len; 
}

void htddl_OnReceived(HtNetIf* netif, const hwaddr_t src, pbuf* data)
{
}

hwaddr_t bindaddr = {0x12,0x34,0x56,0x78};
HtNetIf netifUplink;

uint8_t  htddl_getHwAddr(void* nic, uint8_t** pBindaddr)  { *pBindaddr = bindaddr; return sizeof(hwaddr_t); }

void HtNode_OnObjectQueried(HtNetIf* nic, uint8_t objectId, uint8_t subIdx)
{
}

void HtNode_OnObjectChanged(HtNetIf* nic, uint8_t objectId, uint8_t subIdx, const uint8_t* newValue, uint8_t vlen)
{
}

NodeId_t nidThis = 0x123456;
__IO__ uint32_t gClock_msec =0;

// sample OD declaration:
uint16_t data1, data2;
USR_OD_DECLARE_START()
  OD_DECLARE(data1,  0x0001, ODF_Readable, &data1, 2)
  OD_DECLARE(data2,  0x0001, ODF_Readable, &data1, 2)
USR_OD_DECLARE_END()

// sample OD declaration:
enum {
ODEvent_USER_Event1 = ODEvent_USER_MIN, 
};
//
static const uint8_t event_objIds[] = {ODID_data1, ODID_data2, ODID_NONE};
USR_ODEVENT_DECLARE_START()
 	ODEVENT_DECLARE(USER_Event1, event_objIds)
USR_ODEVENT_DECLARE_END()

// ---------------------------------------------------------------------------
// dummy for HtDict
// ---------------------------------------------------------------------------
void HtEdge_OnNodeAdvertize(HtNetIf* netif, uint8_t eNid, NodeId_t theNode, NodeId_t nidBy)
{
	// TODO: collect (orphanNodeId, srcNodeId, ttl, count++)
	// and wait for a while, then pick the path has maximal count and maximal ttl
	// overwrite the orphan->srcNodeId by calling LRUMap_set(downLinks, theParty, srcNodeId) for the picked path

}

void HtNode_OnObjectValues(uint8_t eNid, uint8_t objectId, uint8_t subIdx, uint8_t* value, uint8_t vlen)
{
}
