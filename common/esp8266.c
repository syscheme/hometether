#include "esp8266.h"
#define NEWLINE  "\r\n"

// #define AT_TEST_READ  readAT

#define WAIT(_MSEC) sleep(_MSEC)
#define NODE_OF_SUBNET (50)

void ESP8266_init(ESP8266* chip, USART* usart)
{
	memset(chip, 0x00, sizeof(ESP8266));
	chip->flags = 0xffff;
	chip->usart = usart;
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CWMODE=3" NEWLINE));        WAIT(100);
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+RST" NEWLINE));             WAIT(3000);
	// USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CWJAP=\"ashao@ygmd\",\"???????\"" NEWLINE));             WAIT(3000);
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIPMUX=1" NEWLINE));        WAIT(100);
	chip->flags = 0;
}

hterr ESP8266_listenEx(ESP8266* chip, uint16_t port, uint8_t udp)
{
	char portnum[8], i;

	for (i =100; i>0 && (ESP8266_IO_BUSY & chip->flags); i--)
		WAIT(20);

	if (ESP8266_IO_BUSY & chip->flags)
		return ERR_OPERATION_ABORTED;

	snprintf(portnum, sizeof(portnum)-1, "%d", port);
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIPSERVER=1,")); 
	USART_transmit(chip->usart, (uint8_t*) portnum, strlen(portnum)); 
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE NEWLINE)); 
	WAIT(100);
	return ERR_SUCCESS;
}

hterr ESP8266_listen(ESP8266* chip, uint16_t port)
{
	return ESP8266_listenEx(chip, port, 0);
}

void ESP8266_check(ESP8266* chip)
{
	if (ESP8266_IO_BUSY & chip->flags)
		return;

	if (0 == chip->bind.ip)
		USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIFSR" NEWLINE));        WAIT(100);
	// USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIPSERVER=1,8888" NEWLINE)); WAIT(100);
	if (chip->flags & ESP8266_CONN_DIRTY)
	{
		chip->flags &= ~ESP8266_CONN_DIRTY;
		USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIPSTATUS" NEWLINE));    WAIT(200);
	}

#ifdef NODE_OF_SUBNET
	// make sure the local IP is xxx.xxx.xxx.<NODE_OF_SUBNET>
	if (chip->bind.ip && (chip->bind.ip & 0xff) != NODE_OF_SUBNET)
	{
		USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIPSTATUS" NEWLINE));
		WAIT(200);
	}
#endif // NODE_OF_SUBNET
}

hterr ESP8266_connect(ESP8266* chip, uint8_t connId, const char* peerName, uint16_t peerPort, uint8_t udp)
{
	// pbuf* buf = pbuf_malloc(PBUF_MEMPOOL2_BSIZE-10);
	// if (NULL == buf || NULL == buf->payload)
	// 	return ERR_NOT_ENOUGH_MEMORY;
	char buf[100], i;

	for (i =100; i>0 && (ESP8266_IO_BUSY & chip->flags); i--)
		WAIT(20);

	if (ESP8266_IO_BUSY & chip->flags)
		return ERR_OPERATION_ABORTED;
	chip->flags |= ESP8266_CONNECT_BUSY;

	if (connId >= ESP8266_MAX_CONNS)
		connId =0;


	// snprintf((char*)buf->payload, buf->len, "%d,\"%s\",\"%s\",%d", connId, (udp)?"UDP":"TCP", peerName, peerPort);
	snprintf(buf, sizeof(buf) -2, "%d,\"%s\",\"%s\",%d", connId, (udp)?"UDP":"TCP", peerName, peerPort);
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIPSTART="));
	// USART_transmit(chip->usart, buf->payload, strlen((const char*) buf->payload));
	USART_transmit(chip->usart, (uint8_t*)buf, strlen(buf));
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE));
	// pbuf_free(buf);
	chip->flags &= ~ESP8266_CONNECT_BUSY;
	chip->flags |= ESP8266_CONN_DIRTY;
	WAIT(1000);
	return ERR_SUCCESS;
}

