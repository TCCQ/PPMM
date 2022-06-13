#include "capsule.h"
/*
 * This is the small implimentation for binary joins, see fork and
 * join in scheduler.c
 */
typedef unsigned char BYTE;

typedef struct {
     BYTE data; //top byte is counter, bottom is data
     Capsule continuation;
} Set;

Set SetInitialize(Capsule);
