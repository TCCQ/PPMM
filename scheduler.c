#include "scheduler.h"
#include "capsule.h" /* for commiting capsules (installing) */

/* the deque that this process owns and can push to, is a ptr to PM */
Job localDeque[STACK_SIZE];
int* bot;
int* top;
int localIdx; /* these should be initilized in process init*/

/* TODO proc specific init, set localIdx and localDeque and others(?) */


/* TODO write pushBottom, fork, scheduler, helpPopTop from paper */

/*
 * TODO when does the new local Job get intialized and filled when a
 * steal happens? is it in helpTheif or in steal? when the local thing
 * gets set to local? when the original gets set to theif w/ a pointer
 * to it? I know the Job being stolen is returned, but it also needs
 * to store it somehwere, as it is lost in the taken job entry (I
 * think?), but never filled in the pointed to local entry
 */

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


/*
 * TODO should this take a function pointer? a capsule? a Job?
 *
 * TODO this has the same issue as the others, contains a cap bound, need to pass a way to return (setjmp)
 */
void pushBottom(/*type TBD */ ) {
     int bottomCopy = *bot;
     int currentCounterCopy = getCounter(&localDeque[bottomCopy]);
     int nextCounterCopy = getCounter(&localDeque[bottomCopy+1]);
     commitCurrent(); //cap bound
     Job currentJobCopy = localDeque[bound];
     if (currentJobCopy.counter == currentCounterCopy && isLocal(&currentCounterCopy)) {
	  localDeque[bottomCopy+1] = makeLocal(localDeque[bottomCopy+1]);
	  /* TODO the line above strikes me as unsafe and not idempotent, consider */
	  *bot = bottomCopy+1;
	  /*
	   * TODO make a scheduled job entry with the arguemnt if not
	   * already, then use as replacement below
	   */
	  CAMJob(&localDeque[bottomCopy], currentJobCopy, /* replacement */);
     } else if (isEmpty(&localDeque[bottomCopy+1])) {
	  pushBottom(/* input argument */); //TODO I guess this rereads bottomCopy? 
     }
}

/*
 * regular function called from findWork
 *
 * Returns job (now local) with info on where work is and args, etc,
 * or empty job on failure
 *
 * TODO can this return a raw work pointer? I think I need to pass
 * arguments and a return value spot and a sync Set and whatnot as
 * well.
 *
 * TODO this doesn't work atm, becuase there is a commit inside, so
 * the return will be undefined as the stack could be lost. Should do
 * it with a set/longjmp I think
 *
 * TODO I am suspicious of this bot-1, maybe a bot+1? bot should point
 * to the first local entry if work is done (so this func should end
 * with bot pointing to replacement), but if this proc doesn't a local
 * work currently, it should point to "the first empty entry after the
 * jobs". So when I make a job local, bot should INCREASE, otherwise
 * the stack is upside down and top < bot.
 */
Job popBottom(void) {
     int botCopy = *bot;
     Job jobCopy = localDeque[botCopy-1];
     commitCurrent(); //capsule install
     if (isScheduled(&jobCopy)) { //not yet being worked on by anyone
	  
	  /* construct a new local job from the unscheduled job */
	  Job replacement = makeLocal(&jobCopy); //version that is claimed by this proc
	  CAMJob(&(localDeque[botCopy-1], jobCopy, replacement));
	  if (CompareJob(&(localDeque[botCopy-1]), &replacement)) {
	       *bot = b-1; //update the global value (PM)
	       return replacement; //TODO should this be a pointer? does it need to be?
	  }
     }
     return makeEmpty(); //no jobs on this deque
}

/*
 * steal from another proc, called from findWork
 *
 * should return a taken job that points to a local job on the
 * stealers deque if I am understanding this correctly, or empty if
 * there is nothing to steal
 *
 * TODO if this were to return a pointer, then I could return NULL on
 * error, making this slightly more readable, see above
 * 
 * TODO has same problem as popBottom, has capBound in it, so I can't
 * use a regular function here, need to "pass" return setjmp I think.
 */
Job steal(int victimProcIdx, Job* outputLoc, int counterCopy) {
     helpThief(victimProcIdx);
     int topCopy = getTop(victimProcIdx);
     Job toStealCopy = deques[victimProcIdx][getTop(victimProcIdx)];
     Job updatedEntry;
     commitCurrent(); // capsule boundary
     switch (getId(&toStealCopy)) {
     case emptyId: //Nothing to steal
	  return newEmpty(); 
     case takenId: //someone stole it first, help them
	  helpThief(victimProcIdx);
	  return newEmpty();
     case scheduledId: //something to steal
	  updatedEntry = makeTaken(&toStealCopy, outputLoc);
	  /* TODO somehow I need to add a local entry at outputLoc
	   * that is a copy and mark, I think that happens in
	   * helpThief */
	  CAMJob(&(deques[victimProcIdx][topCopy]), toStealCopy, updatedEntry);
	  helpThief(victimProcIdx);
	  if (!CompareJob(&(deques[victimProcIdx][topCopy]), &updatedEntry))
	       return newEmpty(); //TODO see concerns above
	  return updatedEntry;
     case localId: //could grave rob
	  if (isLive(victimProcIdx) && CompareJob(&(deques[victimProcIdx][topCopy]), &toStealCopy)) {
	       commitCurrent(); //cap bound. TODO see concerns above
	       updatedEntry = makeTaken(&toStealCopy, outputLoc);
	       deques[victimProcIdx][topCopy+1] = makeEmpty(&deques[victimProcIdx][topCopy+1]);
	       /* TODO above is changing the PM stack and strikes me as unsafe, matches paper */
	       CAMJob(&(deques[victimProcIdx][topCopy]), toStealCopy, updatedEntry);
	       helpThief(victimProcIdx);
	       if (!CompareJob(&(deques[victimProcIdx][topCopy]), &updatedEntry)) {
		    return newEmpty(); //TODO see concerns above
	       }
	  }
     }
}

/*
 * do local work or steal, called by scheduler
 */
void findWork(void) {
     Job fromLocalDeque = popBottom();
     if (!isEmpty(fromLocalDeque)){
	  /* TODO do the work in fromLocalDeque, could have wrapper,
	   * need to pass a return setjmp loc and whatnot. Not really
	   * "pass" but place them in PM somewhere so it can get
	   * back */
     }
     while (true) {/* TODO make a quiting mechanism */
	  yield();
	  int victim = getVictim();
	  int counterCopy = getCounter(&localDeque[*bot]);
	  Job stolen = steal(victim, &(localDeque[*bot]), counterCopy);
	  if (!isEmpty(stolen)) {
	       /* TODO see previous in this function */
	  }
     }

}

/*
 * fork, push new job onto the stack, this should be visible to the
 * user I think,
 *
 * TODO make a choice vis-a-vis what the user sees when talking about
 * this sort of control flow. maybe use funcPtr_t here?
 */
void fork(/* type */) {
     /*
      * TODO this seems like the place to construct the job to be
      * pushed, then pass it to pushBottom
      */
     pushBottom(/* constructed job I guess */)
}

/*
 * main loop
 */
void scheduler(void) {
     localDeque[*bot] = makeEmpty(localDeque[*bot]);
     /* seems unsafe, see other TODOs */
     Job workToDo = findWork();
     /*
      * TODO decide how to actually break the ice and get down to
      * buisness, that happens here. This is also the function that
      * the setjmp shenanigans happen in I think
      */
}
