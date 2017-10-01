/* 
 * Simple, 32-bit and 64-bit clean allocator based on an implicit free list,
 * first fit placement, and boundary tag coalescing, as described in the
 * CS:APP2e text.  Blocks are aligned to double-word boundaries.  This
 * yields 8-byte aligned blocks on a 32-bit processor, and 16-byte aligned
 * blocks on a 64-bit processor.  However, 16-byte alignment is stricter
 * than necessary; the assignment only requires 8-byte alignment.  The
 * minimum block size is four words.
 *
 * This allocator uses the size of a pointer, e.g., sizeof(void *), to
 * define the size of a word.  This allocator also uses the standard
 * type uintptr_t to define unsigned integers that are the same size
 * as a pointer, i.e., sizeof(uintptr_t) == sizeof(void *).
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
	/* Team name */
	"Dynamo",
	/* First member's full name */
	"Devanshu Jain",
	/* First member's email address */
	"201201133@daiict.ac.in",
	/* Second member's full name (leave blank if none) */
	"Akshat Gupta",
	/* Second member's email address (leave blank if none) */
	"201201135@daiict.ac.in"
};

/* Basic constants and macros: */
#define WSIZE      sizeof(void *) /* Word and header/footer size (bytes) */
#define DSIZE      (2 * WSIZE)    /* Doubleword size (bytes) */
#define CHUNKSIZE  (1 << 12)      /* Extend heap by this amount (bytes) */

#define MAX(x, y)  ((x) > (y) ? (x) : (y))  

/* Pack a size and allocated bit into a word. */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p. */
#define GET(p)       (*(uintptr_t *)(p))
#define PUT(p, val)  (*(uintptr_t *)(p) = (val))


/* Read the size and allocated fields from address p. */
#define GET_SIZE(p)   (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)  (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer. */
#define HDRP(bp)  ((bp) - WSIZE)
#define FTRP(bp)  ((bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks. */
#define NEXT_BLKP(bp)  ((bp) + GET_SIZE(((bp) - WSIZE)))
#define PREV_BLKP(bp)  ((bp) - GET_SIZE(((bp) - DSIZE)))

/* Global variables: */
static char *heap_listp; /* Pointer to first block */  
//static char *heap_end;


/* Global pointers for the free list */
static char **first_free;
static char **last_free;

/* Function prototypes for internal helper routines: */
static void *coalesce(void *bp);
static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

/* Function prototypes for heap consistency checker routines: */
static void checkblock(void *bp);
static void checkheap(bool verbose);
static void printblock(void *bp); 

/* Function prototype to add a pointer to the free list */
static void add_free_list(void *bp);
static void remove_free_list(void *bp);

/* Variables for the cache */
int slot1, slot2;
int timer1, timer2;
int freq1, freq2;
int pol_flag1, pol_flag2;
int count;
int policy_off;
int count2;

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Initialize the memory manager.  Returns 0 if the memory manager was
 *   successfully initialized and -1 otherwise.
 */
int
mm_init(void) 
{

//	printf("\nThis is init()\n");

	first_free = NULL;
	last_free = NULL;
	count2 = 0;
	
	/* Create the initial empty heap. */
	if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
		return (-1);
	PUT(heap_listp, 0);                            /* Alignment padding */
	PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
	PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
	PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
	heap_listp += (2 * WSIZE);


	/* Extend the empty heap with a free block of CHUNKSIZE bytes. */
	char **bp = extend_heap(CHUNKSIZE / WSIZE);

	if (bp == NULL)
		return (-1);
	*bp = NULL;
	*(bp + 1) = NULL;
	first_free = bp;
	last_free = bp;
	
	slot1 = -1;
	slot2 = -1;
	
	count2 = 1;

	timer1 = -1;
	timer2 = -1;

	freq1 = -1;
	freq2 = -1;	
	
	pol_flag1 = 0;
	pol_flag2 = 0;
		
	count = 0;
	policy_off = 0;
	return (0);
}

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Allocate a block with at least "size" bytes of payload, unless "size" is
 *   zero.  Returns the address of this block if the allocation was successful
 *   and NULL otherwise.
 */
