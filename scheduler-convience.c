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
     out.id = emptyId;
     out.counter = c;
     return out;
}

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
     if (a.id == b.id && a.counter == b.counter) {
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
 * counter. use a substruct to do both at once. TODO CONFIRM WITH CHARLIE
 */
void CAMJob(Job*, Job, Job);


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

