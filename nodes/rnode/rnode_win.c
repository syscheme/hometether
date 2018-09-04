#include "includes.h"
#include "pulses.h"
#include "htddl.h"
#include "htcluster.h"
#include "textmsg.h"

void TEST_pulseCap(void);
void TEST_htddl(void);
void TEST_LRU();

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

#define mainREGION_1_SIZE	2001
#define mainREGION_2_SIZE	18005
#define mainREGION_3_SIZE	1007
static void  prvInitialiseHeap( void );
volatile uint8_t _interruptDepth=0;

void Task_Main(void* p_arg);

// -----------------------------
// global declares
// -----------------------------
#define TaskPrio_tMain                (TaskPrio_Base +0)
#define TaskStkSz_tMain               (configMINIMAL_STACK_SIZE) //96
void    Task_tMain(void* p_arg);

// nRF24L01 nrf24l01 = { RF_NODEID_THIS, NULL, SPI2, {GPIOB, GPIO_Pin_1}, {GPIOB, GPIO_Pin_0}, EXTI_Line8, NULL};
void sendWirelessTxtMsg(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum, const char* txtMsg);

uint16_t ADC_DMAbuf[ADC_CHS];

uint16_t temp, adc, tempadc;
char txBuf[256];

/*
void nRF24L01_OnReceived(nRF24L01* chip, uint8_t pipeId, uint8_t* payload, uint8_t payloadLen)
{
}
*/

void nRF24L01_OnSent(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNun, uint8_t* payload)
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

void BSP_Init(void)
{}

void RNode_blink(uint8_t mode, uint8_t reloadA, uint8_t reloadB, uint8_t blinks)
{
}

PulseCapture pulsechs[CCP_NUM];
DS18B20  ds18b20;
nRF24L01 nrf24l01 = {0x00, 0x00, NULL, NULL, NULL, 0, 0 };
SI4432   si4432   = {0x00, NULL, NULL, NULL, NULL, 0}; 

///*
//static void  prvInitialiseHeap( void )
//{
///* This demo uses heap_5.c, so start by defining some heap regions.  This is
//only done to provide an example as this demo could easily create one large heap
//region instead of multiple smaller heap regions - in which case heap_4.c would
//be the more appropriate choice.  No initialisation is required when heap_4.c is
//used.  The xHeapRegions structure requires the regions to be defined in order,
//so this just creates one big array, then populates the structure with offsets
//into the array - with gaps in between and messy alignment just for test
//purposes. */
//static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
//volatile uint32_t ulAdditionalOffset = 19; /* Just to prevent 'condition is always true' warnings in configASSERT(). */
//const HeapRegion_t xHeapRegions[] =
//{
//	/* Start address with dummy offsets						Size */
//	{ ucHeap + 1,											mainREGION_1_SIZE },
//	{ ucHeap + 15 + mainREGION_1_SIZE,						mainREGION_2_SIZE },
//	{ ucHeap + 19 + mainREGION_1_SIZE + mainREGION_2_SIZE,	mainREGION_3_SIZE },
//	{ NULL, 0 }
//};
//
//	/* Sanity check that the sizes and offsets defined actually fit into the
//	array. */
//	configASSERT( ( ulAdditionalOffset + mainREGION_1_SIZE + mainREGION_2_SIZE + mainREGION_3_SIZE ) < configTOTAL_HEAP_SIZE );
//
//	vPortDefineHeapRegions( xHeapRegions );
//}
//*/

// ==================================
// TEST: loopback pluse-capture
// ==================================

PulseCapture ch = { {&ch, 0x00}, };

void HtFrame_OnReceived(PulseCapture* ch, uint8_t* msg, uint8_t len)
{
	printf("htframe[%dB]: ", len);
	for (; len>0; len--)
		printf("%02x ", *(msg++));
	printf("\n");
}

