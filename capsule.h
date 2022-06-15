#ifndef persistentReturn
#include "scheduler.h"
#endif
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
     Set* joinLocs[JOIN_SIZE]; //size should match bits in forkPath 
     int joinHead; //which location is the one to join
} Capsule;

/* control flow declarations */
typedef Capsule (*funcPtr_t)(void); 
/*
 * 'funcPtr_t' is bound to a function pointer type that takes no args
 * and returns a continuation capsule. This return type CANNOT be a
 * pointer, because it needs to outlive the function and I don't want
 * to mess with Capsule allocation
 */

Capsule** currentlyInstalled;
/*
 * this is PM pointer, and needs to be init somewhere TODO
 *
 * this is a pointer to PM, and from there another PM pointer to the
 * actually installed capsule. Do not write directly, instead just use
 * the trampoline setup with returned continuations. To start the
 * cycle, read the comment above trampolineCapsule in capsule.c. this
 * location can be safely read by the process that owns it, or by
 * another if the owner is dead
 * 
 */

void retrieveCapsuleArguments(void*, int);
/*
 * do the reverse of the argument part of makeCap. Called from user
 * code. Reads from the currently installed capsule
 */

#define getCapArgs(s) retrieveCapsuleArguments(&s, sizeof(s))
/* see mCapStruct below */


/*
 * TODO add initializers / constructors / destructor for Capsule
 *
 * we have constructors now, and I don't think I need a destructor.
 * the only thing to be suspicious of is freeing the Sets, but that
 * should be handled by the joins, assuming that frees can be done
 * from any process
 */

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
 * lines up properly. this is the only thing in this file that
 * requires the scheduler.h inclusion. This can be moved (possibly
 * with persistentCall) to another file to prevent circular includes,
 * the conditional include may already solve that
 */



