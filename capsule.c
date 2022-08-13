#include "capsule.h"
#include "pStack-internal.h"
#include "pStack.h"
#include "procMap.h" //for hardWhoAmI
#include "typesAndDecs.h"

PMem currentlyInstalled; //see capsule.h

Capsule makeCapsule(funcPtr_t instructions) {
     Capsule output;
     output.rstPtr = instructions;
     
     /*
      * by default should copy the fork history from the caller, this
      * can be reset as nessiary
      */
     for (int i = 0; i < JOIN_SIZE; i++) {
	  output.joinLocs[i] = (*currentlyInstalled)->joinLocs[i];
     }
     output.joinHead = currentlyInstalled->joinHead;

     /* 
      * make the clean version match the dirty version
      *
      * The holders are reset to their base value, as determined by a
      * soft whoAmI, see the capsule dec in capsule.h. 
      */
     output.pStackHeadClean = pStackDirty; 
     output.cntHolderClean = myCntHolder;
     output.callHolderClean = myCalleeHolder; 
     
     return output;
}

/* 
 * prevent having to retype all this garbage all the time to access
 * current capsule. (hopefully I don't need to do that often anyway)
 */
Capsule quickGetInstalled(void) {
     int hardWhoAmI = hardWhoAmI();
     PMem myPointer = ( (PMem*)PMAddr(currentlyInstalled) )[hardWhoAmI];
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
     return quickGetInstalled.whoAmI;
}


/* 
 * this is a true process local variable. it is a convience pointer to
 * the 2long array of capsules that this process swaps between. The current
 * capsule needs to be cross accessable, but the off capsule for any
 * process is only meaningful for that process. on hard fault, the
 * steal moves the currently installed capsule to the thief's area, so
 * this is never seen by anyone other than the owner
 *
 * TODO init (should be declared extern somewhere to access)
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
 * TODO this needs to be declared somewhere internally visible so the
 * main loop can start. maybe in some seperate initialization header?
 *
 * TODO ensure the atomics writes work properly here
 */
void trampolineCapsule(void) {
     Capsule next;
     PMem pointerToCurrent;
     int hardWhoAmI = hardWhoAmI();
     while (true) { // TODO make quiting mechinism


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
	  
	  /*
	   * This is the first part of the quick get code, but without the last deref
	   */
	  pointerToCurrent = ( (PMem*)PMAddr(currentlyInstalled) )[hardWhoAmI];
	  
	  next = ((*( (Capsule*) PMAddr(myPointer) )).rstPtr)(); //do the work and save the next capsule

	  if (pointerToCurrent.offset == installedSwap.offset) { 
	       //the first is installed, copy into second
	       *(((Capsule*) PMAddr(installedSwap) )+1) = next;
	       
	       __atomic_store_n((Capsule*) PMAddr(pointerToCurrent), ((Capsule*) PMAddr(installedSwap))+1, __ATOMIC_SEQ_CST); 
	  } else {
	       //the second is installed, copy into first
	       *((Capsule*) PMAddr(installedSwap) ) = next;
	       __atomic_store_n((Capsule*) PMAddr(pointerToCurrent), (Capsule*) PMAddr(installedSwap), __ATOMIC_SEQ_CST); 
	  }
     }
}
