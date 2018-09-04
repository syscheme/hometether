#include "bsp.h"
#include "STC12C5AXX.h"
#include <reg52.h>
#include <intrins.h>
#include "defines.h"

#ifdef STC12C5AXX // sfr IAP_CONTR   = 0xC7;
void MCU_softReset()
{
	IAP_CONTR =0x20; 
}

void MCU_softResetToISP()
{
	IAP_CONTR =0x60; 
}
#endif // STC12C5AXX

// -----------------------------
// UART funcs
// -----------------------------
typedef struct _UART_baud
{
	uint16_t  baudX100; // baudrate in hundred bps
	uint8_t   reload; // the reload value
} UART_baud;

code UART_baud UART_baudTable[] = {

#ifdef Fosc_1T // ======== 1T chip ===============
// http://www.keil.com/products/c51/baudrate.asp, clockdiv=1, timer="time1,smod=0"

#if FoscMhz == 11 // 11.0592Mhz
	{  24, 0x70}, // error=0%
	{  48, 0xb8}, // error=0%
	{  96, 0xdc}, // error=0%
	{ 144, 0xe8}, // error=0%
	{ 192, 0xee}, // error=0%
	{ 288, 0xf4}, // error=0%
	{ 384, 0xf7}, // error=0%
	{ 576, 0xfa}, // error=0%
	{1152, 0xfd}, // error=0%
#endif // 11.0592Mhz

#  if FoscMhz == 12 // 12Mhz
	{  24, 0x64}, // err=0.16%
	{  48, 0xb2}, // err=0.16%
	{  96, 0xd9}, // err=0.16%
	{ 144, 0xe6}, // err=0.16%
	{ 192, 0xe4}, // err=-2.34%
	{ 288, 0xf3}, // err=0.16%
	{ 384, 0xf6}, // err=-2.34%
#  endif // 12Mhz

#if FoscMhz == 22 // 22.1184Mhz
	{  48, 0x70}, // error=0%
	{  96, 0xb8}, // error=0%
	{ 144, 0xd0}, // error=0%
	{ 192, 0xdc}, // error=0%
	{ 288, 0xe8}, // error=0%
	{ 384, 0xee}, // error=0%
	{ 576, 0xf4}, // error=0%
	{1152, 0xfa}, // error=0%
#endif // 22.1184Mhz

#if FoscMhz == 24 // 24Mhz
	{  48,   0x64}, // error=0.16%
	{  96,   0xb2}, // error=0.16%
	{ 144,   0xcc}, // error=0.16%
	{ 192,   0xd9}, // error=0.16%
	{ 288,   0xe6}, // error=0.16%
	{ 384,   0xec}, // error=-2.34%
	{ 576,   0xf3}, // error=0.16%
#endif // 24Mhz

#else // ======== !Fosc_1T, 12T chip, take T2 as the generator, T2CON = 0x34 ===============
	// http://www.keil.com/products/c51/baudrate.asp, clockdiv=32, timer="t2con=0x34"
#undef SMOD_1 // no SMOD_1 allowed

#if FoscMhz == 11 // 11.0592Mhz
	{ 12,  0xfee0}, // 1200, 0.00%
	{ 24,  0xff70}, // 2400, 0.00%
	{ 48,  0xffb8}, // 4800, 0.00%
	{ 96,  0xffdc}, // 9600, 0.00%
	{144,  0xffe8}, // 9600, 0.00%
	{192,  0xffee}, //19600, 0.00%
	{288,  0xfff4}, //28800, 0.00%
	{384,  0xfff7}, //28800, 0.00%
	{576,  0xfffa}, //28800, 0.00%
	{1152, 0xfffd}, //28800, 0.00%
#endif // 11.0592Mhz

#if FoscMhz == 12 // 12Mhz
	{  12,   0xfec8}, // error=0.16%
	{  24,   0xff64}, // error=0.16%
	{  48,   0xffb2}, // error=0.16%
	{  96,   0xffd8}, // error=0.16% // {  96,   0xffd9}, // error=0.16%
	{ 144,   0xffe6}, // error=0.16%
	{ 288,   0xfff3}, // error=0.16%
#endif // 12Mhz

#if FoscMhz == 22 // 22.1184Mhz
	{  12, 0xffc0}, // error=0%
	{  24, 0xffe0}, // error=0%
	{  48, 0xff70}, // error=0%
	{  96, 0xffb8}, // error=0%
	{ 144, 0xffd0}, // error=0%
	{ 192, 0xffdc}, // error=0%
	{ 288, 0xffe8}, // error=0%
	{ 384, 0xffee}, // error=0%
	{ 576, 0xfff4}, // error=0%
	{1152, 0xfffa}, // error=0%
#endif // 22.1184Mhz

#if FoscMhz == 24 // 24Mhz
	{  12, 0xfd8f}, // error=0.00%
	{  24, 0xfec8}, // error=0.16%
	{  48, 0xff64}, // error=0.16%
	{  96, 0xffb2}, // error=0.16%
	{ 144, 0xffcc}, // error=0.16%
	{ 192, 0xffd9}, // error=0.16%
	{ 288, 0xffe6}, // error=0.16%
	{ 576, 0xfff3}, // error=0.16%
#endif // 24Mhz

#endif // !Fosc_1T
	{   0, 0x00}
};

