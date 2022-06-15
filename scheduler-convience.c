#include "scheduler.h"
#include "capsule.h"

/*
 * impliementations of the stuff declared in scheduler.h for use in scheduler.c
 *
 * TODO some of these aren't used, possibly prune
 */

/*
 * these make a copy of the passed job and change the job type. They also increment the counter
 */
Job makeEmpty(Job in) {
     in.id = emptyId;
     in.counter++;
     return in;
}
Job makeLocal(Job in) {
     in.id = localId;
     in.counter++;
     return in;
}
Job makeScheduled(Job in) {
     in.id = scheduledId;
     in.counter++;
     return in;
}
Job makeTaken(Job in, const Job* targetJob, int counterCopy) {
     in.id = takenId;
     in.target = targetJob;
     in.counter = counterCopy;
     return in;
}

//TODO should these take pointers or values?
/* getters */
int getCounter(Job in) {
     return in.counter;
}
int getId(Job) {
     return in.id;
}

/* dumb checks, boolean typedef is unsigned char in scheduler.h */
boolean isEmpty(Job in) {
     return in.id == emptyId;
}
boolean isLocal(Job in) {
     return in.id == localId;
}
boolean isScheduled(Job in) {
     return in.id == scheduledId;
}
boolean isTaken(Job in) {
     return in.id == takenId;
}

/*
 * we need a comparison for capsules, I am making it local because
 * that is not something the user should be doing
 */
boolean CompareCapsule(Capsule a, Capsule b) {
     if (a.rstPtr != b.rstPtr || a.joinHead != b.joinHead || a.forkPath != b.forkPath) {
	  return false
     }
     //argSize is option, not checked here
     for (int i = 0; i < ARGUMENT_SIZE; i++) {
	  if (a.arguments[i] != b.arguments[i]) return false;
     }
     for (int i = 0; i < JOIN_SIZE; i++) {
	  if (a.joinLocs[i] != b.joinLocs[i]) return false;
     }
     return true;
}

/* Comparison, == but for jobs, 1(true) for equal, 0(false) to != */
boolean CompareJob(Job a, Job b) {
     if (a.id == b.id && a.counter == b.counter) {
	  if (isTaken(a)) {
	       if (a.target == b.target) return true;
	       else return false;
	  } else {
	       if (CompareCapsule(a.work, b.work)) return true;
	       else return false;
	  }
     } else return false;
} 


/*
 * CAM specifically for jobs, (target, expected, replaced)
 *
 * TODO need to cam id and counter, but Capsules are too big to cam.
 *
 * I need that if the cam succeeds then the Job is in a defined and
 * complete state as if the whole thing was cam'd. however if you
 * write capsule after cam then if the capsule faults then it is an
 * issue. If you write it before, then someone else could also write
 * it then the you succeed in the cam and they fail, resulting in your
 * type but their work.
 *
 * the capsule is only changed when switching to a scheduled capsule,
 * or when a new local entry is made for a stolen job. so perhaps it
 * could only be written then? scheduled jobs are only made from local
 * jobs, which does not change their capsule (CONFIRM IN SCHEDULER
 * CODE), and local jobs are only reached from empty states. So I
 * think only the owner of a deque will ever be writing capsules, so
 * it is safe to write the capsule first and then cam the id and
 * counter. use a substruct to do both at once. CONFIRM WITH CHARLIE
 */
void CAMJob(Job*, Job, Job);


/*
 * tell the kernel to schedule other stuff first. TODO impliment
 */
void yield(void); 

/*
 * Pick a target to steal from, returns the procIdx of the victim
 */
int getVictim(void) {
     //TODO pick a random method that makes sense, could just use stdlib.h rand(), need to set seed somewhere
}

/*
 * get top/bottom of the deque of another proc by idx
 */
int getTop(int targetIdx) {
     return tops[targetIdx];
}
int getBot(int targetIdx) {
     return bots[targetIdx];
}
