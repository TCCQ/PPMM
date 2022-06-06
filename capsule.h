/*
 * declarations for capsule code
 */

typedef struct {
     /* TODO implment (use setjmp/longjmp, read man pages) */
} Capsule;

/*
 * installs a new capsule 
 */
void commit(Capsule);

/*
 * creates a capsule at the current location and installs it
 */
void commitCurrent(void);

/* TODO add initializers / constructors / deletor for Capsule */
