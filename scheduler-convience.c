#include "scheduler.h"
#include "capsule.h"
#include <sched.h> //for yield
#include "assertion.h"
#include <stdlib.h> //for rand

/*
 * impliementations of the stuff declared in scheduler.h for use in scheduler.c
 */

//new empty with a set counter
Job newEmtpyWithCounter(int c) {
     Job out;
     out.tag.id = emptyId;
     outtag..counter = c;
     return out;
}

/*
 * these make a copy of the passed job and change the job type. They also increment the counter
 */
Job makeEmpty(Job in) {
     in.tag.id = emptyId;
     in.tag.counter++;
     return in;
}
Job makeLocal(Job in) {
     in.tag.id = localId;
     in.tag.counter++;
     return in;
}
Job makeScheduled(Job in) {
     in.tag.id = scheduledId;
     in.tag.counter++;
     return in;
}
Job makeTaken(Job in, const Job* targetJob, int counterCopy) {
     in.tag.id = takenId;
     in.target = targetJob;
     in.tag.counter = counterCopy;
     return in;
}

/* getters */
int getCounter(Job in) {
     return in.tag.counter;
}
int getId(Job) {
     return in.tag.id;
}

/* dumb checks, boolean typedef is unsigned char in scheduler.h */
boolean isEmpty(Job in) {
     return in.tag.id == emptyId;
}
boolean isLocal(Job in) {
     return in.tag.id == localId;
}
boolean isScheduled(Job in) {
     return in.tag.id == scheduledId;
}
boolean isTaken(Job in) {
     return in.tag.id == takenId;
}

/*
 * Comparison, == but for jobs, 1(true) for equal, 0(false) to !=
 *
 * if we move forkpath to Job, compare that instead, as that should be unqiue
 * to a thread of execution, so values can be repeated, but they can
 * never coexist at the same time with another of the same value in
 * two valid Jobs (alive and not stolen and all, in use basically)
 *
 * currently compares all the job stuff, and if those all match then
 * compare the side and join set of the capsules. those are non-unique
 * over the whole program, but should never (I think?) be the same at
 * the same time.
 */
boolean CompareJob(Job a, Job b) {
     if (a.tag.id == b.tag.id && a.tag.counter == b.tag.counter) {
	  if (isTaken(a)) {
	       if (a.target == b.target) return true;
	       else return false;
	  } else {
	       if (a.work.joinLoc == b.work.joinLoc && a.work.forkSide == b.work.forkSide) return true;
	       else return false;
	  }
     } else return false;
} 


/*
 * CAM specifically for jobs, (target, expected, replaced)
 *
 * TODO need to cam id and counter, but Capsules are too big to cam.
 *
 * This is not a complete solution, but it's a start. It would be safe
 * if we could cam id, counter, and target all at once, but the target
 * needs to be a PMem, which is 8 bytes by itself.
 *
 * currently unsafe under adversarial scheduling
 */
void CAMJob(Job* loc, Job old, Job new) {
     CAM(&(loc->target), old.target, new.target);
     if (loc->target.offset == new.target.offset) {
	  //first cam worked
	  CAM(&(loc->tag), old.tag, new.tag);
     } else {
	  //some one else moved in
	  return;
     }
}


/*
 * tell the kernel to schedule other stuff first.
 *
 * we are going to try sched.h, which works on the thread level, but
 * this is (to the OS) a single thread process, so it should work
 */
void yield(void) {
     int ret = sched_yield();
     rassert(ret == 0, "yield failed");
}

/*
 * Pick a target to steal from, returns the procIdx of the victim
 */
int getVictim(void) {
     return rand() % NUM_PROC;
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

