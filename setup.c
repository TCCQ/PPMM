#include <sys/shm.h>
//TODO includes to get all the pointers to fill

/*
 * This file should setup the shared memory (TODO for now this is
 * volatile System V shared memory)
 *
 * This file should also fill in all of the pointers to shared data
 * that an individual process will need 
 */

//TODO readability, should these be defines?
const void* sharedRoot = 0x00000000; //the base address of the shared memory, pick something page aligned
const size_t sharedSize = 1 << (20 + 3); //megabyte + log_2(# of megabytes)

int sharedId;
void* sharedAttached;

int firstTimeSetup(key_t key) {/*TODO where is key_t defined? shm.h? */
     sharedId = shmget(key, sharedSize, IPC_CREAT | /* TODO permissions */);
     //get the segment or create one if it doesn't exist
     if (sharedId == -1) {
	  return -1; //could not get the segment, check errno
     }
     sharedAttached = shmat(sharedId, sharedRoot, 0);
     if (sharedAttached == (void*)-1 ) {
	  return -2; //couldn't attach the segment, check errno
     }

     /*
      * TODO fill in all the required personal pointers according to
      * procIdx, this function should be called *AFTER* a resurrected
      * proc recieves a procIdx
      */
}
