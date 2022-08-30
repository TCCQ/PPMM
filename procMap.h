#ifndef PROC_MAP_HEADER
#define PROC_MAP_HEADER

#include <unistd.h>

/* 
 * this struct needs to be visible from setup, so it is in a header
 */

struct procData {
     pid_t pid; 
};

/* 
 * this is a struct just in case I want to edit it later, if not
 * consolidate to just pid_t in the functions below
 */

boolean isLive(int);
int hardWhoAmI(void);
int getIdx(void);
int getMyPid(void);

#endif
