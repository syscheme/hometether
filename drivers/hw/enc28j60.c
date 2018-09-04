/*********************************************
* vim:sw=8:ts=8:si:et
* To use the above modeline in vim you must have "set modeline" in your .vimrc
* Author: Guido Socher 
* Copyright: GPL V2
* http://www.gnu.org/licenses/gpl.html
*
* Based on the enc28j60.c file from the AVRlib library by Pascal Stang.
* For AVRlib See http://www.procyonengineering.com/
* Used with explicit permission of Pascal Stang.
*
* Title: Microchip ENC28J60 Ethernet Interface Driver
* Chip type           : ATMEGA88 with ENC28J60
*********************************************/
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

#include "enc28j60.h"

//static uint8_t Enc28j60Bank;
//static uint16_t NextPacketPtr;

// SPI Chip Select
#define CSPASSIVE(_CHIP)  pinH(_CHIP->pinCSN) 
#define CSACTIVE(_CHIP)   pinL(_CHIP->pinCSN) 

//
#define waitspi(_SPI) while(SET == SPI_GetFlagStatus(_SPI, SPI_FLAG_BSY))
#define ShiftToWordH(_B) (((uint16_t) _B)<<8)

uint8_t  ENC28J60_readOp(ENC28J60* nic, uint8_t op, uint8_t address);
void     ENC28J60_writeOp(ENC28J60* nic, uint8_t op, uint8_t address, uint8_t data);
void     ENC28J60_readBuf(ENC28J60* nic, uint16_t len, uint8_t* data);
void     ENC28J60_writeBuf(ENC28J60* nic, uint16_t len, uint8_t* data);

#ifndef SPI_GetFlagStatus
#  define SPI_GetFlagStatus      SPI_I2S_GetFlagStatus
#  define SPI_SendData           SPI_I2S_SendData
#  define SPI_ReceiveData        SPI_I2S_ReceiveData
#  define SPI_FLAG_RXNE          SPI_I2S_FLAG_RXNE
#  define SPI_FLAG_TXE           SPI_I2S_FLAG_TXE
#  define SPI_FLAG_BSY           SPI_I2S_FLAG_BSY
#endif

uint8_t ENC28J60_readOp(ENC28J60* nic, uint8_t op, uint8_t address)
{
	int temp=0;
	CSACTIVE(nic);

	SPI_SendData(nic->spi, (op | (address & ADDR_MASK)));
	waitspi(nic->spi);
	temp=SPI_ReceiveData(nic->spi);	 // the leading invalid byte
	SPI_SendData(nic->spi, 0x00);
	waitspi(nic->spi);

	// do dummy read if needed (for mac and mii, see datasheet page 29)
	if(address & 0x80)
	{
		SPI_ReceiveData(nic->spi);
		SPI_SendData(nic->spi, 0x00);
		waitspi(nic->spi);
	}

	temp=SPI_ReceiveData(nic->spi);
	CSPASSIVE(nic);
	return (temp);
}

void ENC28J60_writeOp(ENC28J60* nic, uint8_t op, uint8_t address, uint8_t data)
{
	CSACTIVE(nic);

	// temp = SPI_RW(nic->spi, (op | (address & ADDR_MASK)));
	SPI_SendData(nic->spi, op | (address & ADDR_MASK));
	waitspi(nic->spi);

	SPI_SendData(nic->spi, data);
	waitspi(nic->spi);

	CSPASSIVE(nic);

//	rt_hw_interrupt_enable(level);
}

void ENC28J60_readBuf(ENC28J60* nic, uint16_t len, uint8_t* data)
{
	CSACTIVE(nic);

	SPI_SendData(nic->spi, ENC28J60_READ_BUF_MEM);
	waitspi(nic->spi);

	SPI_ReceiveData(nic->spi);

	while(len--)
	{
	    SPI_SendData(nic->spi, 0x00);
		waitspi(nic->spi);

	    *data++= SPI_ReceiveData(nic->spi);
	}

	CSPASSIVE(nic);
}

