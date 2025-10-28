#include "trap.h" // TRAP_TRACE_LEVEL

#include <hardware.h> // UserContext
#include <yalnix.h> // YALNIX_MASK, TracePrintf

/**
 * TODO:
 * if the syscall does not exist, set the return value to be error
 * if the syscall does exist, route necessary data from context to the syscall handler
 *
 * set the return value of the context to be the return value of the syscall handler
 */
void KernelTrapHandler(UserContext* ctx) {
   int code = ctx->code;
   // CP2: Temporary code
   if ((code & ~YALNIX_MASK) == YALNIX_PREFIX) {
      TracePrintf(TRAP_TRACE_LEVEL, "[KERNEL_TRAP] Valid syscall (%x) requested\n", code);
      return;
   }
   // Should NOT REACH HERE!!!
   TracePrintf(TRAP_TRACE_LEVEL, "[KERNEL_TRAP] Invalid syscall (%x) requested\n", code);
}
