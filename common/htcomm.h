#ifndef __HT_Common_H__
#define __HT_Common_H__

#include "types.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#if defined(HT_CLUSTER) && !defined(HT_DDL)
#  define HT_DDL
#endif // HT_DDL

enum {
	ERR_SUCCESS =0,
	ERR_NOT_ENOUGH_MEMORY,
	ERR_INVALID_PARAMETER,    // one or more parameters are invalid.
	ERR_NOT_FOUND,
	ERR_OPERATION_ABORTED,    // overlapped operation aborted.
	ERR_OVERFLOW,             // buffer overflow
	ERR_UNDERFLOW,            // buffer underflow
	ERR_IO,                   // Generic I/O error
	ERR_IO_INCOMPLETE,        // Overlapped I/O event object not in signaled state.
	ERR_IO_PENDING,           // Overlapped operations will complete later.
	ERR_INTR,                 // interrupted function call.
	ERR_ACCESS,               // permission denied.
	ERR_ADDRFAULT,            // bad address.
	ERR_IN_PROGRESS,          // operation now in progress.
	ERR_MSG_TOO_LONG,         // Message too long.
	ERR_NOT_SUPPORTED,        // operation not supported.
	ERR_NO_BUFS,              // no buffer space available.
	ERR_CRC,                  // CRC err
};

typedef int8_t hterr;

#ifndef __IO__
#  define __IO__ volatile
#endif // __IO__

#define FLAG(n) (1<<n)

#ifndef max
#  define max(A, B)         (((A)>(B))?(A):(B))
#endif
#ifndef min
#  define min(A, B)         (((A)<(B))?(A):(B))
#endif

// -----------------------------
// OS-specific definitions 
// -----------------------------
#define CRITICAL_ENTER()
#define CRITICAL_LEAVE()

volatile extern uint8_t _interruptDepth; //the depth of interrupts

#ifdef FreeRTOS // FreeRTOS style
#  include "FreeRTOS.h"
#  include "task.h"
#  undef  CRITICAL_ENTER
#  undef  CRITICAL_LEAVE
#  define CRITICAL_ENTER()	  vTaskSuspendAll()
#  define CRITICAL_LEAVE()	  xTaskResumeAll()
#  define getTickXmsec()            ((_interruptDepth? xTaskGetTickCountFromISR(): xTaskGetTickCount())*1000/configTICK_RATE_HZ)
#  define createTask(NAME, PPARAM)  xTaskCreate(Task_##NAME, #NAME, TaskStkSz_##NAME, PPARAM, TaskPrio_##NAME, NULL)
#  undef sleep
#  define sleep(msec)			    vTaskDelay(msec*configTICK_RATE_HZ/1000)
// Declaration of each task should include TaskPrio_##NAME, TaskStkSz_##NAME and Task_##NAME, such as:
//   #define TaskPrio_Main                (TaskPrio_Base +0)
//   #define TaskStkSz_Main               (configMINIMAL_STACK_SIZE)
//   void Task_Main(void* p_arg);
#  define runTaskScheduler()        vTaskStartScheduler()

#elif defined(UCOSII)
#  define getTickXmsec()            (OSTimeGet()*1000 /OS_TICKS_PER_SEC) // in msec
#  define sleep(msec)			    OSTimeDlyHMSM( msec/1000/60/60, msec/1000/60, msec /1000, msec %1000)
#endif // __CC_ARM

#ifdef WIN32
#  define snprintf _snprintf
#  define trace(fmt, ...)             printf("TRACE> " fmt "\r\n", __VA_ARGS__)
#else
#  define trace(fmt, ...)             // printf("TRACE> " fmt "\r\n", __VA_ARGS__)
#endif // WIN32

#ifndef sleep
#  define sleep delayXmsec
#endif // sleep

#ifndef getTickXmsec
#  define getTickXmsec() (gClock_msec)
#endif // sleep

#if 0

// -----------------------------
// memory
// -----------------------------
void memset(void *dest, char c, uint32_t len);
int  memcpy(void *dest, void *src, uint_t len);
int  memcmp(void *dst, void *src, uint_t len);

// -----------------------------
// string & character
// -----------------------------
int   strlen(const char *s);
void  strcpy(char *s1, const char *s2);
int   strcmp(const char *s1, const char *s2);
char* strcat(char *s1, const char *s2);
char* strstr(const char* src, const char* dst);  

long  atol(char *str);
uint8_t ltoa(uint_t val, char *buffer);

