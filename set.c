#include "set.h"
#include "capsule.h"
#include "flow.h" //join declaration
#include "memUtilities.h"
#include "procMap.h" //hard who am I
#include "pStack.h"
#include "scheduler.h"
#include "assertion.h"
#include "typesAndDecs.h"

/* 
 * points to a SET_POOL_SIZE array of sets in PM
 */
PMem setPool;

Set* quickGetSet(int idx) {
     return ((Set*) PMAddr(setPool))+idx;
}

/* 
 * persistent function to be used with pcall, takes continuation Job
 * as an argument, and a owner tag, acquires a new set and initializes it. Then returns
 * the idx of that set in the pool. this should be stored and
 * retrieved during a join call. 
 */
Capsule getAndInitSet(void) {
     pPushCalleeArg((int)0);
     //Job, owner goes through to cnt
     pcall(&loopGetSet, &gotSet);
}

Capsule aquireSet(void);

/* 
 * gets an int, tries to acquire said set
 *
 * does not block, fail assert if goes off the end of the list,
 * consider if that is acceptable 
 */
Capsule loopGetSet(void) {
     int idx;
     pPopArg(idx);

     rassert(idx < SET_POOL_SIZE, "could not allocate an additional set from set pool");
     
     Set* ptr = quickGetSet(idx); //in pool
     if (ptr->tagAndData.data) { //TODO atomic read. write is a CAM
	  pPushCntArg(idx+1) //move on, this is in use
	  pcnt(&loopGetSet);
     } else {
	  //open? try to grab
	  pPushCntArg(ptr->tagAndData);
	  pPushCntArg(idx);
	  pcnt(&aquireSet);
     }
}

/* 
 * gets idx of attempted acquire, ret idx on success, popping
 * newOwner, cnt loop+1 on
 * fail, repush newOwnser (DOES NOT BLOCK, safe)
 */
Capsule aquireSet(void) {
     int idx;
     struct swappable old;
     pPopArg(old);
     pPopArg(idx);
     int newOwner;
     pPopArg(newOwner); //from getAndInitSet

     struct swappable replacement;
     replacement.data = 2;
     replacement.owner = newOwner;
     replacement.isLast = 0; //not in use;
     
     Set* ptr = quickGetSet(idx);
     CAM(&(ptr->tagAndData), old, replacement);

     if (ptr->tagAndData.owner == newOwner) {
	  //success!, return idx
	  pret(idx);
     } else {
	  //failed, back to loop
	  pPushCntArg(newOwner); //save for next time
	  pPushCntArg(idx+1);
	  pcnt(&loopGetSet);
     }
}

/* 
 * gets a idx of a grabbed set and a Job, do init
 */
Capsule gotSet(void) {
     int idx;
     Job postJoin;
     pPopArg(idx);
     pPopArg(postJoin);
     
     Set* setPtr = quickGetSet(idx);

     //owner is set already
     setPtr->continuation = postJoin;
     pret(idx);
}


/* 
 * takes no args, jumps to scheduler or cnt depending on order, reach
 * via pcnt 
 */
Capsule join(void) {
     Capsule installed = quickGetInstalled();
     int mySet = installed.joinLoc;
     byte mySide = installed.forkSide;
     int expectedOwner = installed.expectedOwner;
     pPushCntArg(mySide);
     pPushCntArg(expectedOwner);
     pPushCntArg(mySet);
     pcnt(&checkOut);
}

/* 
 * takes idx and side
 */
Capsule checkOut(void) {
     int idx;
     int expectedOwner;
     byte mySide;
     pPopArg(idx);
     pPopArg(expectedOwner);
     pPopArg(mySide);

     Set* ptr = quickGetSet(idx);

     struct swappable current;
     struct swappable rep;

     /* 
      * This is slightly strange, but I am worried about proceeding
      * thinking that a CAM has worked without checking even in
      * situation where I think it is safe. so even if you edit to be
      * the last, then you still go around again to check that you
      * are. Someone else can go over this and confirm that it is safe
      * and optimize or whatever later
      */
     while (true) {
	  current = ptr->tagAndData;
	  rep.owner = expectedOwner;
     
	  if (current.owner == expectedOwner) {
	       if (current.data == 2) {
		    //untouched
		    rep.data = 1;
		    rep.isLast = 0x02 | mySide;
		    CAM(&(ptr->tagAndData), current, rep);
	       
	       } else if (current.data == 1 && current.isLast == (0x02 | mySide)) {
		    //I was the last edit and the first to arrive, free to leave
		    pcnt(&scheduler);
	       
	       } else if (current.data == 1 && current.isLast == (0x02 | ((~mySide) & 0x01))) {
		    //I am second to arrive and the other guy has edited
		    rep.data = 0;
		    rep.isLast = 0x02 | mySide;
		    CAM(&(ptr->tagAndData), current, rep); 
	       
	       } else if (current.data == 0 && current.isLast == (0x02 | mySide)) {
		    //I was last edit and last to arrive, do the work
		    pPushCntArg(idx);
		    pcnt(&cleanup);
	       
	       } else if (current.data == 0 && current.isLast == (0x02 | ((~mySide) & 0x01))) {
		    //I was not the last edit or the last to arrive, am
		    //free to go
		    pcnt(&scheduler);
	       
	       } else {
		    /*
		     * current.data == 0 && !(current.isLast & 0x02)
		     *
		     * I am last to arrive and the other guy has started
		     * cleanup
		     */
		    pcnt(&scheduler);
	       }
	  } else {
	       /* 
		* owner has changed, the other guy has cleaned up and
		* someone else has moved in
		*/
	       pcnt(&scheduler);
	  }	  
     }
}

/* 
 * I was last and have to pick up the work. takes idx
 */
Capsule cleanup(void) {
     int idx;
     pPopArg(idx);

     Set* ptr = quickGetSet(idx);
     pPushCntArg(ptr->continuation);
     pPushCntArg(idx);
     pcnt(&cleanupCnt);
}

Capsule cleanupCnt(void) {
     int idx;
     pPopArg(idx);

     quickGetSet(idx)->tagAndData.isLast = 0; //not in use anymore
     //job goes through
     pcnt(&singleJobPush); //handover to scheduler to work there
}
