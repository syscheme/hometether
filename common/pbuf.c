#include "pbuf.h"

#include <string.h>

#ifndef CRITICAL_ENTER
#  define PBUF_CRITICAL_ENTER()
#  define PBUF_CRITICAL_LEAVE()
#else
#  define PBUF_CRITICAL_ENTER()	  CRITICAL_ENTER()
#  define PBUF_CRITICAL_LEAVE()   CRITICAL_LEAVE()
#endif

// -----------------------------
// DataStruct pbuf
// -----------------------------
#define TYPE_START_IDX_REF   (4)
#define TYPE_START_IDX_HEAP  (TYPE_START_IDX_REF   +PBUF_REF_SIZE)
#define TYPE_START_IDX_POOL1 (TYPE_START_IDX_HEAP  +PBUF_HEAP_SIZE)
#define TYPE_START_IDX_POOL2 (TYPE_START_IDX_POOL1 +PBUF_MEMPOOL1_SIZE)
#define PBUF_SIZE_TOTAL      (TYPE_START_IDX_POOL2 +PBUF_MEMPOOL2_SIZE)

static pbuf pbuf_pool[PBUF_SIZE_TOTAL] = {  // the pbuf control pool
	{ pbuf_pool+ PBUF_SIZE_TOTAL, NULL, 0xffff,0xff,0xff},
	{ pbuf_pool+ PBUF_SIZE_TOTAL, NULL, 0xffff,0xff,0xff},
	{ pbuf_pool+ PBUF_SIZE_TOTAL, NULL, 0xffff,0xff,0xff},
	{ pbuf_pool+ PBUF_SIZE_TOTAL, NULL, 0xffff,0xff,0xff}, };

#define _freePbuf_REF  (pbuf_pool[0])
#define _freePbuf_HEAP (pbuf_pool[1])
#define _freePbuf_P1   (pbuf_pool[2])
#define _freePbuf_P2   (pbuf_pool[3])

#define IDX2PBUF(IDX)  ((IDX >=TYPE_START_IDX_REF && IDX <PBUF_SIZE_TOTAL) ? (pbuf_pool +IDX) : NULL)

#if PBUF_MEMPOOL1_SIZE >0
static uint8_t pbuf_pool1[PBUF_MEMPOOL1_BSIZE *PBUF_MEMPOOL1_SIZE]; // the pbuf mem pool L1
#endif // PBUF_MEMPOOL1_BSIZE

#if PBUF_MEMPOOL2_SIZE >0
static uint8_t pbuf_pool2[PBUF_MEMPOOL2_BSIZE *PBUF_MEMPOOL2_SIZE]; // the pbuf mem pool L2
#endif // PBUF_MEMPOOL2_SIZE

static uint8_t pbuf_type_priv(pbuf* pb)
{
//	*idxOfType =0;
	if (pb <pbuf_pool +TYPE_START_IDX_REF || pb >= (pbuf_pool + PBUF_SIZE_TOTAL))
		return PBUF_TYPE_INVALID;

//	*idxOfType = (uint8_t)(pb - pbuf_pool);
	if (pb >= pbuf_pool +TYPE_START_IDX_POOL2)
		return PBUF_TYPE_POOL2;

	if (pb >= pbuf_pool +TYPE_START_IDX_POOL1)
		return PBUF_TYPE_POOL1;

	if (pb >= pbuf_pool +TYPE_START_IDX_HEAP)
		return PBUF_TYPE_HEAP;

	return PBUF_TYPE_REFERENCE;
}

