// FreeRTOS V8.1.2 - Copyright (C) 2014 Real Time Engineers Ltd. 
// All rights reserved
//
// VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.
//
// ***************************************************************************
//  *                                                                       *
//  *    FreeRTOS provides completely free yet professionally developed,    *
//  *    robust, strictly quality controlled, supported, and cross          *
//  *    platform software that has become a de facto standard.             *
//  *                                                                       *
//  *    Help yourself get started quickly and support the FreeRTOS         *
//  *    project by purchasing a FreeRTOS tutorial book, reference          *
//  *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
//  *                                                                       *
//  *    Thank you!                                                         *
//  *                                                                       *
// ***************************************************************************
//
// This file is part of the FreeRTOS distribution.
//
// FreeRTOS is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License (version 2) as published by the
// Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.
//
// >>!   NOTE: The modification to the GPL is included to allow you to     !<<
// >>!   distribute a combined work that includes FreeRTOS without being   !<<
// >>!   obliged to provide the source code for proprietary components     !<<
// >>!   outside of the FreeRTOS kernel.                                   !<<
//
// FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  Full license text is available from the following
// link: http://www.freertos.org/a00114.html
//
// 1 tab == 4 spaces!
//
// ***************************************************************************
//  *                                                                       *
//  *    Having a problem?  Start by reading the FAQ "My application does   *
//  *    not run, what could be wrong?"                                     *
//  *                                                                       *
//  *    http://www.FreeRTOS.org/FAQHelp.html                               *
//  *                                                                       *
// ***************************************************************************
//
// http://www.FreeRTOS.org - Documentation, books, training, latest versions,
// license and Real Time Engineers Ltd. contact details.
//
// http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
// including FreeRTOS+Trace - an indispensable productivity tool, a DOS
// compatible FAT file system, and our tiny thread aware UDP/IP stack.
//
// http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
// Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
// licenses offer ticketed support, indemnification and middleware.
//
// http://www.SafeRTOS.com - High Integrity Systems also provide a safety
// engineered and independently SIL3 certified version for use in safety and
// mission critical applications that require provable dependability.
//
// 1 tab == 4 spaces!

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// #include "includes.h"

// -----------------------------------------------------------
// * Application specific definitions.
// *
// * These definitions should be adjusted for your particular hardware and
// * application requirements.
// *
// * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
// * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE. 
// *
// * See http://www.freertos.org/a00110.html.
// -----------------------------------------------------------

#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK			0
#define configUSE_TICK_HOOK			0
#define configCPU_CLOCK_HZ			( ( unsigned long ) 72000000 )	
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES		( 5 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_TRACE_FACILITY	0
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1

// STM32F103xExx:64KB RAM(0x2000000)
// STM32F103xDxx:64KB RAM(0x2000000)
// STM32F103xCxx:48KB RAM(0x200C000)
// STM32F103xBxx:20KB RAM(0x2005000)
// STM32F103x8xx:20KB RAM(0x2005000)
// STM32F103x6xx:10KB RAM(0x2002800)
#ifdef  STM32F10X_MD
#  define configTOTAL_HEAP_SIZE		( ( size_t ) ( 11 * 1024 ) )
#else
#  define configTOTAL_HEAP_SIZE		( ( size_t ) ( 17 * 1024 ) )
#endif // 

// Co-routine definitions.
#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

// Set the following definitions to 1 to include the API function, or zero
// to exclude the API function.
#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1

// This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
// (lowest) to 0 (1?) (highest).
#define configKERNEL_INTERRUPT_PRIORITY 		255
// !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
// see http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191 // equivalent to 0xb0, or priority 11.


// This is the value being used as per the ST library which permits 16
// priority values, 0 to 15.  This must correspond to the
// configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
// NVIC value of 255.
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

// #define uIRQ_WWDG               ISR_funcXXX
// #define uIRQ_PVD                ISR_funcXXX
// #define uIRQ_TAMPER             ISR_funcXXX
// #define uIRQ_RTC                ISR_funcXXX
// #define uIRQ_FLASH              ISR_funcXXX
// #define uIRQ_RCC                ISR_funcXXX
// #define uIRQ_EXTI0              ISR_funcXXX
// #define uIRQ_EXTI1              ISR_funcXXX
// #define uIRQ_EXTI2              ISR_funcXXX
// #define uIRQ_EXTI3              ISR_funcXXX
// #define uIRQ_EXTI4              ISR_funcXXX
#define uIRQ_DMAChannel1        ISR_ADC1DMA
// #define uIRQ_DMAChannel2        ISR_funcXXX
// #define uIRQ_DMAChannel3        ISR_funcXXX
// #define uIRQ_DMAChannel4        ISR_funcXXX
// #define uIRQ_DMAChannel5        ISR_funcXXX
// #define uIRQ_DMAChannel6        ISR_funcXXX
// #define uIRQ_DMAChannel7        ISR_funcXXX
// #define uIRQ_ADC                ISR_funcXXX
// #define uIRQ_USB_HP_CAN_TX      ISR_funcXXX
// #define uIRQ_USB_LP_CAN_RX0     ISR_funcXXX
// #define uIRQ_CAN_RX1            ISR_funcXXX
// #define uIRQ_CAN_SCE            ISR_funcXXX
// #define uIRQ_EXTI9_5            ISR_funcXXX
// #define uIRQ_TIM1_BRK           ISR_funcXXX
// #define uIRQ_TIM1_UP            ISR_funcXXX
// #define uIRQ_TIM1_TRG_COM       ISR_funcXXX
// #define uIRQ_TIM1_CC            ISR_funcXXX
#define uIRQ_TIM2               ISR_OnTimer_1msec
// #define uIRQ_TIM3               ISR_funcXXX
// #define uIRQ_TIM4               ISR_funcXXX
// #define uIRQ_I2C1_EV            ISR_funcXXX
// #define uIRQ_I2C1_ER            ISR_funcXXX
// #define uIRQ_I2C2_EV            ISR_funcXXX
// #define uIRQ_I2C2_ER            ISR_funcXXX
// #define uIRQ_SPI1               ISR_funcXXX
// #define uIRQ_SPI2               ISR_funcXXX
#define uIRQ_USART1             ISR_RS232
// #define uIRQ_USART2             ISR_funcXXX
#define uIRQ_USART3             ISR_RS485
// #define uIRQ_EXTI15_10          ISR_funcXXX
// #define uIRQ_RTCAlarm           ISR_funcXXX
// #define uIRQ_USBWakeUp          ISR_funcXXX

//------------------------------
// ISRs in isr_secu.c
//------------------------------
void ISR_OnTimer_1msec(void);
void ISR_RS232(void);
void ISR_RS485(void);
void ISR_ADC1DMA(void);

#endif // FREERTOS_CONFIG_H

