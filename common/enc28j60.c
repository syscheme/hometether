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

// ENC28J60 Control Registers
// Control register definitions are a combination of address,
// bank number, and Ethernet/MAC/PHY indicator bits.
// - Register address        (bits 0-4)
// - Bank number        (bits 5-6)
// - MAC/PHY indicator        (bit 7)
#define ADDR_MASK        0x1F
#define BANK_MASK        0x60
#define SPRD_MASK        0x80

// All-bank registers
#define EIE              0x1B
#define EIR              0x1C
#define ESTAT            0x1D
#define ECON2            0x1E
#define ECON1            0x1F

// Bank 0 registers
#define ERDPTL           (0x00|0x00)
#define ERDPTH           (0x01|0x00)
#define EWRPTL           (0x02|0x00)
#define EWRPTH           (0x03|0x00)
#define ETXSTL           (0x04|0x00)
#define ETXSTH           (0x05|0x00)
#define ETXNDL           (0x06|0x00)
#define ETXNDH           (0x07|0x00)
#define ERXSTL           (0x08|0x00)
#define ERXSTH           (0x09|0x00)
#define ERXNDL           (0x0A|0x00)
#define ERXNDH           (0x0B|0x00)
#define ERXRDPTL         (0x0C|0x00)
#define ERXRDPTH         (0x0D|0x00)
#define ERXWRPTL         (0x0E|0x00)
#define ERXWRPTH         (0x0F|0x00)
#define EDMASTL          (0x10|0x00)
#define EDMASTH          (0x11|0x00)
#define EDMANDL          (0x12|0x00)
#define EDMANDH          (0x13|0x00)
#define EDMADSTL         (0x14|0x00)
#define EDMADSTH         (0x15|0x00)
#define EDMACSL          (0x16|0x00)
#define EDMACSH          (0x17|0x00)

// Bank 1 registers
#define EHT0             (0x00|0x20)
#define EHT1             (0x01|0x20)
#define EHT2             (0x02|0x20)
#define EHT3             (0x03|0x20)
#define EHT4             (0x04|0x20)
#define EHT5             (0x05|0x20)
#define EHT6             (0x06|0x20)
#define EHT7             (0x07|0x20)
#define EPMM0            (0x08|0x20)
#define EPMM1            (0x09|0x20)
#define EPMM2            (0x0A|0x20)
#define EPMM3            (0x0B|0x20)
#define EPMM4            (0x0C|0x20)
#define EPMM5            (0x0D|0x20)
#define EPMM6            (0x0E|0x20)
#define EPMM7            (0x0F|0x20)
#define EPMCSL           (0x10|0x20)
#define EPMCSH           (0x11|0x20)
#define EPMOL            (0x14|0x20)
#define EPMOH            (0x15|0x20)
#define EWOLIE           (0x16|0x20)
#define EWOLIR           (0x17|0x20)
#define ERXFCON          (0x18|0x20)
#define EPKTCNT          (0x19|0x20)

// Bank 2 registers
#define MACON1           (0x00|0x40|0x80)
#define MACON2           (0x01|0x40|0x80)
#define MACON3           (0x02|0x40|0x80)
#define MACON4           (0x03|0x40|0x80)
#define MABBIPG          (0x04|0x40|0x80)
#define MAIPGL           (0x06|0x40|0x80)
#define MAIPGH           (0x07|0x40|0x80)
#define MACLCON1         (0x08|0x40|0x80)
#define MACLCON2         (0x09|0x40|0x80)
#define MAMXFLL          (0x0A|0x40|0x80)
#define MAMXFLH          (0x0B|0x40|0x80)
#define MAPHSUP          (0x0D|0x40|0x80)
#define MICON            (0x11|0x40|0x80)
#define MICMD            (0x12|0x40|0x80)
#define MIREGADR         (0x14|0x40|0x80)
#define MIWRL            (0x16|0x40|0x80)
#define MIWRH            (0x17|0x40|0x80)
#define MIRDL            (0x18|0x40|0x80)
#define MIRDH            (0x19|0x40|0x80)