static void pbuf_init(void)
{
	pbuf_size_t i=0;
	if (_freePbuf_REF.next < pbuf_pool+ PBUF_SIZE_TOTAL) // must have already been initialized
		return;

	// initialize the pool
	PBUF_CRITICAL_ENTER();

	memset(pbuf_pool, 0x00, sizeof(pbuf_pool));
	for (i=TYPE_START_IDX_REF; i<PBUF_SIZE_TOTAL-1; i++)
		pbuf_pool[i].next = pbuf_pool +i+1;

#if PBUF_REF_SIZE >0
	_freePbuf_REF.next  = pbuf_pool +TYPE_START_IDX_REF;
#endif
#if PBUF_HEAP_SIZE >0
	_freePbuf_HEAP.next = pbuf_pool +TYPE_START_IDX_HEAP;
#endif
	_freePbuf_REF.len   = PBUF_REF_SIZE;
	_freePbuf_HEAP.len  = PBUF_HEAP_SIZE;

	pbuf_pool[TYPE_START_IDX_HEAP  -1].next = NULL;
	pbuf_pool[TYPE_START_IDX_POOL1 -1].next = NULL;

	_freePbuf_P1.next    = pbuf_pool +TYPE_START_IDX_POOL1;
	_freePbuf_P1.tot_len = PBUF_MEMPOOL1_BSIZE * PBUF_MEMPOOL1_SIZE;
	_freePbuf_P1.len     = PBUF_MEMPOOL1_SIZE;
#if PBUF_MEMPOOL1_SIZE >0
	for (i=TYPE_START_IDX_POOL1; i<TYPE_START_IDX_POOL2; i++)
	{
		pbuf_pool[i].payload = pbuf_pool1 + (i -TYPE_START_IDX_POOL1)* PBUF_MEMPOOL1_BSIZE;
		pbuf_pool[i].len     = PBUF_MEMPOOL1_BSIZE;
		pbuf_pool[i].tot_len = (TYPE_START_IDX_POOL2 -i)* PBUF_MEMPOOL1_BSIZE;
	}
#endif // PBUF_MEMPOOL1_SIZE >0

	pbuf_pool[TYPE_START_IDX_POOL2 -1].next = NULL;

	_freePbuf_P2.next   = pbuf_pool +TYPE_START_IDX_POOL2;
	_freePbuf_P2.tot_len = PBUF_MEMPOOL2_BSIZE * PBUF_MEMPOOL2_SIZE;
	_freePbuf_P2.len = PBUF_MEMPOOL2_SIZE;
#if PBUF_MEMPOOL2_SIZE >0
	for (i=TYPE_START_IDX_POOL2; i<PBUF_SIZE_TOTAL; i++)
	{
		pbuf_pool[i].payload = pbuf_pool2 + (i -TYPE_START_IDX_POOL2)* PBUF_MEMPOOL2_BSIZE;
		pbuf_pool[i].len     = PBUF_MEMPOOL2_BSIZE;
		pbuf_pool[i].tot_len = (PBUF_SIZE_TOTAL -i)* PBUF_MEMPOOL2_BSIZE;
	}
#endif // PBUF_MEMPOOL2_SIZE >0
	pbuf_pool[PBUF_SIZE_TOTAL      -1].next = NULL;

	PBUF_CRITICAL_LEAVE();
}

static pbuf* pbuf_cballoc_priv(pbuf* freelist, pbuf_size_t blocksz)
{
	pbuf *p=NULL;
	PBUF_CRITICAL_ENTER();
	p = freelist->next;
	if (p)
	{
		freelist->next = p->next;
		freelist->tot_len -= blocksz;
		freelist->len--;
		p->next  = NULL;
		p->flags = 0;
	}
	else freelist->tot_len =0;

	PBUF_CRITICAL_LEAVE();
	return p;
}

// -----------------------------
// pbuf_halloc()
// -----------------------------
pbuf* pbuf_halloc(pbuf_size_t length)
{
	pbuf *p=NULL;
#ifdef PBUF_WITH_HEAP
	pbuf_init();

	p = pbuf_cballoc_priv(&_freePbuf_HEAP, length);
	if (NULL ==p)
		return NULL;

	p->payload = heap_malloc(length); // if pbuf is to be allocated in RAM, allocate memory for it.
	if (NULL == p->payload)
	{
		pbuf_free(p);
		return NULL;
	}
	p->tot_len = p->len = length;
	p->ref              = 1;
#endif // PBUF_WITH_HEAP

	return p;
}

