/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                          (c) Copyright 2003-2006; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************

*********************************************************************************************************
*/

#ifndef  __INCLUDES_H__
#define  __INCLUDES_H__

#if defined(STM32F10X_HD) || defined(STM32F10X_MD) || defined(STM32F10X_SD)
#  define CHIP_STM32F10X
#endif

#ifdef CHIP_STM32F10X
#include "common.h"
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#define true  TRUE
#define false FALSE
#else
typedef char           int8_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef int            int32_t;
#endif // CHIP_STM32F10X

#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <stdarg.h>

#ifdef WITH_UCOSII

#include  <uCOS-II/Source/ucos_ii.h>
#include  <uC-CPU/ARM-Cortex-M3/cpu.h>
#include  <uC-LIB/lib_def.h>
#include  <uC-LIB/lib_mem.h>
#include  <uC-LIB/lib_str.h>

#endif // WITH_UCOSII

#ifdef CHIP_STM32F10X
#  include  "bsp.h"
// #include  "usart.h"
#endif

enum
{
	CanBaud_10Kbps,
	CanBaud_20Kbps,
	CanBaud_50Kbps,
	CanBaud_100Kbps,
	CanBaud_125Kbps,
	CanBaud_250Kbps,
	CanBaud_500Kbps,
	CanBaud_800Kpbs,
	CanBaud_1Mbps
};

uint8_t CAN_setBaud(uint8_t baud);

#define NODE_ID (0x20)

#endif // __INCLUDES_H__

