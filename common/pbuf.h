#ifndef __HT_PBUF_H__
#define __HT_PBUF_H__

#include "htcomm.h"

#ifdef FreeRTOS
#  include "task.h"
#  include "queue.h"
#endif //FreeRTOS

typedef int16_t pbuf_size_t;

// -----------------------------
// DataStruct pbuf
// -----------------------------

enum {
	PBUF_TYPE_REFERENCE  =0x00, PBUF_TYPE_HEAP, PBUF_TYPE_POOL1, PBUF_TYPE_POOL2, 
	PBUF_TYPE_INVALID
};

#define PBUF_REF_SIZE          (32)
#define PBUF_MEMPOOL1_BSIZE    (32)
#define PBUF_MEMPOOL2_BSIZE   (256)

#if defined(FreeRTOS) && defined(configTOTAL_HEAP_SIZE)
#  define PBUF_WITH_HEAP
#  define heap_malloc             pvPortMalloc
#  define heap_free               vPortFree
#  define pbuf_malloc             pbuf_halloc  // map the malloc to allocating from heap
#  define PBUF_HEAP_SIZE            (20)
#  define PBUF_MEMPOOL1_SIZE        (0)
#  define PBUF_MEMPOOL2_SIZE        (0)
#else
#  define pbuf_malloc             pbuf_palloc  // map the malloc to allocating from pool
#  define PBUF_HEAP_SIZE            (0)
#  define PBUF_MEMPOOL1_SIZE      (100)
#  define PBUF_MEMPOOL2_SIZE        (8)
#endif // configTOTAL_HEAP_SIZE

#ifdef PBUF_WITH_HEAP
#  include "heap.h"
#endif // PBUF_WITH_HEAP

typedef struct _pbuf {
	// next pbuf in singly linked pbuf chain
	struct _pbuf* next;

	// pointer to the actual data in the buffer
	uint8_t* payload;

	// total length of this buffer and all next buffers in chain
	pbuf_size_t tot_len;

	// length of this buffer
	pbuf_size_t len;

	// misc flags, refers to PBUF_FLAG_XXXX
	uint8_t flags;

	// the reference count always equals the number of pointers that refer to this pbuf. 
	// This can be pointers from an application, the stack itself, or pbuf->next pointers from a chain.
	uint8_t ref;
} pbuf;

pbuf* pbuf_halloc(pbuf_size_t length);  // allocate from heap
pbuf* pbuf_palloc(pbuf_size_t length); // allocate from pool
pbuf* pbuf_mmap(void* buf, pbuf_size_t length);
pbuf* pbuf_dup(void* buf, pbuf_size_t length);

void    pbuf_ref(pbuf* p);
uint8_t pbuf_free(pbuf *p);

uint8_t pbuf_count(pbuf *p);
void pbuf_realloc(pbuf *p, pbuf_size_t new_len);

// Concatenate two pbufs (each may be a pbuf chain) and take over the caller's reference of the tail pbuf.
// @note The caller MAY NOT reference the tail pbuf afterwards, otherwise use pbuf_chain() for that purpose.
// @see pbuf_chain()
void pbuf_cat(pbuf* h, pbuf** t);

// Chain two pbufs (or pbuf chains) together.
// The caller MUST call pbuf_free(t) once it has stopped using it. Use pbuf_cat() instead if you
// no longer use t.
// @param h head pbuf (chain)
// @param t tail pbuf (chain)
// The ->tot_len fields of all pbufs of the head chain are adjusted.
// The ->next field of the last pbuf of the head chain is adjusted.
// The ->ref field of the first pbuf of the tail chain is adjusted.
void pbuf_chain(pbuf *h, pbuf *t);

// copy data into pbuf
// @param p pbuf to parse
// @param offset offset into p of the byte to return
// @return byte at an offset into p OR ZERO IF 'offset' >= p->tot_len
pbuf_size_t pbuf_write(pbuf* p, pbuf_size_t offset, const uint8_t* buf, pbuf_size_t n);

