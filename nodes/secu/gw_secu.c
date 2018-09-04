#include "bsp.h"
#include "app_cfg.h"
#include "../htod.h"

extern uint8_t fdadm;
extern uint8_t fdext;
extern uint8_t fdcan;
extern uint8_t fdeth;

// about flags in EcuConf_fwdAdm and EcuConf_fwdExt
#define FWD_FROM_CAN           0x03  // two-bit type of _eForwarder_Type
#define FWD_FROM_OTHER         0x06  // two-bit type of _eForwarder_Type
#define FWD_EXT_FROM_ADMIN     FWD_FROM_OTHER
#define FWD_ADMIN_FROM_EXT     FWD_FROM_OTHER

enum _eForwarder_Type {
	fwdt_None=0, fwdt_Original, fwdt_CompactText, fwdt_RichText
};

uint8_t  EcuConf_fwdAdm = fwdt_RichText;
uint8_t  EcuConf_fwdExt = 0; // fwdt_CompactText;

#ifndef __CC_ARM

int fputcx(int ch, FILE *f)
{
	putc(ch, (((uint8_t)f) == fdext) ? stderr : stdout);
	return (ch);
}

#define fputc fputcx
#endif // __CC_ARM

#ifdef WIN32
extern void WinSim_sendMsg(uint8_t fdout, const char* msg, uint8_t len);
#endif // WIN32

uint8_t announcePacket[ETH_HEADER_LEN + IP_HEADER_LEN + UDP_HEADER_LEN + 260];
void AdminExt_sendMsg(uint8_t fdout, char* msg, uint8_t len)
{
	if (len <=0)
		return;

#ifdef __CC_ARM
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	for (; msg && len>0; len--)
		fputc(*msg++, (FILE*)fdout);
	fputc('\r', (FILE*)fdout); fputc('\n', (FILE*)fdout);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#elif defined (_WIN32)
	WinSim_sendMsg(fdout, msg, len);
#endif // __CC_ARM

	if (fdadm == fdout)
	{
		fill_udp_data_p(announcePacket, 0,   (const uint8_t*) msg, len);
		fill_udp_data_p(announcePacket, len, (const uint8_t*) "\r\n", 3);
		mcast(&nic, announcePacket, GroupIP, GROUP_PORT_Adm, 2013, len+3);
	}
}

int GW_formatHtMessage(char* msg, uint8_t verb, const uint16_t nodeId, const char* objectUri, int cparams, const char* params[], const char* values[])
{
	char* p =msg;
	int i=0;
	if (NULL == p || NULL ==objectUri || verb >=VERB_MAX)
		return 0;

	p += sprintf(p, "%s %x/%s?", Verbs[verb], nodeId, objectUri);
	for (i=0; i < cparams && params[i]; i++)
		p += sprintf(p, "%s=%s&", params[i], values[i] ? values[i]:"");

	p += sprintf(p, "\r\n");
	return (p-msg);
}

bool HtCan_convertParamsToCanMsg(HTCanMessage* m, uint8_t verb, const uint16_t nodeId, const char* objectUri, int cparams, const char* params[], const char* values[])
{
	int i=0;
	uint8_t v;

	memset(m, 0, sizeof(HTCanMessage));

	if (0 == strcmp(objectUri, "can"))
	{
		// compact text message mode
		if (cparams<4)
			return false;

		for (cparams--; cparams >=0; cparams--)
		{
			if (0 == strcmp(params[cparams], "cobId"))
			{
				for (i=0; values[cparams][i]; i++)
				{
					v = hexchval(values[cparams][i]);
					if (v >0x0f) break;
					m->cob_id <<=4; m->cob_id += v;
				}
			}
			else if (0 == strcmp(params[cparams], "len"))
			{
				v = hexchval(values[cparams][0]);
				if (v <=8)
					m->len = v;
			}
			else if (0 == strcmp(params[cparams], "rtr"))
			{
				m->rtr = (0 == strcmp(values[cparams], "00"))? 0:1;
			}
			else if (0 == strcmp(params[cparams], "data"))
			{
				for (i=0; i<8; i++)
					m->data[i] = (hexchval(values[cparams][i*2]) <<4) | hexchval(values[cparams][i*2+1]);
			}
		}
	}
	else
	{
		// TODO rich text mode
	}

	return (m->len >0) && (m->len <=8);
}

