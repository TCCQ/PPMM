#ifndef ASSERTION_HEADER
#define ASSERTION_HEADER

#include <assert.h>
#include <stdio.h>
/* 
 * This file declares some ways so do safety assertions in this code
 * base. Specifically there is a static assertion and a runtime
 * assertion. 
 */

//switch to enable runtime assertions, static are always enabled
#define DO_ASSERT 1

/* 
 * TODO I guess I thought asserts could take messages? idk
 */
#ifdef DO_ASSERT
#define rassert(exp, message) { if (exp) { perror(message); assert(0); } }
#else
#define rassert(exp, message) 
#endif


//static assertions, save some typing, requires C11 or later
#define sassert(exp, message) _Static_assert(exp, message)

#endif