hterr ESP8266_connectPeer(ESP8266* chip, uint8_t connId, TCPPeer* peer)
{
	char peerAddr[32];
	snprintf(peerAddr, sizeof(peerAddr)-1, "%d.%d.%d.%d",
	    U32_IP_Bx(peer->ip, 0), U32_IP_Bx(peer->ip, 1), U32_IP_Bx(peer->ip, 2), U32_IP_Bx(peer->ip, 3));

	return ESP8266_connect(chip, connId, peerAddr, peer->port, PEER_FLG_TYPE_UDP & peer->flags);
}

hterr ESP8266_close(ESP8266* chip, uint8_t connId)
{
	int i;
	for (i =100; i>0 && (ESP8266_IO_BUSY & chip->flags); i--)
		WAIT(20);

	if (ESP8266_IO_BUSY & chip->flags)
		return ERR_OPERATION_ABORTED;
	chip->flags |= ESP8266_CONNECT_BUSY;

	if (connId >= ESP8266_MAX_CONNS)
		connId =0;
	connId += '0';
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE "AT+CIPCLOSE="));
	USART_transmit(chip->usart, &connId, 1);
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE NEWLINE NEWLINE));
	chip->conns[connId].ip =0;
	chip->flags &= ~ESP8266_CONNECT_BUSY;
	chip->flags |= ESP8266_CONN_DIRTY;
	WAIT(500);
	return ERR_SUCCESS;
}

hterr ESP8266_transmit(ESP8266* chip, uint8_t connId, uint8_t* msg, uint16_t len)
{
	char buf[10], i;
	if (connId >= ESP8266_MAX_CONNS)
		connId =0;

	for (i =100; i>0 && (ESP8266_IO_BUSY & chip->flags); i--)
		WAIT(20);

	if (ESP8266_IO_BUSY & chip->flags)
		return ERR_OPERATION_ABORTED;

	

	snprintf(buf, sizeof(buf)-1, "%d,%d", connId, len);
	chip->flags |= (1<<connId);
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE NEWLINE "AT+CIPSEND=")); 
	USART_transmit(chip->usart, (uint8_t*)buf, strlen(buf));
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE));
	WAIT(300);
	USART_transmit(chip->usart, msg, len);
	USART_transmit(chip->usart, CONST_STR_AND_LEN2(NEWLINE NEWLINE NEWLINE));

	for (i=100; i>0; i--)
	{
		if (0 == chip->flags & (1<<connId))
			return ERR_SUCCESS;
	}

	chip->flags &= ~(1 <<connId);
	return ERR_IN_PROGRESS;
}

//@return the chars processed, 0 means not recoganized
static uint16_t ESP8266_doATCmd(ESP8266* chip, char* msg, uint16_t len)
{
	uint32_t i; 
	uint16_t j;
	char* p =msg+3, *q, *t;

	if (0 == strncmp(p, CONST_STR_AND_LEN2("+CWLAP")))
	{
		p += sizeof("+CWLAP");
		p = strstr(p, "\nOK");

		if (NULL ==p) // not yet completed
			return 0;
	}

	// AT+CIFSR\n\n192.168.199.205\n
	if (0 == strncmp(p, CONST_STR_AND_LEN2("+CIFSR")))
	{
		p += sizeof("+CIFSR");
		q = strchr(p, '1'); // bind address always start with '1': 192.xxx, 10.xxx, 172.xxx
		if (NULL == q)
			return (uint16_t)(p -msg);

		i = chip->bind.ip; // backup

		t = strchr(q, '\n');
		// ip address
		i = atoi(q); // ipB0
		if (NULL == (q = strchr(++q, '.')) || q > t)
			return 0;
		i = (i<<8) | atoi(++q) &0xff; // ipB1
		if (NULL == (q = strchr(++q, '.')) || q > t)
			return 0;
		i = (i<<8) | atoi(++q) &0xff; // ipB2
		if (NULL == (q = strchr(++q, '.')) || q > t)
			return 0;
		i = (i<<8) | atoi(++q) &0xff; // ipB3
		chip->bind.ip = i;

		return (uint16_t) (q+2 -msg);
	}

	// AT+CIPSEND=1,20sdfasfas\n\n> AT+CIPSEND=1,20sdfasfas\nbusy\n\n\nSEND OK\n"
	if (0 == strncmp(p, CONST_STR_AND_LEN2("+CIPSEND=")))
	{
		p += sizeof("+CIPSEND=")-1;
		q = strchr(p, ','); 
		if (NULL != q)
		{
			j = atoi(p);
			i = atoi(++q);
		}
		else
		{
			j =0;
			i = atoi(p);
		}

		chip->activeConn = (chip->activeConn & 0x0f) | (j<<4);

		p += i;
		return (uint16_t) min(len, p-msg);
	}

	return 0;
}

