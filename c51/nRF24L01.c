#include "nRF24L01.h"

// uart:9600BPS
// -----------------------------

// -----------------------------
// Declarition of global variables
// -----------------------------
#define BYTE unsigned char
#define TX_ADR_WIDTH    5   // 5 bytes TX(RX) address width
#define TX_PLOAD_WIDTH  20  // 20 bytes TX payload

const BYTE TX_ADDRESS[TX_ADR_WIDTH]  = {0x34, 0x43, 0x10, 0x10, 0x01}; // Define a static TX address

BYTE rx_buf[TX_PLOAD_WIDTH];
BYTE tx_buf[TX_PLOAD_WIDTH];
BYTE flag;

// -------------------------------------------------------------------------------------------------
// Interface to nRF24L01, nRF24L01 pins
// -------------------------------------------------------------------------------------------------
// 1 - GND,   2 - VCC
// 3 - CE,    4 - CSN
// 5 - SCK,   6 - MOSI
// 7 - MISO,  8 - IRQ

sbit CE =  P1^0;
sbit CSN=  P1^1;
sbit SCK=  P1^2;
sbit MOSI= P1^3;
sbit MISO= P1^4;
sbit IRQ = P1^5;

// -----------------------------
BYTE 	bdata   sta;
sbit	RX_DR	=sta^6;
sbit	TX_DS	=sta^5;
sbit	MAX_RT	=sta^4;
// -----------------------------

/*
// Macro to read SPI Interrupt flag
#define WAIT_SPIF (!(SPI0CN & 0x80))  // SPI interrupt flag(�C platform dependent)

// Declare SW/HW SPI modes
#define SW_MODE   0x00
#define HW_MODE   0x01

// Define nRF24L01 interrupt flag's
#define MAX_RT  0x10  // Max #of TX retrans interrupt
#define TX_DS   0x20  // TX data sent interrupt
#define RX_DR   0x40  // RX data received

#define SPI_CFG 0x40  // SPI Configuration register value
#define SPI_CTR 0x01  // SPI Control register values
#define SPI_CLK 0x00  // SYSCLK/2*(SPI_CLK+1) == > 12MHz / 2 = 6MHz
#define SPI0E   0x02  // SPI Enable in XBR0 register
*/

// -------------------------------------------------------------------------------------------------
// SPI(nRF24L01) commands
// -------------------------------------------------------------------------------------------------
#define READ_REG        0x00  // Define read command to register
#define WRITE_REG       0x20  // Define write command to register
#define RD_RX_PLOAD     0x61  // Define RX payload register address
#define WR_TX_PLOAD     0xA0  // Define TX payload register address
#define FLUSH_TX        0xE1  // Define flush TX register command
#define FLUSH_RX        0xE2  // Define flush RX register command
#define REUSE_TX_PL     0xE3  // Define reuse TX payload register command
#define NOP             0xFF  // Define No Operation, might be used to read status register

// -------------------------------------------------------------------------------------------------
// SPI(nRF24L01) registers(addresses)
// -------------------------------------------------------------------------------------------------
#define CONFIG          0x00  // 'Config' register address
#define EN_AA           0x01  // 'Enable Auto Acknowledgment' register address
#define EN_RXADDR       0x02  // 'Enabled RX addresses' register address
#define SETUP_AW        0x03  // 'Setup address width' register address
#define SETUP_RETR      0x04  // 'Setup Auto. Retrans' register address
#define RF_CH           0x05  // 'RF channel' register address
#define RF_SETUP        0x06  // 'RF setup' register address
#define STATUS          0x07  // 'Status' register address
#define OBSERVE_TX      0x08  // 'Observe TX' register address
#define CD              0x09  // 'Carrier Detect' register address
#define RX_ADDR_P0      0x0A  // 'RX address pipe0' register address
#define RX_ADDR_P1      0x0B  // 'RX address pipe1' register address
#define RX_ADDR_P2      0x0C  // 'RX address pipe2' register address
#define RX_ADDR_P3      0x0D  // 'RX address pipe3' register address
#define RX_ADDR_P4      0x0E  // 'RX address pipe4' register address
#define RX_ADDR_P5      0x0F  // 'RX address pipe5' register address
#define TX_ADDR         0x10  // 'TX address' register address
#define RX_PW_P0        0x11  // 'RX payload width, pipe0' register address
#define RX_PW_P1        0x12  // 'RX payload width, pipe1' register address
#define RX_PW_P2        0x13  // 'RX payload width, pipe2' register address
#define RX_PW_P3        0x14  // 'RX payload width, pipe3' register address
#define RX_PW_P4        0x15  // 'RX payload width, pipe4' register address
#define RX_PW_P5        0x16  // 'RX payload width, pipe5' register address
#define FIFO_STATUS     0x17  // 'FIFO Status Register' register address