// Bank 3 registers
#define MAADR1           (0x00|0x60|0x80)
#define MAADR0           (0x01|0x60|0x80)
#define MAADR3           (0x02|0x60|0x80)
#define MAADR2           (0x03|0x60|0x80)
#define MAADR5           (0x04|0x60|0x80)
#define MAADR4           (0x05|0x60|0x80)
#define EBSTSD           (0x06|0x60)
#define EBSTCON          (0x07|0x60)
#define EBSTCSL          (0x08|0x60)
#define EBSTCSH          (0x09|0x60)
#define MISTAT           (0x0A|0x60|0x80)
#define EREVID           (0x12|0x60)
#define ECOCON           (0x15|0x60)
#define EFLOCON          (0x17|0x60)
#define EPAUSL           (0x18|0x60)
#define EPAUSH           (0x19|0x60)

// PHY registers
#define PHCON1           0x00
#define PHSTAT1          0x01
#define PHHID1           0x02
#define PHHID2           0x03
#define PHCON2           0x10
#define PHSTAT2          0x11
#define PHIE             0x12
#define PHIR             0x13
#define PHLCON           0x14

// ENC28J60 ERXFCON Register Bit Definitions
#define ERXFCON_UCEN     0x80
#define ERXFCON_ANDOR    0x40
#define ERXFCON_CRCEN    0x20
#define ERXFCON_PMEN     0x10
#define ERXFCON_MPEN     0x08
#define ERXFCON_HTEN     0x04
#define ERXFCON_MCEN     0x02
#define ERXFCON_BCEN     0x01

// ENC28J60 EIE Register Bit Definitions
#define EIE_INTIE        0x80
#define EIE_PKTIE        0x40
#define EIE_DMAIE        0x20
#define EIE_LINKIE       0x10
#define EIE_TXIE         0x08
#define EIE_WOLIE        0x04
#define EIE_TXERIE       0x02
#define EIE_RXERIE       0x01

// ENC28J60 EIR Register Bit Definitions
#define EIR_PKTIF        0x40
#define EIR_DMAIF        0x20
#define EIR_LINKIF       0x10
#define EIR_TXIF         0x08
#define EIR_WOLIF        0x04
#define EIR_TXERIF       0x02
#define EIR_RXERIF       0x01
#define EIR_ALL          0xff

// ENC28J60 ESTAT Register Bit Definitions
#define ESTAT_INT        0x80
#define ESTAT_LATECOL    0x10
#define ESTAT_RXBUSY     0x04
#define ESTAT_TXABRT     0x02
#define ESTAT_CLKRDY     0x01
// ENC28J60 ECON2 Register Bit Definitions
#define ECON2_AUTOINC    0x80
#define ECON2_PKTDEC     0x40
#define ECON2_PWRSV      0x20
#define ECON2_VRPS       0x08

// ENC28J60 ECON1 Register Bit Definitions
#define ECON1_TXRST      0x80
#define ECON1_RXRST      0x40
#define ECON1_DMAST      0x20
#define ECON1_CSUMEN     0x10
#define ECON1_TXRTS      0x08
#define ECON1_RXEN       0x04
#define ECON1_BSEL1      0x02
#define ECON1_BSEL0      0x01

// ENC28J60 MACON1 Register Bit Definitions
#define MACON1_LOOPBK    0x10
#define MACON1_TXPAUS    0x08
#define MACON1_RXPAUS    0x04
#define MACON1_PASSALL   0x02
#define MACON1_MARXEN    0x01

// ENC28J60 MACON2 Register Bit Definitions
#define MACON2_MARST     0x80
#define MACON2_RNDRST    0x40
#define MACON2_MARXRST   0x08
#define MACON2_RFUNRST   0x04
#define MACON2_MATXRST   0x02
#define MACON2_TFUNRST   0x01

