#ifndef __HT_TYPES_H__
#define __HT_TYPES_H__

#include <windows.h>
#include <stdlib.h>

#pragma warning (disable: 4800 4355 4786 4503 4275 4251 4290 4786 4996)

#ifndef BOOL
#  define BOOL  uint8_t
#endif // BOOL

#ifndef TRUE
#  define FALSE (0)
#  define TRUE  (1)
#  define BOOL  uint8_t
#endif // TRUE 

#ifndef max 
#  define max(_A, _B) ((_A>_B)?_A:_B)
#endif
#ifndef min 
#  define min(_A, _B) ((_A<_B)?_A:_B)
#endif

#define sleep(msec)			    Sleep(msec)
#define BYTES2WORD( _BYTE_HIGH, _BYTE_LOW) ((((uint16_t)_BYTE_HIGH) <<8) | (_BYTE_LOW &0xff))

typedef unsigned char        uint8_t;
typedef unsigned short       uint16_t;
typedef unsigned int         uint32_t;
typedef unsigned long long   uint64_t;
typedef char				 int8_t;
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

// -----------------------------
// IO_PIN
// -----------------------------
typedef void* IO_PIN;

#define isPin(_IoPin)   1
#define pinGET(_IoPin)  1
#define pinSET(_IoPin,H)

#define PulsePin_LOOPBACK_TEST

typedef void* IO_SPI;
#define SPI_RW(spi, addr) (0)

typedef void* IO_USART;
#define SPI_RW(spi, addr) (0)

#endif // __HT_TYPES_H__