void ENC28J60_writeBuf(ENC28J60* nic, uint16_t len, uint8_t* data)
{
	CSACTIVE(nic);
	// issue write command
	SPI_SendData(nic->spi, ENC28J60_WRITE_BUF_MEM);
	waitspi(nic->spi);

	while (len--)
	{
		// write data
	    SPI_SendData(nic->spi, *data++);
		waitspi(nic->spi);
	}

	CSPASSIVE(nic);
}

void ENC28J60_clrBits(ENC28J60* nic, uint8_t address, uint8_t flags)
{
	ENC28J60_setBank(nic, address);
	// do the write
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, address, flags);
}

void ENC28J60_setBits(ENC28J60* nic, uint8_t address, uint8_t flags)
{
	ENC28J60_setBank(nic, address);
	// do the write
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, address, flags);
}

void ENC28J60_setBank(ENC28J60* nic, uint8_t address)
{
	// set the bank (if needed)
	if ((address & BANK_MASK) != nic->currentBank)
	{
		// set the bank
		ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		nic->currentBank = (address & BANK_MASK);
	}
}

uint8_t ENC28J60_read(ENC28J60* nic, uint8_t address)
{
	// set the bank
	ENC28J60_setBank(nic, address);
	// do the read
	return ENC28J60_readOp(nic, ENC28J60_READ_CTRL_REG, address);
}

void ENC28J60_write(ENC28J60* nic, uint8_t address, uint8_t data)
{
	// set the bank
	ENC28J60_setBank(nic, address);
	// do the write
	ENC28J60_writeOp(nic, ENC28J60_WRITE_CTRL_REG, address, data);
}

void ENC28J60_PhyWrite(ENC28J60* nic, uint8_t address, uint16_t data)
{
	uint8_t tmp;
	// set the PHY register address
	ENC28J60_write(nic, MIREGADR, address);
	// write the PHY data
	ENC28J60_write(nic, MIWRL, data&0xff);
	ENC28J60_write(nic, MIWRH, data>>8);

	// wait until the PHY write completes, CAN BE IGNORED? http://www.phpfans.net/article/htmls/200907/Mjc1MzE0.html
	tmp = ENC28J60_read(nic, MISTAT);
/*
	while(tmp & MISTAT_BUSY)
	{
		delayXusec(15);	
		tmp = ENC28J60_read(nic, MISTAT);
	}
*/
//	while(ENC28J60_read(nic, MISTAT) & MISTAT_BUSY)
//		delayXusec(15);
}

// read upper 8 bits
uint8_t ENC28J60_PhyReadH(ENC28J60* nic, uint8_t address)
{
	// Set the right address and start the register read operation
	ENC28J60_write(nic, MIREGADR, address);
	ENC28J60_write(nic, MICMD, MICMD_MIIRD);
	delayXusec(15);

	// wait until the PHY read completes
	while(ENC28J60_read(nic, MISTAT) & MISTAT_BUSY);

	// reset reading bit
	ENC28J60_write(nic, MICMD, 0x00);
	return (ENC28J60_read(nic, MIRDH));
}

void ENC28J60_clkout(ENC28J60* nic, uint8_t clk)
{
	//setup clkout: 2 is 12.5MHz:
	ENC28J60_write(nic, ECOCON, clk & 0x7);
}

