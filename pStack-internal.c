/* 
 * This is the implimentations of the functions in pStack-internal.h
 * (and in effect the user stuff from pStack.h)
 */

#include "capsule.h"
#include "memUtilities.h"
#include "pStack-internal.h"
#include "pStack.h"

/*
 * TODO 100% sure this is done somewhere else in my code and I should
 * just switch all of them to some system header
 */
#define byte unsigned char

/* 
 * These are the equivalent of dereferencing the clean versions (in
 * the capsule) once. They are reset on fault (TODO) and are read from
 * on capsule turnover
 *
 * Their sizes are not the # of bytes after that are valid, but the #
 * from the start of the holder (pStack's is not meaningful)
 *
 * The clean dirty copying happens in the creation of a new capsule.
 * 
 * these are the real definitions, the .h ones are extern
 */
PMem pStackDirty;
PMem cntHolder;
PMem callHolder;

/* 
 * I need a pStack for each process and a pair of holding areas for
 * capsules (this forces maximum capsule + argument size, but
 * whatever). They all need to be cross accessable
 *
 * These are arrays of PMems for holding areas and pStacks.
 * Dereference, pick yours, deref again to get to the data. All of
 * this because they need to be cross accessible for hard fault steals
 *
 * again real def here, others are extern
 *
 * TODO the holders need to be 2*(JOB_ARG_SIZE + sizeof(funcPtr_t)) sized at least
 */

PMem continuationHolders; 
PMem calleeHolders; 
PMem pStacks; 


/* 
 * This func pushes an argument to be used by the callee, it reads and
 * edits the pointer callee holder area
 */
void ppcaaInternal(void* input, const int size) {
     byte* output = (byte*)PMAddr(callHolderDirty);
     for (int i = 0; i < size; i++) {
	  *(output++) = *(((byte*)input)++);
     }
     callHolderDirty.offset += size;
     callHolderDirty.size += size;
}

//same thing but for cnt
void ppcnaInternal(void* input, const int size) {
     byte* output = (byte*)PMAddr(cntHolderDirty);
     for (int i = 0; i < size; i++) {
	  *(output++) = *(((byte*)input)++);
     }
     cntHolderDirty.offset += size;
     cntHolderDirty.size += size;
}

//pop from stack
void ppaInternal(void* output, const int size) {
     byte* input = (byte*)PMAddr(pStackDirty) - size; //start from bottom, move up
     for (int i = 0; i < size; i++) {
	  *(((byte*)output)++) = *(input++);
     }
     pStackDirty.offset -= size; //this is a pop, move down
}

/* 
 * so the dirty versions are set. the dirty stack should be right
 * above caller of the caller of this. (where to return to when I am
 * done), there should *NOT* be any remnants of the pre-cnt capsule on
 * the stack. the dirty stack needs to be pushed as an argument so
 * that it is preserved on fault. TODO think about ifthat is the best solution
 *
 * The top element that is copied at this stage is the funcPtr_t of
 * the continuation. This should be popped by this code and returned
 * so that it gets executed *after* the new clean stack pointer is
 * copied from the dirty version
 */
Capsule pCntInternal(void) {
     int howMuchToCopy = cntHolderDirty.size;
     byte* input = (byte*)PMAddr(cntHolderDirty);
     byte* output = (byte*) PMAddr(pStackDirty);
     while (howMuchToCopy--) {
	  *(output++) = *(input++);
     }
     pStackDirty.offset += cntHolderDirty.size;
     //now the stack has all the stuff, with the funcPtr at the top
     funcPtr_t continuation;
     pPopArg(continuation);
     //dirty stack is set as it should be for user code
     
     return makeCapsule(continuation); //this fills forkpath, sets stackClean, and resets holders
}

/* 
 * basically just do two versions of the cnt. push the cnt args, the
 * cnt rstPtr, the callee args, and the callee rstPtr, then pop the
 * pointer and make a capsule
 */
Capsule pCallInternal(void) {
     int copy = cntHolderDirty.size;
     byte* input = (byte*)PMAddr(cntHolderDirty);
     byte* output = (byte*)PMAddr(pStackDirty);
     while (copy--) {
	  *(output++) = *(input++);
     }
     input = (byte*)PMAddr(callHolderDirty);
     copy = callHolderDirty.size;
     while (copy--) {
	  *(output++) = *(input++);
     }
     pStackDirty.offset += cntHolderDirty.size + callHolderDirty.size;

     funcPtr_t cnt;
     pPopArg(cnt);

     return makeCapsule(cnt); //does dirty/clean turnover and other overhead
}

/* 
 * The stack pointer should be just above the rstPtr of the caller,
 * and the args for the cnt should be just below that. pop and go
 */
Capsule pRetInternal(void) {
     funcPtr_t next;
     pPopArg(next);

     return makeCapsule(next); //does overhead
}


//same as regular pCnt, but doesn't try to make the capsule, just takes one as argument
Capsule pCntCap(void) {
     int howMuchToCopy = cntHolderDirty.size;
     byte* input = (byte*)PMAddr(cntHolderDirty);
     byte* output = (byte*) PMAddr(pStackDirty);
     while (howMuchToCopy--) {
	  *(output++) = *(input++);
     }
     pStackDirty.offset += cntHolderDirty.size;
     //now the stack has all the stuff, with the funcPtr at the top
     Capsule continuation;
     pPopArg(continuation);
     //dirty stack is set as it should be for user code
     
     return continuation; 
}