uint8_t ESP8266_loopStep(ESP8266* chip)
{
	char* p, *q, *t;
	uint16_t len, otail = chip->tail;
	uint32_t i;
	uint8_t c =0, j;

	if (chip->head >= sizeof(chip->atecho)-1)
	{
		// when chip->atecho is almost full, no reason if no message has been processed in the previous round
		chip->head = chip->tail =0;
	}

#ifdef AT_TEST_READ
	i = AT_TEST_READ(chip->atecho +chip->head, sizeof(chip->atecho) - chip->head);
#else
	i = USART_receive(chip->usart, (uint8_t*) chip->atecho +chip->head, sizeof(chip->atecho) - chip->head);
#endif // AT_TEST_READ
	if (i<=0)
		return c;
	chip->head += i;
	chip->atecho[chip->head] ='\0';
	
	do {
		otail = chip->tail;
		p = chip->atecho + chip->tail;

		// case 0. partial payload of incoming message
		if (chip->nIncommingLeft >0)
		{
			c++;
			len = (uint16_t) min(chip->nIncommingLeft, chip->atecho + chip->head -p);
			chip->nIncommingLeft -= len;

			if (chip->nIncommingLeft >0)
			{
				// payload incomplete in this round
				ESP8266_OnReceived(chip, p, len, 0);
				chip->tail = chip->head =0;
				return c;
			}

			// payload completed in this round
			ESP8266_OnReceived(chip, p, len, 1);
			chip->tail += len;
			p = chip->atecho + chip->tail;
			chip->flags &= ~ESP8266_RECEIVE_BUSY;
		}

		// non-payload always starts with a new line, no shorter than 3 chars
		if (chip->atecho + chip->head -p <3 || NULL == (p = strchr(p, '\n')))
		 	break;

		chip->tail = (uint16_t)(p -chip->atecho);
		c++;

		// case 1. received an incoming message
		if (0 == strncmp(p+1, CONST_STR_AND_LEN2("SEND OK")))
		{
			chip->tail = (uint16_t) (p+ CONST_STR_LEN2("SEND OK") -chip->atecho);
			chip->flags &= ~ESP8266_TRANSMIT_MASK;
			chip->activeConn &= 0x0f;
			continue;
		}

		// case 2. received an incoming message
		if (0 == strncmp(p+1, CONST_STR_AND_LEN2("+IPD,")))
		{	
			q =p +1 + CONST_STR_LEN2("+IPD,");
			j = atoi(++q);

			if (NULL == (q = strchr(q, ',')))
				break;
			i = atoi(++q);

			if (NULL == (q = strchr(q, ':')))
				break;

			if (j >= ESP8266_MAX_CONNS)
				j =0;
			chip->activeConn = chip->activeConn & 0xF0 | j;
			chip->nIncommingLeft = i;

			chip->tail = (uint16_t)(++q - chip->atecho);
			chip->flags |= ESP8266_RECEIVE_BUSY;
			continue;
		} 

		// case 2. a line of +CIPSTATUS:<id>,<type>,<addr>,<port>,<tetype>\n -- the response of AT+CIPSTATUS, but too long
		// ie, +CIPSTATUS:1,udp,192.168.100.134,12345,20\n -- the response of AT+CIPSTATUS, but too long
		if (0 == strncmp(p+1, CONST_STR_AND_LEN2("+CIPSTATUS:")))
		{
			q =p +1 + CONST_STR_LEN2("+CIPSTATUS:");
			if (NULL == (t = strchr(q, '\n')))
				break; // not yet completed

			chip->tail = (uint16_t) (t -chip->atecho);

			j = atoi(q); // connId
			if (j >= ESP8266_MAX_CONNS)
				j =0;

			if (NULL == (q = strchr(q, ',')))
				break;

			chip->conns[j].flags =0; // reset flags
			
			if (0 == strncmp(++q, "udp", 3))
				chip->conns[j].flags |= PEER_FLG_TYPE_UDP;

			if (NULL == (q = strchr(++q, ',')))
				break;

			// ip address
			i = atoi(++q); // ipB0
			if (NULL == (q = strchr(++q, '.')))
				break;
			i = (i<<8) | atoi(++q) &0xff; // ipB1
			if (NULL == (q = strchr(++q, '.')))
				break;
			i = (i<<8) | atoi(++q) &0xff; // ipB2
			if (NULL == (q = strchr(++q, '.')))
				break;
			i = (i<<8) | atoi(++q) &0xff; // ipB3
			
			chip->conns[j].ip = i;

			// ip port
			if (NULL == (q = strchr(++q, ',')))
				break;
			chip->conns[j].port =atoi(++q);

			continue;
		}

		// case 3. echo/response of AT command
		if (0 == strncmp(p+1, CONST_STR_AND_LEN2("AT"))) // && NULL != strchr(p+2,'\n')) // received echo of command and its response
		{
			i = ESP8266_doATCmd(chip, p, (uint16_t)(chip->atecho + chip->head -p));
			if (i > 0)
				chip->tail +=i;
			else if (chip->atecho + chip->head -p >(ESP8266_AT_MSG_MAXLEN -10))
				chip->tail += 4;

			continue;
		}

		// case 4. unrecoganized, step a character
		if (chip->tail +4 >chip->head)
			break;
		chip->tail++;

	} while (otail < chip->tail);

	// shift the prcessed string
	chip->head -= chip->tail;
	if (chip->tail >0 && chip->head >0)
		memcpy(chip->atecho, chip->atecho +chip->tail, chip->head);
	chip->atecho[chip->head] ='\0';
	chip->tail =0;

	return c;
}

