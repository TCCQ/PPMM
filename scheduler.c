#include "scheduler.h"
#include "capsule.h" /* for commiting capsules (installing), fork, and persistence */
#include "set.h" /* used in fork */
#include "flow.h" //fork declaration
#include "memUtilities.h" //PMem
#include "pStack.h" //persistent flow operations
#include "pStack-internal.h"
#include "procMap.h"
#include "assertion.h"
#include "typesAndDecs.h"

#define localDeque ( (Job*)((PMem*)PMAddr(deques) + getProcIDX()) )
#define bot ( (int*)((PMem*)PMAddr(bots) + getProcIDX()) )
#define top ( (int*)((PMem*)PMAddr(tops) + getProcIDX()) )
#define localIdx ( getProcIDX() )
/*
 * These should not be confused with the declarations at the bottom of
 * scheduler.h. Those are the sets of stacks, tops, and bottoms of
 * everyone. These are the ones  for this process. bot here is
 * equivalent to `bots[localIdx]` for convience
 * 
 * localDeque: pointer to STACK_SIZE deque. 
 * 
 * bot/top: PM pointers to top and bottom of the stack. there is an
 * array declared in scheduler.h to check other procs
 *
 * localIdx: quick reference to who this process is. 
 *
 * definitions use lookups with soft whoAmIs, so when scheduling code
 * is stolen it works properly
 */

PMem deques; //is PM array of size: [NUM_PROC][STACK_SIZE]

PMem tops; //PM array of top indicies size NUM_PROC
PMem bots; //PM array of top indicies size NUM_PROC


/*
 * when does the new local Job get intialized and filled when a
 * steal happens? is it in helpTheif or in steal? when the local thing
 * gets set to local? when the original gets set to theif w/ a pointer
 * to it? I know the Job being stolen is returned, but it also needs
 * to store it somehwere, as it is lost in the taken job entry (I
 * think?), but never filled in the pointed to local entry
 *
 * ANSWER it gets written here. 
 */

/* this is a non-persistent leaf function call */
void helpThief(int victimProcIdx) {
     int topCopy = getTop(victimProcIdx);
     Job* victimDeque = (Job*) ( PMAddr( *((PMem*)PMAddr(deques) + victimProcIdx) ) );
     Job foreignJobCopy = victimDeque[topCopy];
     if (isTaken(foreignJobCopy)) {
	  Job* targetOfCopy = (Job*) PMAddr(getTarget(foreignJobCopy));
	  CAMJob(targetOfCopy,
		 newEmptyWithCounter(getCounter(foreignJobCopy)),
		 makeLocal(*targetOfCopy));
	  /*
	   * the above relies on the fact that the taken counter is
	   * the counter of the entry at the time of the steal
	   */
	  CAM(top, topCopy, topCopy+1);
	  targetOfCopy->work = foreignJobCopy.work;
	  targetOfCopy->argSize = foreignJobCopy.argSize;
	  for (int i = 0; i < foreignJobCopy.argSize; i++) {
	       targetOfCopy->args[i] = foreignJobCopy.args[i];
	  }
          //copy the initial capsule to the new target
     }
}


/* forward dec for readability */
Capsule pushBCnt(void);
Capsule forkCnt(void);
/*
 * this function is called by fork and maybe at the begining, and is a
 * persistent call. It either returns to the scheduler at the end, or
 * tail recurses on itself
 *
 * now takes two Jobs, the left and right. It pushes one of them and
 * executes the other. They need to be jobs to include the starting
 * args. The executed Job does have a local entry, and this is
 * currently not filled with any data, just a marker that a thief
 * should check if this proc is alive and then check it's installed
 * cap. That can probably be changed if nessiary
 */
Capsule pushBottom(void) {
     //again implicitly pass args by not popping them, then add several copied counters on top
     int bottomCopy = *bot;
     pPushCntArg(bottomCopy); //where the bottom is now
     pPushCntArg(getCounter(localDeque[bottomCopy])); //counter of current bottom entry
     pPushCntArg(getCounter(localDeque[bottomCopy+1])); //counter of one below that
     pcnt(&pushBCnt); //cap bound
}

