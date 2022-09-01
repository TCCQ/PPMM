#include "capsule.h"
#include "pStack-internal.h"
#include "pStack.h"
#include "procMap.h" //for hardWhoAmI
#include "typesAndDecs.h"

PMem currentlyInstalled; //see capsule.h
PMem trampolineQuit;

Capsule makeCapsule(funcPtr_t instructions) {
     Capsule output;
     output.rstPtr = instructions;

     Capsule installed = quickGetInstalled();
     
     output.joinLoc = installed.joinLoc;
     output.forkSide = installed.forkSide;
     output.expectedOwner = installed.expectedOwner;
     output.whoAmI = installed.whoAmI;
     
     /* 
      * make the clean version match the dirty version
      *
      * The holders are reset to their base value, as determined by a
      * soft whoAmI, see the capsule dec in capsule.h. 
      */
     output.pStackHeadClean = pStackDirty; 
     output.cntHolderClean = cntHolderDirty;
     output.callHolderClean = callHolderDirty; 
     
     return output;
}

/* 
 * prevent having to retype all this garbage all the time to access
 * current capsule. (hopefully I don't need to do that often anyway)
 */
Capsule quickGetInstalled(void) {
     int hard = hardWhoAmI();
     PMem myPointer = ( (PMem*)PMAddr(currentlyInstalled) )[hard];
     return *( (Capsule*) PMAddr(myPointer) );
}

/*
 * This is a soft whoAmI
 * 
 * really a getter for a field in the currently installed capsule, but
 * used in code as a id for which process this is. Does not actually
 * respect that because we need to be able to handle hard fault steals
 * that need to use proc local data
 */
int getProcIDX(void) {
     return quickGetInstalled().whoAmI;
}

/* 
 * sets soft whoamI from hard. edits current. use with pcall, takes
 * and returns nothing
 */
Capsule resetWhoAmI(void) {
     //same as quick get, but edit instead of return
     int hard = hardWhoAmI();
     PMem myPointer = ( (PMem*)PMAddr(currentlyInstalled) )[hard];
     ( (Capsule*) PMAddr(myPointer) )->whoAmI = hard;
     pretvoid;
}

/* 
 * call from usercode that we are done here. This affects all processes
 */
Capsule shutdown(void) {
     *((boolean*)PMAddr(trampolineQuit)) = true;
     return makeCapsule(&shutdown); //have to return some capsule
}

/* 
 * this is a true process local variable. it is a convience pointer to
 * the 2long array of capsules that this process swaps between. The current
 * capsule needs to be cross accessable, but the off capsule for any
 * process is only meaningful for that process. on hard fault, the
 * steal moves the currently installed capsule to the thief's area, so
 * this is never seen by anyone other than the owner
 */
PMem installedSwap; 

/*
 * do the running of capsules and facilitate the continuation
 * capsules. this is the true internal main loop. start by running
 * scheduler capsule, which should get the proper inner loop going.
 
 * if a fault happens before the *currentlyInstalled assignment, then
 * the last capsule is run again. if it happens after, then on restart
 * it will read from currentlyInstalled, and start in the installed
 * capsule.
 *
 * to start the loop for the first time, then write the very first
 * capsule to `installedSwap[0]` or `[1]` and point
 * `*currentlyInstalled` to it, then start this loop. The first
 * capsule (after all pre-fault setup is done) should fork with the
 * starting job and an job that returns immediately. you might be able
 * to use the scheduler instead of a job that returns immediately.
 * 
 * TODO ensure the atomics writes work properly here
 */
void trampolineCapsule(void) {
     Capsule next;
     int hard = hardWhoAmI();
     Capsule* installedOne = ( (Capsule*)PMAddr(installedSwap) ) + 2*hard;
     Capsule* installedTwo = ( (Capsule*)PMAddr(installedSwap) ) + 2*hard + 1;
     PMem* pointerToCurrent = (PMem*)PMAddr(currentlyInstalled) + hard;
     while (!( *((boolean*)PMAddr(trampolineQuit))) ) { 
	  

	  /*
	   * TODO flush cache BEFORE cap change over
	   *
	   * so there is a gcc builtin, but its documentation mentions
	   * only instruction caches, and we want data caches.
	   * Specifically we need to flush the full mapped shared
	   * persistent memory, and everything else can be untouched.
	   * The system call "cacheflush" in sys/cachectl.h seems to
	   * allow for data cache flushing, but does not seem super
	   * portable. Finally there may be some flushing possible
	   * through whatever we are using to do the mapping, but I am
	   * not sure.
	   *
	   * Slight worries about memory ordering and the order of
	   * writing back to the shared visible memory. If those
	   * writes are not in the same order as they are in code then
	   * it is possible that we could have issues. Absolutely no
	   * clue what is to be done about that, so I am not going to
	   * worry about it at the moment. Hopefully it doesn't become
	   * a problem.
	   */
	  

	  Capsule current = *(Capsule*) PMAddr(*pointerToCurrent);

	  pStackDirty = current.pStackHeadClean;
	  cntHolderDirty = current.cntHolderClean;
	  callHolderDirty = current.callHolderClean;
	  
	  next = (current.rstPtr)(); //do the work and save the next capsule

	  if (pointerToCurrent->offset == installedSwap.offset + sizeof(Capsule)*2*hard) { 
	       //the first is installed, copy into second
	       *installedTwo = next;

	       PMem pointerToSecond;
	       pointerToSecond.size = sizeof(Capsule);
	       pointerToSecond.offset = installedSwap.offset + (2*hard+1)*sizeof(Capsule);
	       
	       __atomic_store(pointerToCurrent,
			      &pointerToSecond,
			      __ATOMIC_SEQ_CST); 
	  } else {
	       //the second is installed, copy into first
	       *installedOne = next;

	       PMem pointerToFirst;
	       pointerToFirst.size = sizeof(Capsule);
	       pointerToFirst.offset = installedSwap.offset + (2*hard)*sizeof(Capsule);
	       
	       __atomic_store(pointerToCurrent,
			      &pointerToFirst,
			      __ATOMIC_SEQ_CST); 
	  }
     }
     //we can just cleanly exit here and drop back to main to do the cleanup
}
