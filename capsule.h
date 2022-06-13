/*
 * TODO include the scheduler function from scheduler.c, requires
 * additional non-user header for scheduling
 */
#include "set.h"
/*
 * declarations for capsule code
 */

#define ARGUMENT_SIZE 256
#define JOIN_SIZE 64
//needs to match the length of forkpath in bits
typedef unsigned char BYTE;
/* TODO need to ensure that ^ is exactly 1 byte */

typedef struct {
     funcPtr_t rstPtr; //where the work is for this capsule
     BYTE arguments[ARGUMENT_SIZE]; //data needed for ^
     int argSize; //optional, how much of ^ is used
     unsigned long long forkPath;
     /*
      * each bit is the left/right status of the fork that lead to
      * this capsule. See fork and join in scheduler.c
      */
     Set* joinLocs[JOIN_SIZE]; //size should match bits in forkPath (TODO circular structs?)
     int joinHead; //which location is the one to join
} Capsule;

/* control flow declarations */
typedef Capsule (*funcPtr_t)(void); 
/*
 * 'funcPtr_t' is bound to a function pointer type that takes no args
 * and returns a continuation capsule. This return type CANNOT be a
 * pointer, because it needs to outlive the function and I don't want
 * to mess with Capsule allocation
 *
 * TODO this needs to be visible from user
 * code, consider splitting this file into internal and external
 */

Capsule* currentlyInstalled;
/*
 * this is PM pointer, and needs to be init somewhere TODO
 *
 * TODO decide the visibility of this internally and externally.
 * Consider splitting headers
 *
 * TODO this needs to be atomically swapped, not with other threads
 * but it can't fault in the middle of the write. could be a ptr**,
 * but then we need dynamic allocation. I think an alternating pair
 * system should work
 */

void retrieveCapsuleArguments(void*, int);
/*
 * do the reverse of the argument part of makeCap. Called from user
 * code. Reads from the currently installed capsule
 */

#define getCapArgs(s) retrieveCapsuleArguments(&s, sizeof(s))
/* see mCapStruct below */

void installCapsule(Capsule);
/*
 * installs the given capsule. This should NOT be a pointer, because
 * the upcoming capsule from the function needs to be not function
 * local, see above. This needs to be visible to other files, but not
 * to the user. TODO consider splitting this header into user and
 * not-user visible
 */


/* TODO add initializers / constructors / destructor for Capsule */

//pass NULL, 0 for no args
Capsule makeCapsule(funcPtr_t, void*, int);

/* faster version for arguments that are a single struct (recommended) */
#define mCapStruct(f,s) makeCapsule(f,s,sizeof(s))


/*
 * one of the two definitions below should be at the end of every
 * persistent user function and all of the internal persistent calls
 */
#define persistentCall(cap) return(cap)
/*
 * this is how one does a persistent call. Cap will be run and when it
 * exits, it will return the capsule to run after the call
 */

#define persistentReturn return(&scheduler)
/*
 * take us back to the scheduler if this thread is done
 *
 * TODO this is function pointer to the scheduler func, make sure it
 * lines up properly. See top of this file
 */



