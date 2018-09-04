#include "includes.h"
#include "pulses.h"
#include "htcluster.h"

#include "esp8266.h"

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

extern USART TTY1;
int offset =0;
char buf[100];
ESP8266 esp8266;

void test_ESP8266S()
{
	int i;
	uint32_t stampLastCheck =0;

	// initialize the board
	BSP_Init();
	delayXmsec(2000);

	ESP8266_init(&esp8266, &TTY1);
	ESP8266_listen(&esp8266, 8888);

	while(1)
	{
		if (ESP8266_loopStep(&esp8266) >0)
			continue;

		delayXmsec(1);
		if (gClock_getElapsed(stampLastCheck) <5000)
			continue;

		stampLastCheck = gClock_msec;
		ESP8266_check(&esp8266);
		// ESP8266_transmit(&esp8266, 0, CONST_STR_AND_LEN2("hello"));
	}

	USART_close(&TTY1);
}

// -------------------------------------------------------------------------------
// Dictionary expose for HtNode
// -------------------------------------------------------------------------------
enum {
	ODID_lstatus     = ODID_USER_MIN,      // the Ecu_localState
	ODID_mstatus,      // the Ecu_motionState
	ODID_temps,  ODID_lumins, ODID_adc,

	// configurations
	ODID_CFGtMask, ODID_CFGtIntv,
	ODID_CFGmMask, ODID_CFGmIntv, ODID_CFGmOuter, ODID_CFGmBR,
	ODID_CFGlMask, ODID_CFGlIntv, ODID_CFGlDiff,

};

uint16_t  Ecu_localState           =0x1234;     // =0;
uint16_t  Ecu_motionState          =0xabcd;    // =0;

// OD declaration:
USR_OD_DECLARE_START()
  OD_DECLARE(lstatus,     ODTID_WORD, ODF_Readable, &Ecu_localState,  sizeof(Ecu_localState))
  OD_DECLARE(mstatus,     ODTID_WORD, ODF_Readable, &Ecu_motionState, sizeof(Ecu_motionState))
/*
  OD_DECLARE(temps,       ODTID_BYTES, ODF_Readable, temperatures,     sizeof(temperatures))
  OD_DECLARE(lumins,      ODTID_BYTES, ODF_Readable, &ADC_DMAbuf[1],   sizeof(ADC_DMAbuf)-sizeof(uint16_t))
  OD_DECLARE(adc,         ODTID_BYTES, ODF_Readable, ADC_DMAbuf,       sizeof(ADC_DMAbuf))

  // configuations
  OD_DECLARE(CFGtMask,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_temperatureMask,  sizeof(CONFIG_temperatureMask))
  OD_DECLARE(CFGtIntv,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_temperatureIntv,  sizeof(CONFIG_temperatureIntv))
  OD_DECLARE(CFGmMask,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionDiffMask,   sizeof(CONFIG_motionDiffMask))
  OD_DECLARE(CFGmIntv,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionDiffIntv,   sizeof(CONFIG_motionDiffIntv))
  OD_DECLARE(CFGmOuter,   ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionOuterMask,  sizeof(CONFIG_motionOuterMask))
  OD_DECLARE(CFGmBR,      ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_motionBedMask,    sizeof(CONFIG_motionBedMask))
  OD_DECLARE(CFGlMask,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffMask,   sizeof(CONFIG_lumineDiffMask))
  OD_DECLARE(CFGlIntv,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffIntv,   sizeof(CONFIG_lumineDiffIntv))
  OD_DECLARE(CFGlDiff,    ODTID_BYTES, ODF_Readable|ODF_Writeable, &CONFIG_lumineDiffTshd,   sizeof(CONFIG_lumineDiffTshd))
*/

USR_OD_DECLARE_END()

