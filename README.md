## Simple coroutines
Simple stackfull coroutines using ucontext

#### Usage:
##### Build
`make`

##### Clean
`make clean`

##### Run
`./main`

##### Library functions
See `coroutine.h` file

> Notes:
> - Userspace IO stdout buffers should be disabled `setbuf(stdout, NULL)` because memory leaks detected. (Unknown why)
