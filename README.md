# PPMM
This is a userspace implementation of a work stealing parallel scheduler that is correct and efficient under fixed probability hard and soft faults. It is based on *The Parallel Persistent Memory Model* by Blelloch Et. Al. The initial work on this project was funded by the *Reed College Science Research Fellowship for Faculty-Student Collaborative Research*.

## Terms
**Capsule**: A block of code and the data required to execute it. \
**Thread**: A series of consecutive capsules that should be executed in order. \
**Job**: A representation of a thread in the scheduler. \
**Fork**: A thread splits into two child threads. \
**Join**: A thread synchronizes with it's sibling from the fork that created it. When both siblings arrive, the continuation of the forking thread is run. This effectively destroys the joining child thread. \
**Process**: A computation unit capable of executing code independently of all other processes. There are a fixed maximum number of processes for a given instance of the scheduler. Each process faults independently of the others. \
**Ephemeral Memory**: Memory accessible only by the process that owns it. It is lost on fault. \
**Persistent Memory**: Memory which is accessible to every process. It preserves its state on fault. \
**Soft Fault**: The process faulting restarts it's current capsule. Its ephemeral memory is in an undefined state. \
**Hard Fault**: The process faulting dies and never resurrects. Its ephemeral memory is lost. \
**Theft**: When a process steals work that has not been started from another process's scheduler. \
**Grave Robbing**: When a process steals work that has been started by a process that has hard faulted. \
**Hard Who Am I**: The index of the process that is performing this call. \
**Soft Who Am I**: The index of the process that the work being performed is associated with. Associations are set at scheduler to user code boundaries and are preserved under grave robbing.

## Reading this document
"The paper" will refer to *The Parallel Persistent Memory Model* unless otherwise specified. Capsules are assumed to be Idempotent, and when in relevant locations, atomically idempotent, exceptions will be noted. See the paper for details on what that entails.

## Translations of the paper
The scheduling code is an almost direct translation of the scheduling code from the paper. It features one additional tool for internal use, namely pushing a single Job into the scheduler for use in joins and at the beginning to start the initial user code. \
\
The Capsule representation is loosely based on the paper but noticeably edited. It contains a function pointer to facilitate smooth transitioning into and out of the code associated with that capsule. Currently, it also includes the information to make the join associated with the thread the capsule is part of. This could possibly be moved to the associated Job in the future. Finally it contains the index of the process that this capsule is associated with. Note that this is not always the same as the process executing this capsule. This is a soft who am I. \
\
The Job representation is similar to that of the paper, it contains a tag that allows safe operations between multiple faulting processes and the starting capsule of a thread. However it also contains the arguments (the data the capsule acts on) for the first capsule.

## Persistent Stack
After the arguments for the first capsule of a thread are collected from the Job associated with the thread and the fist capsule is started, a mechanism is required to pass data between capsules in a safe, scalable, and readable way. Said mechanism is the persistent stack. It operates as a pared down version of a traditional C program stack. Each capsule can push arguments for its continuation or from a capsule that it calls as a function. Once arguments are in place, a capsule can either pass control directly to a continuation capsule, or can pass control to a capsule called as a function and specify a second capsule to use as a continuation when the function capsule returns. Note that the function capsule can call other functions or use continuations before returning. On return the callee can pass an argument to the caller's continuation. Thus the traditional program flow of a C program is largely maintained in this fault safe environment. \
\
A current limitation of the persistent stack is that it is a read only stack, in that arguments are written, and from the perspective of the continuation or the callee (the receiver of the arguments), they are read only. Thus there are no statically / stack allocated pieces of persistent memory, as they cannot be written to by the owner of the stack frame. It is unsafe to write to one's own stack frame because it is a write after read conflict. For more information, read the paper. \
\
The persistent stack operates entirely within the confines of a single thread. Operations (fork and join) that influence or control multiple threads occur above the persistent stack level and thus argument passing from child to parent thread must occur by passing pointers for data to children and having them fill them before joining.

## Dynamic Memory
Ephemeral memory can be used as usual, just as one would normally use malloc and free. The restriction is that the allocation and deallocation must occur in the same capsule. For dynamic memory that is valid across capsule boundaries, persistent memory must be used. This implementation provides a system for dynamic persistent memory. This system is entirely original and is not based on anything from the paper.

## Remaining Work
- Naturally more testing is required everywhere
- The dynamic memory system needs testing and the ephemeral dynamic memory needs safety checks under faults.
- currently the simulation of faults is incomplete and untested. For parallel instances, hard faults exist if the process dies, but soft faults are currently not implemented
- testing testing testing
