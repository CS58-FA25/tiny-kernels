#include "traps/trap.h"
#include "kernel.h"
#include "proc.h"
#include "queue.h"
#include "kernel.h"
#include "mem.h"
#include "syscalls/process.h"
#include "syscalls/synchronization.h"
#include <hardware.h> 
#include "syscalls/tty.h"

TrapHandler TRAP_VECTOR[TRAP_VECTOR_SIZE] = {
   // Kernel, Clock, Illegal, Memory (0-3)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,

   // Math, Tty Rx, Tty Tx, Disk (4-7)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler, 

   // Unused (8-11)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,

   // EXTRA (12-15)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
};

/* ========= Forward Declarations of helper functions ========= */
void ClockTrapHandlerHelper(void *arg, PCB *process);
int growStack(unsigned int addr);
int CheckBuffer(void *addr, int len);
/* ========= End of forward Declarations of Helpers ========= */

/**
 * ======== ClockTrapHandler =======
 * See trap.h for more details
*/
void ClockTrapHandler(UserContext* ctx) {
   TracePrintf(0, "ClockTrapHandler: Clock trap triggered. Ticks: %d\n", (int32_t)(tick_count));
   PCB *curr = current_process;
   memcpy(&curr->user_context, ctx, sizeof(UserContext));
   queue_iterate(blocked_queue, NULL, ClockTrapHandlerHelper);

   // Round-Robin scheduling
   PCB *next = (is_empty(ready_queue)) ? idle_proc : queue_dequeue(ready_queue);
   if (!(curr == idle_proc && next == idle_proc)) {
      // Only switch if we are not switching from idle to idle
      TracePrintf(0, "ClockTrapHandler: Switching from PID %d to PID %d\n", curr->pid, next->pid);
      curr->state = PROC_READY;
      if (curr != idle_proc) queue_enqueue(ready_queue, curr); // Making sure we don't queue idle process by accident into ready queue
      KernelContextSwitch(KCSwitch, curr, next);
   }

   memcpy(ctx, &current_process->user_context, sizeof(UserContext));
}