bool isUriCh(const char c)
{
	return (c =='.') || (c >='/' && c <= '9') || (c>='a' && c<='z') || (c>='A' && c <='Z');
}

#define MAX_PARAMS (16)
#define FLG_DIR_FWD_ADM    (1<<0)
#define FLG_DIR_FWD_CAN    (1<<1)
#define FLG_DIR_FWD_EXT    (1<<2)
#define FLG_DIR_CAN_PROC   (1<<3)

typedef struct _todo_t
{
	uint8_t fdIn;
	uint8_t flags;
} todo_t;

static void GW_OnFillCanMsg(HTCanMessage* pMsg, void* pCtx)
{
//	char* pPacketData;
	todo_t* ptodo = ((todo_t*) pCtx);
	if (NULL == ptodo)
		return;

	if (ptodo->flags & FLG_DIR_CAN_PROC)
		HtCan_processReceived(ptodo->fdIn, pMsg);

	if (ptodo->flags & FLG_DIR_FWD_CAN)
		HtCan_send(ptodo->fdIn, pMsg);
}

const char* params[MAX_PARAMS], *values[MAX_PARAMS];
void GW_dispatchTextMessage0(uint8_t fdin, char* msg)
{
	char* p =msg, *varname=NULL;
	uint8_t verb=0, iparam=0;
	uint32_t v=0;
	char* objectUri =NULL;
	uint16_t nodeId =0;
	HTCanMessage m;
	todo_t todos;
	todos.fdIn = fdin;
	todos.flags =0;

	// determin the verb
	for (verb=0; Verbs[verb]; verb++)
	{
		if (0== memcmp(msg, Verbs[verb], sizeof(Verbs[verb])-1))
		{
			p += sizeof(Verbs[verb]);
			break;
		}
	}

	if (verb >=VERB_MAX)
		return;

	p =trimleft(p);
	nodeId=0;

	varname = strchar(p, ':');
	objectUri = strchar(p, '/');
	if (NULL != varname && objectUri >varname) // skip the proto://
	{
		p = varname+1; 
		while ('/' ==*p) p++;
	}

	if (VERB_POST == verb && NULL != varname)
	{
		// if this is the compact CAN text, forward to the CAN bus directly
		if (0 == strncmp(varname -3, "can", sizeof("can") -1) && HtCan_parseCompactText(p, &m) >0)
		{
			if (HtCan_thisNodeId != (m.cob_id & 0x7f))
			{
				// the message MAY be about the other nodes on the CAN bus, forward it
				HtCan_send((uint8_t) fdin, &m);
			}

			HtCan_processReceived((uint8_t) fdin, &m);
			return;
		}
	}

	// start dispatching Rich text URI here,
	// step 1.1 parse the node_id
	p = (char*) hex2int(p, &v);
	nodeId = v;

	// step 2. determining the necessary forwarding by method, fin and nodeId
	todos.flags |= ((fdin != fdadm) && (verb == VERB_POST))  ? FLG_DIR_FWD_ADM:0;
	todos.flags |= ((fdin != fdcan) && (verb != VERB_POST) && ((nodeId & 0x7f) != HtCan_thisNodeId)) ? FLG_DIR_FWD_CAN:0;

	if ((nodeId & 0x7f) == HtCan_thisNodeId)
		todos.flags |= FLG_DIR_CAN_PROC; // about the local node
	else if ((nodeId & SUBNET_SEGMENT_MASK) == (HtCan_thisNodeId & SUBNET_SEGMENT_MASK))
		todos.flags |= FLG_DIR_FWD_EXT; // the extension segment of this node
	else if (0 == nodeId & 0x7f)
		todos.flags |= (FLG_DIR_FWD_CAN|FLG_DIR_CAN_PROC); // a broadcast message on CAN bus

	// step 3. forward to the ADM interface
	if (todos.flags & FLG_DIR_FWD_ADM)
		AdminExt_sendMsg(fdadm, msg, strlen(msg));

	// step 4. forward to the ADM interface
	if (todos.flags & FLG_DIR_FWD_EXT)
		AdminExt_sendMsg(fdext, msg, strlen(msg));

	if (0 == (todos.flags & (FLG_DIR_FWD_CAN | FLG_DIR_CAN_PROC)))
		return; // no need to process further

	// step 5. process CAN message
	// step 5.1. parse for the objectUri
	while (p && *p && *p!='/') p++;

	// seek to the beginning of parametes
	for (objectUri=p+1; p && *p && isUriCh(*p); p++);
	if (p && *p)
		*p++ = '\0';

	// step 5.2. parse for var=val pairs
	for (iparam=0; p && *p && iparam <MAX_PARAMS; iparam++)
	{
		p = trimleft(p); params[iparam]=p; values[iparam]=NULL;
		if (p && NULL != (p = strchar(p, '&')))
			*p++='\0';

		values[iparam] = strchar(params[iparam], '=');

		if (NULL == values[iparam])
			values[iparam] ="";
		else
			*((char*)(values[iparam]++))='\0';
	}

	m.cob_id=m.len=0;

	// step 5.3. forward the CAN msg if it is
	if (!HtCan_parseRichURIEx(verb, nodeId, objectUri, iparam, params, values, GW_OnFillCanMsg, &todos))
		return;
}

