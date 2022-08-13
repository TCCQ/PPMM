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

/* Persistent Memory */

/*
 * Persistent memory is represented as a `PMem` type. This type acts
 * as a pointer into persistent memory via the helper function PMAddr,
 * which returns a void* to the data represented by the passed PMem
 * object. Whenever persistent memory is referenced from user code,
 * users should store the pointer as a PMem type, and perform
 * opperations on it via the return value of PMAddr. This void*
 * pointer is safe to store, but is valid only within the capsule that
 * is was returned from PMAddr. If the data needs to be accessed in
 * multiple sequential capsules, the pointer MUST be required via
 * PMAddr. For safety and readability, simply using PMAddr every time
 * the code accesses the data may be the best option. The user manage
 * the data inside a single PMem reference as they see fit, so long as
 * they do not exceed the size of the referenced data in the reference
 * itself. 
 */

typedef struct {unsigned int offset; unsigned int size} PMem;

/* see above */
(volatile void)* PMAddr(PMem);

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
PMem PMalloc(size_t);

/* Releases the dynamic chunk of persistent memory at the passed
 * address given by a prior call to PMalloc. Can be safely called on a
 * pointer that has already been freed, e.g. if it was freed and then
 * the process faulted and freed again.
 *
 * (TODO ENSURE THAT PROCESSES THAT ARE NOT THE ALLOCATING PROCESS CAN
 * SAFELY FREE)
 */
void PMfree(PMem);

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
 * not guaranteed
 *
 * TODO consider adding asserts here that confirm that value is the
 * right size.
 *
 * TODO fixed the no return value with a inline block scope which
 * should prevent it? not clear could use a overkill with a ({})
 * enclosure ("statement expression" I think), and have the line be 1;
 * or 0;, have it return something constant. idk if I need that though
 *
 * TODO consider including a memsync or a flush here just to be 100%
 * sure. idk how shared memory normally works
 */
#ifndef CAM
#define CAM(loc, old, new) { \
	  (__atomic_compare_exchange_n(loc, &(old), new, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) \
	       }
#endif

