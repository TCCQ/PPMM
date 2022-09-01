#include "typesAndDecs.h"
#include "capsule.h"
#include "assertion.h"
#include "tests.h"
#include "pStack.h"
#include <stdio.h>

funcPtr_t ppmmStartCap = &testMain;

//expects PM to be set up but runs outside of the scheduler for now.
Capsule testMain(void) {
     printf("started the testing suite\n");
     pPushCntArg(7);
     printf("pushed a constant value: 7\n");
     int variable = 12;
     pPushCntArg(variable);
     printf("push variable value: %i\n", variable);
     pcnt(&testCnt);
}

Capsule testCnt(void) {
     printf("reached cnt\n");
     int constant;
     int variable;
     pPopArg(variable);
     pPopArg(constant);
     printf("popped both args, they were, %i, %i\n", constant, variable);
     pPushCntArg(32);
     printf("pushed under variable for after the call: 32\n");
     pPushCalleeArg(50);
     printf("push called arg, 50\n");
     pcall(&testCall, &testPostCall);
}

Capsule testCall(void) {
     printf("reached call\n");
     int arg;
     pPopArg(arg);
     printf("popped callee argument, was %i\n", arg);
     pretvoid;
}

Capsule testPostCall(void) {
     printf("reached post call\n");
     int under;
     pPopArg(under);
     printf("popped argument from under the call, was %i\n", under);
     pcall(&testReturn, &testCleanup);
}

Capsule testReturn(void) {
     printf("reached the returning call, going to return 63\n");
     pret(63);
}

Capsule testCleanup(void) {
     int returned;
     pPopArg(returned);
     printf("popped returned value, was %i\n", returned);
     *((boolean*)PMAddr(trampolineQuit)) = true;
     pcnt(&testCleanup);
}