void PulseCapture_OnFrame(PulseCapture* ch)
{
	int i=0;
	printf("frame[%dB,%dusec,0-%02xH,1-%02xH]: ", PULSEFRAME_LENGTH(ch->frame), eByte2value(ch->frame[PULSEFRAME_OFFSET_SHORTDUR]), ch->frame[PULSEFRAME_OFFSET_STYLE_B0], ch->frame[PULSEFRAME_OFFSET_STYLE_B1]);
	for (i=PULSEFRAME_OFFSET_SIGDATA; i< PULSEFRAME_LENGTH(ch->frame); i++)
		printf("%02x ", ch->frame[i]);
	printf("\n");

	HtFrame_validateAndDispatch(ch);
}


void PulsePin_send(PulsePin pin, uint16_t durH, uint16_t durL)
{
	PulseCapture_Capfill(&ch, durH, 1);
	PulseCapture_Capfill(&ch, durL, 0);
}

#define TaskPrio_HtFrameSender                (TaskPrio_Base +10)
#define TaskStkSz_HtFrameSender               (configMINIMAL_STACK_SIZE)

void Task_HtFrameSender(void *args)
{
	uint8_t msg[] ={0x1f,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0,0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef}, msglen= 1; // sizeof(msg);
	while (1)
	{
		sleep(200);
		++(*((volatile uint16_t*)msg));
		HtFrame_send(ch.capPin, msg, 5, 1, 0x10);
	}
}

#define TaskPrio_PulseCap                (TaskPrio_Base +11)
#define TaskStkSz_PulseCap               (configMINIMAL_STACK_SIZE)
void Task_PulseCap(void *args)
{
	while(1)
	{
		if (!PulseCapture_doParse(&ch))
			sleep(1);
	}
}

void TEST_pulseCap(void)
{
	PulseCapture_init(&ch);

	createTask(HtFrameSender, NULL);
	createTask(PulseCap, NULL);
	runTaskScheduler();
}

// ==================================
// TEST: loopback pluse-capture
// ==================================

static void printpacket(const char* hint, pbuf* packet)
{
	int i=0;
	printf("%s packet[%d]: ", hint, packet->tot_len);
	for (; packet; packet = packet->next)
	{
		for(i=0; i<packet->len; i++)
			printf("%02x ", packet->payload[i]);
	}
	printf("\r\n");
}

hterr TEST_OnReceived(HtNetIf *netif, pbuf* packet, uint8_t powerLevel)
{
	printpacket("LOOPTEST_OnReceived()", packet);
	return NetIf_OnReceived_Default(netif, packet, powerLevel);
}

hterr LOOPTEST_doTX(HtNetIf *netif, pbuf *packet, hwaddr_t destAddr)
{
	char hint[40];
	snprintf(hint, sizeof(hint)-2, "LOOPTEST_doTX(%08x)", *((uint32_t*)destAddr));
	printpacket(hint, packet);
	return netif->cbReceived(netif, packet, 0x0f); // for LOOPBACK test
}

void htddl_OnReceived(HtNetIf* netif, const hwaddr_t src, pbuf* data)
{
	printpacket("htddl_OnReceived()", data);
}

hwaddr_t bindaddr = {0x12,0x34,0x56,0x78};

HtNetIf netifUplink;

#define ATCMD_LEADING "at+"

#define NETIF_COUNT     (3)
HtNetIf gNetIfs[NETIF_COUNT];

// HtNetIf netif_nRF24L01={NULL,}, netif_SI4431={&netif_nRF24L01,}, netif_ENC28j60= {&netif_SI4431,}, *p1stNetIf= &netif_ENC28j60;

