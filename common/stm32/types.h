#ifndef __HT_TYPES_H__
#define __HT_TYPES_H__

#if !defined(STM32F10X_HD) && defined(STM32F10X_MD) && defined(STM32F10X_SD)
#  define STM32F10X_SD	// default take SD
#endif

#include "stm32f10x_conf.h"

#define _STM32F10X

#undef SYSCLK_FREQ_HSE

#ifdef HSE_FREQ_8MHz
#  define SYSCLK_FREQ_HSE   (8*1000000*9)
#endif // HSE_FREQ_8MHz

#ifdef HSE_FREQ_4MHz
#  define SYSCLK_FREQ_HSE   (4*1000000*9)
#endif // HSE_FREQ_4MHz

#define IRQ_DISABLE()     __disable_irq()
#define IRQ_ENABLE()      __enable_irq()

#include "stm32f10x.h"
#include "stm32f10x_flash.h"
#include <stdint.h>

#define __IO__ volatile

#ifndef BOOL
#  define BOOL  uint8_t
#endif // BOOL

#ifndef TRUE
#  define FALSE (0)
#  define TRUE  (1)
#  define BOOL  uint8_t
#endif // TRUE 

#ifndef false
#  define false FALSE
#  define true  TRUE
#endif // false

#ifndef max 
#  define max(_A, _B) ((_A>_B)?_A:_B)
#endif
#ifndef min 
#  define min(_A, _B) ((_A<_B)?_A:_B)
#endif

#define BYTES2WORD( _BYTE_HIGH, _BYTE_LOW) ((((uint16_t)_BYTE_HIGH) <<8) | (_BYTE_LOW &0xff))

typedef unsigned char        uint8_t;
typedef unsigned short       uint16_t;
typedef unsigned int         uint32_t;
typedef unsigned int         uint_t;
typedef unsigned long long   uint64_t;
typedef short                int16_t;
typedef int                  int32_t;
typedef long long            int64_t;
// #endif // uint8_t

#ifndef __cplusplus
#  define bool  uint8_t
#  ifndef NULL
#    define NULL  (0)
#  endif // NULL
#endif // cplusplus

// ==============================
// IO types
// ==============================

// -----------------------------
// 1) IO_PIN
// -----------------------------
typedef struct _IO_PIN
{
	GPIO_TypeDef* port;
	uint16_t      pin;	
} IO_PIN;

#define PIN_DECL(PORT, PIN) {PORT, PIN}
#define isPin(_IoPin)    ((NULL !=_IoPin.port) && (0 !=_IoPin.pin))
#define pinGET(_IoPin)   (GPIO_ReadInputDataBit(_IoPin.port, _IoPin.pin)?1:0)
#define pinSET(_IoPin,H) GPIO_WriteBit(_IoPin.port, _IoPin.pin, (H)?Bit_SET:Bit_RESET)

// -----------------------------
// 2) IO_SPI
// -----------------------------
typedef SPI_TypeDef* IO_SPI;
uint8_t SPI_RW(IO_SPI spi, uint8_t data);

typedef USART_TypeDef* IO_USART;
#define IO_USART_putc(USART_IO, ch) USART_SendData(USART_IO, (uint8_t) ch)
#define IO_USART_getc(USART_IO) USART_ReceiveData(USART_IO)

// -----------------------------
// utilties
// -----------------------------
uint16_t STM32TemperatureADC2dC(uint16_t tempADC);

#endif // __HT_TYPES_H__
