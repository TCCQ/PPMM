#include "scheduler.h"
#include "capsule.h" /* for commiting capsules (installing), fork, and persistence */

/* the deque that this process owns and can push to, is a ptr to PM */
Job localDeque[STACK_SIZE]; //TODO I think is allocating, make a ptr
int* bot;
int* top;
int localIdx; /* these should be initilized in process init*/


/*
 * TODO when does the new local Job get intialized and filled when a
 * steal happens? is it in helpTheif or in steal? when the local thing
 * gets set to local? when the original gets set to theif w/ a pointer
 * to it? I know the Job being stolen is returned, but it also needs
 * to store it somehwere, as it is lost in the taken job entry (I
 * think?), but never filled in the pointed to local entry
 *
 * ANSWER the local job contains no info, all the info is the currently
 * installed capsule. it is just a marker that it was doing work
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
     }
}


/* args and forward dec for readability */
Capsule pushBCnt(void);
struct pushBargs {
     int bCopy;
     int cCountCopy;
     int nextCountCopy;
     Capsule toPush;
     Capsule cnt;
};
/*
 * this function is called by fork and maybe at the begining, and is a
 * persistent call. It either returns to the scheduler at the end, or
 * tail recurses on itself
 *
 * takes the capsule to schedule in cap args
 */
Capsule pushBottom(void) {
     struct pushBargs args;
     getCapArgs(args); //reuse struct, only toPush and cnt are valid before this function returns
     args.bCopy = *bot;
     args.cCountCopy = getCounter(&localDeque[bottomCopy]);
     args.nextCountCopy = getCounter(&localDeque[bottomCopy+1]);
     persistentCall(mCapStruct(&pushBCnt,args)); //this is equivalent to commit;, cap boundary, next is pushBCnt 
}

/* continuation of pushBottom (after the commit) */
Capsule pushBCnt (void) {
     struct pushBargs args;
     getCapArgs(args); //all members are valid
     
     Job currentJobCopy = localDeque[args.bCopy];
     if (currentJobCopy.counter == args.cCountCopy && isLocal(&currentJobCopy)) {
	  /* I guess this makes the replacement happen once? not sure why this is needed */
	  localDeque[args.bCopy+1] = makeLocal(localDeque[args.bCopy+1]);
	  *bot = args.bCopy+1;

	  Job replacement = makeScheduled(currentJobCopy);
	  replacement.work = args.toPush; //the capsule that is getting scheduled
	  CAMJob(&localDeque[bottomCopy], currentJobCopy, replacement);
	  persistentCall(args.cnt); //we are done, take follow the other side of the fork call, or left/right branch
     } else if (isEmpty(&localDeque[args.bCopy+1])) { 
	  persistentCall(mCapStruct(&pushBottom, args)); //this is eq to pushBottom(inputCapsule);
	  //TODO when is this reached?
     }
}


struct forkArgs {
     Capsule left;
     Capsule right;
}
/*
 * fork, push new job onto the stack and start another. It is
 * persistent call that takes two capsules as arguments.
 *
 * TODO make this visible to the end user. need an external sched.h
 */
Capsule fork(void) {
     struct forkArgs incoming;
     getCapArgs(incoming);
     
     struct pushBargs args;
     args.toPush = incoming.right;
     args.cnt = incoming.left;
     persistentCall(mCapStruct(&pushBottom,args)); //jump to pushBottom
}

/* forward decs for readability */
Capsule stealCnt(void);
Capsule graveRob(void);
/* args passed to steal from popBottom (paper version findwork), and from steal to its cnt */
struct stealArgs {
     int victimProcIdx;
     Job* outputLocation;
     int countCopy; 

     //from steal to stealCnt
     int tCopy;
     Job toStealCopy;
};

/*
 * persistent call for finding a victim and trying to steal from them.
 * continuation is steal. takes no arguments, passes stealArgs first
 * half to steal via a continuation
 */
Capsule stealLoop(void) {
     /*
      * the stealing proceedure from findwork in the paper, looping and empty check are in steal
      */
     yield();
     struct stealArgs args;
     args.victimProcIdx = getVictim();
     args.countCopy = getCounter(&localDeque[*bot]);
     args.outputLoc = &(localDeque[*bot]);
     persistentCall(mCapStruct(&steal, args)); //jump to stealing
}

/*
 * steal from another proc, called from findWork (or from popBottom's cnt in this version)
 *
 * jumps to stealLoop if someone steals it first or there is nothing to steal, or they are alive and and we can't grave rob. if the steal works, jumps to the stolen work.
 */
Job steal(void) {
     struct stealArgs args;
     getCapArgs(args); //only first half is valid, fill the rest
     
     helpThief(args.victimProcIdx); //non-persistent leaf call
     args.tCopy = getTop(args.victimProcIdx);
     args.toStealCopy = deques[args.victimProcIdx][getTop(args.victimProcIdx)];
     persistentCall(mCapStruct(&stealCnt, args)); //jump to cnt (cap boundary in paper)
}

