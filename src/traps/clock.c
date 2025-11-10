#include "traps/clock.h"
#include "proc.h"
#include "queue.h"

#include <hardware.h>

static void trapHandlerHelper(void* arg, PCB *process) {
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

void ClockTrapHandler(UserContext* ctx) {
   // Checkpoint 2: Temporary code
   TracePrintf(0, "[CLOCK_TRAP] Clock trap triggered. Ticks: 0x%d\n", (int32_t)(tick_count));

   PCB *curr = current_process;
   memcpy(&curr->user_context, ctx, sizeof(UserContext));
   queueIterate(blocked_queue, NULL, trapHandlerHelper);

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
