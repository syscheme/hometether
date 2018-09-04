#ifndef __Si4432_H__
#define __Si4432_H__

#include "pbuf.h"

#define BAND_434

// ---------------------------------------------------------------------------
// definition of SI4432 chip
// ---------------------------------------------------------------------------
// packet settings 
// #define nRF24L01_ADDR_LEN   (5)
// #define nRF24L01_MAX_PLOAD  (32 -4 -nRF24L01_ADDR_LEN) // (32 -2 -nRF24L01_ADDR_LEN) max payload= 32byte - addrlen - 16bit crc
#define	SI4432_MAX_PAYLOAD					(60)
#define SI4432_PREAMBLE_LENGTH				(10)   // 10bit preamble code
#define SI4432_PD_LENGTH					(8)		// 8bit detection threshold
#define SI4432_PENDING_TX_MAX               (10)

#define SI4432_OPFLAG_SEND_IN_PROGRESS      FLAG(0)  

typedef struct _SI4432
{
	uint32_t       nodeId;
	void*          pCtx;
	//	uint8_t        flags;	  // set of nRF24L01_FLAG_xxxx

	// about IO pins
	IO_SPI        spi;
	IO_PIN        pinCSN; // the pin CSN
	IO_PIN        pinSDN; // the pin SDN

	// about the EXTI
	// uint32_t      extiLine;

	uint16_t volatile ITstatus; // reserved, high-byte maps to SIREG_InterruptStatus2, low-byte maps to SIREG_InterruptStatus1
	uint8_t  volatile OPstatus; // the operation status, interact with user's invocation, see SI4432_OPFLAG_xxxx
	FIFO              txFIFO;

	uint32_t  volatile stampLastIO;	// due to SI4432 is easy to stuck, this stamp is used to test if it is healthy, only be update in the ISR
} SI4432;

// SIREG_InterruptStatus1
#define ITSTATUS_CRC_ERR     (1<<0)
#define ITSTATUS_PK_VALID    (1<<1)
#define ITSTATUS_PK_SENT     (1<<2)
#define ITSTATUS_EXT         (1<<3)
#define ITSTATUS_RX_FULL     (1<<4)
#define ITSTATUS_TX_EMPTY    (1<<5)
#define ITSTATUS_TX_FULL     (1<<6)
#define ITSTATUS_FIFO_ERR    (1<<7)

// SIREG_InterruptStatus2
#define ITSTATUS_POR         (1<<(0+8))
#define ITSTATUS_CHIPRDY     (1<<(1+8))
#define ITSTATUS_LBD         (1<<(2+8))
#define ITSTATUS_WUT         (1<<(3+8))
#define ITSTATUS_RSSI        (1<<(4+8))
#define ITSTATUS_PREAINVAL   (1<<(5+8))
#define ITSTATUS_PREAVAL     (1<<(6+8))
#define ITSTATUS_SWDET       (1<<(7+8))

typedef enum _SIBAUDID
{
	SIBAUD_2400bps			= 0,	//DR = 2.4kbps;	 Fdev = 36kHz;  BBBW = 75.2kHz;
	SIBAUD_4800bps			= 1,	//DR = 4.8kbps;	 Fdev = 45kHz;  BBBW = 95.3kHz; 
	SIBAUD_9600bps			= 2,	//DR = 9.6kbps;  Fdev = 45kHz;  BBBW = 112.8kHz;
	SIBAUD_19200bps			= 3,	//DR = 19.2kbps; Fdev = 9.6kHz; BBBW = 28.8kHz;
	SIBAUD_38400bps			= 4,	//DR = 38.4kbps; Fdev =19.2kHz; BBBW = 57.6kHz;
	SIBAUD_57600bps			= 5,	//DR = 57.6kbps; Fdev =28.8kHz; BBBW = 86.4kHz;
	SIBAUD_115200bps		= 6,	//DR =115.2kbps; Fdev =57.6kHz; BBBW = 172.8kHz;
} SIBAUDID;

// ---------------------------------------------------------------------------
// method declarations
// ---------------------------------------------------------------------------
hterr SI4432_init(SI4432* chip);
hterr SI4432_reset(SI4432* chip);

hterr SI4432_setBaud(SI4432* chip, SIBAUDID baudId);
hterr SI4432_setMode_IDLE(SI4432* chip);
hterr SI4432_setMode_RX(SI4432* chip);
// hterr SI4432_transmit(SI4432* chip, uint8_t * packet, uint8_t length);
hterr SI4432_transmit(SI4432* chip, pbuf* packet);
void  SI4432_doISR(SI4432* chip);

void SI4432_OnReceived(SI4432* chip, pbuf* packet);
void SI4432_OnSent(SI4432* chip, pbuf* packet);

// hterr set_dr(SIBAUDID setting);
hterr SI4432_setPower(SI4432* chip, uint8_t pwr);
hterr SI4432_setFrequence(SI4432* chip, uint8_t frq);

void RFCWTest(void);

#endif // __Si4432_H__

