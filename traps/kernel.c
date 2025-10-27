#include "trap.h"

void KernelTrapHandler(UserContext* ctx) {
   // read data from context->code
   //
   // check that code is in range of syscalls
   // if the syscall does not exist, set the return value to be error
   // if the syscall does exist, route necessary data from context to the syscall handler
   //
   // set the return value of the context to be the return value of the syscall handler
}