// -----------------------------
// pbuf_palloc()
// -----------------------------
pbuf* pbuf_palloc(pbuf_size_t length)
{
	pbuf *pool = &_freePbuf_P1, *p=NULL, *q =NULL;
	pbuf_size_t lenInPool =PBUF_MEMPOOL1_BSIZE;
	pbuf_init();

	do {
		pool = &_freePbuf_P1; lenInPool =PBUF_MEMPOOL1_BSIZE;
		if (_freePbuf_P2.next && (NULL == pool->next || length > min(pool->tot_len/2, PBUF_MEMPOOL2_BSIZE/2) ))
		{
			pool = &_freePbuf_P2;
			lenInPool =PBUF_MEMPOOL2_BSIZE;
		}

		p = pbuf_cballoc_priv(pool, lenInPool); //	IDX2PBUF(pool->idxNext);
		if (NULL == p)
			break;

		p->tot_len = p->len = lenInPool;
		p->ref              = 1;

		if (p->len > length) 
			p->len = length;

		if (q)
		{
			p->tot_len = q->tot_len + p->len;
			p->next =q;
		}
		else if (p->tot_len > length)
			p->tot_len = length;

		q =p;
		length -= p->len;
	} while (length >0);

	if (length>0)
	{
		pbuf_free(q);
		q= NULL;
	}

	return q;
}

// -----------------------------
// pbuf_mmap()
// -----------------------------
pbuf* pbuf_mmap(void* buf, pbuf_size_t length)
{
	pbuf *p;
	pbuf_init();

	//if (0 == _freePbuf_REF.idxNext)
	//	return NULL;

	//p = IDX2PBUF(_freePbuf_REF.idxNext);
	//_freePbuf_REF.idxNext = p->idxNext;
	p = pbuf_cballoc_priv(&_freePbuf_REF, 0);

	if (p)
	{
		// caller must set this field properly, afterwards
		p->payload = buf;

		p->len = p->tot_len = length;
		p->ref              = 1;
	}

	return p;
}

pbuf* pbuf_dup(void* buf, pbuf_size_t length)
{
	pbuf* p =NULL;
	if (NULL == buf || length<=0)
		return NULL;

	if (NULL == (p = pbuf_malloc(length)))
		return NULL;

	pbuf_write(p, 0, (const uint8_t*)buf, length);
	return p;
}


uint8_t pbuf_free(pbuf *p)
{
	pbuf* q=NULL;
	uint8_t count=0;
//	uint8_t j;
#ifdef PBUF_WITH_HEAP
	uint8_t* heapmem =NULL;
#endif // PBUF_WITH_HEAP

	while (p >= (pbuf_pool + TYPE_START_IDX_REF) && p < (pbuf_pool + PBUF_SIZE_TOTAL)) 
	{
		PBUF_CRITICAL_ENTER();
		if ((--(p->ref)) >0)
		{
			p =NULL; // quit the free loop
			PBUF_CRITICAL_LEAVE();
			break;
		}

		p->ref =0;
#ifdef PBUF_WITH_HEAP
		heapmem =NULL;
#endif // PBUF_WITH_HEAP

		// remember next pbuf in chain for next iteration
		q = p->next;
		count++;
		switch(pbuf_type_priv(p))
		{
		case PBUF_TYPE_REFERENCE:
			p->payload = NULL;
			p->next = _freePbuf_REF.next;
			_freePbuf_REF.next = p;
			break;

		case PBUF_TYPE_HEAP:
#ifdef PBUF_WITH_HEAP
			heapmem = p->payload;
#endif // PBUF_WITH_HEAP
			p->payload = NULL;
			p->next = _freePbuf_HEAP.next;
			_freePbuf_HEAP.next = p;
			break;

		case PBUF_TYPE_POOL1:
			p->next = _freePbuf_P1.next;
			_freePbuf_P1.next = p;
			_freePbuf_P1.tot_len += PBUF_MEMPOOL1_BSIZE;
			_freePbuf_P1.len++;
			break;

		case PBUF_TYPE_POOL2:
			p->next = _freePbuf_P2.next;
			_freePbuf_P2.next = p;
			_freePbuf_P2.tot_len += PBUF_MEMPOOL2_BSIZE;
			_freePbuf_P2.len++;
			break;
		}

		p = q;
		PBUF_CRITICAL_LEAVE();

#ifdef PBUF_WITH_HEAP
		if (heapmem)
			heap_free(heapmem);
#endif // PBUF_WIN_HEAP
	}

	return count;
}

