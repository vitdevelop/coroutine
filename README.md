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
> - Userspace IO stdout buffer should be disabled `setbuf(stdout, NULL)` because memory leaks detected. (Unknown why)
> - If userspace IO stdout buffer is disabled, make sure that `\n` is put at end of line to prevent _segmentation fault_.
