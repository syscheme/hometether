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

#define MAX_MSGLEN 100
#define MAX_PENDMSGS (5)

#include  "htcomm.h"

#ifdef _STM32F10X
#  include  "stm32f10x_conf.h"
#  include  "stm32f10x.h"
#endif // _STM32F10X

#include  "bsp.h"
#include  "htcluster.h"
#include  "textmsg.h"

#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <stdarg.h>

#define TaskPrio_Base		         (tskIDLE_PRIORITY + 2 ) // the application-wide lowest priority
#define TaskPrio_Main                (TaskPrio_Base +0)
#define TaskPrio_Exec                (TaskPrio_Base +1)
#define TaskPrio_Communication       (TaskPrio_Base +3)
#define TaskPrio_Capture             (TaskPrio_Base +5)

#define TaskStkSz_Main               (configMINIMAL_STACK_SIZE +64) //96
#define TaskStkSz_Exec               (configMINIMAL_STACK_SIZE)
#define TaskStkSz_Communication      (configMINIMAL_STACK_SIZE)
#define TaskStkSz_Capture            (configMINIMAL_STACK_SIZE)	// 160

//------------------------------
// Thread Tasks	in task_water.c
//------------------------------
void Task_Main(void* p_arg);

enum {
	ODID_data1 = ODID_USER_MIN, ODID_data2
};

//------------------------------
// functions about water2.c
//------------------------------
#define NEXT_SLEEP_MIN               (100)   // 0.1sec
#define NEXT_SLEEP_MAX               (1000)  // 1sec

typedef enum _loopMode {
	LoopMode_IDLE0,
	LoopMode_FULL,
	LoopMode_NORMAL,
	LoopMode_IDLE =0x3
} LoopMode;

extern __IO__ uint8_t   byteStatus;

#define DEFAULT_Timeout_RF        (30*60)  // 30min, the reload value of timeoutRF
extern __IO__ uint16_t  timeoutRF;

void    Water_populate_DS18B20s(void);
void    Water_enablePumper(uint8_t enable);
uint8_t Water_isPumping(void);
void    Water_setLoopMode(LoopMode mode);

void     Water_init(void);
uint16_t Water_do1round(void);

#endif	// __INCLUDES_H__