uint8_t pbuf_count(pbuf *p)
{
	uint8_t c;
	for (c=0; p !=NULL; c++)
		p = p->next;

	return c;
}

#if 0
void pbuf_realloc(pbuf *p, pbuf_size_t new_len)
{
	pbuf *q;
	pbuf_size_t rem_len; // remaining length
	int grow;
	uint8_t bNoError = TRUE;

	// the pbuf chain grows by (new_len - p->tot_len) bytes, which may be negative in case of shrinking
	grow = (int) new_len - p->tot_len;
	q = p;

	do
	{
		// first, step over any pbufs that should remain in the chain
		rem_len = new_len;
		for (q = p; bNoError && q->idxNext && (rem_len > q->len); q= IDX2PBUF(q->idxNext)) 
		{
			if (0 == (p->flags & PBUF_FLAG_HEAP))
			{
				bNoError = FALSE;
				break;
			}

			// decrease remaining length by pbuf length
			rem_len -= q->len;

			// adjust total length indicator
			q->tot_len += grow;
		}

		if (!bNoError)
			break;

		if (rem_len <= q->len)
		{
			// sounds like need to shrink
			q->tot_len = q->len = rem_len;
			pbuf_free(q->next);
			q->next =NULL;
		}
		else if (NULL == q) // reached the last pbuf, sounds like need to append
		{
			q->next = pbuf_malloc(rem_len);
			if (NULL == q->next)
				bNoError = FALSE;
		}

	} while(0);

	if (!bNoError)
	{
		// need to restore the changes made by stepping
		for (p; p!=q; p=p->next)
			p->tot_len -= grow;
	}
}
#endif // 0

// Concatenate two pbufs (each may be a pbuf chain) and take over the caller's reference of the tail pbuf.
// @note The caller MAY NOT reference the tail pbuf afterwards, otherwise use pbuf_chain() for that purpose.
// @see pbuf_chain()
void pbuf_cat(pbuf* h, pbuf** t)
{
	pbuf *p;
	if (NULL == h || NULL==t || NULL == *t)
		return;

	// proceed to last pbuf of chain
	for (p = h; p->next; p = p->next)
	{
		// add total length of second chain to all totals of first chain
		p->tot_len += (*t)->tot_len;
	}

	// add total length of second chain to last pbuf total of first chain
	p->tot_len += (*t)->tot_len;
	// chain last pbuf of head (p) with first of tail (t)
	p->next = (*t);
	*t = NULL;
	// p->next now references t, but the caller will drop its reference to t,
	// so netto there is no change to the reference count of t.
}

// Chain two pbufs (or pbuf chains) together.
// The caller MUST call pbuf_free(t) once it has stopped using it. Use pbuf_cat() instead if you
// no longer use t.
// @param h head pbuf (chain)
// @param t tail pbuf (chain)
// The ->tot_len fields of all pbufs of the head chain are adjusted.
// The ->next field of the last pbuf of the head chain is adjusted.
// The ->ref field of the first pbuf of the tail chain is adjusted.
void pbuf_chain(pbuf *h, pbuf *t)
{
	pbuf* tmp = t;
	pbuf_cat(h, &tmp);
	// t is now referenced by h
	pbuf_ref(t);
}

void pbuf_ref(pbuf* p)
{
	if (!p)
		return;
	PBUF_CRITICAL_ENTER();
	++(p->ref);
	PBUF_CRITICAL_LEAVE();
}