/* continuation of pushBottom (after the commit) */
Capsule pushBCnt (void) {
     int bottomCopy, currentCounterCopy, nextCounterCopy;
     Job left, right;
     pPopArg(nextCounterCopy);
     pPopArg(currentCounterCopy);
     pPopArg(bottomCopy);
     pPopArg(right);
     pPopArg(left);

     Job currentJobCopy = localDeque[bottomCopy];
     if (getCounter(currentJobCopy) == currentCounterCopy && isLocal(currentJobCopy)) {
	  /* I guess this makes the replacement happen once? not sure why this is needed */
	  localDeque[bottomCopy+1] = makeLocal(localDeque[bottomCopy+1]);
	  *bot = bottomCopy+1;
	  //now we have two local entries
	  
	  Job replacement = makeScheduled(left);
	  replacement.tag.counter = getCounter(currentJobCopy)+1;
	  Job* loc = &localDeque[bottomCopy];

	  CAMJob(loc, currentJobCopy, replacement); 
	  loc->argSize = replacement.argSize;
	  for (int i = 0; i < loc->argSize; i++) {
	       loc->args[i] = replacement.args[i];
	  }
	  //one half is done

	  
	  /* 
	   * this is (indirectly) a scheduler-usercode boundary
	   */
	  for (int i = 0; i < right.argSize; i++) {
	       pPushCntArg(right.args[i]); 
	  }
	  //pStackReset will properly empty the stack and copy these
	  //arguments to the bottom of the stack
	  pPushCntArg(right.work); //capsule that uses these argument
	  
	  //use pStackReset here, takes args to push and then the
	  //continuation capsule. declared in pStack-internal.h
	  return makeCapsule(&pStackReset);

     } else if (isEmpty(localDeque[bottomCopy+1])) {
	  pPushCntArg(left);
	  pPushCntArg(right);
	  pcnt(&pushBottom); //basically restarting this call
	  /*
	   * this is reached if a proc hardfaults in this cap, the
	   * theif takes this, and restarts to do the push on their
	   * own deque instead of the victim's deque
	   */
	    
     }
     /* 
      * TODO I don't know when this happens, so I guess I will
      * basically restart?
      */
     pPushCntArg(left);
     pPushCntArg(right);
     pcnt(&pushBottom); //basically restarting this call
}


/*
 * fork, push new job onto the stack and start another. It is
 * persistent call that takes three jobs as arguments, the left
 * and right branches of the fork, and the work to jump to when the
 * two children have joined.
 *
 * declared for user code in flow.h
 *
 * It takes the following, (this is the order that they are
 * popped in):
 *
 * size of join args
 * args
 * join funcPtr
 *
 * the size of the left branch's starting arguments
 * said args
 * the left starting funcPtr
 *
 * the size of the right's args
 * args
 * right funcPtr
 *
 * then this code will pop and divide these into Jobs *not* capsules,
 * even the join one. The set contains a job and not a capsule. That
 * way when the join is complete it pushes a new job rather than
 * continuting without returning to the scheduler. This way we can
 * avoid packaging args within capsules, but store args outside of the
 * pStack (in the job in this case)
 */
Capsule ppmmfork(void) {
     int joinSize;
     byte joinArgs[JOB_ARG_SIZE];
     funcPtr_t joinPtr;
     byte* output;

     pPopArg(joinSize);
     output = joinArgs+joinSize;
     for (int i = 0; i < joinSize; i++) {
	  pPopArg(*(--output)); //copy into local args from stack, backwards
     }
     pPopArg(joinPtr); //done with join


     Job joinJob = newEmptyWithCounter(0); //counters will be set later
     joinJob.work = makeCapsule(joinPtr);
     for (int i = 0; i < joinSize; i++) {
	  joinJob.args[i] = joinArgs[i]; //copy into job struct
     }
     joinJob.argSize = joinSize; //finished with join Job struct

     /* 
      * now the join continuation job is completely set, we can get a
      * set location now. the rest of the arguments go through
      */
     pPushCalleeArg(joinJob);
     pcall(&getAndInitSet, &forkCnt);
}