#ifndef T2CON
//  8052 Extensions
sfr T2CON  = 0xC8;
sfr RCAP2L = 0xCA;
sfr RCAP2H = 0xCB;
sfr TL2    = 0xCC;
sfr TH2    = 0xCD;
#endif // T2CON

uint UART_init(uint baudX100)
{
	uchar i;

	EA      = 0;    // disallow all interrupts
	SCON    = 0x50; // 0101,0000 mode 1 8n1, 10bit-per-byte: startbit=0, 8bit-data, stopbit=1

#ifdef SMOD_1 // SMOD=1 double speed
	PCON   |= 0x80;
	baudX100 >>=1;	// baud   >>=1; // take 1/2 of the wished baud to scan the BaudTable
#endif // SMOD_1

	// scan the baud table for the wished baud rate
	for (i=0; UART_baudTable[i].baudX100; i++)
	{
		if (!UART_baudTable[i+1].baudX100)
			break;

		if (UART_baudTable[i+1].baudX100 > baudX100)
			break;
	}

	// determ the reload value
#ifdef Fosc_1T
	// take the BRT to determine baud: BRTR=1, S1BRS=1
	AUXR   |= 0x15;      // T0x12,T1x12,UART_M0x6,BRTR=1, S2SMOD,BRTx12,EXTRAM,S1BRS=1	
	// BRT     = UART_baudTable[i].reload & 0xff;
#else
	// set the baud rate
	T2CON = 0x34;  // take T2 as the baud generator
	TMOD |= 0x20;  //??? not sure

	// always take T2 as the timer
	RCAP2L = UART_baudTable[i].reload & 0xff;
	RCAP2H = UART_baudTable[i].reload >> 8;
#endif // Fosc_1T

	baudX100 = UART_baudTable[i].baudX100;
#ifdef SMOD_1 // SMOD=1 double speed
	baudX100   <<=1; // double the baud back
#endif // SMOD_1

	TI      = 1;    // clear the SBUF
	ES      = 1;    // allow UART interrupt
	EA      = 1;    // allow interrupts

#ifdef UART_485
	UART_485Dir(0);
#endif // UART_485
	return baudX100;
}

uchar UART_read(uchar* buf, uchar maxLen)
{
	uchar i, j;

#if defined(UART_485) && !defined(UART_DIR_BATCH)
	UART_485Dir(0);
#endif // UART_DIR_BATCH

	if (!buf)
		return 0;

	for (i=0; i < maxLen; i++)
	{
		for(j = 255; !RI && j; j--)
			delayX10us(2);

		if (!j)
			return i;
		
		RI=0;
		buf[i] = SBUF;
	}

	return i;	
}

uchar UART_readbyte(void)
{
	uchar v =0;
	return (UART_read(&v, 1) ? v : 0);
}

uchar UART_write(uchar* buf, uchar maxLen)
{
	uchar i, j;

	if (!buf)
		return 0;

#if defined(UART_485) && !defined(UART_DIR_BATCH)
	UART_485Dir(1);
#endif // UART_DIR_BATCH

	for (i=0; i < maxLen; i++)
	{
		TI =0;
		SBUF = buf[i];
		while(!TI); // wait for the sending gets completed

		for(j=255; !TI && j; j--)
			delayX10us(2);
		TI =0; // reset TI
	}

#if defined(UART_485) && !defined(UART_DIR_BATCH)
	UART_485Dir(0);
#endif // UART_DIR_BATCH
	return i;
}

void UART_writebyte(uchar value)
{
	UART_write(&value, 1);
}

/*
uchar UART_readbyte(void)
{
	uchar value=200;

	for(;!RI && value; value--)
		delayX10us(2);
	if (!value)
		return 0x00;

	RI=0;
	value = SBUF;
	return value;	
}

void UART_writebyte(uchar value)
{
#if defined(UART_485) && !defined(UART_DIR_BATCH)
	UART_485Dir(1);
#endif // UART_DIR_BATCH

	TI =0;
	SBUF = value;
	while(!TI); // wait for the sending gets completed

	for(value=200; !TI && value
	; value--)
		delayX10us(2);
	TI =0; // reset TI

#if defined(UART_485) && !defined(UART_DIR_BATCH)
	UART_485Dir(0);
#endif // UART_DIR_BATCH
}
*/

uint UART_readword(void)
{
#ifdef DPTR
	DPL = UART_readbyte();  DPH = UART_readbyte(); // LSBF
	return DPTR;
#else
	uint value = UART_readbyte();
	value |= ((uint) UART_readbyte()) <<8;
	return value;
#endif // DPTR
}

void UART_writeword(uint value)
{
#ifdef DPTR
	DPTR = value; 
	UART_writebyte(DPL); UART_writebyte(DPH); // LSBF
#else
	UART_writebyte((uchar)(value & 0xff));
	UART_writebyte((uchar)((value >>8) & 0xff));
#endif // DPTR
}


