#ifndef __HLK_RM04_H__
#define __HLK_RM04_H__

#include "htddl.h"
#include "pbuf.h"
#include "usart.h"

// -----------------------------
// HLKRM04
// -----------------------------
typedef struct _HLKRM04
{
	uint32_t		  bindIp;
	uint16_t          bindPort;

	USART*            usart;
	struct _HtNetIf*  netif;
	uint16_t          flags;
} HLKRM04;

#define FLG_MODE_AT   FLAG(0)

hterr HLKRM04_init(HLKRM04* chip, USART* usart, uint32_t bindIp, uint16_t bindPort);
void  HLKRM04_udpConnect(HLKRM04* chip, uint32_t destNodeId, uint16_t destPortNum);
void  HLKRM04_tcpConnect(HLKRM04* chip, uint32_t destNodeId, uint16_t destPortNum);
void  HLKRM04_tcpListen(HLKRM04* chip, uint16_t servePortNum);
void  HLKRM04_status(HLKRM04* chip);

// ---------------------------------------------------------------------------
// async methods based on ISR
// ---------------------------------------------------------------------------
// async transmit
hterr HLKRM04_transmit(HLKRM04* chip, uint32_t destNodeId, uint8_t destPortNum, uint8_t* payload);

// portal callbacks
void HLKRM04_OnReceived(HLKRM04* chip, uint8_t pipeId, uint8_t* payload, uint8_t payloadLen);
void HLKRM04_OnSent(HLKRM04* chip, uint32_t destNodeId, uint8_t destPortNun, uint8_t* payload);

#endif // __HLK_RM04_H__

