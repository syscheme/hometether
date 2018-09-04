#ifndef __nRF24L01_H__
#define __nRF24L01_H__

#include "common.h"
#include "stm32f10x_spi.h"

typedef void*    PRF24L01SpiCtx;

// #define NRF2401_POLL

// ---------------------------------------------------------------------------
// definition of nRF24L01 chip
// ---------------------------------------------------------------------------
#define nRF24L01_ADDR_LEN   (5)
// #define nRF24L01_MAX_PLOAD  (32 -4 -nRF24L01_ADDR_LEN) // (32 -2 -nRF24L01_ADDR_LEN) max payload= 32byte - addrlen - 16bit crc
#define nRF24L01_MAX_PLOAD  16

#ifndef nRF_RX_CH
#  define nRF_RX_CH 5
#endif // nRF_RX_CH

typedef struct _nRF24L01
{
	uint32_t       nodeId;
	PRF24L01SpiCtx pCtx;
//	uint8_t        flags;	  // set of nRF24L01_FLAG_xxxx

	// about IO pins
	SPI_TypeDef*  spi;
	IO_PIN        pinCE; // the pin CE
	IO_PIN        pinCSN; // the pin CE

	// about the EXTI
	uint32_t     extiLine;

	uint32_t     peerNodeId;
	uint8_t      peerPortNum;
} nRF24L01;

// ---------------------------------------------------------------------------
// method declarations
// ---------------------------------------------------------------------------
// void nRF24L01_init(nRF24L01* chip, PRF24L01SpiCtx pCtx, const IO_PIN pinCE, const IO_PIN pinCSN, const uint8_t localAddr[nRF24L01_ADDR_LEN])
void nRF24L01_init(nRF24L01* chip, uint32_t nodeId, const uint8_t* txAddr, uint8_t rxmode);
// void nRF24L01_init(nRF24L01* chip, PRF24L01SpiCtx pCtx, const IO_PIN pinCE, const IO_PIN pinCSN, const uint8_t localAddr[nRF24L01_ADDR_LEN]);
void nRF24L01_setModeRX(nRF24L01* chip);
void nRF24L01_setModeTX(nRF24L01* chip, uint32_t peerNodeId, uint8_t peerPortNum);
void nRF24L01_status(nRF24L01* chip);

void nRF24L01_TxPacket(nRF24L01* chip, uint8_t* bufTX);

// return the pipeId if receive successfully, otherwise 0xff as fail
uint8_t nRF24L01_RxPacket(nRF24L01* chip, uint8_t* bufRX);

extern void OnWirelessMsg(nRF24L01* chip, uint8_t pipeId, uint8_t* bufPlayLoad, uint8_t playLoadLen);

// entry for ExtI
void processIRQ_nRF24L01(nRF24L01* chip);

#endif // __nRF24L01_H__