void ENC28J60_init(ENC28J60* nic)
{
	uint8_t tmp;
	// initialize I/O
	CSACTIVE(nic);
	pinH(nic->pinRESET); // ??

	CSPASSIVE(nic); // ss=0
	
	// step 0. hard-reset the NIC
	pinL(nic->pinRESET); // ??
	delayXmsec(200);
	pinH(nic->pinRESET); // ??
	delayXmsec(50);

 	nic->currentBank =0;

	// step 1. perform system reset
	ENC28J60_writeOp(nic, ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	delayXmsec(50);

	// check CLKRDY bit to see if reset is complete
	// The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
	//while(!(ENC28J60_read(ESTAT) & ESTAT_CLKRDY));
	
	// step 2. do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	nic->ptrNextPacket = RXSTART_INIT;

	// Rx start
	ENC28J60_write(nic, ERXSTL, RXSTART_INIT&0xFF);
	ENC28J60_write(nic, ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address
	ENC28J60_write(nic, ERXRDPTL, RXSTART_INIT&0xFF);
	ENC28J60_write(nic, ERXRDPTH, RXSTART_INIT>>8);
	// RX end
	ENC28J60_write(nic, ERXNDL, RXSTOP_INIT&0xFF);
	ENC28J60_write(nic, ERXNDH, RXSTOP_INIT>>8);
	
	// TX start
	ENC28J60_write(nic, ETXSTL, TXSTART_INIT&0xFF);
	ENC28J60_write(nic, ETXSTH, TXSTART_INIT>>8);
	// set transmission pointer address
	ENC28J60_write(nic, EWRPTL, TXSTART_INIT&0xFF);
	ENC28J60_write(nic, EWRPTH, TXSTART_INIT>>8);
	// TX end
	ENC28J60_write(nic, ETXNDL, TXSTOP_INIT&0xFF);
	ENC28J60_write(nic, ETXNDH, TXSTOP_INIT>>8);

	// step 3. do bank 1 stuff, packet filter:
	// For broadcast packets we allow only ARP packtets
	// All other packets should be unicast only for our mac (MAADR)
	//
	// The pattern to match on is therefore
	// Type     ETH.DST
	// ARP      BROADCAST
	// 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
	// in binary these poitions are:11 0000 0011 1111
	// This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	ENC28J60_write(nic, ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_BCEN);  //?? ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN
	ENC28J60_write(nic, EPMM0, 0x3f);
	ENC28J60_write(nic, EPMM1, 0x30);
	ENC28J60_write(nic, EPMCSL, 0xf9);
	ENC28J60_write(nic, EPMCSH, 0xf7);

	// step 4. do bank 2 stuff
	// 4.1 enable MAC receive
	ENC28J60_write(nic, MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	
	// 4.2 ?? bring MAC out of reset
	ENC28J60_write(nic, MACON2, 0x00);
	
	// 4.3 ?? enable automatic padding to 60bytes and CRC operations
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX); // MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNE
	
	// 4.4 set inter-frame gap (back-to-back)
	ENC28J60_write(nic, MABBIPG, 0x15); // 0x12
	
	ENC28J60_write(nic, MACON4, MACON4_DEFER);
	ENC28J60_write(nic, MACLCON2, 63);
	
	// 4.5. set inter-frame gap (non-back-to-back)
	ENC28J60_write(nic, MAIPGL, 0x12);
	ENC28J60_write(nic, MAIPGH, 0x0C);

	// 4.6 Set the maximum packet size which the controller will accept
	// Do not send packets longer than MAX_FRAMELEN:
	ENC28J60_write(nic, MAMXFLL, MAX_FRAMELEN&0xFF);	
	ENC28J60_write(nic, MAMXFLH, MAX_FRAMELEN>>8);
	
	// step 5. do bank 3 stuff
	// 5.1 write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	ENC28J60_write(nic, MAADR5, nic->macaddr[0]);
	ENC28J60_write(nic, MAADR4, nic->macaddr[1]);
	ENC28J60_write(nic, MAADR3, nic->macaddr[2]);
	ENC28J60_write(nic, MAADR2, nic->macaddr[3]);
	ENC28J60_write(nic, MAADR1, nic->macaddr[4]);
	ENC28J60_write(nic, MAADR0, nic->macaddr[5]);

	// test to read the mac back
	tmp = ENC28J60_read(nic, MAADR0);
	tmp = ENC28J60_read(nic, MAADR1);
	tmp = ENC28J60_read(nic, MAADR2);
	tmp = ENC28J60_read(nic, MAADR3);
	tmp = ENC28J60_read(nic, MAADR4);
	tmp = ENC28J60_read(nic, MAADR5);

	// 5.2 output off
	ENC28J60_write(nic, ECOCON, 0x00);

	// 5.3 full duplex
	ENC28J60_PhyWrite(nic, PHCON1, PHCON1_PDPXMD);

	// 5.4 no loopback of transmitted frames
	ENC28J60_PhyWrite(nic, PHCON2, PHCON2_HDLDIS);

 	// step 6. to complete initialization
	// 6.1 switch to bank 0
	ENC28J60_setBank(nic, ECON1);
	// 6.2 enable interrutps
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE|EIR_TXIF); // EIE_INTIE|EIE_PKTIE
	// 6.3 enable packet reception
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

	ENC28J60_clkout(nic, 2); // change clkout from 6.25MHz to 12.5MHz
	delayXmsec(10);

	ENC28J60_PhyWrite(nic, PHLCON, 0x476);	//0x476	to set the RJ45 LED
	delayXmsec(20);
}

// read the revision of the nic:
uint8_t ENC28J60_getRev(ENC28J60* nic)
{
	return ENC28J60_read(nic, EREVID);
}

// link status
uint8_t ENC28J60_linkup(ENC28J60* nic)
{
	// bit 10 (= bit 3 in upper reg)
	return (ENC28J60_PhyReadH(nic, PHSTAT2) && 4);
}

void ENC28J60_startPacketToSend(ENC28J60* nic, uint16_t len)
{
	// Set the write pointer to start of transmit buffer area
	ENC28J60_write(nic, EWRPTL, TXSTART_INIT & 0xFF);
	ENC28J60_write(nic, EWRPTH, TXSTART_INIT >> 8);
	// Set the TXND pointer to correspond to the packet size given
	ENC28J60_write(nic, ETXNDL, (TXSTART_INIT + len) & 0xFF);
	ENC28J60_write(nic, ETXNDH, (TXSTART_INIT + len) >> 8);

	// write per-packet control byte (0x00 means use macon3 settings)
	ENC28J60_writeOp(nic, ENC28J60_WRITE_BUF_MEM, 0, 0x00);
}

void ENC28J60_fillPacketToSend(ENC28J60* nic, uint16_t len, uint8_t* data)
{
	// copy the packet into the transmit buffer
	ENC28J60_writeBuf(nic, len, data);
}

uint8_t ENC28J60_submitPacketToSend(ENC28J60* nic)
{
	// send the contents of the transmit buffer onto the network
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if ((ENC28J60_read(nic, EIR) & EIR_TXERIF))
	{
		ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
		return 0;
	}

	return 1;
}

uint16_t ENC28J60_sendPacket(ENC28J60* nic, uint16_t len, uint8_t* packet)
{
	ENC28J60_startPacketToSend(nic, len);
	ENC28J60_fillPacketToSend(nic, len, packet);
	return ENC28J60_submitPacketToSend(nic)
}

// Gets a packet from the network receive buffer, if one is available.
// The packet will by headed by an ethernet header.
//      maxlen  The maximum acceptable length of a retrieved packet.
//      packet  Pointer where packet data should be stored.
// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
uint16_t ENC28J60_recvPacket(ENC28J60* nic, uint16_t maxlen, uint8_t* packet)
{
	uint16_t rxstat;
	uint16_t len;
	// check if a packet has been received and buffered
	//if( !(ENC28J60_read(EIR) & EIR_PKTIF) ){
	// The above does not work. See Rev. B4 Silicon Errata point 6.
	if (0 == ENC28J60_read(nic, EPKTCNT))
	{
		// switch to bank 0
		ENC28J60_setBank(nic, ECON1);
		// enable packet reception
		ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

		return 0;
	}

	//step 1. Set the read pointer to the start of the received packet
	ENC28J60_write(nic, ERDPTL, (nic->ptrNextPacket & 0xff));
	ENC28J60_write(nic, ERDPTH, (nic->ptrNextPacket >>8));

	//step 2. save the next packet pointer
	nic->ptrNextPacket  = ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0);
	nic->ptrNextPacket |= ShiftToWordH(ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0));

	//step 3. read the packet length (see datasheet page 43)
	len  = ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0);
	len |= ShiftToWordH(ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0));
	len-=4; //remove the CRC count
	
	// step 4. read the receive status (see datasheet page 43)
	rxstat  = ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0);
	rxstat |= ShiftToWordH(ENC28J60_readOp(nic, ENC28J60_READ_BUF_MEM, 0));
	
	// limit retrieve length
	if (len>maxlen-1)
		len=maxlen-1;

	// check CRC and symbol errors (see datasheet page 44, table 7-3):
	// The ERXFCON.CRCEN is set by default. Normally we should not
	// need to check this.
	if ((rxstat & 0x80)==0)
		len=0; // invalid
	else
		ENC28J60_readBuf(nic, len, packet); // copy the packet from the receive buffer

	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	ENC28J60_write(nic, ERXRDPTL, nic->ptrNextPacket & 0xff);
	ENC28J60_write(nic, ERXRDPTH, nic->ptrNextPacket>>8);

	// decrement the packet counter indicate we are done with this packet
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	return(len);
}

