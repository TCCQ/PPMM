/* 
 * This is the file that should start everything else.
 */
#include <stdlib.h> //for rand
#include "capsule.h"
#include "typesAndDecs.h"
#include "setup.h"
#include "procMap.h"
#include "scheduler.h"
#include "pStack-internal.h"

/* 
 * define the control flags as globals here to be set during parse and
 * read during main
 */
boolean doInit;
boolean startingProc;

/* 
 * returns the index of the first non-caught arg (args to be passed to
 * usercode). returns 0 if there are no such arguments
 */
int parseArgs (int ac, char** av) {
     return 0;
}

int main(int argc, char** argv) {
     srand(time(NULL));

     int skip = parseArgs(argc, argv);

     pmemMount(); //we have access to shared mem. TODO pass key?

     pmemPartition(); //we know where everything is
     
     if (doInit) {
	  //this proc should start things off
	  firstTimeInit();
     }

     
     int hard = getIdx();
     
     Capsule* curInst = PMAddr(*( (PMem*)PMAddr(currentlyInstalled) + hard ));
     
     if (startingProc) {
	  /* 
	   * start the trampoline code on schedule with the initial
	   * job in the first entry
	   */

	  extern funcPtr_t ppmmStartCap;

	  Capsule first = makeCapsule(&ppmmStartCap);
	  first.forkSide = 0;
	  first.joinLoc = 0;
	  first.whoAmI = hard;

	  //should be everything? pStack stuff should be safely read
	  //as zero from initial setup

	  Job fJob = newEmptyWithCounter(0);
	  fJob.work = first;
	  fJob.argSize = 0;
	  /*
	   * TODO add argument passing to init capsule. have to push
	   * backward or make some wrappers about how to pass char
	   * strings as arguments in pstack.
	  
	  int letter;
	  int cursor = 0;
	  for (int arg = argc; arg >= skip; arg--) {
	       letter = 0;
	       while (argv[arg][letter] != 0) {
		    
	       }
	  }
	  */

	  Capsule overhead = makeCapsule(&pCntInternal);
	  overhead.forkSide = 0;
	  overhead.joinLoc = 0;
	  overhead.whoAmI = hard;
	  
	  pPushCntArg(fJob);
	  pPushCntArg(&singleJobPush);
	  
	  
	  *curInst = overhead;

	  /* 
	   * So this reads from currentlyInstalled, executes it, and
	   * then starts the loop from there. The current capsule at
	   * start is some overhead to call the singlePushJob function
	   * to put the initial job on the scheduling deque. Then
	   * singlePushJob will turn over into the regular scheduling
	   * code and we are off to the races. 
	   */
	  trampolineCapsule();

	  
	  
     } else {
	  //not in charge of init, just start trampoline with
	  //scheduling code
	  Capsule sched = makeCapsule(&scheduler);
	  sched.forkSide = 0;
	  sched.joinLoc = 0;
	  sched.whoAmI = hard;
	  
	  *curInst = sched;
	  trampolineCapsule();
     }
}
     
