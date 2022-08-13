#include <unistd.h>
#include <stdio.h>
#include <fcntl.h> //file opening errors
#include <string.h>
#include <sys/stat.h> //checking if file exists
#include "memUtilities.h"
#include "assertion.h"
#include "scheduler.h" //for yield, but this muddies dependencies and
		       //nessesity order TODO
/* 
 * TODO vis-a-vis yielding, I think that is not the solution anyway,
 * and I am not sure we want to be looping anyway, I think we have
 * settled on just having procs die and stay dead for hard faults, and
 * have soft faults be signal driven (at least for now)
 */
#include "typesAndDecs.h"
/*
 * This should declare the internals needed to map processes to indices.
 *
 * We need to be able to look up idx -> pid, pid -> idx, and check if pid is alive
 *
 * Solution is an data structure with idx,pid pairs. A new proc
 * needing an idx runs through and swaps its own pid into the struct
 * for a dead proc.
 *
 * TODO this setup doesn't distinguish hard and soft faults. I can
 * check that someone is dead, they are, and then they get resurrected
 * as a diff proc and both assume they are fine. This won't work as it
 * is. basically only hard faults exist at the moment
 *
 * maybe have the idx passed as an arg to the process? then a long
 * time without resurrection is "hard"? not very reliable
 *
 * TODO make a header for this file, specifically for hardWhoAmI
 */

struct procData {
     pid_t pid; 
};
/* 
 * TODO consider if I want to be caching the live status. The check is
 * certainly expensive, what with string comparisons and file
 * accesses, but I don't see a very clean way to cache it. This can
 * wait, as isLive is a wrapper
 *
 * TODO this is a struct just in case I want to edit it later, if not
 * consolidate to just pid_t in the functions below
 */


PMem mapping;
/* 
 * PM pointer to a NUM_PROC long array of procData structs. access via
 * getter
 *
 * TODO init
 */


//returns a pointer to said element. can copy, access, or write as needed
struct procData* mapGet(int index) {
     return ((struct procData*) PMAddr(mapping))+index;
}

/* 
 * helper for checkPid, expects line to be at least PROCFS_LINE_SIZE long
 */
int fillCmdline(str path, char* line) {
     /*
      * not sure if this is safe to do? I think files are closed on
      * crash so it should be fine?
      */

     FILE f = fopen(path, "r"); //read only
     rassert(f, "something went wrong opening cmdline file")
     /*
      * format is words separated with \0, and two \0 at the end I am
      * going to read manually, the system stuff doesn't quite fit
      * that
      *
      * the size is arbitrary. if the cmdline is longer than that,
      * then we will only compare the first [size] bytes. should be
      * fine for now
      */
     boolean done = false;
     int cur = 0;
     while (!done) {
	  rassert(fread(line+cur, 1, 1, f), "got to the end of the file while reading cmdline");
	  if (cur != 0 &&
	      line[cur] == 0 && line[cur-1] == 0) {
	       done = true;
	  }
	  cur++;
     }
     //done and cur points 1 past the end
     int ret = fclose(f);
     rassert(ret == 0, "could not close cmdline file");
     return cur-1;
}

boolean checkPid(pid_t test) {
     /*
      * check if a process is still around with a check to
      * /proc/[pid]/cmdline
      *
      * if the process is dead and no other has taken the pid, then
      * the file will not exist. If the process is dead and someone
      * else has taken that pid, then the cmdline will not be the
      * same, it will have a differing command and/or arguments.
      *
      * note that this also means that instances of this program with
      * different starting arguments will not recognize each other,
      * which I think is in the best interests of safety
      *
      * read man procfs
      */
     int targetLength, myLength;
     char targetLine[PROCFS_LINE_SIZE];
     char myLine[PROCFS_LINE_SIZE];
     
     str targetPath = "/proc/";
     char targetNumber[13]; //hold the number
     sprintf(targetNumber, "%d", test); 
     targetPath = strcat(targetPath, targetNumber);
     targetPath = strcat(targetPath, "/cmdline");
     
     struct stat exists;
     int ret = stat(targetPath, &exists);
     if (ret) { //something went wrong
	  rassert(errno == ENOENT, "could not stat cmdline file in checkPid");
	  return false; //file doesn't exist, not alive
     } else {
	  targetLength = fillCmdline(targetPath, targetLine);
	  //TODO unsafe char* to str?
     }
     str myPath = "/proc/";
     char myNumber[13]; //hold the number
     sprintf(myNumber, "%d", getMyPid()); 
     myPath = strcat(myPath, myNumber);
     myPath = strcat(myPath, "/cmdline");

     ret = stat(myPath, &exists);
     rassert(ret == 0, "couldn't stat my own cmdline file");
     myLength = fillCmdline(myPath, myLine);

     //everything should be filled, can compare
     if (myLength != targetLength) return false;
     else if (memcmp(myLine, targetLine, myLength)) return false;
     //cant use strcmp cause they are zero terminated words
     else return true
}

//emphemeral call, would be persistent if updating the table, as that requires cam cap
boolean isLive(int testIdx) {
     struct procData* p = mapGet(testIdx);
     return checkPid(p->pid);
}

pid_t getMyPid(void) {
     return getpid(); //os call, can't fail
}

//only really meaningful if you are already somewhere in the table. TODO consider error message here
int hardWhoAmI(void) {
     pid_t myPid = getMyPid();
     for (int i = 0; i < NUM_PROC; i++) {
	  if (mapGet(i)->pid == myPid) return i;
     }
     return -1;
}

//need getIdx, should hang in check-yield loop if none are available (could exit ig)
void getIdx(void) {
     boolean quit = false;
     while (!quit) {
	  for (int i = 0; i < NUM_PROC; i++) {
	       struct procData* loc = mapGet(i);
	       struct procData copy = *loc;
	       pid_t myPid = getMyPid();
	       if (!checkPid(copy.pid)) { //this does isLive, but we can hold on to the old value for camming
		    //proc is dead, try to take over
		    CAM(&(loc->pid), copy.pid, myPid);
		    if (loc->pid == myPid) {
			 //it worked, get to work
			 trampolineCapsule();
			 quit = true;
			 /*
			  * during the first init, the setup process should
			  * properly initialize all the currently installed
			  * capsules to be the scheduler, except perhaps
			  * one that pushes the first job. TODO
			  */
		    } //failed cam, keep moving
	       }
	  }
	  yield();
     }
}