/**
 * ======== KernelTrapHandler =======
 * See trap.h for more details
*/
void KernelTrapHandler(UserContext* ctx) {
    int syscall_number = ctx->code;
    switch (syscall_number) {
        case YALNIX_DELAY: {
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
        }
        case YALNIX_BRK: {
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
        }
         case YALNIX_FORK: {
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Fork syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int fork_result = Fork();
            if (fork_result == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL, "Failed to execute Fork syscall!\n");
               Halt();
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            break;
         }
         case YALNIX_GETPID: {
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
         case YALNIX_WAIT: {
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Wait syscall for process PID %d\n", current_process->pid);
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int *status_ptr = (int *)ctx->regs[0];
            int wait_pid = Wait(status_ptr);
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = wait_pid;
            break;
         }
         case YALNIX_EXIT: {
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Exit syscall for process PID %d\n", current_process->pid);
            int exit_status = ctx->regs[0];
            Exit(exit_status);
            break;
         }
         case YALNIX_EXEC: {
            TracePrintf(TRAP_TRACE_LEVEL, "Executing Exec syscall for process PID %d\n", current_process->pid);
            char *filename = (char *)ctx->regs[0];
            char **argvec = (char **)ctx->regs[1];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int exec_result = Exec(filename, argvec);
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = exec_result;
            break;
         }
         case YALNIX_TTY_READ: {
            TracePrintf(TRAP_TRACE_LEVEL, "Executing TtyRead syscall for process PID %d\n", current_process->pid);
            int tty_id = ctx->regs[0];
            void *buf = (void *)ctx->regs[1];
            int len = ctx->regs[2];

            // Ensure pointer is in User Space
            if (CheckBuffer(buf, len) == ERROR) {
                TracePrintf(0, "Trap: Illegal memory access in TtyRead by PID %d\n", current_process->pid);
                ctx->regs[0] = ERROR;
                break;
            }
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = TtyRead(tty_id, buf, len);
            // If TtyRead returned success, we might have data sitting in the kernel stash.
            if (rc > 0 && current_process->tty_kernel_read_buf != NULL) {
               memcpy(buf, current_process->tty_kernel_read_buf, current_process->kernel_read_size);
               // Cleanup the stash
               free(current_process->tty_kernel_read_buf);
               current_process->tty_kernel_read_buf = NULL;
               current_process->kernel_read_size = 0;
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_TTY_WRITE: {
            TracePrintf(TRAP_TRACE_LEVEL, "Executing TtyWrite syscall for process PID %d\n", current_process->pid);
            int tty_id = ctx->regs[0];
            void *buf = (void *)ctx->regs[1];
            int len = ctx->regs[2];

            // Ensure we aren't printing kernel memory secrets
            if (CheckBuffer(buf, len) == ERROR) {
                TracePrintf(0, "Trap: Illegal memory access in TtyWrite by PID %d\n", current_process->pid);
                ctx->regs[0] = ERROR;
                break;
            }
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = TtyWrite(tty_id, buf, len);
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_PIPE_INIT: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing PipeInit syscall for process PID %d.\n", current_process->pid);
            int *pipe_idp = (int *)ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = PipeInit(pipe_idp);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHandler: Error, failed to initialize a pipe for process PID %d.\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_LOCK_INIT: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing LockInit syscall for process PID %d.\n", current_process->pid);
            int *lock_idp = (int *)ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = LockInit(lock_idp);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHandler: Error, failed to initialize a lock for process PID %d.\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_CVAR_INIT: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing CvarInit syscall for process PID %d.\n", current_process->pid);
            int *cvar_idp = (int *)ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = CvarInit(cvar_idp);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHandler: Error, failed to initialize a cvar for process PID %d.\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_PIPE_READ: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing PipeRead syscall for process PID %d.\n", current_process->pid);
            int pipe_id = (int) ctx->regs[0];
            void *buf = (void *) ctx->regs[1];
            int len = (int) ctx->regs[2];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = PipeRead(pipe_id, buf, len);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute PipeRead for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_PIPE_WRITE: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing PipeWrite syscall for process PID %d.\n", current_process->pid);
            int pipe_id = (int) ctx->regs[0];
            void *buf = (void *) ctx->regs[1];
            int len = (int) ctx->regs[2];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = PipeWrite(pipe_id, buf, len);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute PipeWrite for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_LOCK_ACQUIRE: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing Acquire syscall for process PID %d.\n", current_process->pid);
            int lock_id = (int) ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = Acquire(lock_id);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute Acquire for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_LOCK_RELEASE: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing Release syscall for process PID %d.\n", current_process->pid);
            int lock_id = (int) ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = Release(lock_id);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute Release for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_CVAR_WAIT: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing CvarWait syscall for process PID %d.\n", current_process->pid);
            int cvar_id = (int) ctx->regs[0];
            int lock_id = (int) ctx->regs[1];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = CvarWait(cvar_id, lock_id);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute CvarWait for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_CVAR_SIGNAL: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing CvarSignal syscall for process PID %d.\n", current_process->pid);
            int cvar_id = (int) ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = CvarSignal(cvar_id);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute CvarSignal for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_CVAR_BROADCAST: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing CvarBroadcast syscall for process PID %d.\n", current_process->pid);
            int cvar_id = (int) ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = CvarBroadcast(cvar_id);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute CvarBroadcast for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
         case YALNIX_RECLAIM: {
            TracePrintf(TRAP_TRACE_LEVEL, "KernelTrapHanlder: Executing Reclaim syscall for process PID %d.\n", current_process->pid);
            int id = (int) ctx->regs[0];
            memcpy(&current_process->user_context, ctx, sizeof(UserContext));
            int rc = Reclaim(id);
            if (rc == ERROR) {
               TracePrintf(TRAP_TRACE_LEVEL," KernelTrapHandler_ERROR: Failed to execute Reclaim for process PID %d!\n", current_process->pid);
            }
            memcpy(ctx, &current_process->user_context, sizeof(UserContext));
            ctx->regs[0] = rc;
            break;
         }
    }

}

/**
 * ======== MathTrapHandler =======
 * See trap.h for more details
*/
void MathTrapHandler(UserContext* ctx) {
   TracePrintf(0, "Kernel: Math trap in PID %d at user PC 0x%x. Terminating process.\n", current_process->pid, ctx->pc);
   Exit(ERROR); // Killing the process

   TracePrintf(0, "Error: Exit syscall returned in MathTrapHandler! This shouldn't happen. Halting the machine!\n");
   Halt();
}


/**
 * ======== MemoryTrapHandler =======
 * See trap.h for more details
*/
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


/**
 * ======== IllegalInstructionTrapHandler =======
 * See trap.h for more details
*/
void IllegalInstructionTrapHandler(UserContext* ctx) {
   void *pc = ctx->pc;
   TracePrintf(0, "Kernel: Process PID %d tried executing an illegal instruction at pc 0x%x. Killing the process!\n", current_process->pid, pc);
   Exit(ERROR); // Killing the process
   
   TracePrintf(0, "Error: Exit syscall returned in IllegalInstructionTrapHandler! This shouldn't happen. Halting the machine!\n");
   Halt();
}


/**
 * ======== TtyTrapTransmitHandler =======
 * See trap.h for more details
*/
void TtyTrapTransmitHandler(UserContext* ctx) {
   int tty_id = ctx->code;
   terminal_t *terminal = &terminals[tty_id];

   // Do we still have more characters to write?
   if (terminal->write_buffer_position < terminal->write_buffer_len) {
      // Determine how many bytes we can write
      TracePrintf(0, "TtyTrapTransmitHandler: Terminal tty_id %d still has %d bytes to write.\n", tty_id,(terminal->write_buffer_len - terminal->write_buffer_position));
      int bytes_to_write = ((terminal->write_buffer_len - terminal->write_buffer_position) > TERMINAL_MAX_LINE) ? TERMINAL_MAX_LINE : (terminal->write_buffer_len - terminal->write_buffer_position);
      
      TracePrintf(0, "TtyTrapTansmitHanlder: Terminal tty_id %d writing the next %d bytes.\n", tty_id, bytes_to_write);
      TtyTransmit(tty_id, terminal->write_buffer + terminal->write_buffer_position, bytes_to_write);
      terminal->write_buffer_position += bytes_to_write;
      return;

   } else {
      // If we are done with writing every thing into the terminal
      // Do some cleanup
      int bytes_written = terminal->write_buffer_len;
      free(terminal->write_buffer);
      terminal->write_buffer = NULL;
      terminal->write_buffer_len = 0;
      terminal->write_buffer_position = 0;

      // Get the current writer
      PCB *finished_writer = terminal->current_writer;
      terminal->current_writer = NULL;

      if (finished_writer != NULL) {
         TracePrintf(0, "Trap: Waking finished writer PID %d.\n", finished_writer->pid);
         finished_writer->state = PROC_READY;
         finished_writer->user_context.regs[0] = bytes_written; // Set return value
         queue_remove(blocked_queue, finished_writer);
         queue_enqueue(ready_queue, finished_writer);
      }

      // Now, we need to wake any other writers if they is any waiting processes
      if (!is_empty(terminal->blocked_writers)) {
         TracePrintf(0, "Trap: Waking next blocked writer for terminal %d.\n", tty_id);
         PCB *next_writer = queue_dequeue(terminal->blocked_writers);
         
         // Let's wake this process
         next_writer->state = PROC_READY;
         queue_remove(blocked_queue, next_writer);
         queue_enqueue(ready_queue, next_writer);

      } else {
         TracePrintf(0, "Trap: Terminal %d is now free.\n", tty_id);
         terminal->in_use = 0;
      }
   }
}

/**
 * ======== TtyTrapReceiveHandler =======
 * See trap.h for more details
*/
void TtyTrapReceiveHandler(UserContext* ctx) {
   int tty_id = ctx->code;
   terminal_t *terminal = &terminals[tty_id];

   int len = TtyReceive(tty_id, terminal->read_buffer + terminal->read_buffer_len, TERMINAL_MAX_LINE - terminal->read_buffer_len);
   terminal->read_buffer_len += len;

   TracePrintf(0, "TtyTrapReceiveHandler: Terminal tty_id %d now has %d bytes available for reading.\n", terminal->tty_id, terminal->read_buffer_len);
   TracePrintf(0, "TtyTrapReceiveHandler: Checking if there are any processes waiting to read from terminal tty_id %d...\n", terminal->tty_id);

   while (!is_empty(terminal->blocked_readers) && terminal->read_buffer_len > 0) {
      PCB *reader = queue_dequeue(terminal->blocked_readers);
      int bytes_to_read = (reader->tty_read_len < terminal->read_buffer_len) ? reader->tty_read_len : terminal->read_buffer_len;
      TracePrintf(0, "TtyTrapReceiveHandler: Process PID %d has woken up to read %d bytes!\n", reader->pid, bytes_to_read);

      char *kernel_buffer = malloc(bytes_to_read);
      if (kernel_buffer != NULL) {
         memcpy(kernel_buffer, terminal->read_buffer, bytes_to_read);
         reader->tty_kernel_read_buf = kernel_buffer;
         reader->kernel_read_size = bytes_to_read;
         reader->user_context.regs[0] = bytes_to_read;
      } else {
         // In case the allocation failed. Was thinking of killing that process but no need for that
         reader->user_context.regs[0] = 0;
         bytes_to_read = 0;
      }
      // Set return value to number of bytes read
      TracePrintf(0, "TtyTrapReceiveHandler: Process PID %d read %d bytes from terminal tty_id %d.\n", reader->pid, bytes_to_read, terminal->tty_id);
      
      // Shift terminal buffer
      if (bytes_to_read > 0) {
         if (bytes_to_read < terminal->read_buffer_len) {
            memmove(terminal->read_buffer, terminal->read_buffer + bytes_to_read, terminal->read_buffer_len - bytes_to_read);
            terminal->read_buffer_len -= bytes_to_read;
         } else {
            // In case we read the whole data on the terminal
            terminal->read_buffer_len = 0;
         }
      }

      // Remove the reader from the blocked queue
      queue_remove(blocked_queue, reader);

      // Now put back this process into ready queue
      reader->state = PROC_READY;
      queue_enqueue(ready_queue, reader);
   }
}

/* =============== Start of Helper functions =============== */

/**
 * Description: Helper function used in queue_iterate to update the delay ticks for any process blocked
 *              for calling Delay. If a process finished all of its delay ticks, the helper removes it
 *              from blocked queue and puts it in ready queue
 * ======= Parameters ======
 * @param arg: NULL pointer.
 * @param process: Pointer to the PCB of the process we are currently processing in the queue.
 * ====== Return =======
 * @return void
*/
void ClockTrapHandlerHelper(void *arg, PCB *process) {
   if (process->delay_ticks > 0) {
      process->delay_ticks--;
      if (process->delay_ticks == 0) {
            TracePrintf(0, "Process PID %d delay has elapsed!\n", process->pid);

            queue_remove(blocked_queue, process);
            process->state = PROC_READY;
            queue_enqueue(ready_queue, process);
         }
   }
}

/**
 * Description: Grows the stack of the current process to the given address.
 * ======== Parameters ======
 * @param addr: The address to which to expand the current process' stack
 * ======== Return =======
 * @return SUCCESS if growth succedded, KILL otherwise
*/
int growStack(unsigned int addr) {
   unsigned int addr_aligned_to_page_base = DOWN_TO_PAGE(addr);
   unsigned int stack_base_aligned_to_page_base = DOWN_TO_PAGE((unsigned int)(current_process->user_stack_base_vaddr));

   int target_vpn = (addr_aligned_to_page_base - VMEM_1_BASE) >> PAGESHIFT;
   int stack_base_vpn = (stack_base_aligned_to_page_base - VMEM_1_BASE) >> PAGESHIFT;
   int num_pages_requested = stack_base_vpn - target_vpn;

   pte_t *pt_region1 = current_process->ptbr;
   for (int i = 0; i < num_pages_requested; i++) {
      int pfn = AllocFrame(FRAME_USER, current_process->pid);
      if (pfn == -1) {
         TracePrintf(0, "Kernel: Ran out of physical frames while trying to grow user stack for process PID %d!\n", current_process->pid);
         // Need to deallocate all the frames that we already allocated
         for (int j = 0; j < i; j++) {
            int pfn_to_free = pt_region1[target_vpn + j].pfn;
            FreeFrame(pfn_to_free);
            pt_region1[target_vpn + j].valid = 0;
            // Do i need to zero out the frame?
         }
         return KILL;
      }

      pt_region1[target_vpn + i].valid = 1;
      pt_region1[target_vpn + i].pfn = pfn;
      pt_region1[target_vpn + i].prot = PROT_READ | PROT_WRITE;
      WriteRegister(REG_TLB_FLUSH, (unsigned int) (VMEM_1_BASE + ((target_vpn + i) << PAGESHIFT)));
   }
   current_process->user_stack_base_vaddr = addr_aligned_to_page_base;
   TracePrintf(0, "Succesfully grew process PID %d stack to %u.\n", current_process->pid, addr);
   return SUCCESS;
}

/**
 * Description: Checks if a user-space buffer lies within region 1 of the pagetable.
 * ======= Parameters ======
 * @param addr: Pointer to the start of the buffer.
 * @param len: Length of the buffer.
 * ======= Returns =====
 * @return SUCCESS if true, ERROR otherwise
*/
int CheckBuffer(void *addr, int len) {
    unsigned long start = (unsigned long)addr;
    unsigned long end = start + len;
    if (start < VMEM_1_BASE || end > VMEM_1_LIMIT) {
        return ERROR;
    }
    return SUCCESS;
}

/* =============== End of Helper functions =============== */


/* ========= Unimplemented trap handlers =========== */
// Does nothing
void NotImplementedTrapHandler(UserContext* ctx) {}


void DiskTrapHandler(UserContext* ctx) {
    NotImplementedTrapHandler(ctx);
}