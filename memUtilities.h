/* TODO add an inclusion for size_t  */



/* Dynamic Emphemerial Memory Managament */

/*
 * Attempts to allocate a chunk of ephemeral process local memory of
 * the given size and returns a pointer to the first byte of it
 */
void* EMalloc(size_t);

/*
 * Called on a pointer returned from EMalloc, returns the dynamic
 * memory after use
 */
void EMfree(void*);


/* Dynamic Persistent Memory Mangagement */

/*
 * Returns a pointer to the first byte of a region of the passed size
 * in persistent memory. This memory region can be accessed by the
 * calling and all other processes. If memory is allocated, the
 * process faults, and the alloc is called again, so long as the
 * requested size remains the same and no other alloc calls were made
 * from this process, the same region in memory will be returned. This
 * call is idempotently safe.
 *
 * (TODO ENSURE THAT I CHECK PROCESSIDX INSIDE, OR SOMEHOW ENSURE NO
 * MEMLEAKS OTHERWISE)
 */
void* PMalloc(size_t);

/* Releases the dynamic chunk of persistent memory at the passed
 * address given by a prior call to PMalloc. Can be safely called on a
 * pointer that has already been freed, e.g. if it was freed and then
 * the process faulted and freed again.
 *
 * (TODO ENSURE THAT PROCESSES THAT ARE NOT THE ALLOCATING PROCESS CAN
 * SAFELY FREE)
 */
void PMfree(void*);

/* flushes the changes to persistent memory out to ensure that other
 * processes can see them
 */
void PMflush(void);

/* A wrapper for everything you would normally use getProcId for. A
 * process after a fault (and thus a unix process restart) may not
 * have the same real pid, so using this should give a low
 * integer(sequential probably) that is which process (in the sense
 * the model uses the word) the caller is. Also for use in finding
 * which stack/local state/reset pointer/etc is the given
 * process's
 */
int getProcIDX(void);

/* Persistent Memory Actions */

/* needs to be supported bu system compare and exchange instructions 
 * should be large enough to hold a void* pointer.
 * Or maybe write several versions for different sizes?
 *
 * TODO think about this
 *
 * Seems like the gcc __sync_val_compare_and_swap supports int scalars
 * of 1,2,4,8 bytes, so long long works (8bytes) if we raw data cast
 * the structure and the lengths are matching. I should assert
 * somewhere about that.
 *
 * TODO write a version that does the swap interally
 *
 * Seems like the above is deprecated in favor of
 * (https://gcc.gnu.org/onlinedocs/gcc-12.1.0/gcc/_005f_005fatomic-Builtins.html#g_t_005f_005fatomic-Builtins),
 * It says you can pass other data and types and it will figure it
 * out.
 *
 * TODO so that says I should just write a Cam that takes jobs and
 * maybe one that takes restart pointers
 */ 
typedef unsigned long long camChunk;

/* compare and modify, should only be used on persistent memory */
//void CAM(void*, void*, camChunk);
/* TODO I think I should make custom versions for jobs and whatnot, but for scalar types just use a macro */
#ifndef CAM
#define CAM(loc, old, new) (/* TODO impliment w/ __sync_whatever, see worriesAndTodos.txt */)
#endif
