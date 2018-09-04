// Generic platform configuration file

#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

#include "buf.h"
#include "sermux.h"
#include "legc.h"
#include "platform.h"
// #include "auxmods.h"
#include "lualib.h"

//#define BUILD_XMODEM
//#define BUILD_SHELL
#define BUILD_ROMFS
//#define BUILD_MMCFS
#define BUILD_TERM
//#define BUILD_UIP
//#define BUILD_DHCPC
//#define BUILD_DNS
#define BUILD_CON_GENERIC
#define BUILD_ADC
#define BUILD_RPC

#define CON_FLOW_TYPE	PLATFORM_UART_FLOW_NONE
#define TERMLINES	PLATFORM_UART_FLOW_NONE

// FIXME: ASF ill behaved - compiler.h does not check for previous definition of "UNDEFINED before defining it.  
//  It is also defined in lua source (but there it checks to be sure not defined).
//  Moving include of ELUA_CPU_HEADER before lua things solves that problem, but maybe there is another way?
#include "cpu_stm32f103re.h"
// #include ELUA_BOARD_HEADER
#include "platform_generic.h"     // generic platform header (include whatever else is missing here)

#define MEM_START_ADDRESS { (u32) INTERNAL_RAM1_FIRST_FREE }
#define MEM_END_ADDRESS   { (u32) INTERNAL_RAM1_LAST_FREE }

#endif

