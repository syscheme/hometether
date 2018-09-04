#include "secu.h"
#include "bsp.h"
#include "app_cfg.h"
#include "../htod.h"
#include "hlkrm04.h"
#include "textmsg.h"

// -----------------------------
// global declares
// -----------------------------

// -----------------------------
// utility functions
// -----------------------------
static void InitProcedure(void)
{
	int i;

	// play the start-up song
	trace("start up song\r\n");
	for (i =0; 0 != startSong[i].pitchReload && 0 != startSong[i].lenByQuarter; i++)
		DQBoard_beep(startSong[i].pitchReload, startSong[i].lenByQuarter *80);

	// blicking the leds
	for (i =0; i<8; i++)
	{
		DQBoard_setLed(0, 1); delayXmsec(80); DQBoard_setLed(0, 0); 
		DQBoard_setLed(1, 1); delayXmsec(80); DQBoard_setLed(1, 0); 
		DQBoard_setLed(2, 1); delayXmsec(80); DQBoard_setLed(2, 0); 
		DQBoard_setLed(3, 1); delayXmsec(80); DQBoard_setLed(3, 0);
	}

	// test to operate the relays
	for (i =0; i < CHANNEL_SZ_Relay*2; i++)
	{
		SECU_setRelay(i, 1);  delayXmsec(300);
		SECU_setRelay(i, 0);  delayXmsec(300);
	}
}

void ThreadStep_doMainLoop(void)
{
	uint16_t nextSleep =0;

	SECU_init(1);

	trace("  * Task_main loop start\r\n");

	// dispatch on states and monitoring
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

		// scan and update the device status
		SECU_scanDevice();
		// ADJUST_BY_RANGE(nextSleep, NEXT_SLEEP_MIN, NEXT_SLEEP_MAX);
		nextSleep = NEXT_SLEEP_DEFAULT;

		ECU_blinkLED(0, 1, 20);
	}

	trace("  * Task_Main quits\r\n");
}

void AdminExt_sendMsg(uint8_t fdout, char* msg, uint8_t len);

// -----------------------------
// Task_Comm()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------

TextLineCollector tlcRS232 = {NULL, 0,};
HLKRM04  hlkrm04;
void Task_Comm(void* p_arg)
{
	int16_t i;
	char buf[16];

	trace("  * Task_Comm() starts\r\n");

	HLKRM04_init(&hlkrm04, &RS232, 0, HLK_TCP_SERVE_PORT);
	HLKRM04_tcpListen(&hlkrm04, HLK_TCP_SERVE_PORT);
	TextLineCollector_init(&tlcRS232, TextMsg_procLine, &RS232);

	// dispatch on states and monitoring
	while (1)
	{
#ifdef ECU_WITH_ENC28J60
		ThreadStep_doNicProc();
#endif // ECU_WITH_ENC28J60

		i = USART_receive(&RS232, (uint8_t*)buf, sizeof(buf));
		if (i>0)
		{
			TextLineCollector_push(&tlcRS232, buf, i);
			ECU_blinkLED(1, 1, 1);
			continue;
		}

//		ThreadStep_doMsgProc();
//		ThreadStep_doCanMsgIO();

		sleep(1);

	}

	USART_close(&RS232);
	
	trace("  * Task_Comm() quits\r\n");
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
	trace("  * Task_Timer starts\r\n");

	while (1)
	{
		sleep(1);
// 		ThreadStep_do1msecTimerProc();
	}

	trace("  * Task_Timer quits\r\n");
}

// -----------------------------
// Task_Main()
// Description : The task to dispatch by states and collect the sensor values
// Argument: p_arg  Argument passed to 'Task_Start()' by 'OSTaskCreate()'.
// Return   : none.
// Note    : none.
// -----------------------------
void Task_Main(void* p_arg)
{
	trace("  * Task_Main starts at %s mode\r\n", (_deviceState & dsf_Debug)?"DEBUG":"NORMAL");

	// play the start-up song
	InitProcedure();

	trace("  * creating sub tasks\r\n");

 	// create the Task_Comm()
	createTask(Comm, NULL);

	// create the Task_Timer()
	createTask(Timer, NULL);

	// do the main loop
	ThreadStep_doMainLoop();
}