void *
mm_malloc(size_t size) 
{
	
//	printf("\nThis is malloc\n");

	count2 --;

	size_t asize;      /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	void *bp;
	count ++;

        if (size <= DSIZE)
                asize = 2 * DSIZE;
        else
                asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

        /* Ignore spurious requests. */
        if (size == 0)
                return (NULL);

	/* Cache management */
	if(slot1 == -1 && size <= 512 && size >= 16 && !policy_off)
	{
		slot1= (int)size;
		timer1 = 101;
		freq1 = 0;
	}
	else if ((slot1 != (int)size) && (slot2 == -1) && (size <= 512) && (size >= 16) && !policy_off)
	{
		slot2 = (int)size;
		timer2 = 101;
		freq2 = 0;
	}

	if(timer1 != -1)
	{
		timer1 --;
	}
	if(timer2 != -1)
	{
		timer2 --;
	}

	if(slot1 == (int)size)
	{
		freq1 ++;
	}
	else if(slot2 == (int)size)
	{
		freq2 ++;
	}

	if(timer1 >= 0 && freq1 >= 50)
	{
		timer1 = 100;
		freq1 = 1;
		pol_flag1 = 1;
	}
	
	if(pol_flag1 == 1 && (int)size == slot1)
	{
		bp = mem_sbrk(asize);
		if(bp != (void *) - 1)
		{
		        PUT(HDRP(bp), PACK(asize, 1));         /* Free block header */
		        PUT(FTRP(bp), PACK(asize, 1));         /* Free block footer */
		        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
			return bp;
		}
		
		// policy is valid for this size
	}
	else if(timer1 < 0)
	{		
		if(slot1 >= 0)
		{
			slot1 = -1;
			timer1 = -1;
			freq1 = -1;		
			pol_flag1 = 0;
	                // the request didn't come that often
		}
	}

        if(timer2 >= 0 && freq2 >= 50)
        {
  		timer2 = 100;
		freq2 = 1;
		pol_flag2 = 1;
        }

        if(pol_flag2 == 1 && (int)size == slot2)
        {
                bp = mem_sbrk(asize);
                if(bp != (void *) - 1)
                {
                        PUT(HDRP(bp), PACK(asize, 1));         /* Free block header */
                        PUT(FTRP(bp), PACK(asize, 1));         /* Free block footer */
                        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
			return bp;
                }

                // policy is valid for this size
        }       
        else if(timer2 < 0)
        {
		if(slot2 >= 0)
		{
			timer2 = -1;
			freq2 = -1;
			slot2 = -1;
			pol_flag2 = 0;
			// the reqest didn't come that often
		}
        }
	/* Search the free list for a fit. */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return (bp);
	}

	/* No fit found.  Get more memory and place the block. */
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL)  
	{
		return (NULL);
	}
	place(bp, asize);
	return (bp);
} 

/* 
 * Requires:
 *   "bp" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Free a block.
 */
void
mm_free(void *bp)
{
	
//	printf("\nThis is free()\n");

	count ++;

	timer1 --;
	timer2 --;

	size_t size;

	/* Ignore spurious requests. */
	if (bp == NULL)
		return;

	/* Free and coalesce the block. */
	size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	*(char **)bp = NULL;
	*((char **)bp + 1) = NULL;

	coalesce(bp);
}

/*
 * Requires:
 *   "ptr" is either the address of an allocated block or NULL.
 *
 * Effects:
 *   Reallocates the block "ptr" to a block with at least "size" bytes of
 *   payload, unless "size" is zero.  If "size" is zero, frees the block
 *   "ptr" and returns NULL.  If the block "ptr" is already a block with at
 *   least "size" bytes of payload, then "ptr" may optionally be returned.
 *   Otherwise, a new block is allocated and the contents of the old block
 *   "ptr" are copied to that new block.  Returns the address of this new
 *   block if the allocation was successful and NULL otherwise.
 */
