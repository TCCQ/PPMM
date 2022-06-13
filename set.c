#include "set.h"
#include "capsule.h"
#include "flow.h" //join declaration
/*
 * the forkPath of the outbound capsule is managed internally and
 * should not be specified by the user
 *
 * This is an ephemeral leaf call
 */
Set SetiInitialize(Capsule cnt) {
     Set out;
     out.data = 0x0003; // 0x00 b00000011
     out.continuation = cnt;
     return out;
}

struct clearArgs {
     Set* s;
     BYTE oldData
     Capsule self;
};

/*
 * Persistent call. Takes the continuation of the join as a sole
 * argument. Cleans the allocated memory for this set object
 * associated with the join
 */
Capsule cleanup(void) {
     Capsule cnt;
     getCapArgs(&cnt);
     Set* toFree = cnt.joinLocs[cnt.joinHead--];
     PMfree(toFree); //matches alloc in fork call
     cnt.forkpath >>= 1;
     persistentCall(cnt);
     /*
      * Since it is unsafe to edit the currently installed capsule, we
      * pop the fork history and whatnot in the next capsule
      */
}

/*
 * TODO this can be cleaned up. I don't need to pass all of self, just
 * the last bit of forkpath. that way there is no danger of having the
 * wrong self struct. I don't think that is an issue, but I think it
 * would be easier to read
 */
Capsule tryClear(void) {
     struct clearArgs args;
     getCapArgs(&args);
     BYTE replacement = args.oldData;
     BYTE mask = 1 << (args.self.forkpath & 1);
     replacement &= ~(mask); //clear bit
     replacement &= ~((~(args.self.forkpath & 1)) << 7);
     /* set the high bit to which side did the editing */
     CAM(&args.s->data, args.oldData, replacement);
     //only left OR right can succeed at 1 time, and the high bit will tell which

     
     if (args.s->data & 0x03 == 0) {
	  /* the data is clear */
	  if (args.s->data >> 7 == args.self.forkpath & 1) {
	       /* this thread was the final clear */
	       persistentCall(mCapStruct(&cleanup, cnt)); 
	  } else {
	       /*
		* this join is done, but not by this side. I must have
		* faulted. The other side will continue, I am done
		*/
	       persistentReturn; 
	  }
     } else {
	  /*
	   * the data is not clear. Either I am here first or my CAM
	   * failed
	   */
	  if (args.s->data == replacement) {
	       /*
		* my cam worked, I am just the first. I am free to leave
		*/
	       persistentReturn; 
	  } else {
	       /* my cam failed, the other guy got here first */
	       args.oldData = args.s->data; //load new data
	       persistentCall(mCapStruct(&tryClear, args)); //try again
	  }
     }
}

/*
 * persistent call that uses currently installed capsule info to perform a join. takes no arguments
 */
Capsule join(void) {
     struct clearArgs args;
     args.s = currentlyInstalled->joinLocs[currentlyInstalled->joinHead];
     args.self = *currentlyInstalled;
     args.oldData = args.s->data;
     persistentCall(mCapStruct(&tryClear, args));
}
