#include "memUtilites.h"
#include "pStack.h"
#include "capsule.h"
#include "typesAndDecs.h"
#include "assertion.h"
/* 
 * Implementations for stuff in the corresponding header, particularly
 * the dynamic persistent memory stuff
 */

PMem memTable;
/* 
 * This is a pointer to a MEMORY_TABLE_SIZE long array of entries. The
 * first entry is always in the list and thus a safe place to start
 * iterating over the active section of the table. Null downlinks
 * should link back to the original (value of 0).
 *
 * TODO init, all should start with {_, no, {0,no}, 0, no} except for
 * the initial which should start with {{0,DYNAMIC_POOL_SIZE}, yes,
 * {0, no}, 0, no}.
 */

/* convenience functions for table management */

/* 
 * table index access
 */
entry* quickEntryGet(int idx) {
     return (entry*)(PMAddr(memTable))+idx;
}

void wipeTag(struct entryTag* toWipe) {
     int i = 0;
     byte* output;
     while (i < sizeof(struct entryTag)) {
	  output = ((byte*)toWipe) + i;
	  output = 0;
	  i++;
     }
}

/* 
 * takes index in table of entry to try to grab (note this is NOT the
 * index in the active list). returns true if successful, false otherwise
 */
Capsule tryGrab(void) {
     int idx;
     pPopArg(idx);

     rassert(idx >= 0 && idx < MEMORY_TABLE_SIZE, "index out of bounds on attempted memory table grab");

     int self = getProcIdx();
     entry* interest = quickEntryGet(idx);

     struct entryTag replacement;
     wipeTag(&replacement);
     //wipes replacement to zeros, worry about padding
     replacement.owner = self;
     replacement.isGrabbed = true;
     
     struct entryTag old;
     old = interest->tag;
     
     CAM(&(interest->tag), old, replacement);
     if (interest->tag.owner == self && interest->tag.isGrabbed) {
	  pret((boolean)true);
     } else {
	  pret((boolean)false);
     }
}

/* 
 * takes index in table to release. should never fail. returns nothing
 */
Capsule release(void) {
     int idx;
     pPopArg(idx);

     int self = getProcIdx();
     entry* interest = quickEntryGet(idx);

     if (!interest->tag.isGrabbed || interest->tag.owner != self) {
	  //I have released and faulted, someone else has possibly
	  //moved in
	  pretvoid;
     } else {
	  //I am still grabbed
	  interest->tag.isGrabbed = false; //TODO atomic write
	  pretvoid;
     }
}

/* functions for merging */

//forward decs
Capsule mLoop(void);
Capsule mPostGrab(void);
Capsule mPostSecondGrab(void);
Capsule mReleaseOrginal(void);

/* 
 * go through and try to merge contiguous free memory blocks
 */
Capsule merge(void) {
     pPushCntArg((int)0);
     pcnt(&mLoop);
}

Capsule mLoop(void) {
     int idx;
     pPopArg(idx);

     if (idx >= MEMORY_TABLE_SIZE) {
	  //ran off the end, back to allocating
	  //the desired size is under the merge stack frame and gets
	  //popped by alloc
	  pcnt(&PMalloc);
     }
     
     entry* interest = quickEntryGet(idx);
     if (interest->tag.isGrabbed || interest->isInUse) {
	  pPushCntArg(interest->next);
	  pcnt(&mLoop);
     } else {
	  pPushCalleeArg(idx);
	  pPushCntArg(idx);
	  pcall(&tryGrab, &mPostGrab);
     }
}

Capsule mPostGrab(void) {
     boolean grabbed;
     int idx;
     pPopArg(grabbed);
     pPopArg(idx);

     if (grabbed) {
	  //grabbed, look for next

	  entry* interest = quickEntryGet(idx);
	  if (interest->isInUse || !interest->inList) {
	       //someone changed it before our grab
	       pPushCalleeArg(idx);
	       pPushCntArg(interest->next);
	       pcall(&release, &mLoop);
	  } else {
	       //as expected
	       entry* next = quickEntryGet(interest->next);
	       if (next->tag.isGrabbed || next->isInUse) {
		    //no good, release current and continue
		    pPushCalleeArg(idx);
		    pPushCntArg(interest->next);
		    pcall(&release, &mLoop);
	       } else {
		    //try to grab next
		    pPushCalleeArg(interest->next);
		    pPushCntArg(idx);
		    pcall(&tryGrab, &mPostSecondGrab);
	       }
	  }
     } else {
	  //continue
	  pPushCntArg(quickEntryGet(idx)->next);
	  pcnt(&mLoop);
     }
}