void *
mm_realloc(void *ptr, size_t size)
{
	count2 ++;

//	printf("\nThis realloc for size : %d\n", size);

	policy_off = 1;
	size_t asize, oldsize;
	/* If size == 0 then this is just free, and we return NULL. */
	if (size == 0) {
		mm_free(ptr);
		return (NULL);
	}

	/* If oldptr is NULL, then this is just malloc. */
	if (ptr == NULL)
		return (mm_malloc(size));
/*

	if(count2 == 10)
	{
		count2 = 0;
		size = size + 4000;
	}
*/
	oldsize = GET_SIZE(HDRP(ptr));
	
        if (size <= DSIZE)
                asize = 2 * DSIZE;
        else
                asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);


        /* if the size is same as before */
	if(oldsize == asize)
	{
		return ptr;
	}

	/* if the size is smallr than before */
	if(oldsize > asize)
	{
//		PUT(HDRP(ptr), PACK(asize, 1));
//		PUT(FTRP(ptr), PACK(asize, 1));
//		PUT(HDRP(NEXT_BLKP(ptr)), PUT(oldsize - asize, 1));
//		PUT(FTRP(NEXT_BLKP(ptr)), PUT(oldsize - asize, 1));
//		mm_free(NEXT_BLKP(ptr));
		return ptr;
	}

	/* if the next block is free and contain sufficient size... */
//	int prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
	int next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
	
	void *next = NEXT_BLKP(ptr);
//	void *prev = PREV_BLKP(ptr);

//	size_t prev_size = GET_SIZE(HDRP(prev));
	size_t next_size = GET_SIZE(HDRP(next));

	if(!next_alloc && (next_size + oldsize) >= asize)
	{
		remove_free_list(next);
		int extra = next_size + oldsize - asize;
		memcpy(ptr, ptr, oldsize);
		if(extra <= 50)
		{
			PUT(HDRP(ptr), PACK(next_size + oldsize, 1));
			PUT(FTRP(ptr), PACK(next_size + oldsize, 1));
		}
		else
		{
			PUT(HDRP(ptr), PACK(asize, 1));
			PUT(FTRP(ptr), PACK(asize, 1));			
			PUT(HDRP(NEXT_BLKP(ptr)), PACK(next_size + oldsize - asize, 1));
			PUT(FTRP(NEXT_BLKP(ptr)), PACK(next_size + oldsize - asize, 1));
			mm_free(NEXT_BLKP(ptr));
		}
		return ptr;
	}

/*	if(!next_alloc && (next_size + oldsize) >= asize )
	{
		remove_free_list(next);
		int p = 0;
		memcpy(ptr, ptr, oldsize);
		if((next_size + oldsize) <= asize + DSIZE)
		{
			p = DSIZE;
			if((next_size + oldsize) == asize)
			{
				p = 0;
			}
		}
		PUT(HDRP(ptr), PACK(asize + p, 1));
		PUT(FTRP(ptr), PACK(asize + p, 1));

		if((next_size + oldsize) > asize + 8)
		{
			PUT(HDRP(NEXT_BLKP(ptr)), PACK(next_size + oldsize - asize, 1));
			PUT(FTRP(NEXT_BLKP(ptr)), PACK(next_size + oldsize - asize, 1));
			mm_free(NEXT_BLKP(ptr));
		}
		return ptr;
	}
*/	
	void *newptr;
	newptr = mm_malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if (newptr == NULL)
		return (NULL);

	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(ptr));
	if (size < oldsize)
		oldsize = size;
	memcpy(newptr, ptr, oldsize);

	/* Free the old block. */
	mm_free(ptr);

	return (newptr);
}

static void remove_free_list(void *bp)
{

//	printf("\nThis is remove the free list\n");	

	if(first_free == (char **)bp)
	{
		first_free = (char **)(*first_free);
		*(first_free + 1) = NULL;
	}
	if(last_free == (char **)bp)
	{
		last_free = (char **)(*(last_free + 1));
		*last_free = NULL;
	}
	if(*(char **)bp != NULL)
	{
		*((char **)(*(char **)bp + WSIZE)) = *((char **)bp + 1);
	}
	if(*((char **)bp + 1) != NULL)
	{
		*((char **)(*((char **)bp + 1))) = *(char **)bp;
	}
}


