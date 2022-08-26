CC=gcc
BOTH_FLAGS=-std=gnu18 -Wall -Wextra -g -O0
#consider what C standard to use
COMP_FLAGS=-c
LINK_FLAGS=

COMP_COMMAND=$CC $BOTH_FLAGS $COMP_FLAGS $< -o $@
#assumes one .c per .o and that it is the first dependency. could maybe use $^?

LINK_COMMAND=$CC $BOTH_FLAGS $LINK_FLAGS $^ -o $@


#this is the final output of the project, link against something that defines 'Capsule ppmmStartCap(void);' and output an executable to run usercode

ppmm.o:main.o memUtilities.o procMap.o pStack-internal.o \
 scheduler.o scheduler-convience.o set.o setup.o
	$LINK_COMMAND


###
# deps are generated with gcc -MM *.c

capsule.o: capsule.c capsule.h set.h scheduler.h memUtilities.h \
 typesAndDecs.h pStack-internal.h pStack.h procMap.h
	$COMP_COMMAND

main.o: main.c capsule.h set.h scheduler.h memUtilities.h typesAndDecs.h \
 setup.h procMap.h pStack-internal.h
	$COMP_COMMAND

memUtilities.o: memUtilities.c memUtilities.h capsule.h set.h scheduler.h \
 typesAndDecs.h pStack.h pStack-internal.h assertion.h
	$COMP_COMMAND

procMap.o: procMap.c memUtilities.h capsule.h set.h scheduler.h \
 typesAndDecs.h assertion.h procMap.h
	$COMP_COMMAND

pStack-internal.o: pStack-internal.c capsule.h set.h scheduler.h \
 memUtilities.h typesAndDecs.h pStack-internal.h pStack.h
	$COMP_COMMAND

scheduler.o: scheduler.c scheduler.h capsule.h set.h memUtilities.h \
 typesAndDecs.h flow.h pStack.h pStack-internal.h
	$COMP_COMMAND

scheduler-convience.o: scheduler-convience.c scheduler.h capsule.h set.h \
 memUtilities.h typesAndDecs.h assertion.h
	$COMP_COMMAND

set.o: set.c set.h scheduler.h capsule.h memUtilities.h typesAndDecs.h \
 flow.h procMap.h pStack.h pStack-internal.h
	$COMP_COMMAND

setup.o: setup.c typesAndDecs.h setup.h assertion.h memUtilities.h \
 capsule.h set.h scheduler.h procMap.h
	$COMP_COMMAND
