#include "capsule.h" 
/* Interally visible stuff for the scheduler code */

/*
 * A 'job' is an entry on a scheduling deque.
 * A 'capsule' is a block of code with capsule boundaries on either side, as in the paper.
 * A job is one or more capsules.
 */

/* compile time constants */
#define NUM_PROC 4
#define STACK_SIZE 256

/* readability */
typedef unsigned char boolean;

/* job struct declartions */
typedef unsigned char id_t;
typedef unsigned char counter_t;
enum JOB_IDS{
     emptyId=0, localId, scheduledId, takenId
};

/* this is the type that the deque should be. */
typedef struct {
     id_t id;
     counter_t counter;
     union {
	  Job* target;
	  Capsule work;
     }
}  Job;

/* TODO add new/delete functions and other housekeeping */
/*
 * These are to be used when creating a entry for the first time,
 * after that the make___ version should be used everyehre I think
 * 
 * not all of these will be used I think, but they are here for now
 */
Job newEmpty(void);
Job newEmptyWithCounter(int);
Job newLocal(void);
Job newScheduled(void);
Job newTaken(const Job*); //this isn't useful, use make version (TODO remove(?))

/* these make a copy of the passed job and change the job type */
Job makeEmpty(const Job*); //TODO remember to increment counter
Job makeLocal(const Job*);
Job makeScheduled(const Job*);
Job makeTaken(const Job*, const Job*, int); //takes counter copy too
/*
 * this copies the first arg and makes it point to the second
 * this also copies the counter value of the output location and stores in in the taken entry
 */
Job makeCopyJob(const Job*); //works for all types.

//TODO should these take pointers or values?
/* getters */
int getCounter(const Job*);
int getId(const Job*);
/* dumb checks */
boolean isEmpty(const Job*);
boolean isLocal(const Job*);
boolean isScheduled(const Job*);
boolean isTaken(const Job*);
/* Comparison, == but for jobs, 1(true) for equal, 0(false) to != */
boolean CompareJob(const Job*, const Job*); 


/* CAM specifically for jobs, (target, expected, replaced) */
void CAMJob(Job*, const Job, const Job);


/*
 * tell the kernel to schedule other stuff first.
 */
void yield(void); 

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
/* TODO this should be a PM pointer, currently this is being allocated here in EM */
Job deques[NUM_PROC][STACK_SIZE];
