#include "traps/trap.h"
#include "kernel.h"
#include "proc.h"
#include "queue.h"
#include "kernel.h"
#include "mem.h"
#include "syscalls/process.h"
#include <hardware.h> 
#include "syscalls/tty.h"

void trapHandlerHelper(void *arg, PCB *process);
int growStack(unsigned int addr);

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
      
      // If current was running, put it back in ready status and put in ready queue.
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
         case YALNIX_TTY_READ:
         TracePrintf(TRAP_TRACE_LEVEL, "Executing TtyRead syscall for process PID %d\n", current_process->pid);
         int tty_id = ctx->regs[0];
         void *buf = (void *)ctx->regs[1]; // Probably need to check here if the buffer address is in region 1
         int len = ctx->regs[2];
        
         PCB *curr = current_process;
         memcpy(&curr->user_context, ctx, sizeof(UserContext));
         int rc = TtyRead(tty_id, buf, len);
         // Checking if we managed to read anything and save it in the kernel read_buf and then finally move it to the user space memory at address buf
         if (curr->tty_kernel_read_buf != NULL) {
            memcpy(buf, curr->tty_kernel_read_buf, curr->kernel_read_size);
            free(curr->tty_kernel_read_buf);
            curr->tty_kernel_read_buf = NULL;
            curr->kernel_read_size = 0;
         }

         memcpy(ctx, &curr->user_context, sizeof(UserContext));
         ctx->regs[0] = rc;
         break;

    }

}

void MathTrapHandler(UserContext* ctx) {
   TracePrintf(0, "Kernel: Math trap in PID %d at user PC 0x%x. Terminating process.\n", current_process->pid, ctx->pc);
   Exit(ERROR); // Killing the process

   TracePrintf(0, "Error: Exit syscall returned in MathTrapHandler! This shouldn't happen. Halting the machine!\n");
   Halt();
}


// Does nothing
void NotImplementedTrapHandler(UserContext* ctx) {}


void DiskTrapHandler(UserContext* ctx) {
    NotImplementedTrapHandler(ctx);
}


void MemoryTrapHandler(UserContext* ctx) {
   unsigned int fault_addr = (unsigned int)ctx->addr;
   TracePrintf(0, "Fault address is %u\n", fault_addr);
   unsigned int user_heap_limit_addr = UP_TO_PAGE((unsigned int)(current_process->user_heap_end_vaddr));
   unsigned int user_stack_base_addr = DOWN_TO_PAGE((unsigned int)(current_process->user_stack_base_vaddr));
   if (ctx->code == YALNIX_MAPERR &&
      fault_addr > user_heap_limit_addr &&
      fault_addr < user_stack_base_addr
      )
   {
      TracePrintf(0, "Kernel: Memory Trap Handler growing stack for user process PID %d.\n", current_process->pid);
      int result = growStack(fault_addr);
      if (result == KILL) {
         TracePrintf(0, "Kernel: Memory trap handler killing process PID %d because of a failure at growing user stack!\n", current_process->pid);
         Exit(ERROR);
      }
      return;
   }
   if (ctx -> code == YALNIX_MAPERR)
   if (ctx->code == YALNIX_MAPERR) {
      TracePrintf(0, "Kernel: Error, page is not mapped!\n");
      TracePrintf(0, "Kernel: Killing process PID %d.\n", current_process->pid);
      Exit(ERROR);
   } else if (ctx->code == YALNIX_ACCERR) {
      TracePrintf(0, "Kernel: Invalid memory access for process PID %d!\n", current_process->pid);
      TracePrintf(0, "Kernel: Killing process PID %d.\n", current_process->pid);
      Exit(ERROR);
   }
   return;
}

void IllegalInstructionTrapHandler(UserContext* ctx) {
   void *pc = ctx->pc;
   TracePrintf(0, "Kernel: Process PID %d tried executing an illegal instruction at pc 0x%x. Killing the process!\n", current_process->pid, pc);
   Exit(ERROR); // Killing the process
   
   TracePrintf(0, "Error: Exit syscall returned in IllegalInstructionTrapHandler! This shouldn't happen. Halting the machine!\n");
   Halt();
}


