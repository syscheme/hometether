#ifndef __defines_h__
#define __defines_h__

// #include "bsp.h"

#ifdef STC15W204AS
#define FoscHz  (11059200L)
#define Fosc_1T
#endif // STC types


#ifdef __STC15F2K60S2_H_
#define STC_1T
#endif // __STC15F2K60S2_H_

#ifdef  STC_1T
#  define Fosc_1T
#  define SMOD_1
#endif // STC12C5AXX

#ifndef FoscMhz
#define FoscMhz  12 // the default Fosc
#endif

#include <intrins.h>

#define BYTE unsigned char
#define WORD unsigned int
#define NULL 0x00

#define uchar 	unsigned char 
#define uint8 	unsigned char 
#define uint16 	unsigned int  
#define uint 	unsigned int  
#define int8 	         char 
#define int16 	         int  
#define ulong 	unsigned long
#define bool    uint8
#define false   ((uint8) 0)
#define true    ((uint8) 1)

#define uint8_t      uint8
#define uint16_t     uint16
#define int8_t       int8
#define int16_t      int16
#define FALSE        false
#define TRUE         true

#define UINT8X  unsigned char xdata
#define UINT8   unsigned char
#define UINT16X unsigned int xdata
#define UINT16  unsigned int
#define UINT32X unsgined long xdata
#define UINT32  unsigned long

#define INT8X   char xdata
#define INT8    char 
#define INT16X  int  xdata
#define INT16   int
#define INT32X  long xdata
#define INT32   long

// typedef sbit IO_PIN;

// -----------------------------
// delay utilities
// -----------------------------
void delayIOSet() reentrant;
void delayX10us(char n) reentrant;
void delayXms(uint n); // (n, error%) are (1,-1%), (2,-0.35%) (3,0.03%) (4,0.05%) (5,0.04%)...(10,0.17%)...(20,0.24%)...(100, 0.287%)...(INFINITE, 0.3%)

// -----------------------------
// MCU utilities
// -----------------------------
void MCU_softReset();
void MCU_softResetToISP();

#define min(_X, _Y) (_X < _Y ? _X : _Y)
#define max(_V1, _V2) (_V1>_V2 ?_V1 :_V2)

#define STATE_Green_Ease       0
//#define STATE_Blue_Lauch     1
#define STATE_Yellow_Caution   1
#define STATE_Orange_Warning   2
#define STATE_Red_Alert        3

#define FLAG_bglight           (1<<3)
#define FLAG_numpad            (1<<4)
#define FLAG_beep              (1<<5)

extern data uchar MCU_state;

#ifdef TIMER_BY_INT
#  define TIMEOUT_touchpad       1000
#else
#  define TIMEOUT_touchpad       64000
#endif

#ifdef Fosc_1T
#   define T1MS (65536 - FoscMhz*1000)
#else // assume the MCU takes 12T per instruction
#   define T1MS (65536 - FoscMhz*1000/12)
#endif // Fosc_T

#define SMOD_1

// -----------------------------
// UART operations, UART1
// -----------------------------
void UART_485Dir(bit sendMode);

uint  UART_init(uint baud);
uchar UART_readbyte(void);
void  UART_writebyte(uchar value);
uint  UART_readword(void);
void  UART_writeword(uint value);

// -----------------------------
// TIMER0 operations
// -----------------------------
void Timer0_reload(void);
void Timer0_init(uint Xmsec);

// -----------------------------
// FIFO operations
// -----------------------------
typedef struct _FIFO {
	uint8* buf;
	uint8 header, tail;
	uint8 size;
} FIFO_t;

bool FIFO_init(FIFO_t* fifo, uint8 buf[], uint8 size);
bool FIFO_tryWrite(FIFO_t* fifo, uint8 dat);
bool FIFO_tryRead(FIFO_t* fifo, uint8* dat);
bool FIFO_write(FIFO_t* fifo, uint8 dat);

// -----------------------------
// processing about message line that ends with \r or \n
// \0 will be taken as a intermedia terminator of one transmision
// -----------------------------
#ifndef MAX_MSGLEN
#define MAX_MSGLEN     60
#endif

#ifndef MAX_PENDMSGS
#define MAX_PENDMSGS   (4)
#endif

typedef struct _MsgLine
{
	uint8 chId, offset, timeout;
	char msg[MAX_MSGLEN];
} MsgLine;

extern volatile MsgLine xdata _pendingLines[MAX_PENDMSGS]; //  = { {} };

volatile MsgLine* MsgLine_findLineToFill(uint8 chId) reentrant;
void MsgLine_recv(uint8 chId, uint8* inBuf, uint8 maxLen);

const char* hex2int(const char* hexstr, uint16* pVal);

uint8_t calcCRC8(const uint8_t* msg, uint8_t len);


#endif // __defines_h__
