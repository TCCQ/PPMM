#include "capsule.h"
#include "assert.h"

/*
 * This is the file implimenting the capsule.h stuff. TODO move the interal-only declarations here
 */

Capsule* currentlyInstalled;
/*
 * this is PM pointer, and needs to be init somewhere TODO
 *
 * This should only be visible internally, and there should be exactly
 * 1 per process, so it should be declared here in a source file that
 * doesn't get included anywhere
 */

void retrieveCapsuleArguments(void* toFill, int size) {
     BYTE* inputByte = (BYTE*)(currentlyInstalled->arguments);
     BYTE* outputByte = toFill;
     for (int i = 0; i < size; i++) {
	  *(outputByte++) = *(inputByte++); //copy and increment both
     }
}

void installCapsule(Capsule) {
     /*
      * TODO unsure exactly how to do this, just want to atomically
      * write the capsule, this is not a racy write location, so I
      * don't really need a CAM here, but a CAM might be the easiest
      * way to have atomiticity
      */
}
/*
 * This installs and jumps to a capsule. 
 * TODO this seems super ugly, because it makes
 *
 * [work]
 * int ret = funcCall(arg)
 * [more work]
 *
 * into
 *
 * [work, including creating Capsule struct]
 * persistentCall(funcCap)
 * [end of current function/scope]
 * 
 * ret nextFunction(args) {
 *   [more work]
 * }
 *
 * which is long, ugly, and prone to error
 *
 * fixing requies a compiler I think
 */


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
     return output;
}

/*
 * do the running of capsules and facilitate the continuation
 * capsules. this is the true internal main loop. start by running
 * scheduler capsule, which should get the proper inner loop going.
 */
void trampolineCapsule(Capsule initial) {
     installCapsule(initial);
     Capsule next;
     // TODO make quiting mechinism
     while (true) { 
	  next = (*currentlyInstalled.rstPtr)(); //do the work and save the continuation
	  installCapsule(next); //sets currentlyInstalled
     }
}