bool ENC28J60_linkStatus(ENC28J60* nic)
{
	uint16_t duplex;
	uint16_t reg = ENC28J60_PhyReadH(nic, PHSTAT2);
	duplex = reg & PHSTAT2_DPXSTAT;

	return (duplex) ? TRUE : FALSE;	// on or off
}

#if 0
// ---------------------------------------------------------------------------
//  sample RX handlers
// ---------------------------------------------------------------------------
// ignore PKTIF because is unreliable! (look at the errata datasheet)
// check EPKTCNT is the suggested workaround.
// We don't need to clear interrupt flag, automatically done when
// enc28j60_hw_rx() decrements the packet counter.
static uint8_t packet[MTU_SIZE+1];

void ENC28J60_ISR(ENC28J60* nic)
{
	uint16_t plen=0;

	do {
		// step 1, get the next new packet:
		plen = ENC28J60_recvPacket(&nic, sizeof(packet)-1, packet);

		// plen will ne unequal to zero if there is a valid packet (without crc error)
		if (plen <=0)
			break;

		// step 2. process the arp packet directly
		//   arp is broadcast if unknown but a host may also verify the mac address by sending it to 
		//   a unicast address.
		if (eth_type_is_arp_and_my_ip(packet, plen))
		{
			arp_answer_from_request(&nic, packet);
			continue;
		}

		// start processing IP packets here

		// step 3. check if ip packets are for us:
		if (0 == eth_type_is_ip_and_my_ip(packet, plen) ==0)
			continue;

		// step 4. respond ICMP directly
		if (packet[IP_PROTO_P]==IP_PROTO_ICMP_V && packet[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V)
		{
			// a ping packet, let's send pong
			echo_reply_from_request(&nic, packet,plen);
			continue;
		}

		// step 5. process TCP here
		if (packet[IP_PROTO_P]==IP_PROTO_TCP_V)
		{
			// if (packet[TCP_DST_PORT_L_P]/packet[TCP_DST_PORT_L_P] is NOT any listening or connection port)
			// 	continue;
		}

	} while (1);

}

void ENC28J60_ISR(ENC28J60* nic)
{
	// Variable definitions can be made now
	volatile uint32_t eir, pk_counter;
	volatile bool rx_activiated = FALSE;

	// get EIR
	eir = ENC28J60_read(nic, EIR);
	// rt_kprintf("eir: 0x%08x\n", eir);

	do
	{
		// errata #4, PKTIF does not reliable
	    pk_counter = ENC28J60_read(nic, EPKTCNT);
	    if (pk_counter)
	    {
	        // a frame has been received
	        eth_device_ready((struct eth_device*)&(enc28j60_dev->parent));

			// switch to bank 0
			ENC28J60_setBank(nic, EIE);
			// disable rx interrutps
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIE, EIE_PKTIE);
	    }

		// clear PKTIF
		if (eir & EIR_PKTIF)
		{
			ENC28J60_setBank(nic, EIR);
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_PKTIF);

			rx_activiated = TRUE;
		}

		// clear DMAIF
	    if (eir & EIR_DMAIF)
		{
			ENC28J60_setBank(nic, EIR);
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_DMAIF);
		}

	    // LINK changed handler
	    if ( eir & EIR_LINKIF)
	    {
	        ENC28J60_linkStatus(nic);

	        // read PHIR to clear the flag
	        ENC28J60_PhyRead(nic, PHIR);

			ENC28J60_setBank(nic, EIR);
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_LINKIF);
	    }

		if (eir & EIR_TXIF)
		{
			// A frame has been transmitted.
			ENC28J60_setBank(nic, EIR);
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXIF);
		}

		// TX Error handler/
		if ((eir & EIR_TXERIF) != 0)
		{
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF);
		}

		eir = ENC28J60_read(nic, EIR);
		// rt_kprintf("inner eir: 0x%08x\n", eir);
	} while ((rx_activiated != TRUE && eir != 0));
}
#endif // 0
