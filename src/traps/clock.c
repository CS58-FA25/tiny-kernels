#include "trap.h"
#include "clock.h"

#include "../kernel.h"
#include "../proc.h"
#include "../queue.h"

#include <hardware.h> // UserContext

/**
 * TODO:
 * necessary duration of time has passed
 *
 * update processes, timers of the time change
 * update scheduler of the time change
 * ^
 * |_____ these updates will handle the actions on "time passed"
 *
 * context does not need to be modified for this
 */
void ClockTrapHandler(UserContext* ctx) {
   // Checkpoint 2: Temporary code
   TracePrintf(0, "[CLOCK_TRAP] Clock trap triggered. Ticks: 0x%x\n", tick_count);

   PCB *curr = current_process;
   memcpy(&curr->user_context, ctx, sizeof(UserContext));

   PCB *blocked_curr = blocked_queue->head;
   while (blocked_curr) {
      PCB *next_blocked_proc = blocked_curr->next;
      if (blocked_curr->delay_ticks > 0) {
         blocked_curr->delay_ticks--;
         
         // If the number of ticks originally allocated have already passed, remove it from the blocked queue
         // and add it to the ready queue
         if (blocked_curr->delay_ticks == 0) {
            TracePrintf(0, "Process PID %d delay has elapsed!\n", blocked_curr->pid);
            queueRemove(blocked_queue, blocked_curr);
            blocked_curr->state = PROC_READY;
            queueEnqueue(ready_queue, blocked_curr);

         }
      }
      blocked_curr = next_blocked_proc;
   }

   // TODO: handle stuff with the scheduler in mind
   // For cp3, lets swap out processes at every clock tick
   PCB *next_proc = queueDequeue(ready_queue);
   // If we found another ready process, switch out to it
   if (next_proc) {
      TracePrintf(0, "Switching from PID %d to PID %d", curr->pid, next_proc->pid);
      
      // If current was running, put it back in ready status and put in ready queue
      if (curr->state == PROC_RUNNING) {
         curr->state = PROC_READY;
         queueEnqueue(ready_queue, curr);
      }
      KernelContextSwitch(KCSwitch, curr, next_proc);
   }


   memcpy(ctx, &current_process->user_context, sizeof(UserContext));
}

