#ifndef __BSP_H__
#define __BSP_H__

#if STC_CLASS==89
#else
#endif // STC_CLASS

#include "STC12C5AXX.h"
#include "defines.h"
#include <intrins.h>

#define UART_485

#ifdef STC15W204AS
// -------------------------------------------------------------------------------
// STC15W204AS pin connections
// -------------------------------------------------------------------------------
sbit VALVE2to1A       = P1^0;   // the relay to control the 2-to-1 valve, in4/pin15 of L293D
sbit VALVE2to1B       = P1^1;   // the relay to control the 2-to-1 valve, in3/pin10 of L293D

sbit InUse_any        = P1^0;	// pin3-ADC0 the main flow sensor, should be set to high-impedance state, see P0M1/P0M2
sbit InUse_veranda    = P1^1;	// pin4- should be removed: the flow sensor on the pipe to verandas, should be set to high-impedance state, see P0M1/P0M2

sbit RELAY_Pumper     = ??P1^4;   // pin28 the relay to control the pumper, L-active
sbit RELAY_Other      = ??P1^5;   // pin27 the relay to control other, L-active

sbit LED_sig          = P5^5;   // pin13, the signal led
#else
// this water control takes Baud=9600 with 
// -------------------------------------------------------------------------------
// MCU pin connections
// -------------------------------------------------------------------------------
sbit VALVE2to1A       = P1^0;   // the relay to control the 2-to-1 valve, in4/pin15 of L293D
sbit VALVE2to1B       = P1^1;   // the relay to control the 2-to-1 valve, in3/pin10 of L293D

sbit InUse_any        = P0^1;	// the main flow sensor, should be set to high-impedance state, see P0M1/P0M2
sbit InUse_veranda    = P0^2;	// the flow sensor on the pipe to verandas, should be set to high-impedance state, see P0M1/P0M2

sbit RELAY_Pumper     = P1^4;   // the relay to control the pumper, L-active
sbit RELAY_Other      = P1^5;   // the relay to control other, L-active

sbit LED_sig          = P3^5;   // the signal led
#endif

sbit MAX485_DE 		  = P3^7;   // the pin connects to max485's DE and RE/

#define P_DS18B20           P0  // the io port where those DS18B20 chips connect to 
#define P_DS18B20_BASE_PIN  3   // the first pin of P_DS18B20 where those DS18B20s connect to 



#endif // __BSP_H__