#define FIXED_HTTP_HEADS  "Content-Type: application/json\r\n"

#define JSON_MSG_MAXLEN  (1024)

#ifndef	JSON_MSG_MAXLEN
#  define JSON_MSG_MAXLEN          PBUF_MEMPOOL2_BSIZE
#endif // JSON_MSG_MAXLEN

char jsonMsg[JSON_MSG_MAXLEN];
extern uint32_t BSP_nodeId;
#if 0
void ESP8266_jsonPost2Coap0(ESP8266* chip, const char* rdAddr, uint16_t rdPort, uint32_t nodeId, uint8_t klvc, const KLV eklvs[])
{
	char *p = jsonMsg, *q;
	static uint8_t cseq =0;

	ESP8266_close(chip, 1);

	// content body
	*p++ = '{';
	p += TextMsg_KLVs2Json(p, (uint16_t)(jsonMsg + sizeof(jsonMsg) -p -2), klvc, eklvs);
	*p++ = '}',  *p++ = '\r',  *p++ = '\n';

	// headers
	q =p;
	
	// startline
	// q += snprintf(q, jsonMsg + sizeof(jsonMsg) -q -2, "POST coap://%s:%d/rd?n=%08X HTTP/1.1\r\n" FIX_HTTP_HEADS, rdAddr, rdPort, nodeId);
	q += snprintf(q, jsonMsg + sizeof(jsonMsg) -q -2, "POST /rd?n=%08X HTTP/1.1\r\n", nodeId);

	// stamp headers
	q += snprintf(q, jsonMsg + sizeof(jsonMsg) -q -2, FIXED_HTTP_HEADS "CSeq: %d\r\nx-Stamp=%lu\r\n", ++cseq, gClock_msec);
	
	// header From
	if (chip->bind.ip)
	{
		q += snprintf(q, jsonMsg + sizeof(jsonMsg) -q -2, "From: %08X@%d.%d.%d.%d\r\n", BSP_nodeId,
	    	U32_IP_Bx(chip->bind.ip, 0), U32_IP_Bx(chip->bind.ip, 1), U32_IP_Bx(chip->bind.ip, 2), U32_IP_Bx(chip->bind.ip, 3));
	}

	// header Content-Length and EOH
	q += snprintf(q, jsonMsg + sizeof(jsonMsg) -q -2, "Content-Length: %d\r\n\r\n", (uint16_t)(p-jsonMsg));

	if (ERR_SUCCESS != ESP8266_connect(chip, 1, rdAddr, rdPort, 0))
		return;

	ESP8266_transmit(chip, 1, (uint8_t*)p, (uint16_t)(q -p));
	ESP8266_transmit(chip, 1, (uint8_t*)jsonMsg, (uint16_t)(p -jsonMsg));

//	sleep(500);
}
#endif // 0 

