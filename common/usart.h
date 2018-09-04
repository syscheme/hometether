#ifndef __USART_H__
#define __USART_H__

#include "htcomm.h"
#include "pbuf.h"

#ifdef WIN32
#  define USART_BY_FILE
#  define IO_USART HANDLE
#elif defined(LINUX)
#  define USART_BY_FILE
#  define IO_USART FILE*;
#endif // OS/chip

#ifndef USART_PENDING_MAX_TX
#  define USART_PENDING_MAX_TX      (0)
#endif // USART_PENDING_MAX_TX
#ifndef USART_PENDING_MAX_RX
#  define USART_PENDING_MAX_RX      (PBUF_MEMPOOL2_BSIZE -10)
#endif // USART_PENDING_MAX_RX

// ---------------------------------------------------------------------------
// definition of USART port
// ---------------------------------------------------------------------------
typedef struct _USART
{
	// about IO pins
	IO_USART      port;
	FIFO          txFIFO, rxFIFO;	// an iteration in FIFO ch
	__IO__ uint8_t opStatus;	  // flags of USART_OPFLAG_xxxx
	__IO__ uint8_t txTimeout;
#ifdef HT_DDL
	struct _HtNetIf* netif;
#endif // HT_DDL
} USART;

#define USART_OPFLAG_SEND_IN_PROGRESS      FLAG(0)  

// ---------------------------------------------------------------------------
// native methods
// ---------------------------------------------------------------------------
#ifdef USART_BY_FILE
hterr USART_open(USART* USARTx, const char* fnCOM, uint16_t baudX100);
void  USART_close(USART* USARTx);
#else
 // chip-specified that impl in io.c
hterr USART_open(USART* USARTx, uint16_t baudX100);
void  USART_doISR(USART* USARTx);
#define USART_close(USARTx)  // do nothing
#endif // OS

hterr USART_sendByte(USART* USARTx, uint8_t ch);
hterr USART_transmit(USART* USARTx, const uint8_t* data, uint16_t len);
uint16_t USART_receive(USART* USARTx, uint8_t* data, uint16_t maxlen);

// callback
void USART_OnReceived(USART* chip, uint8_t* data, uint16_t len);
void USART_OnSent(USART* chip);

#ifdef HT_DDL
#include "htddl.h"
hterr USARTNetIf_attach(USART* USARTx, HtNetIf* netif, const char* name);
void HtDDL_pollUSART(USART* USARTx);
#endif // HT_DDL

#endif // __USART_H__
