/* 
 * This is the implimentations of the functions in pStack-internal.h
 * (and in effect the user stuff from pStack.h)
 */

#include "capsule.h"
#include "memUtilities.h"
#include "pStack-internal.h"
#include "pStack.h"
#include "typesAndDecs.h"

/* 
 * These are the equivalent of dereferencing the clean versions (in
 * the capsule) once (TODO is that right? confirm with code). They are
 * reset on fault (TODO) and are read from on capsule turnover
 *
 * Their sizes are not the # of bytes after that are valid, but the #
 * from the start of the holder 
 *
 * The clean dirty copying happens in the creation of a new capsule.
 * 
 * these are the real definitions, the .h ones are extern
 */
PMem pStackDirty;
PMem cntHolderDirty;
PMem callHolderDirty;

/* 
 * I need a pStack for each process and a pair of holding areas for
 * capsules (this forces maximum capsule + argument size, but
 * whatever). They all need to be cross accessible
 *
 * These are arrays of PMems for holding areas and pStacks.
 * Dereference, pick yours, deref again to get to the data. All of
 * this because they need to be cross accessible for hard fault steals
 *
 * again real def here, others are extern
 */

PMem continuationHolders; 
PMem calleeHolders; 
PMem pStacks; 



/* 
 * This func pushes an argument to be used by the callee, it reads and
 * edits the pointer callee holder area
 */
void ppcaaInternal(void* input, const int size) {
     byte* in = (byte*)input;
     byte* output = (byte*)PMAddr(callHolderDirty);
     for (int i = 0; i < size; i++) {
	  *(output++) = *(in++);
     }
     callHolderDirty.offset += size;
     callHolderDirty.size += size;
}

//same thing but for cnt
void ppcnaInternal(void* input, const int size) {
     byte* in = (byte*)input;
     byte* output = (byte*)PMAddr(cntHolderDirty);
     for (int i = 0; i < size; i++) {
	  *(output++) = *(in++);
     }
     cntHolderDirty.offset += size;
     cntHolderDirty.size += size;
}

//pop from stack
void ppaInternal(void* output, const int size) {
     byte* input = (byte*)PMAddr(pStackDirty) - size;
     //start from bottom, move up
     byte* out = (byte*) output;
     for (int i = 0; i < size; i++) {
	  *(out++) = *(input++);
     }
     pStackDirty.offset -= size; //this is a pop, move down
     pStackDirty.size -= size;
}

/* 
 * so the dirty versions are set. the dirty stack should be right
 * above caller of the caller of this. (where to return to when I am
 * done), there should *NOT* be any remnants of the pre-cnt capsule on
 * the stack.
 *
 * The top element that is copied at this stage is the funcPtr_t of
 * the continuation. This should be popped by this code and returned
 * so that it gets executed *after* the new clean stack pointer is
 * copied from the dirty version
 */
Capsule pCntInternal(void) {
     int howMuchToCopy = cntHolderDirty.size;
     byte* input = (byte*)PMAddr(myCntHolder);
     byte* output = (byte*) PMAddr(pStackDirty);
     while (howMuchToCopy--) {
	  *(output++) = *(input++);
     }
     pStackDirty.offset += cntHolderDirty.size;
     pStackDirty.size += cntHolderDirty.size;
     //now the stack has all the stuff, with the funcPtr at the top

     cntHolderDirty.offset -= cntHolderDirty.size;
     cntHolderDirty.size = 0;
     //reset the holder
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
     byte* input = (byte*)PMAddr(myCntHolder);
     byte* output = (byte*)PMAddr(pStackDirty);
     while (copy--) {
	  *(output++) = *(input++);
     }
     input = (byte*)PMAddr(myCalleeHolder);
     copy = callHolderDirty.size;
     while (copy--) {
	  *(output++) = *(input++);
     }
     pStackDirty.offset += cntHolderDirty.size + callHolderDirty.size;
     pStackDirty.size += cntHolderDirty.size + callHolderDirty.size;

     cntHolderDirty.offset -= cntHolderDirty.size;
     cntHolderDirty.size = 0;
     callHolderDirty.offset -= callHolderDirty.size;
     callHolderDirty.size = 0;
     
     //reset the holders
     
     funcPtr_t cnt;
     pPopArg(cnt);

     return makeCapsule(cnt); //does dirty/clean turnover and other overhead
}

/* 
 * The stack pointer should be just above the rstPtr of the caller,
 * and the args for the cnt should be just below that. pop and go. The
 * return value, if there is one, is in the cntHolder
 */
Capsule pRetInternal(void) {
     funcPtr_t next;
     pPopArg(next);
     
     pPushCalleeArg(next);
     
     //store pointer in the callee holder
     //the return argument is in the cnt holder, now put the next
     //capsule on top of it (retCnt), and move both it and the arg to
     //the stack
     
     pcnt(&pRetCntInternal);
     /* 
      * THIS RELIES ON THE FACT THAT PCNT DOES *NOT* CLEAR THE CALL HOLDER
      */
}

/* 
 * confusing name. this is the second half of returning a value. we
 * need to mix some data around and it requires a second cap boundary
 */
Capsule pRetCntInternal(void) {
     //now the return arg is on the stack and the return funcPtr_t is
     //in the call holder
     funcPtr_t next;
     byte* input = (byte*)PMAddr(myCalleeHolder);
     byte* output = (byte*) &next;
     for (int i = 0; i < sizeof(next); i++) {
	  *(output++) = *(input++);
     }

     callHolderDirty.offset -= callHolderDirty.size;
     callHolderDirty.size = 0;
     //empty call holder
     pcnt(next);
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
     pStackDirty.size += cntHolderDirty.size;
     //now the stack has all the stuff, with the capsule at the top
     Capsule continuation;
     pPopArg(continuation);
     //dirty stack is set as it should be for user code
     
     return continuation; 
}

//set the 
Capsule pStackReset(void) {
     pStackDirty.offset -= pStackDirty.size;
     pStackDirty.size = 0;
     //stack is now empty
     int howMuchToCopy = cntHolderDirty.size;
     byte* input = (byte*)PMAddr(cntHolderDirty);
     byte* output = (byte*) PMAddr(pStackDirty);
     while (howMuchToCopy--) {
	  *(output++) = *(input++);
     }
     pStackDirty.offset += cntHolderDirty.size;
     pStackDirty.size += cntHolderDirty.size;
     //now the stack has all the stuff, with the capsule at the top
     Capsule continuation;
     pPopArg(continuation);
     //dirty stack is set as it should be for user code
     
     return continuation; 
}
