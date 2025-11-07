#include "traps/trap.h"
#include "kernel.h"
#include "proc.h"
#include "queue.h"
#include "kernel.h"
#include "syscalls/process.h"
#include <hardware.h> 

void ClockTrapHandler(UserContext* ctx) {
   // Checkpoint 2: Temporary code
   TracePrintf(0, "[CLOCK_TRAP] Clock trap triggered. Ticks: 0x%d\n", (int32_t)(tick_count));

   PCB *curr = current_process;
   memcpy(&curr->user_context, ctx, sizeof(UserContext));

   PCB *blocked_curr = blocked_queue->head;
   while (blocked_curr) {
      PCB *next_blocked_proc = blocked_curr->next;
      if (blocked_curr->delay_ticks >= 0) {
         blocked_curr->delay_ticks--;
         
         // If the number of ticks originally allocated have already passed, remove it from the blocked queue
         // and add it to the ready queue
         if (blocked_curr->delay_ticks == -1) {
            TracePrintf(0, "Process PID %d delay has elapsed!\n", blocked_curr->pid);
            queueRemove(blocked_queue, blocked_curr);
            blocked_curr->state = PROC_READY;
            queueEnqueue(ready_queue, blocked_curr);

         }
      }
      blocked_curr = next_blocked_proc;
   }

   // For cp3, lets swap out processes at every clock tick
   PCB *next_proc = queueDequeue(ready_queue);

   // If we found another ready process, switch out to it
   if (next_proc) {
      TracePrintf(0, "Switching from PID %d to PID %d\n", curr->pid, next_proc->pid);
      
      // If current was running, put it back in ready status and put in ready queue
      if (curr->state == PROC_RUNNING && curr->pid != 0) {
         curr->state = PROC_READY;
         queueEnqueue(ready_queue, curr);
      }
      KernelContextSwitch(KCSwitch, curr, next_proc);
   }


   memcpy(ctx, &current_process->user_context, sizeof(UserContext));
}

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
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Brk syscall for process PID %d\n", current_process->pid);
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

    }

}

void MathTrapHandler(UserContext* ctx) {
   // if it's possible to get information outside of offending address
   // out of the context, then use this here to inform loggers/kernel/listeners/etc.
   // of the exception
   //
   // kill the offending process
   // notify scheduler that it was killed, do not switch back to it
   // instead, scheduler should handle necessary cleanup (outside of initial cleanup handled here)
}


// Does nothing
void NotImplementedTrapHandler(UserContext* ctx) {}


void DiskTrapHandler(UserContext* ctx) {
    NotImplementedTrapHandler(ctx);
}


void MemoryTrapHandler(UserContext* ctx) {
   // get the ctx->code to determine what type of memory exception this is
   // based on the exception, report this information to whoever needs to be aware (loggers, kernel)
   //
   // kill the process and inform the scheduler of this change
   // ---> scheduler should be aware of upcoming deletion (post-completion)
}

void IllegalInstructionTrapHandler(UserContext* ctx) {
   // get the offending address, current pc
   // report this information
   //
   // kill the process and inform the scheduler of this change
   // ---> scheduler should be aware of upcoming deletion (post-completion)
}

void TtyTrapTxHandler(UserContext* ctx) {
   // get whether this is a tx/rx operation
   //
   // if it's read, then a complete line of input is ready to be read
   // inform anyone waiting on a line of input from the tty
   //
   // if it's a write, then a complete line of input has been written to the tty
   // inform anyone waiting (such as a write handler) that this operation has succeeded
   //
   // update scheduler on fulfillment of requirements for waiters
}

void TtyTrapRxHandler(UserContext* ctx) {

}


