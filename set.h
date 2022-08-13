#include "scheduler.h"
#include "capsule.h"
#include "typesAndDecs.h"
/*
 * This is the small implimentation for binary joins, see fork and
 * join in scheduler.c
 */

struct swappable {
     int owner; 
     byte data; // counter
     byte isLast; //bottom bit is last edit, 2nd bit is in use
     
     /* pad to atomic'able size */
     byte padding[8-sizeof(struct swappable)];
}

typedef struct {
     struct swappable tagAndData;
     Job continuation;
} Set;

Capsule getAndInitSet(void);

Capsule checkOut(void);

Capsule checkOutCnt(void);

Capsule loopGetSet(void);

Capsule acquireSet(void);

Capsule gotSet(void);

Capsule releaseSet(void);

Capsule preHandoverEdits(void);

Capsule cleanup(void);

Capsule cleanupCnt(void);
