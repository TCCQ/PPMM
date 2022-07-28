#include "scheduler.h"
#include "capsule.h" /* for commiting capsules (installing), fork, and persistence */
#include "set.h" /* used in fork */
#include "flow.h" //fork declaration
#include "memUtilities.h" //PMem
#include "pStack.h" //persistent flow operations
#include "pStack-internal.h"

/* the deque that this process owns and can push to, is a ptr to PM */
Job* localDeque; 
int* bot; 
int* top; 
int localIdx; 
/*
 * These should not be confused with the declarations at the bottom of
 * scheduler.h. Those are the sets of stacks, tops, and bottoms of
 * everyone. These are said things for this process. bot here is
 * equivalent to `bots[localIdx]` for convience
 * 
 * localDeque: pointer to STACK_SIZE deque. TODO init somewhere
 * 
 * bot/top: PM pointers to top and bottom of the stack. there is an
 * array declared in scheduler.h to check other procs
 *
 * localIdx: quick reference to who this process is. TODO should be
 * moved so that it can be init in the init file
 *
 * these are pointers to PM that are visible only to this process. as
 * such, they can be safely converted to regular pointers and
 * initialized at the start of the process.
 *
 * TODO I think the above is true, but I need to make PMem objects out
 * of derived references to these pointers sometimes so how should I
 * do that? I can't make a PMem constructor that takes a void* and
 * calculates relative to the root ptr, because ptr subtraction is not
 * defined in general. I think these have to be PMem references.
 * ponder
 *
 * consider making these definitions for lookups with soft whoAmIs, so
 * when scheduling code is stolen it works properly
 */

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
     Job foreignJobCopy = deques[victimProcIdx][topCopy];
     if (isTaken(foreignJobCopy)) {
	  CAMJob(foreignJobCopy.target,
		 newEmptyWithCounter(foreignJobCopy.counter),
		 makeLocal(foreignJobCopy.target));
	  /*
	   * the above relies on the fact that the taken counter is
	   * the counter of the entry at the time of the steal
	   */
	  CAM(&top, topCopy, topCopy+1);
	  foreignJobCopy.target.work = foreignJobCopy.work; //copy the initial capsule to the new target
     }
}


/* 
 * ephemeral leaf call, empties the pStack and then copies the
 * starting arguments into the stack. This needs to be called from
 * some place that can make progress without the pStack (basically
 * just the last capsule before the usercode capsule jump)
 */
