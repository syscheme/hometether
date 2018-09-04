#ifndef __STM32_Common_h__
#define __STM32_Common_h__

#error here
#if defined(STM32F10X_HD) || defined(STM32F10X_MD) || defined(STM32F10X_SD)
#  define _STM32F10X
#endif

#ifdef _STM32F10X
#include "stm32f10x.h"
#include "stm32f10x_flash.h"
#include "types.h"

// #include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#define SystemFrequency (72000)	  //TODO need to correct

#if 0
// -----------------------------
// memory
// -----------------------------
void memset(void *dest, char c, uint32_t len);
int  memcpy(void *dest, void *src, unsigned int len);
int  memcmp(void *dst, void *src, unsigned int len);

// -----------------------------
// string & character
// -----------------------------
int  strlen(const char *s);
void strcpy(char *s1, const char *s2);
int  strcmp(const char *s1, const char *s2);
char* strcat(char *s1, const char *s2);
long atol(char *str);
char* strstr(const char* src, const char* dst);  
#endif // 0

char* trimleft(const char *s);
char* strchar(const char *s, char c);	 // ???????

unsigned char ltoa(unsigned int val, char *buffer);

#endif // _STM32F10X
/*
typedef unsigned char    uint8_t;
typedef unsigned short   uint16_t;
typedef unsigned int     uint32_t;
typedef unsigned long     uint64_t;
typedef char             int8_t;
typedef short            int16_t;
typedef int              int32_t;
typedef long        int64_t;

#define FALSE (0)
#define TRUE  (1)

#define bool  uint8_t
#define NULL (0)
*/

char hexchar(const uint8_t n);
uint8_t hexchval(char ch);
const char* hex2int(const char* hexstr, uint32_t* pVal);

uint8_t calcCRC8(const uint8_t* databuf, uint8_t len);


/*
char *str_char(char *s, char c);
int is_ascii(char c);
void get_quotient(unsigned int dividend, unsigned int divisor, char * buf, unsigned char num);
unsigned char calculate_xor_val(unsigned char * addr, unsigned int len);
*/
// -----------------------------
// debug
// -----------------------------
#define DEBUG
#ifdef DEBUG
extern uint8_t log_level;

#pragma __printf_args
void log(uint8_t level, const char * __restrict /*format*/, ...);
#endif // DEBUG

// -----------------------------
// delay
// -----------------------------
void delayXusec(int32_t usec);
void delayXmsec(int32_t msec);

// -----------------------------
// processing about message line that ends with \r or \n
// \0 will be taken as a intermedia terminator of one transmision
// -----------------------------
#ifndef MAX_MSGLEN
#define MAX_MSGLEN     80
#endif

#ifndef MAX_PENDMSGS
#define MAX_PENDMSGS   (2)
#endif

uint32_t STM32x_get_device_id(void);

typedef struct _MsgLine
{
	uint8_t chId, offset, timeout;
	char msg[MAX_MSGLEN];
} MsgLine;

extern volatile MsgLine _pendingLines[MAX_PENDMSGS]; //  = { {} };
void MsgLine_init(void);
volatile MsgLine* MsgLine_allocate(uint8_t chId, uint8_t reservedTimeout);
void MsgLine_free(volatile MsgLine* pLine);
int  MsgLine_recv(uint8_t pipeId, uint8_t* inBuf, uint8_t maxLen);
void MsgLine_log(uint8_t ifChId, char * fmt, ...);

// -----------------------------
// BlinkPair
// -----------------------------
// about blink an output
#ifndef BlinkPair_MAX
#  define BlinkPair_MAX (1)
#endif // BlinkPair_MAX
typedef struct _BlinkPair
{
	IO_PIN   pinA, pinB;
	volatile uint8_t  ctrl, blinks;
	volatile uint8_t  reloadA, reloadB;
	uint8_t  count;
} BlinkPair;

int  BlinkPair_register(BlinkPair* bp);
void BlinkPair_OnTick(void);

#define BlinkPair_FLAG_A_ENABLE    (1<<0)
#define BlinkPair_FLAG_A_ON_VAL    (1<<1)
#define BlinkPair_FLAG_B_ENABLE    (1<<2)
#define BlinkPair_FLAG_B_ON_VAL    (1<<3)
//@param mode - 4bit mode
// bit 3        2        1        0 
//     +--------+--------+--------+--------+
//     |B-on-val|enableB |A-on-val|enableA |
//     +--------+--------+--------+--------+
//  X-on-val: 1- set pin to 1 to turn on the led, otherwise to set to '0' to turn on the led
//@param reloadA, reloadB in 50msec
//@param blinks - the time of blinks, 0-means INFINIT
void BlinkPair_set(BlinkPair* bp, uint8_t mode, uint8_t reloadA, uint8_t reloadB, uint8_t blinks);
// void Blink_configByUri(BlinkTick* pPT, char* msg);

// -----------------------------
// timer array
// -----------------------------
// Usage:
//    1) call Timer_init(void) to initialiaze TIMER_MAX_NUM timers, and call the bsp func to setup the timer every 1msec
//    2) make the 1msec interrupt ISR_OnTick() to call Timer_scan(), that latter will decrease the counter of each live timer
//    3) make your loop to call Timer_exec(), the latter will check if the counter of any timer has been reached, execute it if so
//    4) call Timer_start() with timervalue and callback to employ a timer to wake up the callback in the given msec
//    5) call Timer_stop() to disable an active timer
#define TIMER_MAX_NUM          		3
typedef void (*func_OnTimer)(void* pCtx);

void Timer_init(void);
void Timer_start(uint8_t n, uint32_t msec, func_OnTimer func, void* pCtx);
void Timer_stop(uint8_t n);

void Timer_OnTick(void);
void Timer_scan(void);
uint32_t Timer_isStepping(uint8_t n);

uint8_t value2eByte(uint16_t v);
unsigned int eByte2value(uint8_t e);

#endif // __STM32_Common_h__
