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
// -----------------------------
// global declares
// -----------------------------
void Task_Main(void* p_arg);

// nRF24L01 nrf24l01 = { RF_NODEID_THIS, NULL, SPI2, {GPIOB, GPIO_Pin_1}, {GPIOB, GPIO_Pin_0}, EXTI_Line8, NULL};
void sendWirelessTxtMsg(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg);

uint16_t ADC_DMAbuf[ADC_CHS];

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
	uint8_t cBusy =0, i;
	// const char msg[] = "hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n";
	pbuf* packet = pbuf_mmap((void*)"hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n", 60);

	BSP_Init();         // initialize        BSP functions.

	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 10, 10, 0);
	delayXmsec(2000);
	SI4432_init(&si4432);
#ifdef TEST_RX
	SI4432_setMode_RX(&si4432);
	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 4, 4, 2);
#else
	delayXmsec(3000);
	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 4, 4, 2);

	// create the main task.
	// xTaskCreate( Task_Main, "main", TaskStkSz_Main, NULL, TaskPrio_Main, NULL );
	createTask(Main, NULL);

	while(1)
	{
		SI4432_transmit(&si4432, packet);
		delayXmsec(2000);
	}
#endif // TEST_RX

	while(1)
	{
		for (i=0, cBusy=0; i < CCP_NUM; i++)
		{
			if (PulseCapture_doParse(&pulsechs[i]) >0)
				cBusy++;
		}

		if (cBusy >0)
			continue;

/*		// printf("IDLE\r\n");

		if (timeout_idle) // 1sec
		{
			cContinuousIdle =10;
			continue;
		}

		// continuously has no capture, enter the sleep mode
		// step 1. reset the pulse channel ctx first
		memset(&pch0, 0x00, sizeof(pch0));
#if CCP_NUM >1
		memset(&pch1, 0x00, sizeof(pch1));
#endif // CCP_NUM

		if (cContinuousIdle--)
			continue;
		cContinuousIdle = 10;

#ifdef SLEEP_AT_IDLE
		// step 2. enter the sleep mode
		PCON |= 0x02;

		// after wake-up, the following _nop_() will be executed for the clock becomes stable 
		_nop_();  _nop_(); _nop_(); _nop_(); 
		timeout_idle = TIMEOUT_IDLE_RELOAD;
#endif // SLEEP_AT_IDLE
*/
	}

	return 0;
}

uint16_t temp, adc, tempadc;
char txBuf[256];

void nRF24L01_OnReceived(nRF24L01* chip, uint8_t pipeId, uint8_t* payload, uint8_t payloadLen)
{
}

void nRF24L01_OnSent(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNun, uint8_t* payload)
{
}

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