/*
 * The following routines are internal helper routines.
 */

/*
 * Requires:
 *   "bp" is the address of a newly freed block.
 *
 * Effects:
 *   Perform boundary tag coalescing.  Returns the address of the coalesced
 *   block.
 */
static void *
coalesce(void *bp) 
{

//	printf("\nThis is coalescing\n");

	int prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	int next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

	char **next = (char **)NEXT_BLKP(bp);

	if(prev_alloc && next_alloc) 
	{
		add_free_list(bp);
		return (bp);
	}
	else if (prev_alloc && !next_alloc)
	{         
	        size_t size = GET_SIZE(HDRP(bp));
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
		*(char **)bp = *next;
		if(*next != NULL)
		{
			*((char **)(*next) + 1) = (char *)bp;
		}
                else 
                {
                        last_free = (char **)bp;
                }

		*((char **)bp + 1) = *(next + 1);

		if(*((char **)bp +1) != NULL)
		{
			*(char **)(*((char **)bp + 1)) = (char *)bp;
		}
		else 
		{
			first_free = (char **)bp;
		}
		return bp;
	} 
	else if(!prev_alloc && next_alloc)
	{        
                size_t size = GET_SIZE(HDRP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		return bp;
	}
	else 
	 {      
	        size_t size = GET_SIZE(HDRP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
		*(char **)bp = *next;
		if(*next != NULL)
		{
			*((char **)(*next) + 1) = (char *)bp;
		}
		else 
		{
			last_free = (char **)bp;
		}
		return bp;
	}
}

static void add_free_list(void *bp)
{

//	printf("\nThis is adding free list\n");

	if(first_free == NULL || last_free == NULL)
	{
		first_free = (char **)bp;
		last_free = (char **)bp;
		*first_free = NULL;
		*(first_free + 1) = NULL;
	}
	
	else if(bp < (void *)first_free)
	{
		*(char **)bp = (char *)first_free;
		*((char **)bp + 1) = NULL;
		*(first_free + 1) = (char *)bp;	
		first_free = (char **)bp;
	}
	else if(bp > (void *)last_free)
	{
		*last_free = bp;
		*((char **)bp + 1) = (char *)last_free;
		*(char **)bp = NULL;
		last_free = (char **)bp;
	}
	else
	{
		char **loop;
		char **prev = NULL, **next = NULL;
		for(loop = first_free ; loop <= (char **)bp  ; )
		{
			prev = loop;
			loop = (char **)(*loop);
		}
		next = (char **)(*prev);
		*prev = (char *)bp;
		*(char **)bp = (char *)next;
		*((char **)bp + 1) = (char *)prev;
		*(next + 1) = (char *)bp;
	}
}

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Extend the heap with a free block and return that block's address.
 */
static void * extend_heap(size_t words) 
{
	
	void *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment. */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((bp = mem_sbrk(size)) == (void *)-1)  
		return (NULL);

	/* Initialize free block header/footer and the epilogue header. */
	PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
	PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */


	/* Coalesce if the previous block was free. */
	return (coalesce(bp));
}

/*
 * Requires:
 *   None.
 *
 * Effects:
 *   Find a fit for a block with "asize" bytes.  Returns that block's address
 *   or NULL if no suitable block was found. 
 */
static void *
find_fit(size_t asize)
{

	char **loop;
	if(first_free == NULL)
	{
		return NULL;
	}
	/* Search for the first fit. */
	for (loop = first_free; loop != NULL; ) 
	{
		void *bp = (void *)loop;
		if (!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp)))
		{
			return (bp);
		}
		loop = (char **)(*loop);
	}
	/* No fit was found. */
	return (NULL);
}

