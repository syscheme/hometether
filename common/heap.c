#include "heap.h"

typedef unsigned char uint8_t;

#ifndef HEAP_POINTER_SIZE_TYPE
#  define HEAP_POINTER_SIZE_TYPE unsigned long
#endif

#if HEAP_BYTE_ALIGNMENT == 8
#  define HEAP_BYTE_ALIGNMENT_MASK    (0x0007)
#endif

#if HEAP_BYTE_ALIGNMENT == 4
#  define HEAP_BYTE_ALIGNMENT_MASK	(0x0003)
#endif

#if HEAP_BYTE_ALIGNMENT == 2
#  define HEAP_BYTE_ALIGNMENT_MASK	(0x0001)
#endif

#if HEAP_BYTE_ALIGNMENT == 1
#  define HEAP_BYTE_ALIGNMENT_MASK	(0x0000)
#endif

#ifndef HEAP_BYTE_ALIGNMENT_MASK
#  error "Invalid HEAP_BYTE_ALIGNMENT definition"
#endif

// block sizes must not get too small. 
#define HEAP_BLOCK_SIZE_MIN	((size_t) (HEAP_STRUCT_SIZE * 2))

// assumes 8bit bytes!
#define HEAP_BITS_PER_BYTE		((size_t) 8)

// a few bytes might be lost to byte aligning the heap start address.
#define HEAP_SIZE_ADJUSTED	(HEAP_SIZE_TOTAL - HEAP_BYTE_ALIGNMENT)

// allocate the memory for the heap. 
static uint8_t HEAP_DATA[HEAP_SIZE_TOTAL];

// define the linked list structure.  This is used to link free blocks in order of their memory address.
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	// The next free block in the list.
	size_t xBlockSize;						// The size of the free block. 
} xBlockLink;

// -----------------------------------------------------------

// Inserts a block of memory that is being freed into the correct position in 
// the list of free memory blocks.  The block being freed will be merged with
// the block in front it and/or the block behind it if the memory blocks are
// adjacent to each other.
static void heap_insertToFreeList(xBlockLink *pxBlockToInsert);

// Called automatically to setup the required heap structures the first time heap_malloc() is called.
static void heap_init(void);

// The size of the structure placed at the beginning of each allocated memory block must by correctly byte aligned.
static const unsigned short HEAP_STRUCT_SIZE	= ((sizeof (xBlockLink)+ (HEAP_BYTE_ALIGNMENT - 1)) & ~HEAP_BYTE_ALIGNMENT_MASK);

// Ensure the pxEnd pointer will end up on the correct byte alignment.
static const size_t xTotalHeapSize = ((size_t)HEAP_SIZE_ADJUSTED)& ((size_t)~HEAP_BYTE_ALIGNMENT_MASK);

// Create a couple of list links to mark the start and end of the list.
static xBlockLink xStart, *pxEnd = NULL;

// Keeps track of the number of free bytes remaining, but says nothing about fragmentation.
static size_t xFreeBytesRemaining = ((size_t)HEAP_SIZE_ADJUSTED)& ((size_t)~HEAP_BYTE_ALIGNMENT_MASK);

// Gets set to the top bit of an size_t type.  When this bit in the xBlockSize member of an xBlockLink
// structure is set then the block belongs to the application. When the bit is free the block is still
// part of the free heap space.
static size_t xBlockAllocatedBit = 0;

