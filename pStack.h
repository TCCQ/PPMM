#ifndef PSTACK_HEADER
#define PSTACK_HEADER

#include "pStack-internal.h"
#include "memUtilities.h"
#include "capsule.h"
/* 
 * This is the file that declares the persistent stack functions that
 * should be used in the persistent code internally and in user code. 
 */

/* 
 * I need to define a persistent continuation (equivalent to a capsule
 * boundary), a persistent call, a persistent return, and tool for
 * managing the pStack's arguments
 */

/* 
 * Basically I want do just pass (&arg, sizeof(arg)) to the internal
 * functions, but I can't just do that cause I need to be able to use
 * computed/returned values. rvalues basically. 
 */

#define pPushCalleeArg(arg) {			\
	  __auto_type _arg = arg;		\
          ppcaaInternal(&(_arg), sizeof(_arg));	\
	  }

#define pPushCntArg(arg) {			\
	  __auto_type _arg = arg;		\
          ppcnaInternal(&(_arg), sizeof(_arg));	\
	  }

#define pPopArg(arg) ppaInternal(&arg, sizeof(arg))

#define pcnt(cnt) {					\
	  __auto_type _cnt = cnt;			\
	  ppcnaInternal(&(_cnt), sizeof(_cnt));		\
     }							\
     return makeCapsule(&pCntInternal)

#define pcall(callee, cnt) {				\
	  __auto_type _callee = callee;			\
	  __auto_type _cnt = cnt;			\
	  ppcnaInternal(&(_cnt), sizeof(_cnt));		\
	  ppcaaInternal(&(_callee), sizeof(_callee));	\
     }							\
     return makeCapsule(&pCallInternal)

#define pret(arg) {				\
     __auto_type _arg = arg;			\
     ppcnaInternal(&(_arg), sizeof(_arg));	\
     }						\
     return makeCapsule(&pRetInternal)

#define pretvoid return makeCapsule(&pRetInternal);

//these have to be defined and not calls cause they have to use the trampoline system

#endif
