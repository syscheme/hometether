#include "includes.h"
#include "pulses.h"
#include "htcluster.h"

#include <stdio.h>

// Scheduler includes.
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Demo app includes
#include "death.h"

// Task priorities. 
#define mainCREATOR_TASK_PRIORITY           ( tskIDLE_PRIORITY + 3 )

//-----------------------------------------------------------

int main( void )
{
#ifdef DEBUG
	debug();
#endif

	BSP_Init();

	// create the main task.
//    xTaskCreate(Task_Main, "Main", TaskStkSz_Main, NULL, TaskPrio_Main, NULL);
//	xTaskCreate( vBlockingQueueConsumer, "QConsB1", blckqSTACK_SIZE, ( void * ) pxQueueParameters1, uxPriority, NULL );
	createTask(Main, NULL);

	// The suicide tasks must be created last as they need to know how many
	// tasks were running prior to their creation in order to ascertain whether
	// or not the correct/expected number of tasks are running at any given time. 
	vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );

	// Configure the timers used by the fast interrupt timer test.
	// vSetupTimerTest();

	// Start the scheduler.
	vTaskStartScheduler();

	// Will only get here if there was not enough heap space to create the
	// idle task.
	return 0;
}

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

#ifdef  DEBUG
// keep the linker happy.
void assert_failed(uint8_t* file, uint32_t line)
{
	for( ;; )
	{
	}
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////
#define PACKET_ASK_PREFIX   "~3:"   // 315Mhz ASK
#define PACKET_ASK_OFFSET   (sizeof(PACKET_ASK_PREFIX)-1)
uint8_t  pktOfFrame[PULSE_CAP_FRAME_MAXLEN+16] = PACKET_ASK_PREFIX;
void PulseCapture_OnFrame(PulseCapture* ch)
{
	uint8_t* p = pktOfFrame + PACKET_ASK_OFFSET;
	pbuf* packet = NULL;

	RNode_blink(BlinkPair_FLAG_A_ENABLE, 1, 1, 1);
	sleep(BLINK_INTERVAL_MSEC + (BLINK_INTERVAL_MSEC>>1)); // in order to blink before that of SI4432_OnSent()

//	pbuf* packet = pbuf_malloc(PULSEFRAME_LENGTH(ch->frame)+16);
//	offset += pbuf_write(packet, offset, CONST_STR_AND_LEN(~3:));
//	offset += pbuf_write(packet, offset, ch->frame, PULSEFRAME_LENGTH(ch->frame)+3);
//	offset += pbuf_write(packet, offset, CONST_STR_AND_LEN(\n\0));

	memcpy(p, ch->frame, PULSEFRAME_LENGTH(ch->frame)+3), p += PULSEFRAME_LENGTH(ch->frame)+3;
	memcpy(p, CONST_STR_AND_LEN(\n\0)), p += CONST_STR_LEN(\n\0);
	packet = pbuf_dup(pktOfFrame, p -pktOfFrame);

	SI4432_transmit(&si4432, packet);
	pbuf_free(packet);
}

void HtFrame_OnReceived(PulseCapture* ch, uint8_t* msg, uint8_t len)
{
}

void SI4432_OnReceived(SI4432* chip, pbuf* packet)
{
	uint8_t blinks=2;
	if (60 == packet->tot_len && 'h'== packet->payload[0] && 'i'== packet->payload[1])
		blinks =3;
	RNode_blink(BlinkPair_FLAG_B_ENABLE, 1, 2, blinks);
}

void SI4432_OnSent(SI4432* chip, pbuf* packet)
{
	RNode_blink(BlinkPair_FLAG_B_ENABLE, 1, 2, 1);
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