const char* doATCommand(char* cmd, const char* cmdtail, char** ppOutput, uint16_t* outlen)
{
	int i, j =0;
	HtNetIf *pNetIf = NULL;
	uint8_t tmpb, eNid; 
	uint8_t *p, *q;
	char tmpbuf[20]={0};

	do {
		// CMD>> list interfaces: at+ifs=?
		//                 resp  at+ifs=2
		if (0 == strncmp(cmd, CONST_STR_AND_LEN(ifs=?))) // list interfaces
		{
			j = snprintf(*ppOutput, *outlen, ATCMD_LEADING "ifs=%d\n", NETIF_COUNT); *ppOutput += j; outlen -=j;
			break;
		}

		// CMD>> query interfaces: at+if1=?
		//          sample resp: at+if1=24L01;1000;16
		if (0 == strncmp(cmd, CONST_STR_AND_LEN(if)) && 0 ==strncmp(cmd+3, CONST_STR_AND_LEN(=?)))
		{
			j = atoi(cmd +CONST_STR_LEN(if)); // the no. of interface
			if (j >= NETIF_COUNT)
			{
				j = snprintf(*ppOutput, *outlen, ATCMD_LEADING "if%d=null\n", j); *ppOutput += j; outlen -=j;
				break;
			}

			gNetIfs[j].hwaddr_len = min(gNetIfs[j].hwaddr_len, sizeof(hwaddr_t));
			for (i=0; i< gNetIfs[j].hwaddr_len; i++)
				snprintf(tmpbuf + 2*i, sizeof(tmpbuf)-2*i, "%02x", gNetIfs[j].hwaddr[i] & 0xff);

			j = snprintf(*ppOutput, *outlen, ATCMD_LEADING "if%d=%s;%s;%d\n", j, gNetIfs[j].name, tmpbuf, gNetIfs[j].mtu); *ppOutput += j; outlen -=j;
			break;
		}

/*
		// CMD>> set the bindaddr of interfaces: at+bind1=<hex-localaddr>
		//                          sample echo: at+bind1=0123456789ab
		if (0 == strncmp(cmd, CONST_STR_AND_LEN(bind)) && 0 ==strncmp(cmd+5, CONST_STR_AND_LEN(=)))
		{
			j = atoi(cmd+4); // the no. of interface
			if (j >= NETIF_COUNT)
			{
				j = snprintf(*ppOutput, *outlen, ATCMD_LEADING "bind%d=null\n", j); *ppOutput += j; outlen -=j;
				break;
			}
			j = snprintf(*ppOutput, *outlen, ATCMD_LEADING "bind1=%012x\n", 0x01234567); *ppOutput += j; outlen -=j;
			break;
		}
*/
		// CMD>> send a string thru interfaces: at+send1=<destAddr>;<payload-len>;<string>
		//                          sample resp: OK

		// CMD>> send a hexstr thru interfaces: at+shex1=<destAddr>;<payload-len>;<hex-data>
		//                          sample resp: OK

		// CMD>> adopt a node thru interfaces: at+adopt1=<assigned-eNid>;<destNodeId>;<thruNodeId>[;<accessKey>]
		//                          sample resp: OK
		//       in responding to handle HtEdge_OnNodeAdvertize()
		if (0 == strncmp(cmd, CONST_STR_AND_LEN(adopt)) && 0 ==strncmp(cmd +CONST_STR_LEN(adopt)+1, CONST_STR_AND_LEN(=)))
		{
			// the netIf specified
			j = atoi(cmd +CONST_STR_LEN(adopt));
			if (j >= NETIF_COUNT)
				break;

			pNetIf = &gNetIfs[j];

			// the eNid specified
			p = cmd + CONST_STR_LEN(adopt)+2;
			p += hex2byte(p, &eNid); 
			if (';' != *p++)
				break;
			
			// the destNodeId specified
			for (i=0, q=tmpbuf; i < sizeof(NodeId_t); i++)
			{
				j = hex2byte(p, &tmpb);
				if (j <=0)
					break;
				p +=j;
				*q++ = tmpb;
			}

			if (';' != *p++)
				break;

			// the thruNodeId specified
			for (i=0, q=tmpbuf +sizeof(NodeId_t); i < sizeof(NodeId_t); i++)
			{
				j = hex2byte(p, &tmpb);
				if (j <=0)
					break;
				p +=j;
				*q++ = tmpb;
			}

			// the accessKey if specified
			memset(tmpbuf +sizeof(NodeId_t)*2, 0x00, sizeof(uint32_t));

			if (';' == *p++)
			{
				for (i=0, q=tmpbuf +sizeof(NodeId_t)*2; i < sizeof(uint32_t); i++)
				{
				j = hex2byte(p, &tmpb);
				if (j <=0)
					break;
				p +=j;
				*q++ = tmpb;
				}
			}

			// send the ADOPT message
			HtEdge_sendADOPT(pNetIf, eNid, *((NodeId_t*)tmpbuf), *((NodeId_t*)(tmpbuf+sizeof(NodeId_t))),*((uint32_t*)(tmpbuf+sizeof(NodeId_t)*2)));
			// respond OK
			j = snprintf(*ppOutput, *outlen, "OK\n"); *ppOutput += j; outlen -=j;
			break;
		}

		// CMD>> get obj value thru interfaces: at+gov=<eNid>/<objId>+<subIdx>
		//                          async resp: see HtNode_OnObjectValues()
		if (0 == strncmp(cmd, CONST_STR_AND_LEN(gov=)))
		{
			// the eNid specified
			p = cmd + CONST_STR_LEN(gov=);
			p += hex2byte(p, &eNid); 

			// the object specified
			if ('/' != *p++ || (j = hex2byte(p, &tmpb)) <=0)
				break;
			else p += j;

			tmpbuf[0] = tmpb;

			// the subIdx specified
			if ('+' != *p++ || (j = hex2byte(p, &tmpb)) <=0)
				break;
			else p += j;

			tmpbuf[1] = tmpb;

			// send the GETOBJ message
			HtNode_sendGETOBJ(eNid, tmpbuf[0], tmpbuf[1]);
			// respond OK
			j = snprintf(*ppOutput, *outlen, "OK\n"); *ppOutput += j; outlen -=j;
			break;
		}

		// CMD>> set obj value thru interfaces: at+sov=<eNid>/<objId>+<subIdx>:<len>;<hex-data>
		//                         sample resp: OK
		if (0 == strncmp(cmd, CONST_STR_AND_LEN(sov=)))
		{
			// the eNid specified
			p = cmd + CONST_STR_LEN(sov=);
			p += hex2byte(p, &eNid); 

			// the object specified
			if ('/' != *p++ || (j = hex2byte(p, &tmpb)) <=0)
				break;
			else p += j;

			tmpbuf[0] = tmpb;

			// the subIdx specified
			if ('+' != *p++ || (j = hex2byte(p, &tmpb)) <=0)
				break;
			else p += j;

			tmpbuf[1] = tmpb;

			// the value-len specified
			if (':' != *p++ || (j = hex2byte(p, &tmpb)) <=0)
				break;
			else p += j;

			tmpbuf[2] = tmpb;

			// the hex-value
			if (';' != *p++)
				break;

			for (i=0, q=tmpbuf +3; i < tmpbuf[2]; i++)
			{
				j = hex2byte(p, &tmpb);
				if (j <=0)
					break;
				p +=j;
				*q++ = tmpb;
			}

			// send the GETOBJ message
			HtNode_sendSETOBJ(eNid, tmpbuf[0], tmpbuf[1], tmpbuf +3, tmpbuf[2]);
			// respond OK
			j = snprintf(*ppOutput, *outlen, "OK\n"); *ppOutput += j; outlen -=j;
			break;
		}
	} while(0);

	return cmdtail;
}