#define BODY_OFFSET (220)
#define POST_UDP
#ifdef POST_UDP
#  define POST_CONNID (0)
#  define POST_PROTO  (1)
#else
#  define POST_CONNID (1)
#  define POST_PROTO  (0)
#endif // POST_UDP

void ESP8266_jsonPost2Coap(ESP8266* chip, const char* rdAddr, uint16_t rdPort, uint32_t nodeId, uint8_t klvc, const KLV eklvs[])
{
	char *p = jsonMsg +BODY_OFFSET, *q;
	static uint8_t cseq =0;
	int16_t clen =0;

#ifndef POST_UDP
	ESP8266_close(chip, 1);
#else
//	ESP8266_listen(chip, 1);
#endif // POST_UDP
	// content body
	*p++ = '{';
	p += TextMsg_KLVs2Json(p, (uint16_t)(jsonMsg + sizeof(jsonMsg) -p -2), klvc, eklvs);
	*p++ = '}',  *p++ = '\r',  *p++ = '\n';
	clen = (int16_t) (p - (jsonMsg+BODY_OFFSET));

	// headers
	q = jsonMsg;
	
	// startline
	// q += snprintf(q, jsonMsg + sizeof(jsonMsg) -q -2, "POST coap://%s:%d/rd?n=%08X HTTP/1.1\r\n" FIX_HTTP_HEADS, rdAddr, rdPort, nodeId);
	q += snprintf(q, jsonMsg + BODY_OFFSET -q -2, "POST /rd?n=%08X HTTP/1.1\r\n", nodeId);

	// stamp headers
	while (0 == ++cseq);
	q += snprintf(q, jsonMsg + BODY_OFFSET -q -2, FIXED_HTTP_HEADS "CSeq: %d\r\nx-Stamp: %lu\r\n", cseq, gClock_msec);
	
	// header From
	if (chip->bind.ip)
	{
		q += snprintf(q, jsonMsg + BODY_OFFSET -q -2, "From: %08X@%d.%d.%d.%d\r\n", BSP_nodeId,
	    	U32_IP_Bx(chip->bind.ip, 0), U32_IP_Bx(chip->bind.ip, 1), U32_IP_Bx(chip->bind.ip, 2), U32_IP_Bx(chip->bind.ip, 3));
	}

	// header Content-Length and EOH
	q += snprintf(q, jsonMsg + BODY_OFFSET -q -2, "Content-Length: %d\r\n\r\n", clen);
	memcpy(q, jsonMsg +BODY_OFFSET, clen); q+= clen; // shift the body

	if (ERR_SUCCESS != ESP8266_connect(chip, POST_CONNID, rdAddr, rdPort, POST_PROTO))
		return;

	ESP8266_transmit(chip, POST_CONNID, (uint8_t*)jsonMsg, (uint16_t)(q -jsonMsg));

//	sleep(500);
}
