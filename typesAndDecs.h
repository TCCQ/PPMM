#ifndef TYPES_HEADER
#define TYPES_HEADER

/* 
 * This file should have all the typedefs and constants that I will
 * want to use in more than 1 place. Also it should include the stuff
 * that might become a runtime setup constant later
 *
 * It should be motivationally prior to everything else, so it will
 * not include any of the other headers, and will not declare anything
 * like capsules or funcPtr_t or Jobs, which come later
 */

/* 
 * general readability and portability
 */
typedef unsigned char boolean;
typedef unsigned char byte;
#define true 1
#define false 0
#include <errno.h>

/* 
 * I don't really like having this here because it's not quite were it
 * needs to be, it should be with the other user visible stuff, but I
 * think its fine for the moment. Prevents some inclusion loops.
 * Consider having it have its own header
 */
typedef struct {
     unsigned int offset;
     unsigned int size;
} PMem;

/* 
 * scheduling constants
 */
#define NUM_PROC 4
#define STACK_SIZE 256
#define JOB_ARG_SIZE 256

/* 
 * Set / Join declarations
 */
#define LAST_EDIT_MASK 0x80
#define COUNTER_MASK 0x03
#define SET_POOL_SIZE 256

/* 
 * ProcMap constants
 */
#define PROCFS_LINE_SIZE 128
/* 
 * consider defining type for procIdx, shorter than int for better
 * swapping (short + byte + byte is 4, could be better than 8)
 */


/* 
 * pstack size constants
 */
#define PSTACK_HOLDING_SIZE (4*JOB_ARG_SIZE) 
#define PSTACK_STACK_SIZE (8 * (1 << 20))

/* 
 * memUtilities / Allocation constants
 */
#define DYNAMIC_POOL_SIZE (32 * (1 << 20))
//number * megabytes
#define MEMORY_TABLE_SIZE 2048
//number of entries in table

/* 
 * setup constants
 */
#define SETUP_PATH_LENGTH 128
#define SETUP_PROJECT_ID 'P'

/* 
 * total size of the persistent memory
 *
 * this calculating method is extremely unsafe, TODO improve
 */
#define TOTAL_PMEM_SIZE ((NUM_PROC * ((JOB_ARG_SIZE * 2 *STACK_SIZE) + PSTACK_STACK_SIZE + (2*PSTACK_HOLDING_SIZE))) + (MEMORY_TABLE_SIZE * 16) + DYNAMIC_POOL_SIZE)



/*
 * tell the kernel to schedule other stuff first.
 *
 * There does not seem to be any good header anywhere for this, so idk
 * really what to do with it. it goes here now I guess
 */
void yield(void); 

#endif
