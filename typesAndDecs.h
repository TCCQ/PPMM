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


/* 
 * scheduling constants
 */
#define NUM_PROC 4
#define STACK_SIZE 256
#define JOB_ARG_SIZE 256

/* 
 * capsule / forkpath constants
 *
 * TODO do I need this?
 */
#define JOIN_SIZE 64
//needs to match the length of forkpath in bits

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