// copy data out of pbuf to buf
// @param p pbuf to parse
// @param offset offset into p of the byte to return
// @return byte at an offset into p OR ZERO IF 'offset' >= p->tot_len
pbuf_size_t pbuf_read(pbuf* p, pbuf_size_t offset, uint8_t* buf, pbuf_size_t n);

// Get one byte from the specified position in a pbuf
// WARNING: returns zero for offset >= p->tot_len
// @param p pbuf to parse
// @param offset offset into p of the byte to return
// @return byte at an offset into p OR ZERO IF 'offset' >= p->tot_len
uint8_t pbuf_get_at(pbuf* p, pbuf_size_t offset);

// Compare pbuf contents at specified offset with memory s2, both of length n
// @param p pbuf to compare
// @param offset offset into p at wich to start comparing
// @param s2 buffer to compare
// @param n length of buffer to compare
// @return zero if equal, nonzero otherwise
//         (0xffff if p is too short, diffoffset+1 otherwise)
pbuf_size_t pbuf_memcmp(pbuf* p, pbuf_size_t offset, const void* s2, pbuf_size_t n);

// Find occurrence of mem (with length mem_len) in pbuf p, starting at offset start_offset.
// @param p pbuf to search, maximum length is 0xFFFE since 0xFFFF is used as return value 'not found'
// @param mem search for the contents of this buffer
// @param mem_len length of 'mem'
// @param start_offset offset into p at which to start searching
// @return 0xFFFF if substr was not found in p or the index where it was found
pbuf_size_t pbuf_memfind(pbuf* p, const void* mem, uint16_t mem_len, pbuf_size_t start_offset);


// -----------------------------
// DataStruct FIFO
// -----------------------------
typedef struct _FIFO
{
#ifdef FreeRTOS
	QueueHandle_t iterations;
#else
	pbuf*    iterations;
	uint8_t  __IO__ header, tail;
#endif // FreeRTOS

	uint8_t  count, itLen;
	uint8_t  flags;
} FIFO;

#define FIFO_FLG_ALLOW_OVERWRITE (1<<0)
#define  FIFO_ready(fifo) (NULL != fifo.iterations)

uint8_t FIFO_init(FIFO* fifo, uint8_t count, uint8_t itLen, uint8_t flags);
hterr   FIFO_push(FIFO* fifo, void* data);
// void*   FIFO_reserveNext(FIFO* fifo);
hterr   FIFO_pop(FIFO* fifo, void* data);

uint8_t FIFO_awaitSize(FIFO* fifo);

// -----------------------------
// DataStruct DummyMap
// -----------------------------
typedef struct _DummyMap
{
	uint8_t* data;
	__IO__   uint8_t count;
	uint8_t  totcount;
	uint8_t  keysize, rowsize;
} DummyMap;

hterr DummyMap_init(DummyMap* map, uint8_t keysize, uint8_t datasize, uint8_t totalcount);
void  DummyMap_free(DummyMap* map);

hterr DummyMap_set(DummyMap* map,  const uint8_t* key, const uint8_t* value);
hterr DummyMap_get(DummyMap* map,  const uint8_t* key, uint8_t* value);
hterr DummyMap_erase(DummyMap* map,  const uint8_t* key);

// -----------------------------
// DataStruct LRUMap
// -----------------------------
typedef struct _LRUMap
{
	DummyMap k2vplus;
    __IO__ uint16_t stampLast;
} LRUMap;

hterr LRUMap_init(LRUMap* lru, uint8_t keysize, uint8_t datasize, uint8_t maxRecords);
void    LRUMap_free(LRUMap* lru);

//@return the idx where put the data
uint8_t LRUMap_set(LRUMap* lru, const uint8_t* key, const uint8_t* data);

//@return ERR_SUCCESS if success
hterr   LRUMap_get(LRUMap* lru, const uint8_t* key, uint8_t* data, BOOL bRefresh);
hterr   LRUMap_erase(LRUMap* lru, const uint8_t* key);

#endif // __HT_PBUF_H__
