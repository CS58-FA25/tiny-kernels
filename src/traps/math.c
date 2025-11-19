#include <hardware.h>

#include "syscalls/syscall.h"
#include "proc.h"

void MathTrapHandler(UserContext* ctx) {
   TracePrintf(0, "Kernel: Math trap in PID %d at user PC 0x%x. Terminating process.\n", current_process->pid, ctx->pc);
   Exit(ERROR); // Killing the process

   TracePrintf(0, "Error: Exit syscall returned in MathTrapHandler! This shouldn't happen. Halting the machine!\n");
   Halt();
}