// ENC28J60 MACON3 Register Bit Definitions
#define MACON3_PADCFG2   0x80
#define MACON3_PADCFG1   0x40
#define MACON3_PADCFG0   0x20
#define MACON3_TXCRCEN   0x10
#define MACON3_PHDRLEN   0x08
#define MACON3_HFRMLEN   0x04
#define MACON3_FRMLNEN   0x02
#define MACON3_FULDPX    0x01

// ENC28J60 MACON4 Register Bit Definitions
#define	MACON4_DEFER	(1<<6)
#define	MACON4_BPEN		(1<<5)
#define	MACON4_NOBKOFF	(1<<4)

// ENC28J60 MICMD Register Bit Definitions
#define MICMD_MIISCAN    0x02
#define MICMD_MIIRD      0x01

// ENC28J60 MISTAT Register Bit Definitions
#define MISTAT_NVALID    0x04
#define MISTAT_SCAN      0x02
#define MISTAT_BUSY      0x01

// ENC28J60 PHY PHCON1 Register Bit Definitions
#define PHCON1_PRST      0x8000
#define PHCON1_PLOOPBK   0x4000
#define PHCON1_PPWRSV    0x0800
#define PHCON1_PDPXMD    0x0100

// ENC28J60 PHY PHSTAT1 Register Bit Definitions
#define PHSTAT1_PFDPX    0x1000
#define PHSTAT1_PHDPX    0x0800
#define PHSTAT1_LLSTAT   0x0004
#define PHSTAT1_JBSTAT   0x0002

// ENC28J60 PHY PHCON2 Register Bit Definitions
#define PHCON2_FRCLINK   0x4000
#define PHCON2_TXDIS     0x2000
#define PHCON2_JABBER    0x0400
#define PHCON2_HDLDIS    0x0100

// ENC28J60 Packet Control Byte Bit Definitions
#define PKTCTRL_PHUGEEN  0x08
#define PKTCTRL_PPADEN   0x04
#define PKTCTRL_PCRCEN   0x02
#define PKTCTRL_POVERRIDE 0x01

// ENC28J60 PHY PHSTAT2 Register Bit Definitions
#define PHSTAT2_TXSTAT	(1 << 13)
#define PHSTAT2_RXSTAT	(1 << 12)
#define PHSTAT2_COLSTAT	(1 << 11)
#define PHSTAT2_LSTAT	(1 << 10)
#define PHSTAT2_DPXSTAT	(1 << 9)
#define PHSTAT2_PLRITY	(1 << 5)

// SPI operation codes
#define ENC28J60_READ_CTRL_REG       0x00
#define ENC28J60_READ_BUF_MEM        0x3A
#define ENC28J60_WRITE_CTRL_REG      0x40
#define ENC28J60_WRITE_BUF_MEM       0x7A
#define ENC28J60_BIT_FIELD_SET       0x80
#define ENC28J60_BIT_FIELD_CLR       0xA0
#define ENC28J60_SOFT_RESET          0xFF

// SPI Chip Select
#define pinH(_pin)        pinSET(_pin, 1)
#define pinL(_pin)        pinSET(_pin, 0)

#define CSPASSIVE(_CHIP)  pinH(_CHIP->pinCSN) 
#define CSACTIVE(_CHIP)   pinL(_CHIP->pinCSN) 

//
#define ShiftToWordH(_B) (((uint16_t) _B)<<8)

uint8_t  ENC28J60_readOp(ENC28J60* nic, uint8_t op, uint8_t address);
void     ENC28J60_writeOp(ENC28J60* nic, uint8_t op, uint8_t address, uint8_t data);
void     ENC28J60_readBuf(ENC28J60* nic, uint16_t len, uint8_t* data);
void     ENC28J60_writeBuf(ENC28J60* nic, uint16_t len, uint8_t* data);

