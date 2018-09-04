#ifndef __HEAP_H__
#define __HEAP_H__

#define HEAP_SIZE_TOTAL          1000
#define HEAP_BYTE_ALIGNMENT			4
#define HEAP_ENTER_CRITICAL()		
#define HEAP_EXIT_CRITICAL()			
#define HEAP_TRACE(func, ret, size)
#define HEAP_ASSERT(VAR)

// #define size_t int

#ifndef NULL
#  define NULL (0)
#endif // NULL

#ifndef min
#define min(A, B)    ((A)<(B)?(A):(B))
#endif // min

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

// -----------------------------
// class heap
// -----------------------------
void* heap_malloc(size_t xWantedSize);
void heap_free(void *pBuf);

#endif // __HEAP_H__

