#include <sys/types.h> //for size_t

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

/* 
 * consider adding a new public header as a subset of this, cause most
 * of this is not stuff the user needs
 */

/* 
 * These are the definitions for the dynamic memory table
 */
struct entryTag { 
     int owner;
     boolean isGrabbed;
     byte padding[8-sizeof(int)-sizeof(boolean)]; 
};

typedef struct {
     PMem data; //offset and size
     boolean inList; //is this active part of the table (alloc'd or free)
     struct entryTag tag; //atomically swappable ownership
     int next; //linked list downlink, see comments about inactive entries
     boolean isInUse; //is this block allocated or free
} entry;


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


#ifndef CAPSULE_INCLUDED
#include "capsule.h"

/* see above */
(volatile void)* PMAddr(PMem);

/* Dynamic Persistent Memory Management */

/*
 * persistent function call, use with pcall. takes one argument, an
 * int, the size of the desired chunk of memory. It returns a PMem to
 * said memory, or spins until said memory is free.
 *
 * THIS CALL WILL SPIN FOREVER IF NO MEMORY IS AVAILABLE.
 */
Capsule PMalloc(void);

/*
 * persistent function call, use with pcall. takes a PMem, which was
 * identical to the one returned by a prior successful call to
 * PMalloc. Releases said memory back into the pool for someone else
 * to use. Returns nothing
 */
Capsule PMfree(void);

/*
 * flushes the changes to persistent memory out to ensure that other
 * processes can see them
 *
 * TODO. this should be an ephemeral call probably? unclear
 */
void PMflush(void);

#endif

/* Persistent Memory Actions */

/*
 * compare and modify, should only be used on persistent memory
 *
 * takes a pointer and two values. its is the user's resposibility
 * that the data type is 1,2,4,8 bytes long, otherwise atomiticity is
 * not guaranteed
 *
 * consider adding asserts here that confirm that value is the
 * right size.
 *
 * fixed the no return value with a inline block scope which
 * should prevent it? not clear could use a overkill with a ({})
 * enclosure ("statement expression" I think), and have the line be 1;
 * or 0;, have it return something constant. idk if I need that though
 *
 * consider including a memsync or a flush here just to be 100%
 * sure. idk how shared memory normally works
 */
#ifndef CAM
#define CAM(loc, old, new) { \
	  (__atomic_compare_exchange_n(loc, &(old), new, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) \
	       }
#endif