#ifdef ECU_WITH_ENC28J60
#define DATA_BODY_START	(150)
static uint8_t MTU[ENC28J60_MTU_SIZE+1];
static __IO__ uint16_t MTU_offsetContentBody = DATA_BODY_START, MTU_lenContentBody=0;
#endif // ECU_WITH_ENC28J60

void ECU_sendMsg(void* netIf, pbuf* pmsg)
{
	if (NULL == pmsg || NULL == netIf)
		return;
	
	if(&RS232 == netIf)
		USART_transmit(&RS232, pmsg->payload, pmsg->len);

#ifdef ECU_WITH_ENC28J60
	if(&nic == netIf)
	{
		// fill the message into the MTU buffer
		MTU_lenContentBody = min(sizeof(MTU) - MTU_offsetContentBody, pmsg->len);
		memcpy(MTU + MTU_offsetContentBody, pmsg->payload, MTU_lenContentBody);
	}
#endif // ECU_WITH_ENC28J60
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
	pbuf* pmsg = TextMsg_composeRequest(TEXTMSG_VERBCH_OTH, klvc, eklvs);

	ECU_sendMsg(&RS232, pmsg);
	pbuf_free(pmsg);
}

#ifdef ECU_WITH_ENC28J60
void ECU_nicReceiveLoop(ENC28J60* nic);

void ThreadStep_doNicProc(void)
{
	CRITICAL_ENTER();
	ECU_nicReceiveLoop(&nic);
	CRITICAL_LEAVE();
}