void TextMsg_OnResponse(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
}

void TextMsg_OnSetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
	pbuf* pmsg=NULL;
	klvc = HtOD_accessByKLVs(1, klvc, klvs);
	pmsg = TextMsg_composeResponse(cseq, klvc, klvs);
}

void TextMsg_OnGetRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
	pbuf* pmsg=NULL;
	klvc = HtOD_accessByKLVs(0, klvc, klvs);
	pmsg = TextMsg_composeResponse(cseq, klvc, klvs);
}

void TextMsg_OnPostRequest(void* netIf, uint8_t cseq, uint8_t klvc, KLV klvs[])
{
}

void TEST_atCommands(void)
{
	char output[1024], *pout = output;
	uint16_t outlen= sizeof(output)-2;
	const char* msg = "at+ifs=?\n"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
		ATCMD_LEADING "if1=?\n"
		ATCMD_LEADING "bind1=0123456789\n"
		ATCMD_LEADING "dest1=0123456789ab\n"
		ATCMD_LEADING "send1=hello\n"
		ATCMD_LEADING "adopt1=05;01234567;89abcdef\n"
		ATCMD_LEADING "gov=05/01+00\n"
		ATCMD_LEADING "sov=05/01+00:02;0123\n"
		;
	const char* p = msg, *q;

msg = "+00ifs=?\n"
		"+01ws=?\n"
		"\0";

	while (NULL !=p)
	{
		if (NULL == (p = strstr(p, ATCMD_LEADING)))
			break;

		if (NULL == (q = strchr(p, '\n')))
			break;

		// p = doATCommand(p+sizeof(ATCMD_LEADING)-1, q++, &pout, &outlen);
	}
}