// -----------------------------
// heap_malloc()
// -----------------------------
void* heap_malloc(size_t xWantedSize)
{
	xBlockLink *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;

	HEAP_ENTER_CRITICAL();
	{
		// If this is the first call to malloc then the heap will require initialisation to setup the
		// list of free blocks.
		if (pxEnd == NULL)
		{
			heap_init();
		}

		// Check the requested block size is not so large that the top bit is set.
		// The top bit of the block size member of the xBlockLink structure is used to determine who
		// owns the block - the application or the kernel, so it must be free.
		if ((xWantedSize & xBlockAllocatedBit)== 0)
		{
			// The wanted size is increased so it can contain a xBlockLink structure in addition to
			// the requested amount of bytes.
			if (xWantedSize > 0)
			{
				xWantedSize += HEAP_STRUCT_SIZE;

				// Ensure that blocks are always aligned to the required number of bytes. 
				if ((xWantedSize & HEAP_BYTE_ALIGNMENT_MASK)!= 0x00)
				{
					// Byte alignment required
					xWantedSize += (HEAP_BYTE_ALIGNMENT - (xWantedSize & HEAP_BYTE_ALIGNMENT_MASK));
				}
			}

			if ((xWantedSize > 0)&& (xWantedSize <= xFreeBytesRemaining))
			{
				// Traverse the list from the start	(lowest address) block until one of adequate size is found.
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while((pxBlock->xBlockSize < xWantedSize)&& (pxBlock->pxNextFreeBlock != NULL))
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				// If the end marker was reached then a block of adequate size was not found. 
				if (pxBlock != pxEnd)
				{
					// Return the memory space pointed to - jumping over the xBlockLink structure at its start.
					pvReturn = (void *)(((uint8_t *)pxPreviousBlock->pxNextFreeBlock)+ HEAP_STRUCT_SIZE);

					// This block is being returned for use so must be taken out of the list of free blocks.
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					// If the block is larger than required it can be split into two.
					if ((pxBlock->xBlockSize - xWantedSize) > HEAP_BLOCK_SIZE_MIN)
					{
						// This block is to be split into two.  Create a new block following the number of
						// bytes requested. The void cast is used to prevent byte alignment warnings from the
						// compiler. 
						pxNewBlockLink = (void *)(((uint8_t *)pxBlock)+ xWantedSize);

						// Calculate the sizes of two blocks split from the single block.
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						// Insert the new block into the list of free blocks.
						heap_insertToFreeList((pxNewBlockLink));
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					// The block is being returned - it is allocated and owned by the application and
					// has no "next" block.
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
			}
		}

		HEAP_TRACE("malloc", pvReturn, xWantedSize);
	}

	HEAP_EXIT_CRITICAL();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
	{
		if (pvReturn == NULL)
		{
			extern void vApplicationMallocFailedHook(void);
			vApplicationMallocFailedHook();
		}
	}
#endif

	return pvReturn;
}

// -----------------------------
// heap_free()
// -----------------------------
void heap_free(void *pv)
{
	uint8_t *puc = (uint8_t *)pv;
	xBlockLink *pxLink;

	if (pv != NULL)
	{
		// The memory being freed will have an xBlockLink structure immediately before it.
		puc -= HEAP_STRUCT_SIZE;

		// This casting is to keep the compiler from issuing warnings. 
		pxLink = (void *)puc;

		// Check the block is actually allocated. 
		HEAP_ASSERT((pxLink->xBlockSize & xBlockAllocatedBit)!= 0);
		HEAP_ASSERT(pxLink->pxNextFreeBlock == NULL);

		if ((pxLink->xBlockSize & xBlockAllocatedBit)!= 0)
		{
			if (pxLink->pxNextFreeBlock == NULL)
			{
				// The block is being returned to the heap - it is no longer allocated. 
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				HEAP_ENTER_CRITICAL();
				{
					// Add this block to the list of free blocks.
					xFreeBytesRemaining += pxLink->xBlockSize;
					heap_insertToFreeList(((xBlockLink *)pxLink));
					HEAP_TRACE("free", pv, pxLink->xBlockSize);
				}
				HEAP_EXIT_CRITICAL();
			}
		}
	}
}

// -----------------------------
// heap_getFreeSize()
// -----------------------------
size_t heap_getFreeSize(void)
{
	return xFreeBytesRemaining;
}

void vPortInitialiseBlocks(void)
{
	// This just exists to keep the linker quiet.
}

// -----------------------------
// private utilities
// -----------------------------
static void heap_init(void)
{
	xBlockLink *pxFirstFreeBlock;
	uint8_t *pucHeapEnd, *pucAlignedHeap;

	// Ensure the heap starts on a correctly aligned boundary. 
	pucAlignedHeap = (uint8_t *)(((HEAP_POINTER_SIZE_TYPE)&HEAP_DATA[ HEAP_BYTE_ALIGNMENT ])& ((HEAP_POINTER_SIZE_TYPE)~HEAP_BYTE_ALIGNMENT_MASK));

	// xStart is used to hold a pointer to the first item in the list of free blocks. 
	// The void cast is used to prevent compiler warnings. 
	xStart.pxNextFreeBlock = (void *)pucAlignedHeap;
	xStart.xBlockSize = (size_t)0;

	// pxEnd is used to mark the end of the list of free blocks and is inserted at the end of the heap space.
	pucHeapEnd = pucAlignedHeap + xTotalHeapSize;
	pucHeapEnd -= HEAP_STRUCT_SIZE;
	pxEnd = (void *)pucHeapEnd;
	HEAP_ASSERT((((unsigned long)pxEnd)& ((unsigned long)HEAP_BYTE_ALIGNMENT_MASK)) == 0UL);
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	// To start with there is a single free block that is sized to take up the entire heap space, minus the
	// space taken by pxEnd. 
	pxFirstFreeBlock = (void *)pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = xTotalHeapSize - HEAP_STRUCT_SIZE;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	// The heap now contains pxEnd.
	xFreeBytesRemaining -= HEAP_STRUCT_SIZE;

	// Work out the position of the top bit in a size_t variable.
	xBlockAllocatedBit = ((size_t)1)<< ((sizeof(size_t)* HEAP_BITS_PER_BYTE)- 1);
}


static void heap_insertToFreeList(xBlockLink *pxBlockToInsert)
{
	xBlockLink *pxIterator;
	uint8_t *puc;

	// Iterate through the list until a block is found that has a higher address than the block being inserted.
	for(pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock)
	{
		// Nothing to do here, just iterate to the right position. 
	}

	// Do the block being inserted, and the block it is being inserted after make a contiguous block of memory?
	puc = (uint8_t *)pxIterator;
	if ((puc + pxIterator->xBlockSize)== (uint8_t *)pxBlockToInsert)
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}

	// Do the block being inserted, and the block it is being inserted before make a contiguous block of memory? 
	puc = (uint8_t *)pxBlockToInsert;
	if ((puc + pxBlockToInsert->xBlockSize)== (uint8_t *)pxIterator->pxNextFreeBlock)
	{
		if (pxIterator->pxNextFreeBlock != pxEnd)
		{
			// Form one big block from the two blocks. 
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;		
	}

	// If the block being inserted plugged a gab, so was merged with the block
	// before and the block after, then it's pxNextFreeBlock pointer will have
	// already been set, and should not be set here as that would make it point
	// to itself.
	if (pxIterator != pxBlockToInsert)
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
}