// -------------------------------------------------------------------------------------------------
// Function: init_io();
// Description:
//    flash led one time,chip enable(ready to TX or RX Mode), Spi disable, Spi clock line init high
// -------------------------------------------------------------------------------------------------
#define KEY 0xaa
void init_io(void)
{
	P0=KEY;		    // led light
	CE=0;			// chip enable
	CSN=1;			// Spi disable	
	SCK=0;			// Spi clock line init high
	P0=0xff;		// led close
}

// -------------------------------------------------------------------------------------------------
// Function: initUART();
// Description:
//   set uart working mode 
// -------------------------------------------------------------------------------------------------
void initUART(void)
{
	TMOD = 0x20;				//timer1 working mode 1
	TL1 = 0xfd;					//f7=9600 for 16mhz Fosc,and ... 
	TH1 = 0xfd;					//...fd=19200 for 11.0592mhz Fosc
	SCON = 0xd8;				//uart mode 3,ren==1
	PCON = 0x80;				//smod=0
	TR1 = 1;					//start timer1
}

// -------------------------------------------------------------------------------------------------
// Function: init_int0();
// Description:
//   enable int0 interrupt;
// -------------------------------------------------------------------------------------------------
void init_int0(void)
{
	EA=1;
	EX0=1;						// Enable int0 interrupt.
}

// -----------------------------
void delay_ms(unsigned int x)
{
	unsigned int i,j;
	i=0;
	for (i=0;i<x;i++)
	{
		j=108;
		;
		while(j--);
	}
}

// -------------------------------------------------------------------------------------------------
// Function: SPI_RW();
// Description:
//   Writes one byte to nRF24L01, and return the byte read from nRF24L01 during write, according to SPI protocol
// -------------------------------------------------------------------------------------------------
BYTE SPI_RW(BYTE byte)
{
	BYTE bit_ctr;
	for (bit_ctr=0; bit_ctr<8; bit_ctr++)   // output 8-bit
	{
		MOSI = (byte & 0x80);         // output 'byte', MSB to MOSI
		byte = (byte << 1);           // shift next bit into MSB..
		SCK = 1;                      // Set SCK high..
		byte |= MISO;       		  // capture current MISO bit
		SCK = 0;            		  // ..then set SCK low again
	}

	return(byte);           		  // return read byte
}

// -------------------------------------------------------------------------------------------------
// Function: SPI_RW_Reg();
// Description:
//   Writes value 'value' to register 'reg'
// -------------------------------------------------------------------------------------------------
BYTE SPI_RW_Reg(BYTE reg, BYTE value)
{
	BYTE status;

	CSN = 0;                   // CSN low, init SPI transaction
	status = SPI_RW(reg);      // select register
	SPI_RW(value);             // ..and write value to it..
	CSN = 1;                   // CSN high again

	return status;            // return nRF24L01 status byte
}

// -------------------------------------------------------------------------------------------------
// Function: SPI_Read();
// Description:
//   Read one byte from nRF24L01 register, 'reg'
// -------------------------------------------------------------------------------------------------
BYTE SPI_Read(BYTE reg)
{
	BYTE reg_val;

	CSN = 0;                // CSN low, initialize SPI communication...
	SPI_RW(reg);            // Select register to read from..
	reg_val = SPI_RW(0);    // ..then read registervalue
	CSN = 1;                // CSN high, terminate SPI communication

	return(reg_val);        // return register value
}

