#include "syscalls/tty.h"
#include "kernel.h"

// A note on TtyPrintf: I see that it's declared in the header that has
// most of the syscalls listed
//
// However, I also see that it's something that is already implemented
// in libyuser.a
// So I didn't include it here
// If I were to incluse it, it would end up being a wrapper function for TtyWrite that handles the arguments first
terminal_t terminals[NUM_TERMINALS];

int TtyRead(int tty_id, void *buf, int len) {

   if (tty_id < 0 || tty_id > NUM_TERMINALS || buf == NULL || len <= 0) {
      TracePrintf(0, "TtyRead: Invalid arguments passed!\n");
      return ERROR;
   }
   terminal_t *terminal = &terminals[tty_id];
   PCB *curr = current_process;
   TracePrintf(0, "Kernel: Executing TtyRead syscall for process PID %d...\n", curr->pid);

   // If there's already data to read from terminal, return immediately
   if (terminal->read_buffer_len > 0) {
      int bytes_to_read = (len > terminal->read_buffer_len) ? terminal->read_buffer_len : len; // Number of bytes to read
      TracePrintf(0, "TtyRead: Reading %d bytes from terminal %d into process PID %d.\n", bytes_to_read, terminal->tty_id, curr->pid);

      // Temporary kernel buffer to hold this data
      char *kernel_buffer = malloc(bytes_to_read);
      if (kernel_buffer == NULL) {
         TracePrintf(0, "TtyRead: Failed to allocate space for kernel buffer!\n");
         return ERROR;
      }

      // Copy a number of "bytes_to_read" from the terminal read buffer into our kernel buffer
      memcpy(kernel_buffer, terminal->read_buffer, bytes_to_read);
      curr->tty_kernel_read_buf = kernel_buffer;
      curr->kernel_read_size = bytes_to_read;

      if (bytes_to_read < terminal->read_buffer_len) {
         memmove(terminal->read_buffer, terminal->read_buffer + bytes_to_read,  terminal->read_buffer_len - bytes_to_read);
      }
      terminal->read_buffer_len -= bytes_to_read;
      TracePrintf(0, "TtyRead: Returning %d bytes for process PID %d. %d remaining in terminal tty_id %d.\n", bytes_to_read, curr->pid, terminal->read_buffer_len, terminal->tty_id);

      return bytes_to_read;
   }
   // If there's no data available to be read, block the current process, add it to the waiting queue in the terminal to be woken up later
   // when there's data to read.
   TracePrintf(0, "TtyRead: No data available to read for process PID %d at terminal tty_id %d. Blocking process.\n", curr->pid, terminal->tty_id);
   
   // Add it to queue of blocked readers for this terminal
   queueEnqueue(terminal->blocked_readers, curr);

   // Block the process
   queueEnqueue(blocked_queue, curr);
   curr->state = PROC_BLOCKED;

   // Store the buffer to copy to and also how many bytes needed to read in the process PCB for when the process is woken up
   curr->tty_read_buf = buf;
   curr->tty_read_len = len;

   // Switch to idle or another process
   PCB *next = (is_empty(ready_queue) == 1) ? idle_proc : queueDequeue(ready_queue);
   KernelContextSwitch(KCSwitch, curr, next);

   TracePrintf(0, "TtyRead: process PID %d woken up.\n", curr->pid);
   return curr->user_context.regs[0];
}

int TtyWrite(int tty_id, void *buf, int len) {
   // get tty object from a global tty device map
   // if the tty does not exist (out of range), return error
   // 
   // check that TERMINAL_MAX_LINE is greater than len
   // if len is greater than the max amount of data that can be written (`TERMINAL_MAX_LINE`), return error
   //
   // while the tty is being written to, limit tty access
   // if the tty is in use, wait for the tty
   // 
   // write data of size `len` from `buf` into the tty, to be read by the call `TtyRead`
   // (wait until tty write trap has returned success)
   // return number of bytes written
   //
   // allow tty access for the next read/write call
   //
   // if write was unsuccessful, return error
}