void GW_dispatchTextMessage(uint8_t fdin, char* msg)
{
	char* p =msg, *varname=NULL;
	uint8_t verb=0, iparam=0;
	uint32_t v=0;
	char* objectUri =NULL;
	uint16_t nodeId =0;
	HTCanMessage m;
	todo_t todos;
	todos.fdIn = fdin;
	todos.flags =0;

	// determin the verb
	for (verb=0; Verbs[verb]; verb++)
	{
		if (0== memcmp(msg, Verbs[verb], sizeof(Verbs[verb])-1))
		{
			p += sizeof(Verbs[verb]);
			break;
		}
	}

	if (verb >=VERB_MAX)
		return;

	p=trimleft(p);
	nodeId=0;

	varname = strchar(p, ':');
	objectUri = strchar(p, '/');
	if (NULL != varname && objectUri >varname) // skip the proto://
	{
		p = varname+1; 
		while ('/' ==*p) p++;
	}

	if (VERB_POST == verb && NULL != varname)
	{
		v =0;
		if (0 == strncmp(varname -3, "can:", sizeof("can:") -1))
			v =1;
		else if (0 == strncmp(objectUri, "/can?", sizeof("/can?") -1))
		{
			v =1;
			p = objectUri + sizeof("/can?") -1;
		}

		// if this is the compact CAN text, forward to the CAN bus directly
		if (v && HtCan_parseCompactText(p, &m) >0)
		{
			if (HtCan_thisNodeId != (m.cob_id & 0x7f))
			{
				// the message MAY be about the other nodes on the CAN bus, forward it
				HtCan_send((uint8_t) fdin, &m);
				if (0 != (m.cob_id & 0x7f))
					return;
			}

			// local process those CAN message to node=this or node=0
			HtCan_processReceived((uint8_t) fdin, &m);
			return;
		}

		if (fdin != fdext && 0 == strncmp(objectUri, "/ext?", sizeof("/ext?") -1))
		{
			p = objectUri + sizeof("/ext?") -1;
			p = (char*) hex2int(p, &v);
			if (v>0 && 'V' == *p++)
			{
				// a binary message URI in the format of "/ext?<len>V<HexStr> 

				// convert to the binary message
				for (verb =0, iparam = v; *p && verb < iparam; verb++)
				{
					msg[0] = *p++; msg[1] = *p++; msg[2] = '\0';
					hex2int(msg, &v);
					objectUri[verb] = v & 0xff; 
				}

				// TODO send the binary message thru fdext
				return;
			}

			p = strstr(p, "tmsg=");
			if (p)
			{
				p += snprintf(p, 5, "POST"); *p= ' ';
				AdminExt_sendMsg(fdext, p, strlen(p));
			}

			return;
		}
	}

	// start dispatching Rich text URI here,
	// step 1.1 parse the node_id
	p = (char*) hex2int(p, &v);
	nodeId = v & 0x7f;

	// step 2. determining the necessary forwarding by method, fin and nodeId
	todos.flags |= ((fdin != fdadm) && (verb == VERB_POST)) ? FLG_DIR_FWD_ADM:0;
	todos.flags |= ((fdin != fdcan) && (verb != VERB_POST) && ((nodeId & 0xff) != HtCan_thisNodeId)) ? FLG_DIR_FWD_CAN:0;

	if (nodeId == HtCan_thisNodeId)
		todos.flags |= FLG_DIR_CAN_PROC; // about the local node
	else if ((nodeId & SUBNET_SEGMENT_MASK) == (HtCan_thisNodeId & SUBNET_SEGMENT_MASK))
		todos.flags |= FLG_DIR_FWD_EXT; // the extension segment of this node
	else if (0 == nodeId)
		todos.flags |= (FLG_DIR_FWD_CAN | FLG_DIR_CAN_PROC); // a broadcast message on CAN bus

	// step 3. forward to the ADM interface
	if (todos.flags & FLG_DIR_FWD_ADM)
		AdminExt_sendMsg(fdadm, msg, strlen(msg));

	// step 4. forward to the ADM interface
	if (todos.flags & FLG_DIR_FWD_EXT)
		AdminExt_sendMsg(fdext, msg, strlen(msg));

	if (0 == (todos.flags & (FLG_DIR_FWD_CAN | FLG_DIR_CAN_PROC)))
		return; // no need to process further

	// step 5. process CAN message
	// step 5.1. parse for the objectUri
	while (p && *p && *p!='/') p++;

	// seek to the beginning of parametes
	for (objectUri=p+1; p && *p && isUriCh(*p); p++);
	if (p && *p)
		*p++ = '\0';

	// step 5.2. parse for var=val pairs
	for (iparam=0; p && *p && iparam <MAX_PARAMS; iparam++)
	{
		p = trimleft(p); params[iparam]=p; values[iparam]=NULL;
		if (p && NULL != (p = strchar(p, '&')))
			*p++='\0';

		values[iparam] = strchar(params[iparam], '=');

		if (NULL == values[iparam])
			values[iparam] ="";
		else
			*((char*)(values[iparam]++))='\0';
	}

	m.cob_id=m.len=0;

	// step 5.3. forward the CAN msg if it is
	if (!HtCan_parseRichURIEx(verb, nodeId, objectUri, iparam, params, values, GW_OnFillCanMsg, &todos))
		return;
}

