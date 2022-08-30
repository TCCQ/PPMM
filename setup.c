#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include "typesAndDecs.h"
#include "setup.h"
#include "assertion.h"
#include "memUtilities.h" //PMem and entry
#include "capsule.h" //Capsule
#include "scheduler.h" //Job
#include "procMap.h" //struct procData
#include "set.h" //Set
/*
 * This file should setup the shared memory (TODO for now this is
 * volatile System V shared memory)
 *
 * This file should also fill in all of the pointers to shared data
 * that an individual process will need 
 */

byte* basePointer; //where is the whole pmem chunk mounted?
PMem dynamicChunk;

/* 
 * We are defining this here so that basePointer does not need an
 * extended scope
 */
void* PMAddr(PMem input) {
     return (void*) basePointer + input.offset; 
}

/* 
 * this mounts the pmem and sets the base pointer
 */
void pmemMount(key_t key, boolean generate) {
     int shmId;

     //this is really the generating code, but it will work for now
     char selfPath[SETUP_PATH_LENGTH];
     int size = readlink("/proc/self/exe", selfPath, SETUP_PATH_LENGTH);
     if (size == -1) {
	  rassert(0, "could not get my own path name for setup purposes");
     }
     selfPath[size] = 0; //unclear if readlink does that?
     key_t generatedKey = ftok(selfPath, SETUP_PROJECT_ID);
     if (generatedKey == -1) {
	  rassert(0, "could not generate a key");
     }
     key = generatedKey;
     
     if (generate) {
	  
	  
	  int ret = shmget(key, TOTAL_PMEM_SIZE, IPC_CREAT | IPC_EXCL | 0600);
	  if (ret == -1) {
	       rassert(0, "could not create a shared segment, maybe already exists?");
	  } else {
	       //creation worked
	       shmId = ret;
	  }
     } else {
	  //segment already exists, do not create a new one
	  int ret = shmget(key, TOTAL_PMEM_SIZE, 0);
	  if (ret == -1) {
	       rassert(0, "could not get existing shared memory segment. Did you create it?");
	  } else {
	       shmId = ret;
	  }
     }

     //one way or another, we have a shm segment id now
     void* atRet = shmat(shmId, NULL, 0);
     //attach, read and write
     if (atRet == (void*)-1) {
	  rassert(0, "could not attach the shared memory segment");
     } else {
	  basePointer = (byte*) atRet;
     }

     int ret = shmctl(shmId, IPC_RMID, NULL);
     /* 
      * marks the segment for deletion, will only happen after all
      * processes have been detached from it. newly attaching to a
      * segment marked for deletion is only supported by some linux
      * versions, and is thus not portable. I am not sure about the
      * third argument, but I think this command doesn't use it.
      */
     if (ret == -1 && errno != EPERM) {
	  rassert(0, "could not mark the segment for deletion, but it wasn't a permissions issue");
     }
}

/* 
 * this partitions the mounted memory by setting up all the global pmems that
 * other files expect. It can be run multiple times safely. The
 * mapping should be fixed (holding argument constants still) at
 * compile time.
 *
 * for debugging purposes, I might add padding with fixed
 * non-zero values between some of these segments
 */
