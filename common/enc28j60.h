/*****************************************************************************
* vim:sw=8:ts=8:si:et
*
* Title        : Microchip ENC28J60 Ethernet Interface Driver
* Author        : Pascal Stang (c)2005
* Modified by Guido Socher
* Copyright: GPL V2
*
*This driver provides initialization and transmit/receive
*functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
*This nic is novel in that it is a full MAC+PHY interface all in a 28-pin
*nic, using an SPI interface to the host processor.
*
*
*****************************************************************************/
//@{
/*

\\\|///
\\  - -  //
(  @ @  )
+---------------------oOOo-(_)-oOOo-------------------------+
|                       WEB SERVER                          |
|            ported to LPC2103 ARM7TDMI-S CPU               |
|                     by Xiaoran Liu                        |
|                       2007.12.16                          |
|                 ZERO research Instutute                   |
|                      www.the0.net                         |
|                            Oooo                           |
+----------------------oooO--(   )--------------------------+
(   )   ) /
\ (   (_/
\_)     

*/


#ifndef __ENC28J60_H__
#define __ENC28J60_H__

#include "htcomm.h"


// The RXSTART_INIT should be zero. See Rev. B4 Silicon Errata
// buffer boundaries applied to internal 8K ram
// the entire available packet buffer space is allocated
//
// start with recbuf at 0/
#define RXSTART_INIT     0x0
// receive buffer end
#define RXSTOP_INIT      (0x1FFF-0x0600-1)

// start TX buffer at 0x1FFF-0x0600, pace for one full ethernet frame (~1500 bytes)
#define TXSTART_INIT     (0x1FFF-0x0600)
// stp TX buffer at end of mem
#define TXSTOP_INIT      0x1FFF
//
// max frame length which the conroller will accept:
#define        ENC28J60_MTU_SIZE        1500        // (note: maximum ethernet frame length would be 1518)
//#define MAX_FRAMELEN     600

#define ENC28J60_FLG_DUPLEX (1<<0)
// -----------------------------
// nic ENC28J60
// -----------------------------
typedef struct _ENC28J60
{
	uint8_t macaddr[6];
	//	uint8_t ipaddr[4];
	//	PENC28J60SpiCtx pCtx;

	// about IO pins
	IO_SPI        spi;
	IO_PIN        pinCSN; // the pin CS
	IO_PIN        pinRESET; // the pin RESET

	// private ctx of ENC28J60
	__IO__ uint8_t       eir;
	__IO__ uint8_t       flags; // consists of ENC28J60_FLG_XXXX;
	__IO__ uint8_t       currentBank;
	__IO__ uint16_t      ptrNextPacket;

#ifdef HT_DDL
	struct _HtNetIf* netif;
#endif // HT_DDL
} ENC28J60;

// -----------------------------
// basic methods
// -----------------------------
void     ENC28J60_init(ENC28J60* nic);
hterr    ENC28J60_sendPacket(ENC28J60* nic, uint8_t* packet, uint16_t len);

//@return byte-size of the received packet
uint16_t ENC28J60_recvPacket(ENC28J60* nic, uint16_t maxlen, uint8_t* packet);

#ifdef  ENC28J60_ISR_ENABLE
// -----------------------------
// ISR based async methods
// -----------------------------
void ENC28J60_doISR(ENC28J60* nic);
uint8_t  ENC28J60_OnReceived(ENC28J60* nic, uint8_t* packet, uint16_t len);
uint8_t  ENC28J60_OnSent(ENC28J60* nic);
uint8_t  ENC28J60_OnError(ENC28J60* nic, uint8_t flags);
#endif // ENC28J60_ISR_ENABLE

#ifdef HT_DDL
#  include "htddl.h"
// represent ENC28J60 as a HtDDL netif
hterr ENC28J60NetIf_attach(ENC28J60* nic, HtNetIf* netif); 
#endif // HT_DDL

#endif // __ENC28J60_H__