static pbuf_size_t pbuf_io(pbuf* p, pbuf_size_t offset, uint8_t* buf, pbuf_size_t n, uint8_t write)
{
	pbuf_size_t from = offset, c, left=n;
	// get the correct pbuf
	while ((p != NULL) && (p->len <= from))
	{
		from -= p->len;
		p = p->next;
	}

	if (NULL == p)
		return 0;

	while ((p != NULL) && (left >0))
	{
		c = min(p->len - from, left);
		if (write)
			memcpy(p->payload + from, buf, c);
		else
			memcpy(buf, p->payload + from, c);
		
		buf +=c;
		left -= c;
		from =0;
		p = p->next;
	}

	return (n -left);
}

pbuf_size_t pbuf_write(pbuf* p, pbuf_size_t offset, const uint8_t* buf, pbuf_size_t n)
{
	return pbuf_io(p, offset, (uint8_t*)buf, n, 1);
}

// copy data out of pbuf to buf
// @param p pbuf to parse
// @param offset offset into p of the byte to return
// @return byte at an offset into p OR ZERO IF 'offset' >= p->tot_len
pbuf_size_t pbuf_read(pbuf* p, pbuf_size_t offset, uint8_t* buf, pbuf_size_t n)
{
	return pbuf_io(p, offset, buf, n, 0);
}

// Get one byte from the specified position in a pbuf
// WARNING: returns zero for offset >= p->tot_len
// @param p pbuf to parse
// @param offset offset into p of the byte to return
// @return byte at an offset into p OR ZERO IF 'offset' >= p->tot_len
uint8_t pbuf_get_at(pbuf* p, pbuf_size_t offset)
{
	uint8_t v=0;
	if (pbuf_read(p, offset, &v, 1) >0)
		return v;
	else return 0;
}


// Compare pbuf contents at specified offset with memory s2, both of length n
// @param p pbuf to compare
// @param offset offset into p at wich to start comparing
// @param s2 buffer to compare
// @param n length of buffer to compare
// @return zero if equal, nonzero otherwise
//         (0xffff if p is too short, diffoffset+1 otherwise)
pbuf_size_t pbuf_memcmp(pbuf* p, pbuf_size_t offset, const void* s2, pbuf_size_t n)
{
	pbuf_size_t start = offset, i;
	uint8_t a, b;
	pbuf* q = p;

	// get the correct pbuf
	while ((q != NULL) && (q->len <= start))
	{
		start -= q->len;
		q = q->next;
	}

	// return requested data if pbuf is OK
	if ((q != NULL) && (q->len > start))
	{
		for(i = 0; i < n; i++)
		{
			a = pbuf_get_at(q, start + i);
			b = ((uint8_t*) s2)[i];
			if (a != b)
				return i+1;
		}

		return 0;
	}

	return 0xffff;
}

// Find occurrence of mem (with length mem_len) in pbuf p, starting at offset start_offset.
// @param p pbuf to search, maximum length is 0xFFFE since 0xFFFF is used as return value 'not found'
// @param mem search for the contents of this buffer
// @param mem_len length of 'mem'
// @param start_offset offset into p at which to start searching
// @return 0xFFFF if substr was not found in p or the index where it was found
pbuf_size_t pbuf_memfind(pbuf* p, const void* mem, uint16_t mem_len, pbuf_size_t start_offset)
{
	pbuf_size_t i, plus, max = p->tot_len - mem_len;
	if (p->tot_len >= mem_len + start_offset)
	{
		for(i = start_offset; i <= max; )
		{
			plus = pbuf_memcmp(p, i, mem, mem_len);
			if (plus == 0)
				return i;

			i += plus;
		}
	}

	return 0xFFFF;
}

// -----------------------------
// DataStruct FIFO
// -----------------------------
uint8_t FIFO_init(FIFO* fifo, uint8_t count, uint8_t itLen, uint8_t flags)
{
	if (NULL == fifo || count <=0 || itLen <=0)
		return 0;

	fifo->flags = flags;
 	fifo->count = count;
	fifo->itLen = itLen;

#ifdef FreeRTOS
	fifo->iterations = xQueueCreate(count, (unsigned portBASE_TYPE)itLen);
	return (NULL != fifo->iterations) ? count : 0;
#else
	fifo->iterations = pbuf_palloc(count * itLen);
	fifo->header = fifo->tail =0;

	return (NULL != fifo->iterations) ? fifo->count : 0;
#endif // FreeRTOS
}