void pmemPartition(void) {
     PMem cursor;
     cursor.size = 0;
     cursor.offset = 0;
     //set to start of data segment

     /* 
      * see trampoline for quitting mechanism
      */
     cursor.size = sizeof(boolean);
     extern PMem trampolineQuit;
     trampolineQuit = cursor;
     cursor.offset += cursor.size;
	  
     /*
      * capsule.h
      * currentlyInstalled:
      * pointer to array, where each entry is a pointer to a capsule
      */
     cursor.size = sizeof(PMem) * NUM_PROC;
     extern PMem currentlyInstalled;
     currentlyInstalled = cursor;
     cursor.offset += cursor.size;
     
     /*
      * capsule.c
      * installedSwap:
      * pointer to two long array of capsules. used by trampoline
      */
     cursor.size = sizeof(Capsule) * 2 * NUM_PROC;
     extern PMem installedSwap;
     installedSwap = cursor;
     cursor.offset += cursor.size;
     
     /*
      * scheduler.h
      * deques:
      * pointer to NUM_PROC array of pointers, each to a STACK_SIZE of Jobs
      *
      * tops:
      * pointer to NUM_PROC array of ints
      *
      * bots:
      * pointer to NUM_PROC array of ints
      */
     cursor.size = sizeof(Job) * STACK_SIZE * NUM_PROC;
     //actual stacks
     cursor.offset += cursor.size;
     
     cursor.size = sizeof(PMem) * NUM_PROC;
     //pointer to stacks
     extern PMem deques;
     deques = cursor;
     cursor.offset += cursor.size;
     /*
      * consider one super long array, divide by NUM_PROC for small
      * arrays? check usage.
      */
     cursor.size = sizeof(int) * NUM_PROC;
     extern PMem tops;
     tops = cursor;
     cursor.offset += cursor.size;

     //same size
     extern PMem bots;
     bots = cursor;
     cursor.offset += cursor.size;
     

     
     /*
      * pStack-internal.h/.c
      * continuationHolders:
      * pointer to NUM_PROC array of pointers, each to (see types)
      * sized byte array
      *
      * calleeHolders:
      * same as above
      *
      * pStacks:
      * NUM_PROC array of pointers to byte array pStack, see types
      */
     cursor.size = PSTACK_HOLDING_SIZE * NUM_PROC;
     cursor.offset += 2 * cursor.size;
     //both holder's real size
     cursor.size = PSTACK_STACK_SIZE * NUM_PROC;
     //real stacks

     cursor.size = sizeof(PMem) * NUM_PROC;
     extern PMem continuationHolders;
     continuationHolders = cursor;
     cursor.offset += cursor.size;
     extern PMem calleeHolders;
     calleeHolders = cursor;
     cursor.offset += cursor.size;
     extern PMem pStacks;
     pStacks = cursor;
     cursor.offset += cursor.size;
     
     /*
      * procMap.h
      * mapping:
      * NUM_PROC array of procData structs
      */
     cursor.size = sizeof(struct procData) * NUM_PROC;
     extern PMem mapping;
     mapping = cursor;
     cursor.offset += cursor.size;
     
     /*
      * set.c
      * setPool:
      * SET_POOL_SIZE array of Sets
      */
     cursor.size = sizeof(Set) * SET_POOL_SIZE;
     extern PMem setPool;
     setPool = cursor;
     cursor.offset += cursor.size;
     
     /*
      * memUtilities.c
      * memTable:
      * MEMORY_TABLE_SIZE long array of entry structs.
      * 
      * (the base chunk for the dynamic memory is set during the init
      * of the memory table, and so I don't need to store it any where
      * I don't think. I need to reserve the space, (at the end?) but
      * I don't need to set anything specific to point to it)
      *
      * currently this works by using extern to bind local variables
      * to the global ones of the same name in other files. This may
      * not work or be the best way to do it, but it's a start
      */
     cursor.size = sizeof(entry) * MEMORY_TABLE_SIZE;
     extern PMem memTable;
     memTable = cursor;
     cursor.offset += cursor.size;

     /* 
      * dynamic memory
      */
     cursor.size = DYNAMIC_POOL_SIZE;
     dynamicChunk = cursor;
     cursor.offset += cursor.size;
}

/* 
 * this sets up all the initial values of the global pmems. it should
 * be run exactly one time on the partitioned memory.
 */