void TEST_htddl(void)
{
	static const char* msg = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		;

	pbuf* p =pbuf_mmap((void*)msg, 522);
	hwaddr_t dest;
	uint16_t n = htddl_send(NULL, dest, p, HTDDL_MSG_LEN_MAX);
}

void TEST_LRU()
{
	LRUMap lru;
	uint8_t i, j;
	int k;
	LRUMap_init(&lru, sizeof(uint8_t), sizeof(uint8_t), 10);
	for (i=0; i< 20; i++)
		LRUMap_set(&lru, &i, &i);
	for (i=15; i>5; i--)
		LRUMap_set(&lru, &i, &i);
	for (k=0; k< 1; k++)
	{
		j=5;
		LRUMap_get(&lru, &i, &j, TRUE);
	}
	for (k=0; k< 0x8000; k++)
		LRUMap_set(&lru, &i, &i);
	LRUMap_free(&lru);
}

// -----------------------------
// Task_Main()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
void Task_tMain(void* p_arg)
{
	// TEST_LRU();
	TEST_htddl();
	// TEST_pulseCap();
}

////////////////////
void HtNode_OnObjectQueried(HtNetIf* netif, uint8_t objectId, uint8_t subIdx)
{
	vTaskSuspendAll();
	printf("HtNode_OnObjectQueried() object[%d:%d]\n", objectId, subIdx);
	xTaskResumeAll();
}

void HtNode_OnObjectChanged(HtNetIf* netif, uint8_t objectId, uint8_t subIdx, const uint8_t* newValue, uint8_t vlen)
{
	vTaskSuspendAll();
	printf("HtNode_OnObjectChanged() object[%d:%d]: ", objectId, subIdx);
	while (vlen--)
		printf("%02x ", *newValue++);
	printf("\n");
	xTaskResumeAll();
}

// NodeId_t nidThis = 0x123456;
// __IO__ uint32_t gClock_msec =0;

// sample OD declaration:
uint16_t data1=0x1234;
uint32_t data2=0x87654321;

USR_OD_DECLARE_START()
  OD_DECLARE(data1,  ODTID_WORD,  ODF_Readable|ODF_Writeable, &data1, sizeof(data1))
  OD_DECLARE(data2,  ODTID_DWORD, ODF_Readable|ODF_Writeable, &data2, sizeof(data2))
  OD_DECLARE(data2x, ODTID_BYTE,  ODF_Readable|ODF_Writeable, &data2, sizeof(data2))
USR_OD_DECLARE_END()

// sample OD declaration:
enum {
ODEvent_USER_Event1 = ODEvent_USER_MIN, 
};
//
static const uint8_t event_objIds[] = {ODID_data1, ODID_data2, ODID_data2x, ODID_NONE};
USR_ODEVENT_DECLARE_START()
 	ODEVENT_DECLARE(USER_Event1, event_objIds)
