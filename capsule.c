#include "capsule.h"
#include "assert.h"

/*
 * This is the file implimenting the capsule.h stuff. TODO move the interal-only declarations here
 */



void retrieveCapsuleArguments(void* toFill, int size) {
     BYTE* inputByte = (BYTE*)(currentlyInstalled->arguments);
     BYTE* outputByte = toFill;
     for (int i = 0; i < size; i++) {
	  *(outputByte++) = *(inputByte++); //copy and increment both
     }
}

/* TODO make sure this is safe with NULL, 0 for no args */
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
 */
void trampolineCapsule(Capsule initial) {
     // TODO install the initial capsule
     Capsule next;
     // TODO make quiting mechinism
     while (true) { 
	  next = (*currentlyInstalled.rstPtr)(); //do the work and save the continuation
	  /* TODO install the capsule by switching currentlyInstalled
     }
}