hterr FIFO_push(FIFO* fifo, void* data)
{
#ifdef FreeRTOS
	// Send an incrementing number on the queue without blocking.
	if (pdPASS != ((0 == _interruptDepth)?xQueueSendToBack(fifo->iterations, data, 0): xQueueSendToBackFromISR(fifo->iterations, data, 0)))
		return ERR_OVERFLOW;

	return ERR_SUCCESS;

#else
	uint8_t newheader = (fifo->header +1) % fifo->count;
 	if (newheader == fifo->tail)
	{
		if (0 == (FIFO_FLG_ALLOW_OVERWRITE & fifo->flags))
			return ERR_OVERFLOW;
		fifo->tail = (++fifo->tail) % fifo->count;
	}

	pbuf_write(fifo->iterations, fifo->header * fifo->itLen, data, fifo->itLen);
	fifo->header = newheader;
	return ERR_SUCCESS;
#endif // FreeRTOS
}

/*
void* FIFO_reserveNext(FIFO* fifo)
{
	uint8_t oldheader = fifo->header;
	uint8_t newheader = (oldheader +1) % fifo->count;
 	if (newheader == fifo->tail)
	{
		if (0 == (FIFO_FLG_ALLOW_OVERWRITE & fifo->flags))
			return NULL;
		fifo->tail = (++fifo->tail) % fifo->count;
	}

	fifo->header = newheader;
	return fifo->iterations + fifo->header * fifo->itLen;
}
*/

#define uQueueReceive(queue, data, ticks2wait) ()

hterr FIFO_pop(FIFO* fifo, void* data)
{
#ifdef FreeRTOS
	if (NULL == fifo->iterations)
		return ERR_ADDRFAULT;
	if (pdPASS != ((0 == _interruptDepth)?xQueueReceive(fifo->iterations, data, 0): xQueueReceiveFromISR(fifo->iterations, data, 0)))
		return ERR_UNDERFLOW;

	return ERR_SUCCESS;
#else
	if (fifo->tail == fifo->header)
		return ERR_UNDERFLOW;

	pbuf_read(fifo->iterations, fifo->tail *fifo->itLen, data, fifo->itLen);
	fifo->tail = (++fifo->tail) % fifo->count;
	return ERR_SUCCESS;
#endif // FreeRTOS
}

uint8_t FIFO_awaitSize(FIFO* fifo)
{
#ifdef FreeRTOS
	if (NULL == fifo->iterations)
		return ERR_ADDRFAULT;
	return (uint8_t) ((0 == _interruptDepth)?uxQueueMessagesWaiting(fifo->iterations): uxQueueMessagesWaitingFromISR(fifo->iterations));
#else
	return (uint8_t) (((uint16_t) fifo->header) + 0x100 - fifo->tail);
#endif // FreeRTOS
}

#ifdef PBUF_WITH_HEAP
// -----------------------------
// DataStruct DummyMap
// -----------------------------
hterr DummyMap_init(DummyMap* map, uint8_t keysize, uint8_t valuesize, uint8_t totalcount)
{
	if (NULL == map || keysize<=0 || totalcount<=0)
		return ERR_INVALID_PARAMETER;

	PBUF_CRITICAL_ENTER();

	map->keysize  = keysize;
	map->rowsize  = keysize +valuesize;
	map->totcount = totalcount;
	map->count    = 0;

	map->data = heap_malloc(map->rowsize *map->totcount);
	if (NULL == map->data)
	{
		PBUF_CRITICAL_LEAVE();
		return ERR_NOT_ENOUGH_MEMORY;
	}

	memset(map->data, 0x00, map->rowsize *map->totcount);
	PBUF_CRITICAL_LEAVE();

	return ERR_SUCCESS;
}