void jobStartStack(const Job* from) {
     pStackDirty = myStack; //reset based on a soft whoAmI
     for (int i = 0; i < from->argSize; i++) {
	  *(pStackDirty++) = from->args[i];
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
     if (currentJobCopy.counter == currentCounterCopy && isLocal(currentJobCopy)) {
	  /* I guess this makes the replacement happen once? not sure why this is needed */
	  localDeque[bottomCopy+1] = makeLocal(localDeque[bottomCopy+1]);
	  *bot = bottomCopy+1;
	  //now we have two local entries
	  
	  Job replacement = makeScheduled(left);
	  replacement.counter = currentJobCopy.counter+1; 
	  CAMJob(&localDeque[bottomCopy], currentJobCopy, replacement); //one half is done

	  jobStartStack(&right); //copy right args to clean stack
	  
          pCntManual(right.work); //use the manual override version of pcnt to allow manual forkpath editing

     } else if (isEmpty(localDeque[args.bCopy+1])) { 
	  persistentCall(mCapStruct(&pushBottom, args)); //this is eq to pushBottom(inputCapsule);
	  //TODO when is this reached?
     }
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
 * the size of the left branch's starting arguments
 * said args
 * the left starting funcPtr
 *
 * the size of the right's args
 * args
 * right funcPtr
 *
 * size of join args
 * args
 * join funcPtr
 *
 * then this code will pop and divide these into Jobs *not* capsules,
 * even the join one. The set should contain a job and not a capsule.
 * (TODO) That way when the join is complete it pushes a new job
 * rather than continuting without returning to the scheduler. This
 * way we can avoid packaging args within capsules, but store args
 * outside of the pStack (in the job in this case)
 */
Capsule fork(void) {
     //don't pop args here, let them pass through to forkCnt after the PMalloc call
     pPushCalleeArg(sizeof(Set));
     pcall(&PMalloc, &forkCnt);
     /*
      * the callee just gets the size, the cnt doesn't get anything
      * explicit, but implicitly gets everything that was pushed but
      * not popped as and argument to this
      */
}

//after pCall in fork, takes fork args + pMem pointer
Capsule forkCnt(void) {
     int leftSize, rightSize, joinSize;
     byte leftArgs[JOB_ARG_SIZE];
     byte rightArgs[JOB_ARG_SIZE];
     byte joinArgs[JOB_ARG_SIZE];
     funcPtr_t leftPtr, rightPtr, joinPtr;
     byte* output;
     PMem joinSet; //TODO fix type of PMalloc

     pPopArg(joinSet); //PMalloc return value
     
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

     pPopArg(joinSize);
     byte* output = joinArgs+joinSize;
     for (int i = 0; i < joinSize; i++) {
	  pPopArg(*(--output)); //copy into local args from stack, backwards
     }
     pPopArg(joinPtr); //done with join

     //the arguments are all popped and filled now

     /*
      * fill the internal info for the left/right capsules and setup the fork history
      */
     Capsule leftCap = makeCapsule(leftPtr);
     Capsule rightCap = makeCapsule(rightPtr);
     Capsule joinCap = makeCapsule(joinPtr);

     
     leftCap.forkPath = (quickGetInstalled.forkPath << 1) & (~1); //mark left fork
     leftCap.joinLocs[quickGetInstalled.joinHead+1] = joinSet;
     leftCap.joinHead = quickGetInstalled.joinHead+1;

     rightCap.forkPath = (quickGetInstalled.forkPath << 1) | 1; //mark right fork
     rightCap.joinLocs[quickGetInstalled.joinHead+1] = joinSet;
     rightCap.joinHead = quickGetInstalled.joinHead+1;

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
     
     Job joinJob = newEmptyWithCounter(0); //counters will be set later
     joinJob.work = joinCap;
     for (int i = 0; i < joinSize; i++) {
	  joinJob.args[i] = joinArgs[i]; //copy into job struct
     }
     joinJob.argSize = joinSize; //finished with join Job struct

     //now the Jobs are all set
     
     *( (Set*) PMAddr(joinSet) ) = SetInitialize(joinJob); 
     
     pPushCntArg(leftJob);
     pPushCntArg(rightJob);
     pcnt(&pushBottom); //jump to pushBottom
}

/* forward decs for readability */
Capsule stealCnt(void);
Capsule graveRob(void);

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
     pPushCntArg(/* TODO this should be a PMem for the output location, see top of file*/);
     pcnt(&steal);
}

/*
 * steal from another proc, called from findWork in the paper, here from popBottom's cnt 
 *
 * jumps to stealLoop if someone steals it first or there is nothing
 * to steal, or they are alive and and we can't grave rob. if the
 * steal works, jumps to the stolen work.
 */
Job steal(void) {
     struct stealArgs args;
     getCapArgs(args); //only first half is valid, fill the rest
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

     pcnt(&stealcnt); //cap bound
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
     pPopArg(countCopy);
     pPopArg(victimProcIdx);
	  
     
     Job updatedEntry;
     Capsule stolenCap;
     switch (getId(&toStealCopy)) {
     case emptyId: //Nothing to steal
	  pcnt(&stealLoop); //jump steal loop
	  
     case takenId: //someone stole it first, help them
	  helpThief(victimProcIdx);
	  pcnt(&stealLoop); //jump steal loop
	  
     case scheduledId: //something to steal
	  updatedEntry = makeTaken(&toStealCopy, outputLoc, couterCopy);
	  stolenCap = toStealCopy.work;
	  PMem jobArray = ((PMem*) PMAddr(deques))[victimProcIdx];
	  Job* location = ((Job*) PMAddr(jobArray)) +topCopy;
	  CAMJob(location, toStealCopy, updatedEntry);
	  helpThief(victimProcIdx);
	  /* local entry at outputLoc is added in helpThief */
	  if (!CompareJob(*location, updatedEntry))
	       pcnt(&stealLoop); //jump steal loop, we failed to get this one

	  jobStartStack(&toStealCopy);
	  pCntManual(stolenCap); //manual to set forkpath
	  
     case localId: //could grave rob
	  PMem jobArray = ((PMem*) PMAddr(deques))[victimProcIdx];
	  Job onStack = ((Job*) PMAddr(jobArray))[topCopy];
	  if (!isLive(victimProcIdx) &&
	      CompareJob(onStack, toStealCopy)) {
	       
	       pPushCntArg(victimProcIdx);
	       pPushCntArg(countCopy);
	       pPushCntArg(outputLoc);
	       pPushCntArg(topCopy);
	       pPushCntArg(toStealCopy);
	       pcnt(&graveRob); //jump to graveRob and pass args untouched, cap bound
	  }
	  pcnt(&stealLoop); //jump steal loop, stolen or alive
	  
     }
}

/*
 * continuation of stealCnt if trying to rob a dead proc. jumps to
 * work or stealLoop. takes same args as stealCnt
 *
 * TODO fine tooth comb this with charlie, not sure if this is right
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
     pPopArg(countCopy);
     pPopArg(victimProcIdx);
     
     Job updatedEntry;
     Capsule stolenCap;

     stolenCap = toStealCopy.work;
     updatedEntry = makeTaken(toStealCopy, PMAddr(outputLoc), counterCopy);

     PMem jobArray = ((PMem*) PMAddr(deques))[victimProcIdx];
     Job* oldTop = ((Job*) PMAddr(jobArray)) +topCopy;
     *(newTop+1) = makeEmpty(*(newTop+1));
     
     CAMJob(oldTop, toStealCopy, updatedEntry);
     helpThief(victimProcIdx); //non-persistent leaf call, makes local entry
     
     if (!CompareJob(*oldTop, updatedEntry)) {
	  pcnt(&stealLoop); //jump steal loop, we failed to steal this one
     }

     jobStartStack(&toStealCopy); //copy args to clean stack
     pCntManual(stolenCap); //manual cap version to set forkpath
}


//forward dec for readability
Capsule popBCnt(void); 

/* 
 * this is a persitent continuation from scheduler that takes no
 * arguments and either jumps to work to be done if there is any, or
 * to the stealing function through its continuation (split findwork
 * from the paper in half)
 */
Capsule popBottom(void) {
     pPushCntArg(*bot); //copy of bottom
     pPushCntArg(localDeque[botCopy-1]); //copy of job
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
	  CAMJob(&(localDeque[botCopy-1], jobCopy, replacement));
	  if (CompareJob(localDeque[botCopy-1], replacement)) {
	       *bot = botCopy-1; //update the global value (PM)

	       jobStartStack(&replacement); //copy args to clean stack

	       pCntManual(replacement.work); // use manual version to set forkpath properly, schdeduler-usercode boundary
	  }
     }
     //no work to be done here, steam from someone
     pcnt(&stealLoop);
}

/*
 * main loop, looks for, finds, and executes work via continuations. takes no arguments
 */
Capsule scheduler(void) {
     localDeque[*bot] = makeEmpty(localDeque[*bot]);
     pcnt(&popBottom);
     /*
      * findwork from the paper is now run as a continuation of
      * popBottom on failure, and the steal loop and stealing
      * functions are decendents of that
      */
}
