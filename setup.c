#include <sys/shm.h>
#include "typesAndDecs.h"
#include "setup.h"
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

void* basePointer; //where is the whole pmem chunk mounted?

/* 
 * this mounts the pmem and sets the base pointer
 */
void pmemMount(void) {
     
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
      * capsule.h
      * currentlyInstalled:
      * pointer to array, where each entry is a pointer to a capsule
      */
     cursor.size = sizeof(PMem) * NUM_PROC;
     extern PMem currentlyInstalled = cursor;
     cursor.offset += cursor.size;
     
     /*
      * capsule.c
      * installedSwap:
      * pointer to two long array of capsules. used by trampoline
      */
     cursor.size = sizeof(Capsule) * 2 * NUM_PROC;
     extern PMem installedSwap = cursor;
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
     extern PMem deques = cursor;
     cursor.offset += cursor.size;
     /*
      * consider one super long array, divide by NUM_PROC for small
      * arrays? check usage.
      */
     cursor.size = sizeof(int) * NUM_PROC;
     extern PMem tops = cursor;
     cursor.offset += cursor.size;

     //same size
     extern PMem bots = cursor;
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
     extern PMem continuationHolders = cursor;
     cursor.offset += cursor.size;
     extern PMem calleeHolders = cursor;
     cursor.offset += cursor.size;
     extern PMem pStacks = cursor;
     cursor.offset += cursor.size;

     
     
     /*
      * procMap.h
      * mapping:
      * NUM_PROC array of procData structs
      */
     cursor.size = sizeof(struct procData) * NUM_PROC;
     extern PMem mapping = cursor;
     cursor.offset += cursor.size;
     
     /*
      * set.c
      * setPool:
      * SET_POOL_SIZE array of Sets
      */
     cursor.size = sizeof(Set) * SET_POOL_SIZE;
     extern PMem setPool = cursor;
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
     extern PMem memTable = cursor;
     cursor.offset += cursor.size;

     /* 
      * I also need a section for setting pointers of things that
      * don't need allocated memory (like the dirty pointer in pStack-internal.c)
      *
      * TODO
      */
}

/* 
 * this sets up all the initial values of the global pmems. it should
 * be run exactly one time on the partitioned memory.
 */
void firstTimeInit(int myIdx) {

}