void DummyMap_free(DummyMap* map)
{
	if (NULL == map)
		return;

	PBUF_CRITICAL_ENTER();
	heap_free(map->data);
	memset(map, 0x00, sizeof(DummyMap));
	PBUF_CRITICAL_LEAVE();
}

// utility function
//@param[out] m *m equals to the index that supposed to hold such a key
//@return negative means less, 0-equal, postive -more 
static int8_t DummyMap_search(DummyMap* map, const uint8_t* key, uint8_t* m)
{
	int8_t k1=0, k2= map->count -1, diff=1;
	(*m) = (k1 +k2) /2;

	while(k1 <= k2)
	{
		diff = memcmp(map->data + map->rowsize * (*m), key, map->keysize);
		if (diff ==0)
			break;

		if (diff <0)
			k1 = (*m)+1;
		else if (k2 < (*m))
			break;
		else k2= (*m)-1;

		*m = (k1 +k2 +1) /2;
	}

	return diff;
}

hterr DummyMap_set(DummyMap* map, const uint8_t* key, const uint8_t* value)
{
	int8_t diff=0;
	uint8_t m=0, k2;

	if (NULL == map || NULL ==key)
		return ERR_INVALID_PARAMETER;

	PBUF_CRITICAL_ENTER();
	diff = DummyMap_search(map, key, &m);
	if (0 != diff)
	{
		// the key doen't exists, shift the larger keys, the last row may be overflow
		if (map->count >= map->totcount)
			return ERR_NOT_ENOUGH_MEMORY;

		map->count++;
		for (k2 =map->count-1; k2>m; k2--)
			memcpy(map->data + map->rowsize *k2, map->data + map->rowsize *(k2-1), map->rowsize);

		// set the current key
		memcpy(map->data + map->rowsize *m, key, map->keysize);
	}
	
	// overwrite the value anyway
	memcpy(map->data + map->rowsize *m + map->keysize, value, map->rowsize - map->keysize);

	PBUF_CRITICAL_LEAVE();
	return ERR_SUCCESS;
}

hterr DummyMap_get(DummyMap* map, const uint8_t* key, uint8_t* value)
{
	int8_t diff=0;
	uint8_t m=0;

	if (NULL == map || NULL ==key || (map->rowsize >map->keysize && NULL == value))
		return ERR_INVALID_PARAMETER;

	PBUF_CRITICAL_ENTER();
	diff = DummyMap_search(map, key, &m);
	if (0 != diff)
	{
		PBUF_CRITICAL_LEAVE();
		return ERR_NOT_FOUND;
	}

	if (map->rowsize >map->keysize)
		memcpy(value, map->data + map->rowsize *m + map->keysize, map->rowsize -map->keysize);

	PBUF_CRITICAL_LEAVE();
	return ERR_SUCCESS;
}

hterr DummyMap_erase(DummyMap* map,  const uint8_t* key)
{
	int8_t diff=0;
	uint8_t m=0;

	if (NULL == map || NULL ==key)
		return ERR_INVALID_PARAMETER;

	PBUF_CRITICAL_ENTER();
	diff = DummyMap_search(map, key, &m);
	if (0 == diff)
	{
		// the key doen't exists, shift the larger keys, the last row may be overflow
		for (; m < map->count-1; m++)
			memcpy(map->data + map->rowsize *m, map->data + map->rowsize *(m+1), map->rowsize);

		map->count--;
	}
	
	PBUF_CRITICAL_LEAVE();
	return ERR_SUCCESS;
}

// -----------------------------
// DataStruct LRUMap
// -----------------------------
// although it is named as Map, we take list in order to save space
// TODO: there is a bug when stampLast rollover
hterr LRUMap_init(LRUMap* lru, uint8_t keysize, uint8_t valuesize, uint8_t maxRecords)
{
	if (NULL == lru || keysize<=0 || maxRecords<=0)
		return ERR_INVALID_PARAMETER;

	lru->stampLast = 0;
	return DummyMap_init(&lru->k2vplus, keysize, sizeof(uint16_t) +valuesize, maxRecords);
}