#define TCP_DATA_BODY_START (150)
static uint16_t packetDataPos = TCP_DATA_BODY_START;

char txtMsg[100];
void OnPostCanMessage(uint8_t fdin, const HTCanMessage* m, bool localSrc)
{
	//	char txtMsg[100];
	char *p = txtMsg;
	bool bCompactTxtNeeded =false, bRichTxtNeeded =false;

	if (NULL == m)
		return;

	if (((uint8_t)fdeth) == fdin)
	{
		if (HtCan_toJsonVars(txtMsg, sizeof(txtMsg)-3, m))
		{
			p = txtMsg + strlen(txtMsg);
			packetDataPos = fill_tcp_data_str(packetEth, packetDataPos, txtMsg);
		}

		return;
	}

	if (((EcuConf_fwdAdm & FWD_FROM_CAN) == fwdt_CompactText) || ((EcuConf_fwdExt & FWD_FROM_CAN) == fwdt_CompactText))
		bCompactTxtNeeded = true;

	if (((EcuConf_fwdAdm & FWD_FROM_CAN) == fwdt_RichText) || ((EcuConf_fwdExt & FWD_FROM_CAN) == fwdt_RichText))
		bRichTxtNeeded = true;

	if (bCompactTxtNeeded)
	{
		p += snprintf(p, txtMsg+sizeof(txtMsg)-p-3, "POST can:");
		if (HtCan_toCompactText(p, txtMsg+sizeof(txtMsg)-p-3, m))
		{
			// p+=strlen(p); *p++ = '\r'; *p++ = '\n'; *p++ = '\0'; 

			if ((EcuConf_fwdAdm & FWD_FROM_CAN) == fwdt_CompactText)
				AdminExt_sendMsg(fdadm, txtMsg, strlen(txtMsg));

			if ((EcuConf_fwdExt & FWD_FROM_CAN) == fwdt_CompactText)
				AdminExt_sendMsg(fdext, txtMsg, strlen(txtMsg));
		}
	}

	if (bRichTxtNeeded)
	{
		if (NULL != HtCan_toRichURI(txtMsg, sizeof(txtMsg)-3, m))
		{
			// converted to a rich URI if reaches here
			// p = txtMsg + strlen(txtMsg); *p++ ='\r'; *p++ ='\n'; *p++ ='\0';

			if ((EcuConf_fwdAdm & FWD_FROM_CAN) == fwdt_RichText)
				AdminExt_sendMsg(fdadm, txtMsg, strlen(txtMsg));

			if ((EcuConf_fwdExt & FWD_FROM_CAN) == fwdt_RichText)
				AdminExt_sendMsg(fdext, txtMsg, strlen(txtMsg));
		}
	}
}

