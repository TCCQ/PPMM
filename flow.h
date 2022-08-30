#ifndef FLOW_HEADER
#define FLOW_HEADER
#include "capsule.h"
/*
 * This is the externally visible header for the calls that need to be
 * accessable from user code. Only the functions in this file,
 * memUtilities.h, and capsule.h should be used in usercode
 */

Capsule join(void); //implimented in set.c

Capsule ppmmfork(void); //implimented in scheduler.c

#endif