void     ENC28J60_setBank(ENC28J60* nic, uint8_t address);

uint8_t  ENC28J60_read(ENC28J60* nic, uint8_t address);
void     ENC28J60_write(ENC28J60* nic, uint8_t address, uint8_t data);
void     ENC28J60_clrBits(ENC28J60* nic, uint8_t address, uint8_t flags);
void     ENC28J60_setBits(ENC28J60* nic, uint8_t address, uint8_t flags);

void     ENC28J60_PhyWrite(ENC28J60* nic, uint8_t address, uint16_t data);

uint8_t  ENC28J60_getRev(ENC28J60* nic);
uint8_t  ENC28J60_linkup(ENC28J60* nic);

/*
uint8_t ENC28J60_readOp0(ENC28J60* nic, uint8_t op, uint8_t address)
{
int temp=0;
CSACTIVE(nic);

SPI_RW(nic->spi, (op | (address & ADDR_MASK)));
waitspi(nic->spi);
temp=SPI_ReceiveData(nic->spi);	 // the leading invalid byte
SPI_RW(nic->spi, 0x00);
waitspi(nic->spi);

// do dummy read if needed (for mac and mii, see datasheet page 29)
if(address & 0x80)
{
SPI_ReceiveData(nic->spi);
SPI_RW(nic->spi, 0x00);
waitspi(nic->spi);
}

temp=SPI_ReceiveData(nic->spi);
CSPASSIVE(nic);
return (temp);
}
*/

uint8_t ENC28J60_readOp(ENC28J60* nic, uint8_t op, uint8_t address)
{
	int temp=0;
	CSACTIVE(nic);

	temp = SPI_RW(nic->spi, (op | (address & ADDR_MASK)));
	temp = SPI_RW(nic->spi, 0x00);
	if (address & 0x80) // do dummy read if needed (for mac and mii, see datasheet page 29)
		temp = SPI_RW(nic->spi, 0x00);

	CSPASSIVE(nic);

	return (temp);
}

/*
void ENC28J60_writeOp(ENC28J60* nic, uint8_t op, uint8_t address, uint8_t data)
{
CSACTIVE(nic);

// temp = SPI_RW(nic->spi, (op | (address & ADDR_MASK)));
SPI_RW(nic->spi, op | (address & ADDR_MASK));
waitspi(nic->spi);

SPI_RW(nic->spi, data);
waitspi(nic->spi);

CSPASSIVE(nic);

//	rt_hw_interrupt_enable(level);
}
*/

void ENC28J60_writeOp(ENC28J60* nic, uint8_t op, uint8_t address, uint8_t data)
{
	CSACTIVE(nic);

	SPI_RW(nic->spi, op | (address & ADDR_MASK));
	SPI_RW(nic->spi, data);

	CSPASSIVE(nic);
}

void ENC28J60_readBuf(ENC28J60* nic, uint16_t len, uint8_t* data)
{
	CSACTIVE(nic);

	SPI_RW(nic->spi, ENC28J60_READ_BUF_MEM);

	while(len--)
	{
		*data++= SPI_RW(nic->spi, 0x00);
	}

	CSPASSIVE(nic);
}