// -------------------------------------------------------------------------------
// Definition of HtNode Events
// -------------------------------------------------------------------------------
// TODO
enum {
	ODEvent_StateChanged = ODEvent_USER_MIN, 
};
//
static const uint8_t eventOD_StateChanged[] = { ODID_lstatus, ODID_mstatus, ODID_NONE };
USR_ODEVENT_DECLARE_START()
 	ODEVENT_DECLARE(StateChanged, eventOD_StateChanged)
USR_ODEVENT_DECLARE_END()

void test_ESP8266C()
{
	uint32_t stampLastCheck =0;

	// initialize the board
	BSP_Init();
	delayXmsec(2000);

	ESP8266_init(&esp8266, &TTY1);

	while(1)
	{
		if (ESP8266_loopStep(&esp8266) >0)
			continue;

		HtNode_do1msecScan();

		delayXmsec(1);
		if (gClock_getElapsed(stampLastCheck) <2000)
			continue;

		stampLastCheck = gClock_msec;
		HtNode_triggerEvent(ODEvent_StateChanged);
		ESP8266_check(&esp8266);
		// ESP8266_transmit(&esp8266, 0, CONST_STR_AND_LEN2("hello"));
	}

	USART_close(&TTY1);

}


int main(void)
{
	test_ESP8266C();
	return 0;
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
int test_SI4432(void)
{
	uint8_t cBusy =0, i;
	// const char msg[] = "hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n";
	pbuf* packet = pbuf_mmap((void*)"hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n", 60);

	BSP_Init();         // initialize        BSP functions.

	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 10, 10, 0);
	delayXmsec(2000);
	SI4432_init(&si4432);
#ifdef RECVR
	SI4432_setMode_RX(&si4432);
	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 4, 4, 2);
	while(1);
#else
	delayXmsec(3000);
	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 4, 4, 2);

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
	delayXmsec(1);
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
	// if ('s'== packet->payload[0] && 'i'== packet->payload[3])
	if (0 == memcmp(packet->payload, "+FFw", 4))
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

// ---------------------------------------------------------------------------
// supplies of test_ESP8266()
// ---------------------------------------------------------------------------
void ECU_sendMsg(void* netIf, pbuf* pmsg)
{
	if (NULL == pmsg || NULL == netIf)
		return;
	
	if(((void*)&esp8266) == netIf)
	{
		ESP8266_transmit(&esp8266, esp8266.activeConn &0x0f, pmsg->payload, pmsg->len);
		sleep(1000);
		ESP8266_close(&esp8266, esp8266.activeConn &0x0f);
		ESP8266_listen(&esp8266, 8888);
	}
}

// portal impl of TextMsg
void TextMsg_OnResponse(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
}

void TextMsg_OnSetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
	pbuf* pmsg=NULL;
	klvc = HtNode_accessByKLVs(1, klvc, klvs);
	pmsg = TextMsg_composeResponse(cseq, klvc, klvs);

	ECU_sendMsg(netIf, pmsg);
	pbuf_free(pmsg);
}

void TextMsg_OnGetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
	pbuf* pmsg=NULL;
	klvc = HtNode_accessByKLVs(0, klvc, klvs);
	pmsg = TextMsg_composeResponse(cseq, klvc, klvs);

	ECU_sendMsg(netIf, pmsg);
	pbuf_free(pmsg);
}

void TextMsg_OnPostRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
}

void HtNode_OnEventIssued(uint8_t klvc, const KLV eklvs[])
{
	ESP8266_jsonPost2Coap(&esp8266, "192.168.199.120", 8080, BSP_nodeId, klvc, eklvs);
}

void ESP8266_OnReceived(ESP8266* chip, char* msg, int len, bool bEnd)
{
	if (!bEnd || len <=0)
		return;

	TextMsg_procLine(chip, msg);
}

uint8_t heap[PBUF_MEMPOOL2_BSIZE *20];
uint8_t iheap =0;

void* heap_malloc(size_t size)
{
	uint8_t* p = heap + PBUF_MEMPOOL2_BSIZE*iheap++;
	iheap %=20;
	return p;
}

void heap_free(void* p)
{
}

