# Dynamic-Memory-Allocator

In this project I wrote a dynamic storage allocator for C programs, i.e., my
own version of the `malloc`
, 
`free`
, and 
`realloc`
 routines.  You are encouraged to 
explore the design space creatively and implement an allocator 
that is correct, efficient, 
and fast. 
## Description 
Your dynamic storage allocator will consist 
of the following four functions, which are 
declared in 
`mm.h`
 and defined in 
`mm.c`
.   
* `int mm_init(void);`
* `void *mm_malloc(size_t size);`
* `void mm_free(void *ptr);`
* `void *mm_realloc(void *ptr, size_t size);`
The 
`mm.c`
 file we have given you implements a simple memory allocator based on an 
implicit free list, first fit placement, and 
boundary tag coalescing, as described in the 
textbook. Using this as a starting place, modify these functions (and possibly define 
other private 
static
 functions), so that they obey the following semantics:   
* mm_init:Before calling 
`mm_malloc`
, 
`mm_realloc`
, or 
`mm_free`
, the 
application program (i.e., the trace-drive
n driver program that you will use to 
evaluate your implementation) calls 
`mm_init` to perform any necessary 
initializations, such as allocating the initial
 heap area. The return value should be 
−1 if there was a problem in performing the initialization, 0 otherwise.

The driver will call 
mm_init
 before running each trace 
(and after resetting the 
brk
 pointer). Therefore, your 
mm_init 
function should be able to reinitialize 
all state in your allocator each time it is called. In other words, you should not 
assume that it will only be called once. 
* mm_malloc:  The 
`mm_malloc`
 routine returns a pointer
 to an allocated block 
payload of at least 
size
 bytes. The entire allocated block should lie within the 
heap region and should not overlap with
any other allocated chunk. We will 
compare your implementation to the version of 
malloc
supplied in the standard 
C library (
libc
). Since the 
libc
malloc always returns payload pointers that are 
aligned to 8 bytes, your malloc implementation should do likewise and always 
return 8-byte aligned pointers.  
* mm_free:  The 
`mm_free`
 routine frees the block pointed to by 
ptr
It returns 
nothing. This routine is only guaranteed
 to work when the passed pointer (
ptr
) 
was returned by an earlier call to 
`mm_malloc`
or 
`mm_realloc`
 has not yet been 
freed.   
* mm_realloc:  The 
`mm_realloc`
 routine returns a point
er to an allocated 
region of at least 
size 
bytes with the following constraints:   
* if 
`ptr` 
is NULL, the call is equivalent to 
`mm_malloc(size);`  
* if 
`size`
is equal to zero, the call is equivalent to 
`mm_free(ptr);`  
* if 
`ptr`
 is not NULL, it must have been
 returned by an earlier call to 
`mm_malloc`
 or 
`mm_realloc`
.  The call to 
`mm_realloc`
 changes the 
size of the memory block pointed to by 
ptr 
(the 
old block
) to 
size 
bytes and returns the address of the ne
w block. Notice that the address of 
the new block might be the same as th
e old block, or it 
might be different, 
depending on your implementation, the 
amount of internal fragmentation 
in the old block, and the size of the 
realloc
 request.   
The contents of the new block are the same as those of the old 
ptr 
block, 
up to the minimum of the old and 
new sizes. Everything else is 
uninitialized. For example, if the old 
block is 8 bytes and the new block is 
12 bytes, then the first 8 bytes of the 
new block are identical to the first 8 
bytes of the old block and the last 4 
bytes are uninitialized.  Similarly, if 
the old block is 8 bytes and the new block is 4 bytes, then the contents of 
the new block are identical to the first 4 bytes of the old block.   
These semantics must match the 
semantics of the corresponding 
libc malloc
, 
realloc
, and 
free 
routines.  Type 
man malloc 
for complete documentation.

## Approach
In this project I have created a dynamic storage allocator for C programs making our own versions of malloc, free and realloc. For this purpose, I have used the approach “Segregated explicit free list” that contains 10 different categories of block sizes varying from 'less than 2^6' to 'greater than 2^14'  in the intervals of the powers of 2.

#### Why Segregated Free List?
Segregated free list uses different linked lists for different sized blocks. Each of these linked list can be seen as an array of explicit free list. I am using this approach because it outputs performance better than explicit free lists and currently adopted implicit free lists. It performs adding and freeing operations in constant time. In addition to that, segregated best fit approach is faster than the regular/unsegregated best fit since it always searches in a free list for an appropriate index (index signifies our individual linked list for appropriate size). If it fails to fit in this index then it searches for larger indices. It also controls fragmentation of segregated storage by everytime splicing and coalescing the blocks after searching and allocating the block.

#### The things which I have added or updated is described as follows :

* Defining additional macros for segregated free list which are basically getter and setter methods for getting and setting next and previous block pointers in accordance with our block structure.

* Defined additional functions to perform operations on free list, i.e. adding a free block to the free list, removing a newly allocated block from the free list, coalescing between the blocks, etc. In the new created coalescing routine(`main_coalesce`), I have considered all the four cases in which our block can be coalesced. Then by calling the regular coalescing routine, I am adding the new coalesced block to the free list and removing the block from the free list, on which coalescing was being performed. Hence add and remove functions will be used.

* Changed a routine of `find_fit` for segregated free list, in which I am using the best fit method to find a fit for a block of required size. Initially I will fetch the list index from where the appropriate size of block could be achieved and if it fails to find any free block in that particular linked list of the list index (i.e. if free list is empty for that particular list index), it will search for the best fit in the linked list of next list index and so on.

* Updated the malloc and free routines that can call the segregated free list routines which I made initially. Now it calls `find_fit` and for allocating the blocks it calls an updated 'place' routine (which is now able to handle splicing of under-allocated blocks and placing the smaller splice into an appropriate free list). Finally it calls `extend_heap` routine if it fails to allocate a block of the required size. In the 'free' routine there is no change from what was given earlier apart from calling our new `main_coalesce` routine instead of regular coalesce. Updating malloc and free turned out to be a boost in the performance up to 38%. 

* Finally I updated the realloc routine boosting up the performance up to 95%. I have added a few new things in addition to our current implementation. Realloc checks if I can coalesce on the right and creates a resultant block with appropriate size. This greatly increases the performance. Also if old ptr is NULL, then it just calls malloc. And if size is zero then it calls free, and returns NULL.

* Made an updated version of heap consistency checker which can now check for the following : <br>
          1. Checks Prologue header <br>
          2. Checks if the block is double word aligned or not. <br>
          3. Checks if header matches footer or not. <br>
	         4. Checks if contiguous free blocks somehow escaped coalescing. <br>
	         5. Checks if free block is actually in free list. The number of free blocks in heap and segregated list match. <br>
	         6. Checks if pointers in free list points to valid free blocks. <br>
	         7. Checks epilogue header. <br>