uint T0_reload =0;
void Timer0_reload(void)
{
	TH0= T0_reload >> 8;//???,1ms??
	TL0= T0_reload & 0xff; // the low byte
}

void Timer0_init(uint Xmsec)
{
#ifdef Fosc_1T
	T0_reload = 65536 - Xmsec *FoscMhz*1000;
#else // assume the MCU takes 12T per instruction
	T0_reload = 65536 - Xmsec *FoscMhz*1000/12;
#endif // Fosc_T

	TMOD=0x01; // set T0 to run at mode 1
	Timer0_reload();
	ET0=1; // enable timer 0 interrupt
	TR0=1; // start timer0
	EA=1;  // enable interrupts
}

bool FIFO_init(FIFO_t* fifo, uint8 buf[], uint8 size)
{
	if (NULL == fifo || NULL == buf || size <= 0)
		return false;

	fifo->header = 0;
	fifo->tail   = 0;
	fifo->size   = size;
	fifo->buf    = buf;
	return true;
}

bool FIFO_tryWrite(FIFO_t* fifo, uint8 dat)
{
	uint8 newheader = fifo->header+1; 
	if (NULL == fifo || NULL == fifo->buf)
		return false;

	if (newheader >= fifo->size)
		newheader =0;

	if (newheader == fifo->tail)
		return false;

	fifo->buf[fifo->header] = dat;
	fifo->header = newheader;
	return true;
}

bool FIFO_tryRead(FIFO_t* fifo, uint8* dat)
{
	uint8 newtail = fifo->tail+1; 
	if (newtail >= fifo->size)
		newtail =0;

	if (newtail == fifo->header || fifo->tail == fifo->header)
		return false;

	*dat = fifo->buf[fifo->tail];
	fifo->tail = newtail;
	return true;
}

bool FIFO_write(FIFO_t* fifo, uint8 dat)
{
	if (NULL == fifo)
		return false;

	while(!FIFO_tryWrite(fifo, dat))
		delayX10us(1);

	return true;
}

// volatile MsgLine _pendingLines[MAX_PENDMSGS];
volatile MsgLine* MsgLine_findLineToFill(uint8 chId) reentrant
{
	int i =0;
	volatile MsgLine* thisMsg = NULL;

	for (i =0; i < MAX_PENDMSGS; i++)
	{
		if (0 != _pendingLines[i].timeout)
		{
			 _pendingLines[i].timeout--;
			 if (_pendingLines[i].chId == chId && _pendingLines[i].offset >0)
			 	thisMsg = &_pendingLines[i]; // continue with the recent incomplete receving;
		}
		else if (NULL == thisMsg) // take the first idle pend msg
			thisMsg = &_pendingLines[i];
	}

	return thisMsg;
}

void MsgLine_recv(uint8 chId, uint8* inBuf, uint8 maxLen)
{
	volatile MsgLine* thisMsg = MsgLine_findLineToFill(chId);

	if (NULL == thisMsg) // failed to find an available idle pend buffer, simply give up this receiving
		return;

 #define buf_i (thisMsg->offset)
 #define buf   (thisMsg->msg)

 	thisMsg->chId  = chId;
	thisMsg->timeout = (MAX_PENDMSGS<<2)+3; // refill the timeout as 4 times of the buffer

	for (; maxLen>0 && *inBuf; maxLen--)
	{
		buf[buf_i] = *inBuf++;

		// a \0 is known as the data end of this received frame
		if ('\0' == buf[buf_i])
			break;

		// an EOL would be known as a message end
		if ('\r' != buf[buf_i] && '\n' != buf[buf_i])
		{
			buf_i = (++buf_i) % (MAX_MSGLEN-3);
			continue;
		}

		// a line just received
		if (buf_i < MAX_MSGLEN -3) // re-fill in the line-end if the buffer is enough
		{ buf[buf_i++] = '\r'; buf[buf_i++] = '\n'; }
		
		buf[buf_i] = '\0'; // NULL terminate the string
		buf_i =0; // reset for next receiving

		if ('\0' == buf[0]) // empty line, ignore
			continue;

		// a valid line of message here
		// OnWirelessTextMsg(chip->nodeId, pipeId-1, (char*)buf);
		break;
	}
}

uint8 hexchval(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';

	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' +10;

	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' +10;

	return 0xff;
}

const char* hex2int(const char* hexstr, uint16* pVal)
{
	uint8 v =0;
	if (NULL == hexstr || NULL == pVal)
		return NULL;

	for(*pVal =0; *hexstr; hexstr++)
	{
		v = hexchval(*hexstr);
		if (v >0x0f) break;
		*pVal <<=4; *pVal += v;
	}

	return hexstr;
}

code const uint8_t crc8_table[] = {
	0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
uint8_t calcCRC8(const uint8_t* msg, uint8_t len)
{
	uint8_t crc8=0, i;
	for(i=0; i<len; i++)
		crc8 = crc8_table[crc8 ^ msg[i]];
	return crc8;
}
