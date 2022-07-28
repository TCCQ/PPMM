//TODO includes for size_t

/*
 * This should declare the internals needed to map processes to indicies.
 *
 * We need to be able to look up idx -> pid, pid -> idx, and check if pid is alive
 *
 * Solution is an data structure with idx,pid pairs. Then
 * occasionally, some process will go through and confirm that
 * everyone is alive and update the table (run through, check all
 * pids, update isLive status for pair). A new proc needing an idx
 * runs through and swaps its own pid into the struct for a dead proc.
 *
 * TODO this setup doesn't distinguish hard and soft faults. I can
 * check that someone is dead, they are, and then they get resurrected
 * as a diff proc and both assume they are fine. This won't work as it
 * is
 *
 * maybe have the idx passed as an arg to the process? then a long
 * time without resurrection is "hard"? not very reliable
 */

//should have a single header for custom types/aliases
typedef unsigned char boolean;

//emphemeral leaf call, would be persistent if updating the table, as that requires cam cap
boolean isLive(int testIdx) {
     //TODO consider doing the check here instead of updating the table regularly
     return mapping[testIdx].live;
}

//need getIdx, should hang in check-yield loop if none are available (could exit ig)
int getIdx(void) {
     boolean cnt = true;
     int currentCheckIdx = 0;
     while (cnt) {
	  
     }
}

//need countOff, should update table

struct procData {
     size_t pid; //TODO right type?
     int idx; //confirm this is the right type
     boolean live;
     int owner; //needs to be CAMable, prevents collisions, same type as procIdx
};

struct procData mapping[NUM_PROC];
/*
 * TODO NUM_PROC is declared in scheduler.h currently, but should be with
 * the types and whatnot in a seperate file
 *
 * TODO fix, this needs to be in persistent memory
 */

