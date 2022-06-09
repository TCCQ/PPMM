# PPMM
Implementation of the Parallel Persistent Memory Model

TODO explain what this project is



----------------
Mechanics explination

How the scheduler works:
Read the paper

How capsules work:
They are a struct containing arguments and a pointer to a function
that does the work of the capsule and returns the continuation. They
are in theory tail calls, but word on the street is that you can't
trust GCC to fully optimize them, so they are trampolined instead. 
