#ifndef __nRF24L01_H__
#define __nRF24L01_H__

#include "htcomm.h"
#include "pbuf.h"

// ---------------------------------------------------------------------------
// definition of nRF24L01 chip
// ---------------------------------------------------------------------------
#define nRF24L01_ADDR_LEN         (5)
// #define nRF24L01_MAX_PLOAD  (32 -4 -nRF24L01_ADDR_LEN) // (32 -2 -nRF24L01_ADDR_LEN) max payload= 32byte - addrlen - 16bit crc
#define nRF24L01_MAX_PLOAD        (16)
#ifndef nRF24L01_PENDING_TX_MAX
#  define nRF24L01_PENDING_TX_MAX (4)
#endif // nRF24L01_PENDING_TX_MAX

#ifndef nRF_RX_CH
#  define nRF_RX_CH 5
#endif // nRF_RX_CH

#define nRF24L01_FLAG_TX_BUSY    (1<<0)

typedef struct _nRF24L01
{
	uint32_t       nodeId;
	uint8_t   __IO__  flags;	  // set of nRF24L01_FLAG_xxxx

	// about IO pins
	IO_SPI        spi;
	IO_PIN        pinCSN; // the pin CS
	IO_PIN        pinCE; // the pin CE

	uint32_t  __IO__ peerNodeId;
	uint8_t   __IO__ peerPortNum;

	FIFO          txFIFO;	// an iteration in txFIFO is pointer to (destAddr, payload)

#ifdef HT_DDL
	struct _HtNetIf* netif;
#endif // HT_DDL
} nRF24L01;

// ---------------------------------------------------------------------------
// native methods
// ---------------------------------------------------------------------------
hterr nRF24L01_init(nRF24L01* chip, uint32_t nodeId, uint8_t rxmode);
void nRF24L01_setModeRX(nRF24L01* chip);
void nRF24L01_setModeTX(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNum);
void nRF24L01_status(nRF24L01* chip);

void nRF24L01_TxPacket(nRF24L01* chip, uint8_t* bufTX);
//@return the pipeId if receive successfully, otherwise 0xff as fail
uint8_t nRF24L01_RxPacket(nRF24L01* chip, uint8_t* bufRX);

// ---------------------------------------------------------------------------
// async methods based on ISR
// ---------------------------------------------------------------------------
// async transmit
hterr nRF24L01_transmit(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNum, uint8_t* payload);

// portal callbacks
void nRF24L01_OnReceived(nRF24L01* chip, uint8_t pipeId, uint8_t* payload, uint8_t payloadLen);
void nRF24L01_OnSent(nRF24L01* chip, uint32_t destNodeId, uint8_t destPortNun, uint8_t* payload);

// entry for ExtI
void nRF24L01_doISR(nRF24L01* chip);

#ifdef HT_DDL
#  include "htddl.h"
#  if nRF24L01_MAX_PLOAD < HTDDL_MSG_LEN_MAX
#    error nRF24L01_MAX_PLOAD < HTDDL_MSG_LEN_MAX
#  endif

// represent nRF24L01 as a HtDDL netif
hterr nRF24L01NetIf_attach(nRF24L01* chip, HtNetIf* netif); 
#endif // HT_DDL


#endif // __nRF24L01_H__

