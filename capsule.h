#define CAPSULE_INCLUDED 1
//prevent mutually recursive inclusions

#include "set.h"
#include "memUtilities.h"
#include "typesAndDecs.h"
/*
 * declarations for capsule code
 */


typedef struct {
     funcPtr_t rstPtr; //where the work is for this capsule
     
     /*
      * one bit, when I join am I left or right
      */
     byte forkSide;
     /* 
      * the owner that lets us check out safely
      */
     int expectedOwner;
     /* 
      * idx of the set to join
      */
     int joinLoc;

     /* 
      * The clean pointer for the pStack this capsule is working on, and
      * the clean pointers for the cnt and callee holders. read pStack.h
      *
      * These are PMems, deref once to see data
      */
     PMem pStackHeadClean;
     PMem cntHolderClean;
     PMem callHolderClean;

     /* 
      * soft whoAmI, set at the scheduler->usercode edge. use getter
      * (getProcIDX)
      */
     int whoAmI; 
} Capsule;

Capsule quickGetInstalled(void);
int getProcIDX(void); //soft whoAmI, allows for hardfault steals

//use with pcall, returns void takes void. sets swai from hard
Capsule resetWhoAmI(void);

/* control flow declarations */
typedef Capsule (*funcPtr_t)(void); 
/*
 * 'funcPtr_t' is bound to a function pointer type that takes no args
 * and returns a continuation capsule. This return type CANNOT be a
 * pointer, because it needs to outlive the function and I don't want
 * to mess with Capsule allocation
 */

/* 
 * this is the area that holds (indirectly) the current capsule. It is
 * a PMem pointer to an array of elements. There is one element per
 * process. each element is another PMem pointer to the currently
 * installed capsule. The trampoline code in capsule.c atomically
 * swaps the installed capsule between bits of usercode or overhead
 * code.
 *
 * So you should deref, pick based on your hard whoAmI, and then deref
 * again to access the capsule
 */
extern PMem currentlyInstalled;

extern PMem trampolineQuit;
/*
 * construct a new capsule with associated overhead. the output of this
 * should be what is returned to the trampoline code
 */
Capsule makeCapsule(funcPtr_t);

void trampolineCapsule(void);