void OnPdoEvent(uint8_t fdin, const HTCanMessage* m, bool local)
{
	uint8_t eventId = (m->cob_id >>7);
	switch(eventId)
	{
		// TODO: process the event accordingly
	case HTPDO_GlobalStateChanged:
	case HTPDO_DeviceStateChanged:

	default: break;
	}

	OnPostCanMessage(fdin, m, local);
}

void OnSdoData(uint8_t fdin, const HTCanMessage* m, bool localData)
{
	switch(m->data[0])
	{
		// TODO: process the event accordingly
	case SDO_RESP_ERR:
	case SDO_RESP_GET:
	case SDO_RESP_SET:
	default: break;
	}

	OnPostCanMessage(fdin, m, localData);
}

void GW_dispatchCANbyFd(uint8_t fdCAN, const HTCanMessage* m)
{
	if (GROUP_PORT_CanOverIP<=0)
		return;

	fill_udp_data_p(announcePacket, 0, (const uint8_t*)m, sizeof(HTCanMessage));
	mcast(&nic, announcePacket, GroupIP, GROUP_PORT_CanOverIP, 2013, sizeof(HTCanMessage));
}

// =========================================================================
// About NIC interface
// =========================================================================
// ---------------------------------------------------------------------------
//  sample RX handlers
// ---------------------------------------------------------------------------
// ignore PKTIF because is unreliable! (look at the errata datasheet)
// check EPKTCNT is the suggested workaround.
// We don't need to clear interrupt flag, automatically done when
// enc28j60_hw_rx() decrements the packet counter.
uint8_t packetEth[ENC28J60_MTU_SIZE+1];
char  wkurl[100];