Capsule mPostSecondGrab(void) {
     boolean grabbed;
     int idx;
     int nextIdx;
     pPopArg(grabbed);
     pPopArg(idx);
     pPopArg(nextIdx);

     entry* interest = quickEntryGet(idx);
     entry* next = quickEntryGet(nextIdx);
     if (grabbed) {
	  if (next->isInUse) {
	       //someone changed it pre grab
	       pPushCalleeArg(nextIdx);
	       pPushCntArg(idx);
	       pcall(&release, &mReleaseOrginal);
	  } else {
	       //as expected
	       //we have both grabbed and in proper shape
	       interest->next = next->next;
	       //cuts next out of the list. should be safe?
	       next->inList = false;
	       
	       pPushCntArg(idx);
	       pPushCntArg((boolean)true);
	       pPushCalleeArg(nextIdx);
	       pcall(&release, &mPostGrab);
	       //next, try to merge here again, chaining 
	  }
     }
}

Capsule mReleaseOrginal(void) {
     int idx;
     pPopArg(idx);

     pPushCalleeArg(idx);
     pPushCntArg(quickEntryGet(idx)->next);
     pcall(&release, &mLoop);
}

/* functions for alloc */

//forward decs
Capsule allocatingLoop(void);
Capsule allocatingPostGrab(void);
Capsule makeLeftovers(void);
Capsule mlCnt(void);
Capsule ffeLoop(void);
Capsule ffePostGrab(void);

/* 
 * takes size, allocates and returns PMem or calls merge and tries
 * again, effectively spinning until memory is available.
 */
Capsule PMalloc(void) {
     pPushCntArg((int)0);
     pcnt(&allocatingLoop);
}


/* 
 * takes idx to try, desired size goes through
 */
Capsule allocatingLoop(void) {
     int idx;
     pPopArg(idx);

     if (idx >= MEMORY_TABLE_SIZE) {
	  //ran off the end, run a merge
	  //the desired size goes through
	  //under the merge back into the next alloc
	  pcnt(&merge);
     }
     
     entry* interest = quickEntryGet(idx);

     if (interest->tag.isGrabbed) {
	  //someone is working
	  pPushCntArg(interest->next);
	  pcnt(&allocatingLoop);
	  
     } else if (interest->isInUse) {
	  //already allocated
	  pPushCntArg(interest->next);
	  pcnt(&allocatingLoop);
	  
     } else {
	  //free and open, try to grab and work
	  pPushCntArg(idx);
	  pPushCalleeArg(idx);
	  pcall(&tryGrab, &allocatingPostGrab);
     }
}

Capsule allocatingPostGrab(void) {
     boolean succGrab;
     int idx;
     int desiredSize;
     pPopArg(succGrab);
     pPopArg(idx);
     pPopArg(desiredSize);

     if (succGrab) {
	  entry* interest = quickEntryGet(idx);
	  if (!interest->inList) {
	       //got duped, this is not in the list, release and move
	       //forward
	       pPushCalleeArg(idx);
	       pPushCntArg(desiredSize);
	       pPushCntArg(interest->next);
	       pcall(&release, &allocatingLoop); 
	  }
	  
	  //grabbed, confirm if desirable
	  if (interest->data.size == desiredSize) {
	       //fits, use it
	       interest->isInUse = true; //TODO atomic write?
	       pPushCntArg(idx);
	       pPushCalleeArg(idx);
	       pcall(&release, &retPMem);
	       
	  } else if (interest->data.size > desiredSize) {
	       //larger than we need, make leftover entry
	       interest->isInUse = true; //TODO atomic write?
	       pPushCalleArg(interest->data.size - desiredSize);
	       //leftover size
	       pPushCntArg(desiredSize);
	       pPushCntArg(idx);
	       pcall(&findFreeEntry, &makeLeftovers);
	       
	  } else {
	       //too small for us, release and move on
	       pPushCntArg(desiredSize);
	       pPushCntArg(interest->next);
	       pPushCalleeArg(idx);
	       pcall(&release, &allocatingLoop);
	  }
     }
}