Capsule forkCnt(void) {
     int setIdx;
     pPopArg(setIdx);
     
     int leftSize, rightSize;
     byte leftArgs[JOB_ARG_SIZE];
     byte rightArgs[JOB_ARG_SIZE];
     funcPtr_t leftPtr, rightPtr;
     byte* output;
     
     pPopArg(leftSize);
     output = leftArgs+leftSize;
     for (int i = 0; i < leftSize; i++) {
	  pPopArg(*(--output)); //copy into local args from stack, backwards
     }
     pPopArg(leftPtr); //done with left

     pPopArg(rightSize);
     output = rightArgs+rightSize;
     for (int i = 0; i < rightSize; i++) {
	  pPopArg(*(--output)); //copy into local args from stack, backwards
     }
     pPopArg(rightPtr); //done with right


     //the arguments are all popped and filled now

     /*
      * fill the internal info for the left/right capsules and setup the fork history
      */
     Capsule leftCap = makeCapsule(leftPtr);
     Capsule rightCap = makeCapsule(rightPtr);

     
     leftCap.forkSide = 0; //mark left fork
     leftCap.joinLoc = setIdx;

     rightCap.forkSide = 1;
     rightCap.joinLoc = setIdx;

     //joinCap doesn't need edits, because it's path is the same is this cap's, so makeCap should be fine

     Job leftJob = newEmptyWithCounter(0); //counters will be set later
     leftJob.work = leftCap;
     for (int i = 0; i < leftSize; i++) {
	  leftJob.args[i] = leftArgs[i]; //copy into job struct
     }
     leftJob.argSize = leftSize; //finished with left Job struct
     
     Job rightJob = newEmptyWithCounter(0); //counters will be set later
     rightJob.work = rightCap;
     for (int i = 0; i < rightSize; i++) {
	  rightJob.args[i] = rightArgs[i]; //copy into job struct
     }
     rightJob.argSize = rightSize; //finished with right Job struct
     

     //now the Jobs are all set

     pPushCntArg(leftJob);
     pPushCntArg(rightJob);
     pcnt(&pushBottom);
}

/* forward decs for readability */
Capsule stealCnt(void);
Capsule graveRob(void);
Capsule steal(void);

/*
 * persistent cnt for finding a victim and trying to steal from them.
 * continuation is steal. takes no arguments, passes an procIdx, a
 * counter copy, and a PMem output location to steal
 */
Capsule stealLoop(void) {
     /*
      * the stealing proceedure from findwork in the paper, looping and empty check are in steal
      */
     yield();

     pPushCntArg(getVictim);
     pPushCntArg(getCounter(localDeque[*bot]));
     pPushCntArg( *((PMem*)PMAddr(deques) + getProcIDX() + *bot) );
     /*
      * this should be the bottom of the stack, hopefully.
      */
     pcnt(&steal);
}

/*
 * steal from another proc, called from findWork in the paper, here from popBottom's cnt 
 *
 * jumps to stealLoop if someone steals it first or there is nothing
 * to steal, or they are alive and and we can't grave rob. if the
 * steal works, jumps to the stolen work.
 */
Capsule steal(void) {
     int victimIdx;
     int counterCopy;
     PMem outputLoc;
     pPopArg(outputLoc);
     pPopArg(counterCopy);
     pPopArg(victimIdx);
     
     helpThief(victimIdx); //non-persistent leaf call

     pPushCntArg(victimIdx); //who is the victim
     pPushCntArg(counterCopy); //counter of what we are stealing
     pPushCntArg(outputLoc); //PMem location to copy to
     pPushCntArg(getTop(victimIdx)); //where is the top of the victim's stack
     
     PMem jobArray = ((PMem*) PMAddr(deques))[victimIdx];
     pPushCntArg( ((Job*) PMAddr(jobArray))[getTop(victimIdx)]); //copy of job being stolen

     pcnt(&stealCnt); //cap bound
}

