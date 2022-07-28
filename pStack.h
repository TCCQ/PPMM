/* 
 * This is the file that declares the persistent stack functions that
 * should be used in the persistent code internally and in user code. 
 */

#include "pStack-internal.h"
#include "memUtilities.h"
#include "capsule.h"

/* 
 * I need to define a persistent continuation (equivalent to a capsule
 * boundary), a persistent call, a persistent return, and tool for
 * managing the pStack's arguments
 */

#define pPushCalleeArg(arg) ppcaaInternal(&arg, sizeof(arg))

#define pPushCntArg(arg) ppcnaInternal(&arg, sizeof(arg))

#define pPopArg(arg) ppaInternal(&arg, sizeof(arg))

#define pcnt(cnt) ppcnaInternal(&cnt, sizeof(cnt)); \
     return makeCapsule(&pCntInternal)

#define pcall(callee, cnt) ppcnaInternal(&cnt, sizeof(cnt)); \
     ppcaaInternal(&callee, sizeof(callee)); \
     return makeCapsule(&pCallInternal)

#define pret(arg) ppcnaInternal(&arg, sizeof(arg)); \
     return makeCapsule(&pRetInternal)

//these have to be defined and not calls cause they have to use the trampoline system




