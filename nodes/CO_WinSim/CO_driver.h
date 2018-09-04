// ===========================================================================
// Copyright (c) 1997, 1998 by
// CO_OD.c - Variables and Object Dictionary for CANopenNode
//
// Ident : $Id: CO_Driver.c$
// Branch: $Name:  $
// Author: Hui Shao
// Desc  : For short description of standard Object Dictionary entries see CO_OD.txt
// processor: STM32F10x
//
// Revision History: 
// ---------------------------------------------------------------------------
// $Log: /ZQProjs/Common/CO_OD.c $
// ===========================================================================

#ifndef __CO_DRIVER_H__
#define __CO_DRIVER_H__

#include "ZQ_common_conf.h"

// -----------------------------
// Processor specific hardware macros
// -----------------------------
//Oscilator frequency for SJA1000
#define CO_OSCILATOR_FREQ     24 //(4, 8, 16, 20, 24, 32 or 40)

//base address of CAN controller
#define CO_ADDRESS_CAN        0x500

//LED to indicate CAN communication operational 
#define CO_Driver_InitLEDs()     {} // pfe_enable_pio(0, 5)
//LED to indicate error occured on CAN communication
#define CO_Driver_SetCommLED(i)  {} // hal_write_pio(0, i)
#define CO_Driver_SetErrLED(i)   {} // hal_write_pio(1, i)

// -----------------------------------------------
// CANopen basic Data Types
// -----------------------------------------------
#ifndef int8_t
#define uint8_t   unsigned char
#define uint16_t  unsigned short
#define uint32_t  unsigned int
#define uint64_t  unsigned long
#define int8_t    signed char
#define int16_t   short
#define int32_t   int
#define int64_t   long
#endif

//default qualifiers and types
#define CO_FAST_DataTypeSpec                 //on PIC18F local variables are faster if static
#define CO_DEFAULT_IntegerType uint16_t      //uint8_t for 8-bit microcontroller, for 16-bit use uint16_t
#define CO_DEFAULT_IntegerType_MaxV (0xffff)   //max value of the CO_DEFAULT_IntegerType 0xFF for 8-bit microcontroller, for 16-bit use 0xFFFF

// -----------------------------
// Processor specific software macros
// -----------------------------
//qualifier for variables located in read-only memory (C18 compiler put those variables in
//flash memory space (rom qualifier). If located in RAM, 'const' should be used instead of 'rom')
//ROM Variables in Object Dictionary MUST have attribute ATTR_ROM appended. If
//base attribute is ATTR_RW or ATTR_WO, variable can be changed via CAN network (with
//writing to flash memory space in PIC18Fxxx microcontroller)
#define ROM             // const

#define CO_MODIF_Timer1msIsr       //modifier for void CO_ISR_Timer1ms function (nothing for none)

void CO_Driver_Init(void);
void CO_Driver_Remove(void);
void CO_Driver_ReadNodeIdBitrate(void);  //determine NodeId and BitRate
void CO_Driver_SetupCAN(void);             //setup CAN controller
void CO_Driver_OnProcessMain(void);        //process microcontroller specific code
void CO_Driver_InitInterrupts(void);

// -----------------------------
// enable, disable int16_terrupts
// -----------------------------
//if (interrupt enable flag) is set in mainline and timer and cleared in isr
//disable/enable all int16_terrupts from mainline procedure
#define CO_Driver_DisableAllInterrupts()         {} //   asm cli
#define CO_Driver_EnableAllInterrupts()          {} //   asm sti

//disable/enable timer interrupt from mainline procedure
#define CO_Driver_DisableTimer()         {} //   asm cli
#define CO_Driver_EnableTimer()          {} //   asm sti

//disable/enable CANtx interrupt from mainline procedure
#define CO_Driver_DisableCANTX()       {} //   asm cli
#define CO_Driver_EnableCANTX()        {} //   asm sti

//disable/enable CANtx interrupt from timer procedure
#define CO_Driver_DisableCANTX_TMR()   {} //   asm cli
#define CO_Driver_EnableCANTX_TMR()    {} //   asm sti

//disable/enable CANrx interrupt from mainline procedure
#define CO_Driver_DisableCANRX()       {} //   asm cli
#define CO_Driver_EnableCANRX()        {} //   asm sti

//disable/enable CANrx interrupt from timer procedure
#define CO_Driver_DisableCANRX_TMR()   {} //   asm cli
#define CO_Driver_EnableCANRX_TMR()    {} //   asm sti

//Read Transmit Buffer Status
#define CO_Driver_isTxnBufferEmpty()    (1) // ((hal_inportb(CO_ADDRESS_CAN+2)&0x04) != 0)

// read interrupt flag register
#define CO_Driver_readRxnIntFlags()      (hal_inportb(CO_ADDRESS_CAN +3))
#define CO_Driver_readDLC()              (hal_inportb(CO_ADDRESS_CAN +16))

// transmit a CAN message to the device
void CO_Driver_Post2Txn(uint16_t INDEX);

#endif // __CO_DRIVER_H__