/*
 * continuation of steal. should either jump to stolen work via
 * continuation or to the stealing loop on failure, or graverob if
 * stealing from a dead proc
 *
 * local entry for taken is filled in helpThief
 */
Capsule stealCnt(void) {
     int victimProcIdx;
     int counterCopy;
     PMem outputLoc; //read PMem thing at top of file. This is a single Job
     int topCopy;
     Job toStealCopy;
     pPopArg(toStealCopy);
     pPopArg(topCopy);
     pPopArg(outputLoc);
     pPopArg(counterCopy);
     pPopArg(victimProcIdx);
	  
     
     Job updatedEntry;
     PMem jobArray;
     Job* location;
     Job onStack;
     switch (getId(toStealCopy)) {
     case takenId: //someone stole it first, help them
	  helpThief(victimProcIdx);
	  pcnt(&stealLoop); //jump steal loop
	  
     case scheduledId: //something to steal
	  updatedEntry = makeTaken(toStealCopy, outputLoc, counterCopy);
	  jobArray = ((PMem*) PMAddr(deques))[victimProcIdx];
	  location = ((Job*) PMAddr(jobArray)) +topCopy;
	  CAMJob(location, toStealCopy, updatedEntry);
	  helpThief(victimProcIdx);
	  /* local entry at outputLoc is added in helpThief */
	  if (!CompareJob(*location, updatedEntry))
	       pcnt(&stealLoop); //jump steal loop, we failed to get this one

	  /* 
	   * push starting arguments and then usercode cap and then
	   * jump to pStackReset, this is (indirectly) a
	   * scheduler-usercode boundary
	   */
	  for (int i = 0; i < toStealCopy.argSize; i++) {
	       pPushCntArg(toStealCopy.args[i]);
	  }
	  pPushCntArg(toStealCopy.work);

	  return makeCapsule(&pStackReset);
	  
     case localId: //could grave rob
	  jobArray = ((PMem*) PMAddr(deques))[victimProcIdx];
	  onStack = ((Job*) PMAddr(jobArray))[topCopy];
	  if (!isLive(victimProcIdx) &&
	      CompareJob(onStack, toStealCopy)) {
	       
	       pPushCntArg(victimProcIdx);
	       pPushCntArg(counterCopy);
	       pPushCntArg(outputLoc);
	       pPushCntArg(topCopy);
	       pPushCntArg(toStealCopy);
	       pcnt(&graveRob); //jump to graveRob and pass args untouched, cap bound
	  }
	  pcnt(&stealLoop); //jump steal loop, stolen or alive
	  
     case emptyId: //Nothing to steal
	  pcnt(&stealLoop); //jump steal loop

     default:
	  rassert(0, "Encountered a Job of unknown type");
     }
}

/*
 * continuation of stealCnt if trying to rob a dead proc. jumps to
 * work or stealLoop. takes same args as stealCnt
 */
Capsule graveRob(void) {
     int victimProcIdx;
     PMem outputLoc; //read top of file. This is a single Job
     int counterCopy; 
     int topCopy;
     Job toStealCopy;
     pPopArg(toStealCopy);
     pPopArg(topCopy);
     pPopArg(outputLoc);
     pPopArg(counterCopy);
     pPopArg(victimProcIdx);
     
     Job updatedEntry;

     updatedEntry = makeTaken(toStealCopy, outputLoc, counterCopy);

     PMem jobArray = ((PMem*) PMAddr(deques))[victimProcIdx];
     Job* oldTop = ((Job*) PMAddr(jobArray)) +topCopy;
     *(oldTop+1) = makeEmpty(*(oldTop+1));
     
     CAMJob(oldTop, toStealCopy, updatedEntry);
     helpThief(victimProcIdx); //non-persistent leaf call, makes local entry
     
     if (!CompareJob(*oldTop, updatedEntry)) {
	  pcnt(&stealLoop); //jump steal loop, we failed to steal this one
     }

     /* 
      * push starting args and usercode cap and jump to pStackReset.
      * this is (indirectly) a scheduler-usercode boundary
      */
     for (int i = 0; i < toStealCopy.argSize; i++) {
	  pPushCntArg(toStealCopy.args[i]);
     }
     pPushCntArg(toStealCopy.work);

     return makeCapsule(&pStackReset);
     
}