#endif // _STM32F10X

#ifdef _STM32F10X
char* strchar(const char *s, char c);	 // ???????
#endif // _STM32F10X

char* trimleft(const char *s);
char hexchar(const uint8_t n);
uint8_t hexchval(char ch);
const char* hex2int(const char* hexstr, uint32_t* pVal);
uint8_t hex2byte(const char* hexstr, uint8_t* pByte);

uint8_t calcCRC8(const uint8_t* databuf, uint8_t len);

// -----------------------------
// debug
// -----------------------------
#ifdef DEBUG
extern uint8_t log_level;

#pragma __printf_args
void log(uint8_t level, const char * __restrict /*format*/, ...);
#endif // DEBUG

// -----------------------------
// delay
// -----------------------------
void delayXusec(int16_t usec);
void delayXmsec(int16_t msec);

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

uint32_t CPU_id(void);
uint32_t CPU_typeId(void);

typedef struct _MsgLine
{
	uint8_t chId, offset, timeout;
	char msg[MAX_MSGLEN];
} MsgLine;

extern __IO__ MsgLine _pendingLines[MAX_PENDMSGS]; //  = { {} };
void MsgLine_init(void);
__IO__ MsgLine* MsgLine_allocate(uint8_t chId, uint8_t reservedTimeout);
void MsgLine_free(__IO__ MsgLine* pLine);
int  MsgLine_recv(uint8_t pipeId, uint8_t* inBuf, uint8_t maxLen);
void MsgLine_log(uint8_t ifChId, char * fmt, ...);

// -----------------------------
// MemoryRange
// -----------------------------
typedef struct _MemoryRange
{
	uint8_t*  addr;
	uint8_t   len;    // numbers of bytes
	uint8_t   flags;  // combination of MRFLG_XXX, lowest 2bits is reserved as the number of bytes of each iterator, see MRFLG_IteratorSize()
} MemoryRange;

#define MRFLG_ReadOnly       FLAG(2)
#define MRFLG_String         FLAG(3)
#define MRFLG_HexFmt         FLAG(4)

#define MRFLG_IteratorSize(_MR)  (1 << ((_MR).flags & 0x03))

typedef struct _KLV
{
	char*    key;
	uint32_t keyId;
	MemoryRange val;
} KLV;

// -----------------------------
// BlinkPair
// -----------------------------
// about blink an output
#define BLINK_INTERVAL_MSEC (50) // 50msec
typedef struct _BlinkPair
{
	const IO_PIN   pinA, pinB;
	__IO__ uint8_t  ctrl, blinks;
	__IO__ uint8_t  reloadA, reloadB;
	uint16_t  count;
} BlinkPair;

void BlinkPair_OnTick(BlinkPair* bp);

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

#ifndef BlinkList_MAX
#  define BlinkList_MAX (1)
#endif // BlinkPair_MAX

int  BlinkList_register(BlinkPair* bp);

// should be called with interval BLINK_INTERVAL_MSEC
void BlinkList_doTick(void);

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

#define EBYTE_INVALID_VALUE (0xff)
uint8_t value2eByte(uint16_t v);
uint16_t eByte2value(uint8_t e);

#define ADJUST_BY_RANGE(value, RANG_MIN, RANG_MAX) if (value > RANG_MAX) value = RANG_MAX; if (value <= RANG_MIN) value = RANG_MIN;
#define UINT_SUB(UINT_T, A, B)     ((UINT_T)(((UINT_T)A) + (~((UINT_T)B))))

// -----------------------------
// globle clock 
// -----------------------------
// gClock_msec, increased by gClock_step1msec(), is in the range [0, 604800000) that
// represents Sun 00:00::00.000 ~ Sat 23:59:59.999
extern __IO__ uint32_t gClock_msec; 

void     gClock_step1msec(void);           // should be call every 1msec
uint32_t gClock_calcExp(uint16_t timeout); // calculate the expiration
#define  gClock_getElapsed(stampSince)     UINT_SUB(uint32_t, gClock_msec, stampSince)

typedef struct _TCPPeer
{
	uint8_t  flags; // flags of PEER_FLG_xxxx
	uint32_t ip;
	uint16_t port;
} TCPPeer;

#define PEER_FLG_TYPE_UDP   FLAG(0)
#define U32_IP_Bx(u32IP, X)  (((u32IP) >> (8*(3-X))) &0xff)

#endif // __HT_Common_H__