/* 
 * Requires:
 *   "bp" is the address of a free block that is at least "asize" bytes.
 *
 * Effects:
 *   Place a block of "asize" bytes at the start of the free block "bp" and
 *   split that block if the remainder would be at least the minimum block
 *   size. 
 */
static void place(void *bp, size_t asize)
{

//	printf("\nThis is placement\n");

	size_t csize = GET_SIZE(HDRP(bp));   

	if ((csize - asize) >= (2 * DSIZE)) { 
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		char **bp_before, **bp_after;
		bp_before = bp;
		void *bp1;
		bp1 = NEXT_BLKP(bp);
		bp_after = (char **)bp1;
		PUT(HDRP(bp1), PACK(csize - asize, 0));
		PUT(FTRP(bp1), PACK(csize - asize, 0));

		*bp_after = *bp_before;
		*(bp_after + 1) = *(bp_before + 1);
		


		if(*bp_after != NULL)		
			*((char **)(*bp_after) + 1) = (char *)bp_after;
		else 
			last_free = bp_after;

		if(*(bp_after + 1) != NULL)	
		{
			*(char **)(*(bp_after + 1)) = (char *)bp_after;
		}
		else
			first_free = bp_after;

	} else {

	
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));

		char **bp1 = (char **)bp;

		if(*bp1 != NULL)		
			*((char **)(*bp1) + 1) = *(bp1 + 1);
		else
                        last_free = (char **)*(bp1 + 1);

		if(*(bp1 + 1) != NULL)	
			*((char **)(*(bp1 + 1))) = *bp1;
		else
                        first_free = (char **)*bp1;
	}
}

/* 
 * The remaining routines are heap consistency checker routines. 
 */

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Perform a minimal check on the block "bp".
 */
static void
checkblock(void *bp) 
{

	if ((uintptr_t)bp % DSIZE)
		printf("Error: %p is not doubleword aligned\n", bp);
	if (GET(HDRP(bp)) != GET(FTRP(bp)))
		printf("Error: header does not match footer\n");
}

/* 
 * Requires:
 *   None.
 *
 * Effects:
 *   Perform a minimal check of the heap for consistency. 
 */
void
checkheap(bool verbose) 
{
	void *bp;

	if (verbose)
		printf("Heap (%p):\n", heap_listp);

	if (GET_SIZE(HDRP(heap_listp)) != DSIZE ||
	    !GET_ALLOC(HDRP(heap_listp)))
		printf("Bad prologue header\n");
	checkblock(heap_listp);

	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (verbose)
			printblock(bp);
		checkblock(bp);
	}

	if (verbose)
		printblock(bp);
	if (GET_SIZE(HDRP(bp)) != 0 || !GET_ALLOC(HDRP(bp)))
		printf("Bad epilogue header\n");
}

/*
 * Requires:
 *   "bp" is the address of a block.
 *
 * Effects:
 *   Print the block "bp".
 */
static void
printblock(void *bp) 
{
	bool halloc, falloc;
	size_t hsize, fsize;

	checkheap(false);
	hsize = GET_SIZE(HDRP(bp));
	halloc = GET_ALLOC(HDRP(bp));  
	fsize = GET_SIZE(FTRP(bp));
	falloc = GET_ALLOC(FTRP(bp));  

	if (hsize == 0) {
		printf("%p: end of heap\n", bp);
		return;
	}

	printf("%p: header: [%zu:%c] footer: [%zu:%c]\n", bp, 
	    hsize, (halloc ? 'a' : 'f'), 
	    fsize, (falloc ? 'a' : 'f'));
}

/*
 * The last lines of this file configures the behavior of the "Tab" key in
 * emacs.  Emacs has a rudimentary understanding of C syntax and style.  In
 * particular, depressing the "Tab" key once at the start of a new line will
 * insert as many tabs and/or spaces as are needed for proper indentation.
 */

/* Local Variables: */
/* mode: c */
/* c-default-style: "bsd" */
/* c-basic-offset: 8 */
/* c-continued-statement-offset: 4 */
/* indent-tabs-mode: t */
/* End: */