/*
 * continuation of steal. should either jump to stolen work via
 * continuation or to the stealing loop on failure, or graverob if
 * stealing from a dead proc
 *
 * TODO where is the local entry's start capsule filled? How to
 * recover if fault after taken CAM but before installation
 */
Capsule stealCnt(void) {
     struct stealArgs args;
     getCapArgs(args); //all members are valid
     int victimProcIdx = args.victimProcIdx;
     Job* outputLoc = args.outputLocation;
     int counterCopy = args.countCopy;
     int topCopy = args.tCopy;
     Job toStealCopy = args.toStealCopy;
     //for readability, just copy them into local variables
     
     Job updatedEntry;
     Capsule stolenCap;
     switch (getId(&toStealCopy)) {
     case emptyId: //Nothing to steal
	  persistentCall(&stealLoop, NULL, 0); //jump steal loop 
     case takenId: //someone stole it first, help them
	  helpThief(victimProcIdx);
	  persistentCall(&stealLoop, NULL, 0); //jump steal loop 
     case scheduledId: //something to steal
	  updatedEntry = makeTaken(&toStealCopy, outputLoc, couterCopy);
	  stolenCap = toStealCopy.work;
	  CAMJob(&(deques[victimProcIdx][topCopy]), toStealCopy, updatedEntry);
	  helpThief(victimProcIdx);
	  /* local entry at outputLoc is added in helpThief */
	  if (!CompareJob(&(deques[victimProcIdx][topCopy]), &updatedEntry))
	       persistentCall(&stealLoop, NULL, 0); //jump steal loop, we failed to get this one
	  perisistentCall(stolenCap); //we successfully stole it, jump to the work
     case localId: //could grave rob
	  if (!isLive(victimProcIdx) && CompareJob(&(deques[victimProcIdx][topCopy]), &toStealCopy)) {
	       persistentCall(mCapStruct(&graveRob, args)); //jump to graveRob and pass args untouched, cap bound
	  }
	  persistentCall(&stealLoop, NULL, 0); //jump steal loop, stolen or alive
     }
}

/*
 * continuation of stealCnt if trying to rob a dead proc. jumps to
 * work or stealLoop. takes same args as stealCnt
 */
Capsule graveRob(void) {
     struct stealArgs args;
     getCapArgs(args); //all members are valid
     int victimProcIdx = args.victimProcIdx;
     Job* outputLoc = args.outputLocation;
     int counterCopy = args.countCopy; 
     int topCopy = args.tCopy;
     Job toStealCopy = args.toStealCopy;
     //for readability, just copy them into local variables
     
     Job updatedEntry;
     Capsule stolenCap;

     stolenCap = toStealCopy.work;
     updatedEntry = makeTaken(&toStealCopy, outputLoc, counterCopy);
     deques[victimProcIdx][topCopy+1] = makeEmpty(&deques[victimProcIdx][topCopy+1]);
     CAMJob(&(deques[victimProcIdx][topCopy]), toStealCopy, updatedEntry);
     helpThief(victimProcIdx); //non-persistent leaf call, makes local entry
     if (!CompareJob(&(deques[victimProcIdx][topCopy]), &updatedEntry)) {
	  persistentCall(&stealLoop, NULL, 0); //jump steal loop, we failed to steal this one
     }
     persistentCall(stolenCap); //jump to work we stole
}


//arguments and forward dec for readability
Capsule popBCnt(void); 
struct popBargs {
     int bCopy;
     Job jCopy;
};

/* *
 * this is a persitent call called from scheduler that takes no
 * arguments and either jumps to work to be done if there is any, or
 * to the stealing function through its continuation (split findwork
 * from the paper in half)
 */
Capsule popBottom(void) {
     struct popBargs args; 
     args.bCopy = *bot;
     args.jCopy = localDeque[botCopy-1]; 
     persistentCall(mCapStruct(&popBCnt, args)); //this is the cap bound in the middle
     
}

/*
 * continuation of popBottom
 *
 * TODO consider job convience funcs taking by value, might be more
 * readable
 */
Capsule popBCnt(void) {
     struct popBargs args;
     getCapArgs(args);
     if (isScheduled(&(args.jCopy))) { //not yet being worked on by anyone
	  /* construct a new local job from the scheduled job */
	  Job replacement = makeLocal(&(args.jCopy)); //version that is claimed by this proc
	  CAMJob(&(localDeque[args.bCopy-1], args.jCopy, replacement));
	  if (CompareJob(&(localDeque[args.bCopy-1]), &replacement)) {
	       *bot = args.bCopy-1; //update the global value (PM)
	       persistentCall(replacement.work); //jump to work
	  }
     }
     persistentCall(makeCapsule(&stealLoop, NULL, 0)); //jump to steal loop if there is nothing to work on
}

/*
 * main loop, looks for, finds, and executes work via continuations. takes no arguments
 */
Capsule scheduler(void) {
     localDeque[*bot] = makeEmpty(localDeque[*bot]);
     persistentCall(makeCapsule(&popBottom, NULL, 0));
     /*
      * findwork from the paper is now run as a continuation of
      * popBottom on failure, and the steal loop and stealing
      * functions are decendents of that
      */
}
