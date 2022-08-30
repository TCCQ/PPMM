#ifndef SETUP_HEADER
#define SETUP_HEADER

#include <sys/shm.h>
#include "typesAndDecs.h"
/* 
 * this mounts the pmem and sets the base pointer
 *
 * takes the shared memory key as an argument 
 */
void pmemMount(key_t, boolean);

/* 
 * this partitions the mounted memory by setting up all the global pmems that
 * other files expect. It can be run multiple times safely. The
 * mapping should be fixed (holding argument constants still) at
 * compile time.
 *
 * for debugging purposes, I might add padding with fixed
 * non-zero values between some of these segments
 */
void pmemPartition(void);

/* 
 * this sets up all the initial values of the global pmems. it should
 * be run exactly one time on the partitioned memory.
 */
void firstTimeInit();

/* 
 * local initialization that everybody needs. Takes hard wai
 */
void everybodyInit(int hard);

/* 
 * tell the OS that we are done with the shared memory, on success, we
 * are free to exit cleanly
 */
void pmemDetach(void);

#endif
