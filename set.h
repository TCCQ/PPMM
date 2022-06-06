/* TODO inclusions */

/*
 * This file gives the declaration for a set object. The set contains
 * some number of unique positive integers. They can be added and
 * removed, and the set size can be checked.
 */

typedef struct {
     /* TODO implimentation */
} Set;

/* TODO decide if this is the right data type/size */
typedef unsigned int number;

/* readability */
typedef unsigned char boolean;



/*
 * Construct and initialize a new Set object
 */
Set* setNew(void);

/*
 * Cleanly destroy a set object.
 *
 * TODO Maybe this should only be used for Sets on the heap? maybe
 * not, I'll figure it out or split it into two functions maybe.
 */
void setDestroy(Set*);

/*
 * Add an new element to the set. If the element is already in the
 * Set, then nothing happens.
 */
void setAdd(Set*, number);

/*
 * Remove an element of the set. If the element is not in the set,
 * nothing happens.
 */
void setRemove(Set*, number);

/*
 * Return false (zero) if the set is not empty. Return true (one) if
 * it is empty.
 */
boolean setEmpty(Set*);

/*
 * Returns the number of elements in the set currently.
 *
 * TODO worry about if this and the above are atomic or if not what
 * safety is needed.
 */
size_t setSize(Set*);