/* 
 * takes idx, returns pMem of said block. is Persistent to use as cnt
 * of pcalls mostly. see allocatingPostGrab
 */
Capsule retPMem(void) {
     int idx;
     pPopArg(idx);
     pret(quickEntryGet(idx)->data);
}

/* 
 * takes nothing, returns an idx to a grabbed free entry that is NOT
 * part of the list. Grabs but does not edit found entry
 */
Capsule findFreeEntry(void) {
     pPushCntArg(0);
     pcnt(&ffeLoop);
}

Capsule ffeLoop(void) {
     int idx;
     pPopArg(idx);

     rassert(idx >= 0 && idx < MEMORY_TABLE_SIZE, "ran off the end looking for a inactive table entry");

     if (quickEntryGet(idx)->inList) {
	  pPushCntArg(idx+1);
	  pcnt(&ffeLoop);
	  //skip
	  
     } else {
	  pPushCalleeArg(idx);
	  pPushCntArg(idx);
	  pcall(&tryGrab, ffePostGrab);
     }
}

Capsule ffePostGrab(void) {
     boolean grabbed;
     int idx;
     pPopArg(grabbed);
     pPopArg(idx);

     if (grabbed) {
	  //we own this sector now
	  if (quickEntryGet(idx)->inList) {
	       //some one changed it before we grabbed it, keep
	       //looking
	       pPushCalleeArg(idx);
	       pPushCntArg(idx+1);
	       pcall(&release, ffeLoop);
	  } else {
	       //grabbed and matches expected
	       pret(idx); 
	  }
     } else {
	  //didn't get it
	  pPushCntArg(idx+1);
	  pcall(&ffeLoop);
     }
}

/* 
 * gets idx of new entry, idx of old entry, desired size, and leftover
 * size. edit as necessary
 */
Capsule makeLeftovers(void) {
     int leftoverIdx;
     int allocatedIdx;
     int desiredSize;
     int leftoverSize;
     pPopArg(leftoverIdx);
     pPopArg(allocatedIdx);
     pPopArg(desiredSize);
     pPopArg(leftoverSize);

     entry* allocated = quickEntryGet(allocatedIdx);
     entry* leftover = quickEntryGet(leftoverIdx);

     leftover->data.size = leftoverSize;
     leftover->data.offset = allocated->data.offset + desiredSize;
     leftover->isInUse = false;
     leftover->next = allocated->next;

     allocated->data.size = desiredSize;
     pPushCalleeArg(leftoverIdx);
     pPushCntArg(allocatedIdx);
     pPushCntArg(leftoverIdx);
     pcall(&release, &mlCnt);
}

Capsule mlCnt(void) {
     int leftoverIdx;
     int allocatedIdx;
     pPopArg(leftoverIdx);
     pPopArg(allocatedIdx);

     quickEntryGet(allocatedIdx)->next = leftoverIdx;
     pPushCalleeArg(allocatedIdx);
     pPushCntArg(allocatedIdx);
     pcall(&release, &retPMem);
}



/* functions for free */

//forward dec
Capsule freeLoop(void);
Capsule freePostLoop(void);

/* 
 * takes pmem, goes through to loop.
 */
Capsule PMfree(void) {
     pPushCntArg(0);
     pcnt(&freeLoop);
}

Capsule freeLoop (void) {
     int idx;
     PMem desired;
     pPopArg(idx);
     pPopArg(desired);
     
     rassert(idx >= 0 && idx < MEMORY_TABLE_SIZE, "ran off the end during free");

     entry* interest = quickEntryGet(idx);
     if (interest->isInUse && interest->data.offset == desired.offset &&
	                      interest->data.size == desired.size) {
	  //matches
	  pPushCntArg(idx);
	  pPushCalleArg(idx);
	  pcall(&tryGrab, &freePostGrab);
     } else {
	  //move on
	  pPushCntArg(desired);
	  pPushCntArg(interest->next);
	  pcnt(&freeLoop);
     }
}

Capsule freePostLoop(void) {
     boolean grabbed;
     int idx;
     pPopArg(grabbed);
     pPopArg(idx);

     assert(grabbed, "failed to grab matching entry during free");

     quickEntryGet(idx)->isInUse = false;

     pPushCntArg(idx);
     pcnt(&release); //will return void, back to usercode
}