void ENC28J60_writeBuf(ENC28J60* nic, uint16_t len, uint8_t* data)
{
	CSACTIVE(nic);
	// issue write command
	SPI_RW(nic->spi, ENC28J60_WRITE_BUF_MEM);

	while (len--)
	{
		// write data
		SPI_RW(nic->spi, *data++);
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
	// set the PHY register address
	ENC28J60_write(nic, MIREGADR, address);
	// write the PHY data
	ENC28J60_write(nic, MIWRL, data&0xff);
	ENC28J60_write(nic, MIWRH, data>>8);

	// wait until the PHY write completes, CAN BE IGNORED? http://www.phpfans.net/article/htmls/200907/Mjc1MzE0.html
#ifdef  ENC28J60_ISR_ENABLE
	while ENC28J60_read(nic, MISTAT) & MISTAT_BUSY)
	delayXusec(15);	
#endif // ENC28J60_ISR_ENABLE
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
//	uint8_t tmp;
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
	// Do not send packets longer than ENC28J60_MTU_SIZE:
	ENC28J60_write(nic, MAMXFLL, ENC28J60_MTU_SIZE&0xFF);	
	ENC28J60_write(nic, MAMXFLH, ENC28J60_MTU_SIZE>>8);

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
	nic->macaddr[0] = ENC28J60_read(nic, MAADR0);
	nic->macaddr[1] = ENC28J60_read(nic, MAADR1);
	nic->macaddr[2] = ENC28J60_read(nic, MAADR2);
	nic->macaddr[3] = ENC28J60_read(nic, MAADR3);
	nic->macaddr[4] = ENC28J60_read(nic, MAADR4);
	nic->macaddr[5] = ENC28J60_read(nic, MAADR5);

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

hterr ENC28J60_sendPacket(ENC28J60* nic, uint8_t* packet, uint16_t len)
{
	// Set the write pointer to start of transmit buffer area
	ENC28J60_write(nic, EWRPTL, TXSTART_INIT&0xFF);
	ENC28J60_write(nic, EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	ENC28J60_write(nic, ETXNDL, (TXSTART_INIT+len)&0xFF);
	ENC28J60_write(nic, ETXNDH, (TXSTART_INIT+len)>>8);
	// write per-packet control byte (0x00 means use macon3 settings)
	ENC28J60_writeOp(nic, ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	ENC28J60_writeBuf(nic, len, packet);
	// send the contents of the transmit buffer onto the network
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

#ifdef  ENC28J60_ISR_ENABLE
	nic->eir = ENC28J60_read(nic, EIR);
	// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if ( nic->eir & EIR_TXERIF)
	{
		ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
		return 0;
	}
#endif // ENC28J60_ISR_ENABLE

	return ERR_SUCCESS;
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
	if (ENC28J60_PhyReadH(nic, PHSTAT2) &PHSTAT2_DPXSTAT)
		nic->flags |= ENC28J60_FLG_DUPLEX;
	else nic->flags &= ~ENC28J60_FLG_DUPLEX;

	return (nic->flags &ENC28J60_FLG_DUPLEX) ? TRUE : FALSE;	// on or off
}

#ifdef  ENC28J60_ISR_ENABLE
static uint8_t packet[ENC28J60_MTU_SIZE+1];

void ENC28J60_doISR(ENC28J60* nic)
{
	uint16_t len =0;
	//disable further interrupts by clearing the global interrup enable bit
	ENC28J60_setBank(nic, EIR);
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIE, EIE_INTIE);

	// get EIR
	while ((nic->eir = ENC28J60_read(nic, EIR)) & EIR_ALL)
	{
		// PKTIF The Receive Packet Pending Interrupt Flag (PKTIF) is used to
		// indicate the presence of one or more data packets in the receive
		// buffer and to provide a notification means for the arrival of new
		// packets. When the receive buffer has at least one packet in it,
		// EIR.PKTIF will be set. In other words, this interrupt flag will be
		// set anytime the Ethernet Packet Count register (EPKTCNT) is non-zero.
		// The PKTIF bit can only be cleared by the host controller or by a Reset
		// condition. In order to clear PKTIF, the EPKTCNT register must be
		// decremented to 0. If the last data packet in the receive buffer is
		// processed, EPKTCNT will become zero and the PKTIF bit will automatically
		// be cleared.
		// errata #4, PKTIF does not reliable
		len = ENC28J60_recvPacket(nic, ENC28J60_MTU_SIZE, packet);
		if (len>0)
			ENC28J60_OnReceived(nic, packet, len);

		ENC28J60_setBank(nic, EIR);
		// clear PKTIF
		if (nic->eir & EIR_PKTIF)
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_PKTIF);

		// EIR_DMAIF: indicates that the link status has changed.
		// The actual current link status can be obtained from the
		// PHSTAT1.LLSTAT or PHSTAT2.LSTAT. Unlike other interrupt sources, the
		// link status change interrupt is created in the integrated PHY
		// module.
		// To receive it, the host controller must set the PHIE.PLNKIE and
		// PGEIE bits. After setting the two PHY interrupt enable bits, the
		// LINKIF bit will then shadow the contents of the PHIR.PGIF bit.
		// Once LINKIF is set, it can only be cleared by the host controller or
		// by a Reset. The LINKIF bit is read-only. Performing an MII read on
		// the PHIR register will clear the LINKIF, PGIF and PLNKIF bits
		// automatically and allow for future link status change interrupts.
		if (nic->eir & EIR_DMAIF)
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_DMAIF);

		// EIR_LINKIF: indicates that the link status has changed.
		// The actual current link status can be obtained from the
		// PHSTAT1.LLSTAT or PHSTAT2.LSTAT. Unlike other interrupt sources, the
		// link status change interrupt is created in the integrated PHY
		// module.
		// To receive it, the host controller must set the PHIE.PLNKIE and
		// PGEIE bits. After setting the two PHY interrupt enable bits, the
		// LINKIF bit will then shadow the contents of the PHIR.PGIF bit.
		// Once LINKIF is set, it can only be cleared by the host controller or
		// by a Reset. The LINKIF bit is read-only. Performing an MII read on
		// the PHIR register will clear the LINKIF, PGIF and PLNKIF bits
		// automatically and allow for future link status change interrupts.
		if (nic->eir & EIR_LINKIF)
		{
			ENC28J60_linkStatus(nic);

			// read PHIR to clear the flag
			ENC28J60_PhyReadH(nic, PHIR);

			ENC28J60_setBank(nic, EIR);
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_LINKIF);
		}

		// TXIF: is used to indicate that	the requested packet transmission has ended. 
		// Upon transmission completion, abort or transmission cancellation by the host
		// controller, the EIR.TXIF flag will be set to 1.
		//  Once TXIF is set, it can only be cleared by the host controller
		// or by a Reset condition. Once processed, the host controller should
		// use the BFC command to clear the EIR.TXIF bit.
		if (nic->eir & EIR_TXIF)
		{
			// a frame has been transmitted.
			ENC28J60_setBank(nic, EIR);
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXIF);
			ENC28J60_OnSent(nic);
		}

		ENC28J60_setBank(nic, EIR);

		// TXERIF: is used to indicate that a transmit abort has occurred. An
		// abort can occur because of any of the following:
		// 1. Excessive collisions occurred as defined by the Retransmission
		//    Maximum (RETMAX) bits in the MACLCON1 register.
		// 2. A late collision occurred as defined by the Collision Window
		//   (COLWIN) bits in the MACLCON2 register.
		// 3. A collision after transmitting 64 bytes occurred (ESTAT.LATECOL
		//    set).
		// 4. The transmission was unable to gain an opportunity to transmit
		//    the packet because the medium was constantly occupied for too long.
		//    The deferral limit (2.4287 ms) was reached and the MACON4.DEFER bit
		//    was clear.
		// 5. An attempt to transmit a packet larger than the maximum frame
		//    length defined by the MAMXFL registers was made without setting
		//    the MACON3.HFRMEN bit or per packet POVERRIDE and PHUGEEN bits.
		// Upon any of these conditions, the EIR.TXERIF flag is set to 1. Once
		// set, it can only be cleared by the host controller or by a Reset
		// condition.
		// After a transmit abort, the TXRTS bit will be cleared, the
		// ESTAT.TXABRT bit will be set and the transmit status vector will be
		// written at ETXND + 1. The MAC will not automatically attempt to
		// retransmit the packet. The host controller may wish to read the
		// transmit status vector and LATECOL bit to determine the cause of
		// the abort. After determining the problem and solution, the host
		// controller should clear the LATECOL (if set) and TXABRT bits so
		// that future aborts can be detected accurately.
		// In Full-Duplex mode, condition 5 is the only one that should cause
		// this interrupt. Collisions and other problems related to sharing
		// the network are not possible on full-duplex networks. The conditions
		// which cause the transmit error interrupt meet the requirements of the
		// transmit interrupt. As a result, when this interrupt occurs, TXIF
		// will also be simultaneously set.

		// RXERIF: The Receive Error Interrupt Flag (RXERIF) is used to
		// indicate a receive buffer overflow condition. Alternately, this
		// interrupt may indicate that too many packets are in the receive
		// buffer and more cannot be stored without overflowing the EPKTCNT
		// register.  When a packet is being received and the receive buffer
		// runs completely out of space, or EPKTCNT is 255 and cannot be
		// incremented, the packet being received will be aborted (permanently
		// lost) and the EIR.RXERIF bit will be set to 1.
		// Once set, RXERIF can only be cleared by the host controller or by a
		// Reset condition. Normally, upon the receive error condition, the
		// host controller would process any packets pending from the receive
		// buffer and then make additional room for future packets by
		// advancing the ERXRDPT registers (low byte first) and decrementing
		// the EPKTCNT register.
		// Once processed, the host controller should use the BFC command to
		// clear the EIR.RXERIF bit.
		if (nic->eir & (EIR_TXERIF|EIR_RXERIF))
		{
			// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
			ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF|EIR_RXERIF);

			ENC28J60_OnError(nic, nic->eir & (EIR_TXERIF|EIR_RXERIF));
		}

	} // while (read EIR)

	// enable the interrupts
	ENC28J60_setBank(nic, EIR);
	ENC28J60_writeOp(nic, ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE);
}