void firstTimeInit(void) {
     /* 
      * memUtilities.c:
      * memTable
      * is MEMORY_TABLE_SIZE long on entries, starts will all zeros
      * except first entry which is {{0,DYNAMIC_POOL_SIZE}, yes,
      * {0, no}, 0, no}
      */
     entry empty;
     empty.data.offset = 0;
     empty.data.size = 0;
     empty.inList = false;
     empty.tag.owner = 0;
     empty.tag.isGrabbed = false;
     empty.next = 0;
     empty.isInUse = false;

     extern PMem memTable;
     entry* table = (entry*)PMAddr(memTable);
     for (int i = 1; i < MEMORY_TABLE_SIZE; i++) {
	  table[i] = empty;
     }

     
     empty.data = dynamicChunk;
     empty.inList = true;
     //single entry for whole memory block
     table[0] = empty;

     /* 
      * procMap.c:
      * mapping
      * NUM_PROC of procData structs
      *
      * pick some pid that will never be allocated to this process
      */
     struct procData emptyProc = {1};
     extern PMem mapping;
     struct procData* map = (struct procData*)PMAddr(mapping);
     for (int i = 0; i < NUM_PROC; i++) {
	  map[0] = emptyProc;
     }

     /* 
      * set.c:
      * setPool
      * set to all zeros
      */
     Set emptySet;
     { int i = 0; while (i < sizeof(emptySet)) { *((byte*)&emptySet + i) = 0; i++;} }
     //clear it

     extern PMem setPool;
     Set* pool = (Set*)PMAddr(setPool);
     for (int i = 0; i < SET_POOL_SIZE; i++) {
	  pool[i] = emptySet;
     }

     /* 
      * scheduler.h:
      * tops/bots
      * set them all to zero
      */
     extern PMem tops, bots;
     int* ts = (int*)PMAddr(tops);
     int* bs = (int*)PMAddr(bots);
     for (int i =0; i < NUM_PROC; i++) {
	  ts[i] = 0;
	  bs[i] = 0;
     }

     /* 
      * deques
      * should all be empty with zero counter
      */
     extern PMem deques;
     Job emptyJob = newEmptyWithCounter(0);
     Job* jobLoc;
     PMem* individualDeque = (PMem*)PMAddr(deques);
     for (int p = 0; p < NUM_PROC; p++) {
	  jobLoc = (Job*)PMAddr(individualDeque[p]);
	  for (int i = 0; i < STACK_SIZE; i++) {
	       jobLoc[i] = emptyJob;
	  }
     }

     /* 
      * make sure that we aren't quiting off the bat
      */
     extern PMem trampolineQuit;
     boolean* q = PMAddr(trampolineQuit);
     *q = false;

}

/* 
 * every process needs to run this, whether they are the init process
 * or not. 
 */
void everybodyInit(int hard) {
     /* 
      * pStack-internal.c/.h:
      * *Dirty
      *
      * Needs to point to the bottom of the holders
      */     

     extern PMem pStacks, continuationHolders, calleeHolders,
	  pStackDirty, cntHolderDirty, callHolderDirty;
     pStackDirty = ((PMem*)PMAddr(pStacks))[hard];
     pStackDirty.size = 0;
     cntHolderDirty = ((PMem*)PMAddr(continuationHolders))[hard];
     cntHolderDirty.size = 0;
     callHolderDirty = ((PMem*)PMAddr(calleeHolders))[hard];
     callHolderDirty.size = 0;

     /* 
      * each person sets up their own currently Installed pointer
      */
     extern PMem currentlyInstalled;
     extern PMem installedSwap;

     //we want a pmem to the first of the pair in installedSwap
     //assocaited with our hard wai
     PMem startingInstalled;
     startingInstalled.size = sizeof(Capsule);
     startingInstalled.offset = installedSwap.offset + (sizeof(Capsule)*2)*hard;
     
     ((PMem*)PMAddr(currentlyInstalled))[hard] = startingInstalled;
}

void pmemDetach(void) {
     int ret = shmdt((void*)basePointer);
     if (ret == -1) {
	  rassert(0, "could not detach segment");
     }
}