// -------------------------------------------------------------------------------------------------
// Function: SPI_Read_Buf();
// Description:
//    Reads 'bytes' #of bytes from register 'reg'. Typically used to read RX payload, Rx/Tx address
// -------------------------------------------------------------------------------------------------
BYTE SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bufLen)
{
	BYTE status, byte_ctr;

	CSN = 0;                    		// Set CSN low, init SPI tranaction
	status = SPI_RW(reg);       		// Select register to write to and read status byte

	for (byte_ctr=0; byte_ctr < bufLen; byte_ctr++)
		pBuf[byte_ctr] = SPI_RW(0);    // Perform SPI_RW to read byte from nRF24L01

	CSN = 1;                           // Set CSN high again
	return(status);                    // return nRF24L01 status byte
}

// -------------------------------------------------------------------------------------------------
// Function: SPI_Write_Buf();
// Description:
//   Writes contents of buffer '*pBuf' to nRF24L01. Typically used to write TX payload, Rx/Tx address
// -------------------------------------------------------------------------------------------------
BYTE SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bufLen)
{
	BYTE status,byte_ctr;

	CSN = 0;                   // Set CSN low, init SPI tranaction
	status = SPI_RW(reg);    // Select register to write to and read status byte
	for(byte_ctr=0; byte_ctr <bufLen; byte_ctr++) // then write all byte in buffer(*pBuf)
		SPI_RW(*pBuf++);

	CSN = 1;                 // Set CSN high again
	return(status);          // return nRF24L01 status byte
}

// -------------------------------------------------------------------------------------------------
// Function: RX_Mode();
// Description:
//   This function initializes one nRF24L01 device to RX Mode, set RX address, writes RX payload width,
//   select RF channel, datarate & LNA HCURR. After init, CE is toggled high, which means that
//   this device is now ready to receive a datapacket.
// -------------------------------------------------------------------------------------------------
void RX_Mode(void)
{
	CE=0;
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // Use the same address on the RX device as the TX device

	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0
	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0
	SPI_RW_Reg(WRITE_REG + RF_CH, 40);        // Select RF channel 40
	SPI_RW_Reg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH); // Select same RX payload width as TX Payload width
	SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   // TX_PWR:0dBm, Datarate:2Mbps, LNA:HCURR
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);     // Set PWR_UP bit, enable CRC(2 bytes) & Prim:RX. RX_DR enabled..

	CE = 1; // Set CE pin high to enable RX device

	//  This device is now ready to receive one packet of 16 bytes payload from a TX device sending to address
	//  '3443101001', with auto acknowledgment, retransmit count of 10, RF channel 40 and datarate = 2Mbps.
}

// -------------------------------------------------------------------------------------------------
// Function: TX_Mode();
// Description:
//   This function initializes one nRF24L01 device to TX mode, set TX address, set RX address for auto.ack,
//   fill TX payload, select RF channel, datarate & TX pwr. PWR_UP is set, CRC(2 bytes) is enabled, & PRIM:TX.
// ToDo:
//   One high pulse(>10us) on CE will now send this packet and expext an acknowledgment from the RX device.
// -------------------------------------------------------------------------------------------------
void TX_Mode(void)
{
	CE=0;

	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // Writes TX_Address to nRF24L01
	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // RX_Addr0 same as TX_Adr for Auto.Ack
	SPI_Write_Buf(WR_TX_PLOAD, tx_buf, TX_PLOAD_WIDTH); // Writes data to TX payload

	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0
	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0
	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x1a); // 500us + 86us, 10 retrans...
	SPI_RW_Reg(WRITE_REG + RF_CH, 40);        // Select RF channel 40
	SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   // TX_PWR:0dBm, Datarate:2Mbps, LNA:HCURR
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);     // Set PWR_UP bit, enable CRC(2 bytes) & Prim:TX. MAX_RT & TX_DS enabled..

	CE=1;
}

