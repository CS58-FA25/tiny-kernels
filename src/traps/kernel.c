#include "traps/trap.h"
#include "traps/memory.h"

#include "kernel.h"
#include "proc.h"
#include "queue.h"
#include "kernel.h"
#include "mem.h"
#include "syscalls/process.h"
#include "syscalls/tty.h"
#include <hardware.h> 

static void trapHandlerHelper(void *arg, PCB *process);

void KernelTrapHandler(UserContext* ctx) {
    int syscall_number = ctx->code;
    switch (syscall_number) {
        case YALNIX_DELAY:
            TracePrintf(TRAP_TRACE_LEVEL, "Yalnix Delay Syscall Handler\n");
            TracePrintf(0, "process PID %d delaying for %d ticks\n", current_process->pid, ctx->regs[0]);
            int delay = ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));

            int delay_result = Delay(delay);
            if (delay_result == ERROR) {
                TracePrintf(TRAP_TRACE_LEVEL, "ERROR: Failed to execute delay syscall!\n");
                Halt();
            }

            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = delay_result;
            break;
        case YALNIX_BRK:
            TracePrintf(0, "Executing Brk syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));

            void *addr = (void *)ctx->regs[0];
            int brk_result = Brk(addr);
            if (brk_result == ERROR) {
                TracePrintf(TRAP_TRACE_LEVEL, "Failed to execute Brk syscall!\n");
                Halt();
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = brk_result;
            break;
         case YALNIX_FORK:
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Fork syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int fork_result = Fork();
            if (fork_result == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL, "Failed to execute Fork syscall!\n");
               Halt();
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            break;
         case YALNIX_GETPID:
            TracePrintf(TRAP_TRACE_LEVEL, "Executing GetPid syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int pid_result = GetPid();
            if (pid_result == -1) {
               TracePrintf(0, "Failed to execute syscall GetPid!\n");
               Halt();
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = pid_result;
            break;
         case YALNIX_WAIT:
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Wait syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int *status_ptr = (int *)ctx->regs[0];
            int wait_pid = Wait(status_ptr);
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = wait_pid;
            break;
         case YALNIX_EXIT:
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Exit syscall for process PID %d\n", current_process->pid);
            int exit_status = ctx->regs[0];
            Exit(exit_status);
            break;
         case YALNIX_EXEC:
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Exec syscall for process PID %d\n", current_process->pid);
            char *filename = (char *)ctx->regs[0];
            char **argvec = (char **)ctx->regs[1];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int exec_result = Exec(filename, argvec);
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = exec_result;
            break;
         case YALNIX_TTY_WRITE:
            TracePrintf(TRAP_TRACE_LEVEL, "Executing TtyWrite syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int write_id = (int) ctx->regs[0];
            void* write_buf = (void*) ctx->regs[1];
            int write_len = (int) ctx->regs[2];
            // The result here isn't important enough to crash with that operation.
            int write_result = TtyWrite(write_id, write_buf, write_len);
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = write_result;
            break;
         case YALNIX_TTY_READ:
            TracePrintf(TRAP_TRACE_LEVEL, "Executing TtyRead syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int read_id = (int) ctx->regs[0];
            void* read_buf = (void*) ctx->regs[1];
            int read_len = (int) ctx->regs[2];
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = TtyRead(read_id, read_buf, read_len);
            break;
    }

}

void trapHandlerHelper(void *arg, PCB *process) {
   if (process->delay_ticks >= 0) {
      process->delay_ticks--;
      if (process->delay_ticks == -1) {
            TracePrintf(0, "Process PID %d delay has elapsed!\n", process->pid);
            queueRemove(blocked_queue, process);
            process->state = PROC_READY;
            queueEnqueue(ready_queue, process);
         }
   }
}
