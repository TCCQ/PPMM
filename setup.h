/* 
 * this mounts the pmem and sets the base pointer
 */
void pmemMount(void);

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
void firstTimeInit(int myIdx);