void TtyTrapTransmitHandler(UserContext* ctx) {
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

void TtyTrapReceiveHandler(UserContext* ctx) {
   int tty_id = ctx->code;
   terminal_t *terminal = &terminals[tty_id];

   int len = TtyReceive(terminal, terminal->read_buffer + terminal->read_buffer_len, TERMINAL_MAX_LINE - terminal->read_buffer_len);
   terminal->read_buffer_len += len;

   TracePrintf(0, "TtyTrapReceiveHandler: Terminal tty_id %d now has %d bytes available for reading.\n", terminal->tty_id, terminal->read_buffer_len);
   TracePrintf(0, "TtyTrapReceiveHandler: Checking if there are any processes waiting to read from terminal tty_id %d...\n", terminal->tty_id);

   if (!is_empty(terminal->blocked_readers)) {
      PCB *reader = queueDequeue(terminal->blocked_readers);
      int bytes_to_read = (reader->tty_read_len < terminal->read_buffer_len) ? reader->tty_read_len : terminal->read_buffer_len;
      TracePrintf(0, "TtyTrapReceiveHandler: Process PID %d has woken up to read %d bytes!\n", reader->pid, bytes_to_read);

      char *kernel_buffer = malloc(bytes_to_read);
      if (kernel_buffer != NULL) {
         memcpy(kernel_buffer, terminal->read_buffer, bytes_to_read);
         reader->tty_kernel_read_buf = kernel_buffer;
         reader->kernel_read_size = bytes_to_read;
      } else {
         // In case the allocation failed. Was thinking of killing that process but no need for that
         bytes_to_read = 0;
      }

      // Set return value to number of bytes read
      TracePrintf(0, "TtyTrapReceiveHandler: Process PID %d read %d bytes from terminal tty_id %d.\n", reader->pid, bytes_to_read, terminal->tty_id);
      reader->user_context.regs[0] = bytes_to_read;

      if (bytes_to_read < terminal->read_buffer_len) {
         memmove(terminal->read_buffer, terminal->read_buffer + bytes_to_read, terminal->read_buffer_len - bytes_to_read);
         terminal->read_buffer_len -= bytes_to_read;
      } else {
         // In case we read the whole data on the terminal
         terminal->read_buffer_len = 0;
      }

      // Remove the reader from the blocked queues
      queueRemove(blocked_queue, reader);
      queueRemove(terminal->blocked_readers, reader);

      // Now put back this process into ready queue
      reader->state = PROC_READY;
      queueEnqueue(ready_queue, reader);
   }
   TracePrintf(0, "TtyTrapReceiveHandler: Terminal tty_id %d now has only %d bytes left after processing.\n", terminal->tty_id, terminal->read_buffer_len);

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

int growStack(unsigned int addr) {
   unsigned int addr_aligned_to_page_base = DOWN_TO_PAGE(addr);
   unsigned int stack_base_aligned_to_page_base = DOWN_TO_PAGE((unsigned int)(current_process->user_stack_base_vaddr));

   int target_vpn = (addr_aligned_to_page_base - VMEM_1_BASE) >> PAGESHIFT;
   int stack_base_vpn = (stack_base_aligned_to_page_base - VMEM_1_BASE) >> PAGESHIFT;
   int num_pages_requested = stack_base_vpn - target_vpn;

   pte_t *pt_region1 = current_process->ptbr;
   for (int i = 0; i < num_pages_requested; i++) {
      int pfn = allocFrame(FRAME_USER, current_process->pid);
      if (pfn == -1) {
         TracePrintf(0, "Kernel: Ran out of physical frames while trying to grow user stack for process PID %d!\n", current_process->pid);
         // Need to deallocate all the frames that we already allocated
         for (int j = 0; j < i; j++) {
            int pfn_to_free = pt_region1[target_vpn + j].pfn;
            freeFrame(pfn_to_free);
            pt_region1[target_vpn + j].valid = 0;
            // Do i need to zero out the frame?
         }
         return KILL;
      }

      pt_region1[target_vpn + i].valid = 1;
      pt_region1[target_vpn + i].pfn = pfn;
      pt_region1[target_vpn + i].prot = PROT_READ | PROT_WRITE;
   }
   current_process->user_stack_base_vaddr = addr_aligned_to_page_base;
   TracePrintf(0, "Succesfully grew process PID %d stack to %u.\n", current_process->pid, addr);
   return SUCCESS;
}