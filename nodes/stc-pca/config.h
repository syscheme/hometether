
#ifndef		__CONFIG_H
#define		__CONFIG_H

/*********************************************************/

//#define MAIN_Fosc		22118400L	//定义主时钟
// #define MAIN_Fosc		12000000L	//定义主时钟
#define MAIN_Fosc		11059200L	//定义主时钟
//#define MAIN_Fosc		 5529600L	//定义主时钟
//#define MAIN_Fosc		24000000L	//定义主时钟

#define	COM_TX1_Length	60
#define	COM_RX1_Length	60
#define CCP_NUM 2

// #define RECOGANIZE_HT_FRAME
// #define USART1_NO_RX
#define USART1_TEXT_MSG
#define RECOGANIZE_HT_FRAME

#define TIMEOUT_IDLE_RELOAD 1000

#define PULSE_CH_FRAME_MAXLEN	    (31) // (31) // must be less than 32
#define PULSE_CH_FIFO_MAXLEN	    (20)
#define PULSE_CH_LOOP_MAXBITS       (24)

/*********************************************************/

// #include <STC/STC15F2K60S2.H>
#include <STC15Fxxxx.H>
#include <intrins.h>

typedef unsigned char  bool;
typedef unsigned long  uint32_t;

#define uint16 u16
#define int16 short
#define uint8 u8

#define uint16_t  uint16
#define int16_t   int16
#define uint8_t   uint8
#define NULL      (0)

#endif
