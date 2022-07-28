#ifndef persistentReturn
#include "scheduler.h"
#endif
#include "set.h"
#include "memUtilities.h"
/*
 * declarations for capsule code
 */

#define JOIN_SIZE 64
//needs to match the length of forkpath in bits
typedef unsigned char BYTE;
/* TODO need to ensure that ^ is exactly 1 byte */

typedef struct {
     funcPtr_t rstPtr; //where the work is for this capsule
     
     /*
      * each bit is the left/right status of the fork that lead to
      * this capsule. See fork and join in scheduler.c
      *
      * TODO all forking material here should be moved to the Job
      * struct, as this information only changes on scheduler
      * interaction, aka between Jobs
      */
     unsigned long long forkPath; //TODO I don't remember what I called this
     /*
      * This is an array of PM pointers to sets.
      * size should match bits in forkPath
      */
     PMem joinLocs[JOIN_SIZE];
     int joinHead; //which location is the one to join

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
      * TODO add a field for a soft whoAmI, set at the
      * scheduler->usercode edge. and add a getter (this could be
      * getProcIDX in memUtilities, that header is motivatioonally
      * prior to this, consider moving that to here)
      */
     int whoAmI; 
} Capsule;

Capsule quickGetInstalled(void);
int getProcIDX(void); //soft whoAmI, allows for hardfault steals

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

/*
 * construct a new capsule with associated overhead. the output of this
 * should be what is returned to the trampoline code
 */
Capsule makeCapsule(funcPtr_t);