USR_ODEVENT_DECLARE_END()

// ---------------------------------------------------------------------------
// dummy for HtEdge
// ---------------------------------------------------------------------------
void HtEdge_OnNodeAdvertize(HtNetIf* netif, uint8_t eNid, NodeId_t theNode, NodeId_t nidBy)
{
	// TODO: collect (orphanNodeId, srcNodeId, ttl, count++)
	// and wait for a while, then pick the path has maximal count and maximal ttl
	// overwrite the orphan->srcNodeId by calling LRUMap_set(downLinks, theParty, srcNodeId) for the picked path
	// ToUSART: ad:<netif->name>:<eNid>:<theNode>:<nidBy>
	char atmsg[40] = "\0", *p =atmsg;
	snprintf(p, sizeof(atmsg)-3, "AD+%04x:%x;%04x;%s\n", theNode, eNid, nidBy, netif->name); 
	trace("OnNodeAdvertize() %s", atmsg);
}

void HtNode_OnObjectValues(uint8_t eNid, uint8_t objectId, uint8_t subIdx, uint8_t* value, uint8_t vlen)
{
	char atmsg[40] = "\0", *p =atmsg;
	p += snprintf(p, sizeof(atmsg)-3, "OV+%x/%x+%xL%x:", eNid, objectId, subIdx, vlen); 
	while (vlen--)
		p += snprintf(p, atmsg + sizeof(atmsg)-3 -p, "%02x", *(value++));
	*p++ = '\n'; *p='\0';

	trace("OnObjectValues() %s", atmsg);
}

void HtNode_OnEventIssued(uint8_t klvc, const KLV eklvs[])
{
	pbuf* pmsg = TextMsg_composeRequest(TEXTMSG_VERBCH_OTH, klvc, eklvs);
	pbuf_free(pmsg);

	//char jstr[1024];
	//uint8_t len = TextMsg_KLVs2Json(jstr, sizeof(jstr), klvc, eklvs);
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
	char msgbuf[100] = "+12?abc=1234&data1:1&data=safasf&data2=";
	uint8_t cBusy =0, i;
	const char msg[] = "hi world\7\r\nblah 1, blah 2, blah 3, -- a lot of holly crap\r\n";
	pbuf* packet = pbuf_mmap((void*)"hi world, blah 1, blah 2, blah 3, -- a lot of holly crap\r\n", 60);
	// KLV klvs[2] = {{""}};

//	prvInitialiseHeap();
	// packet = TextMsg_composeRequest(msg, 20);
	// i = TextMsg_decode(packet->payload);
	// pbuf_free(packet);

	HtNode_triggerEvent(ODEvent_USER_Event1);
	HtNode_do1msecScan();

	TextMsg_procLine(NULL, msgbuf);
	TEST_atCommands();

	HtNetIf_init(&netifUplink, NULL, "lo0", "",
		HTDDL_MSG_LEN_MAX, NULL, sizeof(NodeId_t), 0,
		TEST_OnReceived, LOOPTEST_doTX, NULL);

/*
	TEST_LRU();
	// TEST_htddl();
	// TEST_pulseCap();

//	BSP_Init();         // initialize        BSP functions.

//	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 10, 10, 0);
	delayXmsec(2000);
//	SI4432_init(&si4432);
#ifdef TEST_RX
	SI4432_setMode_RX(&si4432);
	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 4, 4, 2);
#else
	delayXmsec(3000);
	RNode_blink(BlinkPair_FLAG_A_ENABLE | BlinkPair_FLAG_B_ENABLE, 4, 4, 2);
*/
	// create the main task.
	// xTaskCreate( Task_Main, "main", TaskStkSz_Main, NULL, TaskPrio_Main, NULL );
	createTask(Main, NULL);
	runTaskScheduler();
/*
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

		// printf("IDLE\r\n");

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
	}
*/
	return 0;
}

