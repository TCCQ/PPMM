#include "capsule.h"
#include "assert.h"

/*
 * This is the file implimenting the capsule.h stuff. 
 */

void retrieveCapsuleArguments(void* toFill, int size) {
     BYTE* inputByte = (BYTE*)(currentlyInstalled->arguments);
     BYTE* outputByte = toFill;
     for (int i = 0; i < size; i++) {
	  *(outputByte++) = *(inputByte++); //copy and increment both
     }
}

Capsule makeCapsule(funcPtr_t instructions, void* args, int size) {
     assert(argSize < ARGUMENT_SIZE);
     Capsule output;
     output.rstPtr = instructions;
     output.argSize = size;
     BYTE* inputByte = (BYTE*)args;
     BYTE* outputByte = output.arguments;
     for (int i = 0; i < size; i++) {
	  *(outputByte++) = *(inputByte++); //copy and increment both
     }
     //args have been filled
     
     /*
      * by default should copy the fork history from the caller
      *
      * TODO is there a better place to do this?
      */
     for (int i = 0; i < JOIN_SIZE; i++) {
	  output.joinLocs[i] = currentlyInstalled->joinLocs[i];
     }
     output.joinHead = currentlyInstalled->joinHead;
     return output;
}



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
 * capsule to installedSwap[0] or [1] and point *currentlyInstalled to
 * it, then start this loop. The first capsule (after all pre-fault
 * setup is done) should fork with the starting job and an job that
 * returns immediately. you might be able to use the scheduler instead
 * of a job that returns immediately.
 * 
 * TODO this needs to be declared somewhere internally visible so the
 * main loop can start. maybe in some seperate instal initialization
 * header?
 */
Capsule* installedSwap; //PM pointer to 2 entry capsule array. TODO init
void trampolineCapsule(void) {
     Capsule next;
     while (true) { // TODO make quiting mechinism
	  next = (*currentlyInstalled.rstPtr)(); //do the work and save the continuation

	  if (*currentlyInstalled == &(installedSwap[0])) {
	       //the first is installed
	       installedSwap[1] = next;
	       *currentlyInstalled = &(installedSwap[1]); //TODO make atomic
	  } else {
	       //the second is installed
	       installedSwap[0] = next;
	       *currentlyInstalled = &(installedSwap[0]); //TODO make atomic
	  }
     }
}