// -------------------------------------------------------------------------------------------------
// Function: TxData();
// Description:
//   write data x to SBUF
// -------------------------------------------------------------------------------------------------
void TxData (BYTE x)
{
	SBUF=x;			// write data x to SBUF
	while(TI==0);
	TI=0;
}

// -------------------------------------------------------------------------------------------------
// Function: CheckButtons();
// Description:
//   check buttons, if have press,read the key values, turn on led and transmit it; after transmition,
//   if received ACK, clear TX_DS interrupt and enter RX Mode;  turn off the led
// -------------------------------------------------------------------------------------------------
void CheckButtons()
{
	BYTE Temp,xx,Tempi;

	P0=0xff;
	Temp=P0&KEY;			         //read key value from port P0
	if (Temp!=KEY)
	{	
		delay_ms(10);
		Temp=P0&KEY;				// read key value from port P0

		if (Temp!=KEY)
		{
			xx=Temp;
			Tempi=Temp>>1;		// Left shift 4 bits
			P0=Tempi;		    // Turn On the led
			tx_buf[0]=Tempi;	// Save to tx_buf[0]

			TX_Mode();			// set TX Mode and transmitting
			TxData(xx);			// send data to uart

			//check_ACK();		// if have acknowledgment from RX device,turn on all led
			SPI_RW_Reg(WRITE_REG+STATUS,SPI_Read(READ_REG+STATUS));	// clear interrupt flag(TX_DS)
			delay_ms(500);
			P0=0xff;			// Turn off the led				
			RX_Mode();			// set receive mode

			while((P0 & KEY) !=KEY);
		}
	}
}

/*
// -------------------------------------------------------------------------------------------------
// Function: main();
// Description:
//   control all subprogrammes;
// -------------------------------------------------------------------------------------------------
void main(void)
{
	BYTE xx;
	init_io();		// Initialize IO port
	initUART();		// initialize 232 uart
	//init_int0();	// enable int0 interrupt
	RX_Mode();		// set RX mode

	while(1)
	{
		CheckButtons(); // scan key value and transmit 
	
		sta = SPI_Read(STATUS);	// read register STATUS's value
		if (RX_DR)				// if receive data ready (RX_DR) interrupt
		{
			SPI_Read_Buf(RD_RX_PLOAD, rx_buf, TX_PLOAD_WIDTH);// read receive payload from RX_FIFO buffer
			flag=1;
		}

		if (MAX_RT)
		{
			SPI_RW_Reg(FLUSH_TX,0);
		}

		SPI_RW_Reg(WRITE_REG +STATUS, sta);// clear RX_DR or TX_DS or MAX_RT interrupt flag

		if (flag)		// finish received
		{
			flag=0;		//	set flag=0
			P0=rx_buf[0];	// turn on led
			delay_ms(500);
			P0=0xff;		// turn off led
			xx=rx_buf[0]>>1;// right shift 4 bits
			TxData(xx);		// send data to uart
		}
	}
}
*/
// -------------------------------------------------------------------------------------------------
// Function: ISR_int0() interrupt 0;
// Description:
//   if RX_DR=1 or TX_DS or MAX_RT=1,enter this subprogram;
//   if RX_DR=1, read the payload from RX_FIFO and set flag;
// -------------------------------------------------------------------------------------------------
void ISR_int0(void) interrupt 0
{
	sta=SPI_Read(STATUS);	// read register STATUS's value
	if (RX_DR)				// if receive data ready (RX_DR) interrupt
	{
		SPI_Read_Buf(RD_RX_PLOAD, rx_buf, TX_PLOAD_WIDTH); // read receive payload from RX_FIFO buffer
		flag=1;
	}

	if (MAX_RT)
	{
		SPI_RW_Reg(FLUSH_TX,0);
	}

	SPI_RW_Reg(WRITE_REG+STATUS,sta);// clear RX_DR or TX_DS or MAX_RT interrupt flag
}

