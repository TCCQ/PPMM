CC=gcc
CFLAGS=-std=gnu18 -Wall -Wextra -g -O0
LDFLAGS=-std=gnu18 -Wall -Wextra -g -O0
#consider what C standard to use


#implicit rules, generate with gcc -MM *.c
test: capsule.o main.o memUtilities.o procMap.o \
pStack-internal.o scheduler.o scheduler-convience.o set.o setup.o tests.o
	gcc $^ -o test


capsule.o: capsule.c capsule.h typesAndDecs.h memUtilities.h \
 pStack-internal.h pStack.h procMap.h
main.o: main.c capsule.h typesAndDecs.h memUtilities.h setup.h procMap.h \
 scheduler.h pStack-internal.h assertion.h
memUtilities.o: memUtilities.c memUtilities.h typesAndDecs.h capsule.h \
 pStack.h pStack-internal.h assertion.h
procMap.o: procMap.c memUtilities.h typesAndDecs.h capsule.h assertion.h \
 procMap.h
pStack-internal.o: pStack-internal.c capsule.h typesAndDecs.h \
 memUtilities.h pStack-internal.h pStack.h
scheduler.o: scheduler.c scheduler.h capsule.h typesAndDecs.h \
 memUtilities.h set.h flow.h pStack.h pStack-internal.h
scheduler-convience.o: scheduler-convience.c scheduler.h capsule.h \
 typesAndDecs.h memUtilities.h assertion.h
set.o: set.c set.h scheduler.h capsule.h typesAndDecs.h memUtilities.h \
 flow.h procMap.h pStack.h pStack-internal.h
setup.o: setup.c typesAndDecs.h setup.h assertion.h memUtilities.h \
 capsule.h scheduler.h procMap.h set.h
tests.o: tests.c typesAndDecs.h capsule.h assertion.h tests.h

#end of implicit rules

clean:
	rm -f *.o test
