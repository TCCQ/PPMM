#ifndef PSTACK_INT_HEADER
#define PSTACK_INT_HEADER

/* 
 * This is included from pStack.h and allows for some firewalling of
 * user and internal code. it contains the helper functions for the
 * pStack management
 */

#include "capsule.h"

/* 
 * These are so the trampoline and capsule making code can read and
 * reset them for the running process. They are declared in
 * pStack-internal.c
 */
extern PMem pStackDirty;
extern PMem cntHolderDirty;
extern PMem callHolderDirty;

extern PMem continuationHolders; 
extern PMem calleeHolders; 
extern PMem pStacks; 


//these act as PMems for their namesake
#define myStack (( (PMem*)PMAddr(pStacks) )[getProcIDX()])
#define myCntHolder (( (PMem*)PMAddr(continuationHolders) )[getProcIDX()])
#define myCalleeHolder (( (PMem*)PMAddr(calleeHolders) )[getProcIDX()])


void ppcaaInternal(void*, int); //pushing a callee arg

void ppcnaInternal(void*, int); //pushing a cnt arg

void ppaInternal(void*, int); //pop arg from pStack

/* 
 * these are helper functions that need to be used through the
 * trampoline system in order to cement the arguments they pop
 */

Capsule pCntInternal(void); //perform a continuation

Capsule pCallInternal(void); //perform a pCall

Capsule pRetInternal(void); //perform a pReturn
Capsule pRetCntInternal(void); //more returning

/*
 * manual override for pCnt, takes capsule instead of funcPtr, for
 * when the capsule needs to be edited, see forking
 */
Capsule pCntCap(void);


//defined here to keep it out of usercode
#define pCntManual(cap) ppcnaInternal(cap, sizeof(cap));	\
     return makeCapsule(&pCntCap);

/* 
 * takes starting arguments and continuation in cnt holding area, then
 * resets the stack, copies the arguments, and starts the continuation
 */
Capsule pStackReset(void);

#endif