//forward dec for readability
Capsule popBCnt(void); 

/* 
 * this is a persistent continuation from scheduler that takes no
 * arguments and either jumps to work to be done if there is any, or
 * to the stealing function through its continuation (split findwork
 * from the paper in half)
 */
Capsule popBottom(void) {
     int bc = *bot;
     pPushCntArg(bc); //copy of bottom
     pPushCntArg(localDeque[bc-1]); //copy of job
     pcnt(&popBCnt); //this is the cap bound in the middle
}

/*
 * continuation of popBottom
 */
Capsule popBCnt(void) {
     Job jobCopy;
     int botCopy;
     pPopArg(jobCopy);
     pPopArg(botCopy); 
     
     if (isScheduled(jobCopy)) { //not yet being worked on by anyone
	  /* construct a new local job from the scheduled job */
	  Job replacement = makeLocal(jobCopy); //version that is claimed by this proc
	  CAMJob(&(localDeque[botCopy-1]), jobCopy, replacement);
	  if (CompareJob(localDeque[botCopy-1], replacement)) {
	       *bot = botCopy-1; //update the global value (PM)


	       /* 
		* need to copy job args to stack, and then user
		* capsule. then pStackReset will handle the stack
		* emptying and jumping to user code
		*
		* this is (indirectly) a scheduler-usercode boundary
		*/
	       for (int i = 0; i < replacement.argSize; i++) {
		    pPushCntArg(replacement.args[i]);
	       }
	       pPushCntArg(replacement.work);

	       return makeCapsule(&pStackReset);
	  }
     }
     //no work to be done here, steam from someone
     pcnt(&stealLoop);
}


Capsule sjpCnt(void); //forward dec
/* 
 * takes a job, basically a job continuation swapping mechanism
 */
Capsule singleJobPush(void) {
     int bottomCopy = *bot;
     //toPush job goes through to cnt
     pPushCntArg(bottomCopy);
     pPushCntArg(localDeque[bottomCopy]);
     pPushCntArg(getCounter(localDeque[bottomCopy]));
     pcnt(&sjpCnt);
}

Capsule sjpCnt(void) {
     Job old;
     int botCopy;
     int oldCounter;
     Job new;
     pPopArg(oldCounter);
     pPopArg(old);
     pPopArg(botCopy);
     pPopArg(new); //goes through singleJobPush

     new.tag.counter = oldCounter+1;
     new.tag.id = localId;
     CAMJob(&(localDeque[botCopy]), old, new);
     //not sure if this needs to be a cam or not?
     localDeque[botCopy].argSize = new.argSize;
     for (int i = 0; i < new.argSize; i++) {
	  localDeque[botCopy].args[i] = new.args[i];
     }

     //usercode boundary
     for (int i = 0; i < new.argSize; i++) {
	  pPushCntArg(new.args[i]);
     } //could combine loops, but this is more readable
     pPushCntArg(new.work);

     return makeCapsule(&pStackReset);
}

Capsule sCnt(void); //forward dec
/*
 * main loop, looks for, finds, and executes work via continuations. takes no arguments
 *
 * (this and below)
 */
Capsule scheduler(void) {
     pcall(&resetWhoAmI, &sCnt);
}

Capsule sCnt(void) {
     localDeque[*bot] = makeEmpty(localDeque[*bot]);
     pcnt(&popBottom);
     /*
      * findwork from the paper is now run as a continuation of
      * popBottom on failure, and the steal loop and stealing
      * functions are decendents of that
      */
}
