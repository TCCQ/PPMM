/* 
 * This is the file that should start everything else.
 */
#include <stdlib.h> //for rand
#include <time.h> //for time
#include <string.h>
#include <sys/shm.h> //for key_t
#include "capsule.h"
#include "typesAndDecs.h"
#include "setup.h"
#include "procMap.h"
#include "scheduler.h"
#include "assertion.h"
#include "pStack.h"
#include "pStack-internal.h"
#include "tests.h"
/* 
 * define the control flags as globals here to be set during parse and
 * read during main
 */
boolean doInit;
boolean startingProc;
boolean generate;
boolean testing;

key_t passedKey;

/* 
 * returns the index of the first non-caught arg (args to be passed to
 * usercode). returns 0 if there are no such arguments
 */
int parseArgs (int ac, char** av) {
     generate = false;
     doInit = false;
     startingProc = false;
     testing = false;

     boolean gotKeyOrGenerating = false;
     printf("%i\n", ac);
     for (int i = 1; i < ac; i++) {
	  printf("%s\n", av[i]);
	  if (strcmp(av[i], "-g") == 0) {
	       generate = true;
	       gotKeyOrGenerating = true;
	  } else if (strcmp(av[i], "-i") == 0) {
	       doInit = true;
	  } else if (strcmp(av[i], "-t") == 0) {
	       testing = true;
	  } else if (strcmp(av[i], "-s") == 0) {
	       startingProc = true;
	  } else if (strcmp(av[i], "-k") == 0) {
	       //next should be the key number
	       if (ac <= i+1) rassert(0, "you didn't pass a key");

	       passedKey = atoi(av[i+1]);
	       gotKeyOrGenerating = true;
	  } else {
	       /* if (!gotKeyOrGenerating) rassert(0, "pass a key or generate one"); */
	       return i;
	  }
     }
     //if (!gotKeyOrGenerating) rassert(0, "pass a key or generate one");
     return 0;
}

int main(int argc, char** argv) {
     srand(time(NULL));

     int skip = parseArgs(argc, argv);

     pmemMount(passedKey, generate); //we have access to shared mem 

     pmemPartition(); //we know where everything is
     
     if (doInit) {
	  //this proc should start things off
	  firstTimeInit();
     }

     
     int hard = getIdx();

     everybodyInit(hard);

     
     Capsule* curInst = (Capsule*) PMAddr( (( (PMem*)PMAddr(currentlyInstalled) )[hard]) );
     
     if (startingProc) {
	  /* 
	   * start the trampoline code on schedule with the initial
	   * job in the first entry
	   */

	  extern funcPtr_t ppmmStartCap;

	  Capsule first = makeCapsule(ppmmStartCap);
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
     } else if (testing) {
	  Capsule test = makeCapsule(&testMain);
	  test.forkSide = 0;
	  test.joinLoc = 0;
	  test.whoAmI = hard;
	  *curInst = test;
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
     pmemDetach();
}
     
