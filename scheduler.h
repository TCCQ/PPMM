#ifndef CAPSULE_INCLUDED
#include "capsule.h"
#endif

#include "memUtilities.h"
#include "typesAndDecs.h"
/* Interally visible stuff for the scheduler code */

/*
 * A 'job' is an entry on a scheduling deque.
 * A 'capsule' is a block of code with capsule boundaries on either side, as in the paper.
 * A job is one or more capsules.
 */



/* job struct declartions */
typedef unsigned char id_t;
typedef unsigned char counter_t;
enum JOB_IDS{
     emptyId=0, localId, scheduledId, takenId
};

struct jobSwappable {
     unsigned char id;
     unsigned char counter;
};

/*
 * this is the type that the deque should be. Not all the fields will
 * have defined values at all times.
 */
typedef struct {
     struct jobSwappable tag;
     PMem target; //pointer to single Job
     Capsule work;
     byte args[JOB_ARG_SIZE];
     int argSize;
}  Job;


Job newEmptyWithCounter(int); //make an empty entry with the counter set
/* these make a copy of the passed job and change the job type */
Job makeEmpty(Job); 
Job makeLocal(Job);
Job makeScheduled(Job);
Job makeTaken(Job, PMem, int); //takes counter copy too
/*
 * this copies the first arg and makes it point to the second
 * this also copies the counter value of the output location and stores in in the taken entry
 */
Job makeCopyJob(Job); //works for all types.

/* getters */
int getCounter(Job);
int getId(Job);
/* dumb checks */
boolean isEmpty(Job);
boolean isLocal(Job);
boolean isScheduled(Job);
boolean isTaken(Job);
/* Comparison, == but for jobs, 1(true) for equal, 0(false) to != */
boolean CompareJob(Job, Job); 


/* CAM specifically for jobs, (target, expected, replaced) */
void CAMJob(Job*, Job, Job);

/* 
 * for Job continuations basically. takes a job as an argument. Used
 * in the join call to start the output thread after both sides have
 * joined
 */
Capsule singleJobPush(void);

/* 
 * declared so it can be jumped back to from things like joins 
 */
Capsule scheduler(void);

/*
 * Pick a target to steal from, returns the procIdx of the victim
 */
int getVictim(void);

/*
 * get top/bottom of the deque of another proc by idx
 */
int getTop(int);
int getBot(int);

/* declaration of WS-deques as 2d-array */

/* this is a PM pointer to an array of length NUM_PROC, each is array
 * of Jobs (STACK_SIZE)*/
PMem deques; //is PM array of size: [NUM_PROC][STACK_SIZE]

PMem tops; //PM array of top indicies size NUM_PROC
PMem bots; //PM array of top indicies size NUM_PROC
