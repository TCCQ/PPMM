#include <assert.h>

/* 
 * This file declares some ways so do safety assertions in this code
 * base. Specifically there is a static assertion and a runtime
 * assertion. 
 */

//switch to enable runtime assertions, static are always enabled
#define DO_ASSERT 1

#define rassert(exp, message) #ifdef DO_ASSERT \
     assert(exp, message) \
     #endif


//static assertions, save some typing, requires C11 or later
#define sassert(exp, message) _Static_assert(exp, message)