void LRUMap_free(LRUMap* lru)
{
	DummyMap_free(&lru->k2vplus);
	lru->stampLast = 0;
}

#define STAMP_SHIFT_THRESHOLD  (0x4000)
//@return the idx where put the data
uint8_t LRUMap_set(LRUMap* lru, const uint8_t* key, const uint8_t* value)
{
	uint8_t idx2Take = 0;
	int8_t i;
	uint16_t minStamp=0xffff, tmp;
	int8_t  diff =0;
	DummyMap* map = NULL;

	map = &lru->k2vplus;
	PBUF_CRITICAL_ENTER();
	diff = DummyMap_search(map, key, &idx2Take);
	if ((0 != diff && map->count >= map->totcount) || lru->stampLast > (STAMP_SHIFT_THRESHOLD - lru->k2vplus.totcount))
	{
		// idx2Take to locate the least t
		for (minStamp=0x4001, i=min(map->count, map->totcount)-1; i >=0; i--)
		{
			tmp = *((uint16_t*)(map->data + map->rowsize *i + map->keysize));
			if (minStamp > tmp)
			{
				minStamp = tmp;
				idx2Take = i;
			}
		}

		if (minStamp>0 && lru->stampLast > (STAMP_SHIFT_THRESHOLD - lru->k2vplus.totcount))
		{
			// decrease each stamps by minStamp 
			for (i=0; i < map->count; i++)
				*((uint16_t*) (map->data + map->rowsize *i + map->keysize)) -= minStamp;
			lru->stampLast -= minStamp;
		}

		// not found the existing key
		if (map->count >= map->totcount)
		{
			// erase it
			for (; idx2Take < map->count-1; idx2Take++)
				memcpy(map->data + map->rowsize *idx2Take, map->data + map->rowsize *(idx2Take+1), map->rowsize);
			map->count--;
		}

		// relocate the new idx2Take
		diff = DummyMap_search(map, key, &idx2Take);
	}

	if (0 != diff)
	{
		for (i =map->count-1; i>idx2Take; i--)
			memcpy(map->data + map->rowsize *i, map->data + map->rowsize *(i-1), map->rowsize);

		map->count++;

		// set the current key
		memcpy(map->data + map->rowsize *idx2Take, key, map->keysize);
	}
	
	// overwrite the value anyway
	memcpy(map->data + map->rowsize *idx2Take + map->keysize + sizeof(uint16_t), value, map->rowsize - map->keysize - sizeof(uint16_t));
	*((uint16_t*) (map->data + map->rowsize *idx2Take + map->keysize)) = ++lru->stampLast;
	
	PBUF_CRITICAL_LEAVE();

	return idx2Take;
}

//@return ERR_SUCCESS if success
hterr LRUMap_get(LRUMap* lru, const uint8_t* key, uint8_t* value, BOOL bRefresh)
{
	uint8_t idx2Take = 0xff;
	DummyMap* map = NULL;

	if (NULL == lru || NULL == key || NULL ==value)
		return ERR_INVALID_PARAMETER;

	PBUF_CRITICAL_ENTER();
	map = &lru->k2vplus;
	if (0 != DummyMap_search(map, key, &idx2Take))
	{
		PBUF_CRITICAL_LEAVE();
		return ERR_NOT_FOUND;
	}

	if (map->rowsize > map->keysize +sizeof(uint16_t))
		memcpy(value, map->data + map->rowsize *idx2Take + map->keysize + sizeof(uint16_t), map->rowsize -map->keysize - sizeof(uint16_t));

	if (bRefresh)
		*((uint16_t*)(map->data + (map->rowsize *idx2Take) +map->keysize)) = ++lru->stampLast;

	PBUF_CRITICAL_LEAVE();

	return ERR_SUCCESS;
}

hterr LRUMap_erase(LRUMap* lru, const uint8_t* key)
{
	if (NULL == lru || NULL == key)
		return ERR_INVALID_PARAMETER;

	return DummyMap_erase(&lru->k2vplus, key);
}

#endif // PBUF_WITH_HEAP

