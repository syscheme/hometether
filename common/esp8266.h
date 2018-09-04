#ifndef __ESP8266_H__
#define __ESP8266_H__

#include "htcomm.h"
#include "pbuf.h"
#include "usart.h"
#include "textmsg.h"

#define ESP8266_TRANSMIT_MASK   (0x1f) // lowest 5bits
#define ESP8266_RECEIVE_BUSY    FLAG(5)
#define ESP8266_CONNECT_BUSY    FLAG(6)
#define ESP8266_IO_BUSY         (ESP8266_TRANSMIT_MASK | ESP8266_RECEIVE_BUSY |ESP8266_CONNECT_BUSY)
#define ESP8266_CONN_DIRTY      FLAG(7)

#define ESP8266_MAX_CONNS       (5)
#define ESP8266_AT_MSG_MAXLEN   (1024) // (PBUF_MEMPOOL2_BSIZE -10) // must be able to hold a complete AT line

typedef struct _ESP8266
{
	TCPPeer  bind;
	TCPPeer  conns[ESP8266_MAX_CONNS];
	char     atecho[ESP8266_AT_MSG_MAXLEN];

	uint16_t __IO__ head, tail, nIncommingLeft;
	uint16_t __IO__ flags;      // the lowest 4 bit means connection X outgoing busy
	uint8_t  __IO__ activeConn; // 0x0F incoming

	USART*  usart;
} ESP8266;

void ESP8266_init(ESP8266* chip, USART* usart);
uint8_t ESP8266_loopStep(ESP8266* chip);
void ESP8266_check(ESP8266* chip);

hterr ESP8266_listen(ESP8266* chip, uint16_t port);
hterr ESP8266_connect(ESP8266* chip, uint8_t connId, const char* peerName, uint16_t peerPort, uint8_t udp);
hterr ESP8266_connectPeer(ESP8266* chip, uint8_t connId, TCPPeer* peer);
hterr ESP8266_close(ESP8266* chip, uint8_t connId);

hterr ESP8266_transmit(ESP8266* chip, uint8_t connId, uint8_t* msg, uint16_t len);

// callbacks
void ESP8266_OnReceived(ESP8266* chip, char* msg, int len, bool bEnd);

// other extensions
void ESP8266_jsonPost2Coap(ESP8266* chip, const char* rdAddr, uint16_t rdPort, uint32_t nodeId, uint8_t klvc, const KLV eklvs[]);
#ifdef _WIN32

#endif // _WIN32

#endif // __ESP8266_H__
