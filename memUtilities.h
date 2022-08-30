#ifndef MEM_UTIL_HEADER
#define MEM_UTIL_HEADER

#include <sys/types.h> //for size_t
#include <stdint.h> //fixed size ints
#include "typesAndDecs.h"
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
void* PMAddr(PMem);

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
 * fixed the no return value with a inline block scope which
 * should prevent it? not clear could use a overkill with a ({})
 * enclosure ("statement expression" I think), and have the line be 1;
 * or 0;, have it return something constant. idk if I need that though
 *
 * consider including a memsync or a flush here just to be 100%
 * sure. idk how shared memory normally works
 *
 * WARNING This is extremely unsafe when used on expressions whose
 * evaluation has side effects. DO NOT USE THEM HERE. Copy their
 * result into a variable and then use that if necessary.  
 */
#ifndef CAM

#define CAM(loc, old, new) {						\
	  if (sizeof(old) == 1) {					\
	       uint8_t* _loc = (uint8_t*) (loc);			\
	       uint8_t* _expected = (uint8_t*) (&(old));		\
	       __auto_type _new = new;					\
	       uint8_t _repl = *( (uint8_t*) (&(_new)) );		\
	       __atomic_compare_exchange_n(_loc, _expected, _repl, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
	  } else if (sizeof(old) == 2) {				\
	       uint16_t* _loc = (uint16_t*) (loc);			\
	       uint16_t* _expected = (uint16_t*) (&(old));		\
	       __auto_type _new = new;					\
	       uint16_t _repl = *( (uint16_t*) (&(_new)) );		\
	       __atomic_compare_exchange_n(_loc, _expected, _repl, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
	  } else if (sizeof(old) == 4) {				\
	       uint32_t* _loc = (uint32_t*) (loc);			\
	       uint32_t* _expected = (uint32_t*) (&(old));		\
	       __auto_type _new = new;					\
	       uint32_t _repl = *( (uint32_t*) (&(_new)) );		\
	       __atomic_compare_exchange_n(_loc, _expected, _repl, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
	  } else if (sizeof(old) == 8) {				\
	       uint64_t* _loc = (uint64_t*) (loc);			\
	       uint64_t* _expected = (uint64_t*) (&(old));		\
	       __auto_type _new = new;					\
	       uint64_t _repl = *( (uint64_t*) (&(_new)) );		\
	       __atomic_compare_exchange_n(_loc, _expected, _repl, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
	  } else {							\
	       rassert(0, "wrong size");				\
	  }								\
     }
#endif

/*
  CAM_T* _loc = (CAM_T*) (loc);						\
CAM_T* _expected = (CAM_T*) (&(old));					\
CAM_T _new = *( (CAM_T*) (&(new)) );					\
__atomic_compare_exchange_n(_loc, _expected, _new, false,
__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); \
*/
#endif
