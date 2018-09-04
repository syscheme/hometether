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

#include  <stdio.h>
#include  <string.h>
#include  <ctype.h>
#include  <stdlib.h>
#include  <stdarg.h>

#define TaskPrio_Base		         ( tskIDLE_PRIORITY + 2 ) // the application-wide lowest priority
#define TaskPrio_Main                (TaskPrio_Base +0)
#define TaskPrio_Timer               (TaskPrio_Base +1)
#define TaskPrio_CO                  (TaskPrio_Base +2)
#define TaskPrio_Communication       (TaskPrio_Base +3)

#define TaskStkSz_Main               (configMINIMAL_STACK_SIZE) //96
#define TaskStkSz_Timer              (configMINIMAL_STACK_SIZE)
#define TaskStkSz_Communication      (configMINIMAL_STACK_SIZE+100)
#define TaskStkSz_CO                 (configMINIMAL_STACK_SIZE+100)	// 160

enum {
	ODID_data1 = ODID_USER_MIN, ODID_data2, ODID_data2x
};


#endif	// __INCLUDES_H__

