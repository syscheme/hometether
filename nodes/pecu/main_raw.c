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
	uint32_t stampLastReportEnv =0, stampLastScan =0;
	uint32_t stampLastCheck=0, stampLastReset=0;
//	uint8_t cMsec=0;

#ifdef DEBUG
    debug();
#endif

	// initialize the board
	BSP_Init();
	PECU_init(1);
	ESP8266_init(&esp8266, &TTY1);

	trace("  * main loop start\r\n");

	// dispatch on states and monitoring
	while (1)
	{
		if (ESP8266_loopStep(&esp8266) >0)
			continue;

		// timer-ed in milliseconds
		// ---------------------------------------
		HtNode_do1msecScan();

		if (gClock_getElapsed(stampLastScan) <1000)
			continue;

		stampLastScan = gClock_msec;
		// scan and update the device status
		PECU_scanDevice();

		// notifications by timer
		// ---------------------------------------
		// NT.1 ODEvent_env
		if (CONFIG_notifyIntv_env>0 && gClock_getElapsed(stampLastReportEnv) >= (CONFIG_notifyIntv_env*1000))
		{
			stampLastReportEnv = gClock_msec; 
			HtNode_triggerEvent(ODEvent_env);
			// continue;
		}

		if (gClock_getElapsed(stampLastCheck) > (CONFIG_notifyIntv_env *1000*5))
		{
			stampLastCheck = gClock_msec;
			ESP8266_check(&esp8266);
			sleep(1000);
			continue;
		}

		if (gClock_getElapsed(stampLastReset) > max((CONFIG_notifyIntv_env *20), (120))*1000)
		{
			sleep(2000);
			ESP8266_init(&esp8266, &TTY1);
			sleep(2000);
			stampLastReset = gClock_msec;
			stampLastCheck = 0;
		}
	}

	trace("  * Task_Main quits\r\n");
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