void GW_nicReceiveLoop(ENC28J60* nic)
{
	uint16_t plen=0;
	uint16_t tmp;
	char* p=NULL, *q;
	uint32_t nodeId=0;

	do {
		// step 1, get the next new packet:
		plen = ENC28J60_recvPacket(nic, sizeof(packetEth)-1, packetEth);

		// plen will ne unequal to zero if there is a valid packet (without crc error)
		if (plen <=0)
			break;

		// step 2. process the arp packet directly
		//   arp is broadcast if unknown but a host may also verify the mac address by sending it to 
		//   a unicast address.
		if (eth_type_is_arp_and_my_ip(packetEth, plen))
		{
			arp_answer_from_request(nic, packetEth);
			continue;
		}

		if (!eth_type_is_ip(packetEth, plen))
			continue;

		// start processing IP packets here

		// step 3. check if ip packets are for us:
		if (!is_to_myip(packetEth, plen) && !match_dest_ip(GroupIP, packetEth, plen))
			continue;

		if (match_src_ip(MyIP, packetEth, plen)) // ignore those from self
			continue;

		// step 4. respond ICMP directly
		if (packetEth[IP_PROTO_P]==IP_PROTO_ICMP_V && packetEth[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V)
		{
			// a ping packet, let's send pong
			echo_reply_from_request(nic, packetEth,plen);
			continue;
		}

		// step 5. process TCP here
		if (packetEth[IP_PROTO_P]==IP_PROTO_TCP_V)
		{
			if (MyServerPort != BYTES2WORD(packetEth[TCP_DST_PORT_H_P], packetEth[TCP_DST_PORT_L_P]))
				continue;  // not our listening port

			if (packetEth[TCP_FLAGS_P] & TCP_FLAGS_SYN_V)
			{ 
				// simply ack the SYN
				tcp_synack_from_syn(nic, packetEth); // make_tcp_synack_from_syn does already send the syn,ack
				continue;
			}

			if (packetEth[TCP_FLAGS_P] & TCP_FLAGS_ACK_V)
			{
				init_len_info(packetEth); // init some data structures

				// we can possibly have no data, just ack:
				if (0 == get_tcp_data_pointer())
				{
					if (packetEth[TCP_FLAGS_P] & TCP_FLAGS_FIN_V)
					{
						// finack, answer with ack
						tcp_ack_from_any(nic, packetEth);
					}
					// just an ack with no data, wait for next packet
					continue;
				}

				do { // real TCP data packet

					// terminate the URI string
					p = strstr((char *)&(packetEth[TCP_PAYLOAD_P]), " HTTP/1.");
					if (NULL !=p)
						*p= '\0';

					// fixup the URI
					p = strchr((char *)&(packetEth[TCP_PAYLOAD_P]), ' '); // the space after HTTP verb
					if (NULL ==p)
					{
						plen=fill_tcp_data_str(packetEth, 0, "HTTP/1.0 300 Bad Request\r\nContent-Type: text/html\r\n\r\n<h1>300 Bad Request</h1>");
						break;
					}

					if ('/' != *(++p))
					{
						// the uri is a relative URI, complete it with prefix ht:<thisNodeId>/
						for (q= p +strlen(p) +1; q >= p; q--)
							*(q+6) = *q;

						q++;
						*q++='h'; *q++='t'; *q++=':';
						*q++ = hexchar(HtCan_thisNodeId>>4);  *q++ = hexchar(HtCan_thisNodeId&0xf);
						*q++='/';
					}
					else
					{
						// absolute URI
						q = p;
						q = (char*)hex2int(++q, &nodeId); nodeId &=0x7f;
						if (nodeId == 0)
						{
							plen=fill_tcp_data_str(packetEth,0, "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Node Not Found</h1>");
							break;
						}

						if ((SUBNET_SEGMENT_MASK & HtCan_thisNodeId) != (SUBNET_SEGMENT_MASK & nodeId))
						{
							// this should redirect to another ECU, send the response "HTTP 1.0 302 Object Moved"
							p = (char *)&(packetEth[TCP_PAYLOAD_P + TCP_DATA_BODY_START]);
							sprintf(p, "http://%d.%d.%d.%d:%d/%02x%s", MyIP[0], MyIP[1], MyIP[2], NODEID_TO_IP_B4(nodeId), MyServerPort, nodeId, q);  
							plen=fill_tcp_data_str(packetEth, 0, "HTTP/1.0 302 Object Moved\r\nLocation: ");
							plen=fill_tcp_data_str(packetEth,plen, p);
							plen=fill_tcp_data_str(packetEth,plen, "\r\n\r\n");
							break;
						}

						// fixup the URI with nodeId already presented
						for (q= p +strlen(p) +1; q > p; q--)
							*(q+2) = *q;

						*q++='h'; *q++='t'; *q++=':';
					}

					// this is the right http server, start processing the request

					// reset the packetDataPos
					packetDataPos = TCP_DATA_BODY_START;

					// call GW_dispatchTextMessage(), the relative response data should be filled by OnPostCanMessage()
					GW_dispatchTextMessage(fdeth, (char *)&(packetEth[TCP_PAYLOAD_P]));

					plen=fill_tcp_data_str(packetEth, 0, "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n{ ");
					p = (char *)&(packetEth[TCP_PAYLOAD_P + TCP_DATA_BODY_START]);
					while (' '==*p || '\t'==*p || ','==*p) p++;
					plen=fill_tcp_data_p(packetEth, plen, (uint8_t*)p, (char *)&(packetEth[TCP_PAYLOAD_P + packetDataPos]) - p);
					plen=fill_tcp_data_str(packetEth, plen, " }");

				} while (0);

				// send the TCP response
				tcp_ack_from_any(nic, packetEth); // an ack first
				tcp_ack_with_data(nic, packetEth, plen); // response with data

				continue;
			}  // end of TCP_FLAGS_ACK

			continue;
		}  // end of TCP

		if (packetEth[IP_PROTO_P] == IP_PROTO_UDP_V)
		{
			// process UDP packets now

			if (!match_dest_ip(GroupIP, packetEth, plen))
				continue;

			switch(BYTES2WORD(packetEth[UDP_DST_PORT_H_P], packetEth[UDP_DST_PORT_L_P]))
			{
			case GROUP_PORT_Adm:
				GW_dispatchTextMessage(fdadm, (char *)&(packetEth[UDP_DATA_P]));
				break;

			case GROUP_PORT_CanOverIP:
				{
					HTCanMessage m;
					memcpy(&m, &(packetEth[UDP_DATA_P]), sizeof(m));
					// check if the nodeId belongs to this ECU
					tmp = m.cob_id & 0x7f;

					if (HtCan_thisNodeId != tmp)
					{
						// the message MAY be about the other nodes on the CAN bus, forward it
						HtCan_send((uint8_t) fdcan, &m);
						if (0 != (m.cob_id & 0x7f))
							break;
					}

					// local process those CAN message to node=this or node=0
					HtCan_processReceived((uint8_t) fdcan, &m);
				}

				break;
			}

			continue;
		} // end of UDP

	} while (1);

}