#endif //  ENC28J60_ISR_ENABLE

/*
#ifdef HT_DDL

static hterr ENC28J60NetIf_doTX(HtNetIf *netif, pbuf *packet, hwaddr_t destAddr)
{
	uint32_t destNodeId, tmp=0;
	uint8_t* buf = heap_malloc(nRF24L01_MAX_PLOAD);
	pbuf_read(packet, 0, buf, min(nRF24L01_MAX_PLOAD, packet->tot_len));

	hterr    ENC28J60_sendPacket(ENC28J60* nic, uint8_t* packet, uint16_t len);

	destNodeId = *((uint32_t*) destAddr);
	if (0xE0 == (0xE000 & destNodeId))
		; // multicast
	
	tmp = nRF24L01_transmit(netif->pDriverCtx, destNodeId, tmp, buf);
	heap_free(buf);
	return tmp;
}

hterr ENC28J60NetIf_attach(nRF24L01* chip, HtNetIf* netif, const char* name)
{
	if (NULL == chip || NULL == netif)
		return ERR_INVALID_PARAMETER;

	HtNetIf_init(netif, chip, name, NULL,
				 nRF24L01_MAX_PLOAD, (uint8_t*) &chip->nodeId, 4, 0,
				 NetIf_OnReceived_Default, nRF24L01NetIf_doTX, NULL);
	chip->netif = netif;

	return ERR_SUCCESS;
}

uint8_t ENC28J60_OnReceived(ENC28J60* nic, uint8_t* packet, uint16_t len)
{
	pbuf* packet = pbuf_mmap(packet, len);
	if (NULL == chip || NULL == chip->netif || chip->netif->cbReceived || NULL == packet)
		return;

	chip->netif->cbReceived(chip->netif, packet, HTDDL_DEFAULT_POWERLEVEL);
	pbuf_free(packet);
}

#endif // HT_DDL
*/