void ECU_nicReceiveLoop(ENC28J60* nic)
{
	uint16_t plen=0;
	uint16_t tmp;
	char* p=NULL;
	char* verb=NULL, *uri=NULL;
	KLV klvs[TEXTMSG_ARGVS_MAX];
	uint8_t klvc;

	do {
		// step 1, get the next new packet:
		plen = ENC28J60_recvPacket(nic, sizeof(MTU)-1, MTU);

		// plen will ne unequal to zero if there is a valid packet (without crc error)
		if (plen <=0)
			break;

		// step 2. process the arp packet directly
		//   arp is broadcast if unknown but a host may also verify the mac address by sending it to 
		//   a unicast address.
		if (eth_type_is_arp_and_my_ip(MTU, plen))
		{
			arp_answer_from_request(nic, MTU);
			continue;
		}

		if (!eth_type_is_ip(MTU, plen))
			continue;

		// start processing IP packets here

		// step 3. check if ip packets are for us:
		if (!is_to_myip(MTU, plen) && !match_dest_ip(GroupIP, MTU, plen))
			continue;

		if (match_src_ip(MyIP, MTU, plen)) // ignore those from self
			continue;

		// step 4. respond ICMP directly
		if (MTU[IP_PROTO_P]==IP_PROTO_ICMP_V && MTU[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V)
		{
			// a ping packet, let's send pong
			echo_reply_from_request(nic, MTU,plen);
			continue;
		}

		// step 5. serve as a UDP server here
		if (MTU[IP_PROTO_P] == IP_PROTO_UDP_V)
		{
			// process UDP packets now
			if (!match_dest_ip(MyIP, MTU, plen))
				continue;

			if (MyServerPort != BYTES2WORD(MTU[UDP_DST_PORT_H_P], MTU[UDP_DST_PORT_L_P]))
				continue;  // not our listening port

			tmp = BYTES2WORD(MTU[UDP_SRC_PORT_H_P], MTU[UDP_SRC_PORT_L_P]);

			// initial the pos of the ContentBody buffer
			MTU_offsetContentBody = UDP_DATA_P, MTU_lenContentBody=0;
			TextMsg_procLine(nic, (char*) &MTU[UDP_DATA_P]); // this invocation will callback ECU_sendMsg() to fill the content body [MTU_offsetContentBody, +MTU_lenContentBody]

			udp_reply_with_data(nic, MTU, MTU_lenContentBody, tmp);
			continue;
		} // end of UDP

		// step 6. serve as a TCP/HTTP server here
		if (MTU[IP_PROTO_P]==IP_PROTO_TCP_V)
		{
			if (MyServerPort != BYTES2WORD(MTU[TCP_DST_PORT_H_P], MTU[TCP_DST_PORT_L_P]))
				continue;  // not our listening port

			if (MTU[TCP_FLAGS_P] & TCP_FLAGS_SYN_V)
			{ 
				// simply ack the SYN
				tcp_synack_from_syn(nic, MTU); // make_tcp_synack_from_syn does already send the syn,ack
				continue;
			}

			if (MTU[TCP_FLAGS_P] & TCP_FLAGS_ACK_V)
			{
				init_len_info(MTU); // init some data structures

				// we can possibly have no data, just ack:
				if (0 == get_tcp_data_pointer())
				{
					if (MTU[TCP_FLAGS_P] & TCP_FLAGS_FIN_V)
					{
						// finack, answer with ack
						tcp_ack_from_any(nic, MTU);
					}
					// just an ack with no data, wait for next packet
					continue;
				}

				do { // real TCP data packet

					// terminate the URI string
					p = strstr((char *)&(MTU[TCP_PAYLOAD_P]), " HTTP/1.");
					if (NULL !=p)
						*p= '\0';

					// fixup the URI
					p = strchr((char *)&(MTU[TCP_PAYLOAD_P]), ' '); // the space after HTTP verb
					if (NULL ==p)
					{
						plen=fill_tcp_data_str(MTU, 0, "HTTP/1.0 300 Bad Request\r\nContent-Type: text/html\r\n\r\n<h1>300 Bad Request</h1>");
						break;
					}

					// determin the method
					*p++ = '\0', verb = (char *)&(MTU[TCP_PAYLOAD_P]);
					
					// determine the uri
					uri = p;
					p = strchr(uri, '?');
					if (NULL ==p)
					{
						plen=fill_tcp_data_str(MTU, 0, "HTTP/1.0 300 Bad Request\r\nContent-Type: text/html\r\n\r\n<h1>300 No query parameters</h1>");
						break;
					}

					*p++ = '\0';

					// initial the pos of the ContentBody buffer
					MTU_offsetContentBody = TCP_PAYLOAD_P + DATA_BODY_START, MTU_lenContentBody=0;

					klvc = TextMsg_queryStr2KLVs(p, klvs, TEXTMSG_ARGVS_MAX);
					klvc = HtNode_accessByKLVs((0 == strcmp(verb, "SET")) ?1:0, klvc, klvs);

					// temporarily put the body at MTU[MTU_offsetContentBody] before prepare HTTP headers
					MTU_lenContentBody = TextMsg_KLVs2Json((char*)&MTU[MTU_offsetContentBody], sizeof(MTU) - MTU_offsetContentBody -100, klvc, klvs);

					// put the http header
					plen=fill_tcp_data_str(MTU, 0, "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n{ ");
					
					// move the content body after the header
					plen=fill_tcp_data_p(MTU, plen, (uint8_t*)&MTU[MTU_offsetContentBody], MTU_lenContentBody);
					plen=fill_tcp_data_str(MTU, plen, " }");

				} while (0);

				// send the TCP response
				tcp_ack_from_any(nic, MTU); // an ack first
				tcp_ack_with_data(nic, MTU, plen); // response with data

				continue;
			}  // end of TCP_FLAGS_ACK

			continue;
		}  // end of TCP

	} while (1);

}

#endif // ECU_WITH_ENC28J60

