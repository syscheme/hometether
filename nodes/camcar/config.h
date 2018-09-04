
/*---------------------------------------------------------------------*/
/* --- STC MCU International Limited ----------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.GXWMCU.com --------------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了宏晶科技的资料及程序   */
/*---------------------------------------------------------------------*/


#ifndef		__CONFIG_H
#define		__CONFIG_H

/*********************************************************/

//#define MAIN_Fosc		22118400L	//定义主时钟
// #define MAIN_Fosc		12000000L	//定义主时钟
#define MAIN_Fosc		11059200L	//定义主时钟
//#define MAIN_Fosc		 5529600L	//定义主时钟
//#define MAIN_Fosc		24000000L	//定义主时钟


#define	COM_TX1_Lenth	60
#define	COM_RX1_Lenth	32

/*********************************************************/

// #include <STC/STC15F2K60S2.H>
#include <STC15Fxxxx.H>
#include <intrins.h>

typedef     unsigned char  bool;

#define uint16 u16
#define int16 short
#define uint8 u8

#define uint16_t  uint16
#define int16_t   int16
#define uint8_t   uint8
#define NULL (0)

#endif
