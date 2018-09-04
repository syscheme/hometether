#include "secu.h"
#include "bsp.h"
#include "app_cfg.h"
#include "../htod.h"

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
#ifdef DEBUG
    debug();
#endif

	// initialize the board
	BSP_Init();

	// create the main task.
	createTask(Main, NULL);

	// The suicide tasks must be created last as they need to know how many
	// tasks were running prior to their creation in order to ascertain whether
	// or not the correct/expected number of tasks are running at any given time.
    // vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );
	
	// run the scheduler
	runTaskScheduler();

	// Will only get here if there was not enough heap space to create the idle task. 
	return 0;
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

void HtNode_OnObjectQueried(HtNetIf* nic, uint8_t objectId, uint8_t subIdx)
{
}

void HtNode_OnObjectChanged(HtNetIf* nic, uint8_t objectId, uint8_t subIdx, const uint8_t* newValue, uint8_t vlen)
{
}

// ---------------------------------------------------------------------------
// dummy for HtEdge
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

