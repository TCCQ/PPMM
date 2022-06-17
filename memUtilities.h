#include <sys/types.h> //for size_t



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

/*
 * compare and modify, should only be used on persistent memory
 *
 * takes a pointer and two values. its is the user's resposibility
 * that the data type is 1,2,4,8 bytes long, otherwise atomiticity is
 * not guarenteed (does actually return a value, but it shouldn't be
 * used, its a CAM but secretly a CAS)
 */
#ifndef CAM
#define CAM(loc, old, new) (__atomic_compare_exchange_n(loc, &(old), new, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
#endif
