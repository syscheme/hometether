#include "includes.h"
#include "pulses.h"
// #include "htcluster.h"

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
void Task_Main(void* p_arg);

// -----------------------------
// main()
// Description : This is the standard entry point for C code.  It is assumed that your code will call
//               main() once you have performed all necessary initialization.
// Argument: none.
// Return   : integer code
// Note    : none.
// -----------------------------
// #define TEST_RX
extern uint32_t __IO__ global_timer_msec;
char buf[2048];

#define HEARTBEAT_INTV (5*1000) // 5sec
#define NEXTSLEEP_MIN  (20)
#define NEXTSLEEP_MAX  (HEARTBEAT_INTV-1000)

int main(void)
{
	uint16_t nextSleep =0;
	uint8_t i;
	char*p = buf;
	uint16_t latHB =0;

	// const char msg[] = "hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n";
	//const pbuf* packet = pbuf_mmap((void*)"hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n", 60);
	pbuf* pmsg = pbuf_mmap(buf, sizeof(buf));

	BSP_Init();         // initialize        BSP functions.

	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 10, 10, 0);
	delayXmsec(2000);
	SI4432_init(&si4432);

	// the simple water loop
	Water_init();

	while(1)
	{
		// must take a separate task for PulseCapture_doParse()
		//if (PulseCapture_doParse(&pulsechs[0]) >0)
		//	continue;

		if (nextSleep <NEXTSLEEP_MIN)
			nextSleep = NEXTSLEEP_MIN;
		else if (nextSleep > NEXTSLEEP_MAX)
			nextSleep = NEXTSLEEP_MAX;
		
		delayXmsec(nextSleep);
		latHB += nextSleep;
		
		nextSleep = Water_do1round();

		if (latHB < HEARTBEAT_INTV)
			continue;

		latHB =0;

		p = (char*)pmsg->payload;
		p += snprintf(p, (char*)pmsg->payload + pmsg->len -p-2, "i%d@%d,%d,", STM32TemperatureADC2dC(ADC_DMAbuf[0]), ADC_DMAbuf[0], ADC_DMAbuf[1]);
		for (i=0; i<cTemperatures; i++)
			p += snprintf(p, ((char*)pmsg->payload) + pmsg->len -p-2, "t%d=%d,", i, temperatures[i]);
				
		SI4432_transmit(&si4432, pmsg); // packet);
	}

	return 0;
}

uint16_t temp, adc, tempadc;
char txBuf[256];

void PulseCapture_OnFrame(PulseCapture* ch)
{
//	RNode_blink(BlinkPair_FLAG_A_ENABLE, 1, 1, 2);
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
}

void SI4432_OnSent(SI4432* chip, pbuf* packet)
{
	RNode_blink(BlinkPair_FLAG_B_ENABLE, 1, 1, 2);
}

#if 0 
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
#endif // 0
