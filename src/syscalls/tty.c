#include "syscalls/tty.h"
#include "kernel.h"

/* ============== Forward Declarations & Global variables ============== */
int BeginTtyTransmit(int tty_id, PCB *writer, void *buf, int len);

terminal_t terminals[NUM_TERMINALS];
/* ============== Forward Declarations & Global variables ============== */

/**
 * ======== TtyRead =======
 * See tty.h for more details
*/
int TtyRead(int tty_id, void *buf, int len) {

   if (tty_id < 0 || tty_id >= NUM_TERMINALS || buf == NULL || len <= 0) {
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
   queue_enqueue(terminal->blocked_readers, curr);

   // Block the process
   queue_enqueue(blocked_queue, curr);
   curr->state = PROC_BLOCKED;

   // Store the buffer to copy to and also how many bytes needed to read in the process PCB for when the process is woken up
   curr->tty_read_buf = buf;
   curr->tty_read_len = len;

   // Switch to idle or another process
   PCB *next = (is_empty(ready_queue) == 1) ? idle_proc : queue_dequeue(ready_queue);
   KernelContextSwitch(KCSwitch, curr, next);

   TracePrintf(0, "TtyRead: process PID %d woken up.\n", curr->pid);
   return curr->user_context.regs[0];
}

/**
 * ======== TtyWrite =======
 * See tty.h for more details
*/
int TtyWrite(int tty_id, void *buf, int len) {
   // Validate Arguments
   if (tty_id < 0 || tty_id >= NUM_TERMINALS || buf == NULL || len <= 0) {
      TracePrintf(0, "TtyWrite: Invalid arguments passed!\n");
      return ERROR;
   }

   terminal_t *terminal = &terminals[tty_id];
   PCB *curr = current_process;

   // Acquire the lock
   if (terminal->in_use) {
      TracePrintf(0, "TtyWrite: Terminal %d busy. PID %d waiting for lock.\n", tty_id, curr->pid);
      
      queue_enqueue(terminal->blocked_writers, curr);
      
      // Block purely for the lock
      curr->state = PROC_BLOCKED;
      queue_enqueue(blocked_queue, curr);
      
      PCB *next = (is_empty(ready_queue)) ? idle_proc : queue_dequeue(ready_queue);
      KernelContextSwitch(KCSwitch, curr, next);
      // WAKE UP! If we get here, the trap handler dequeued us and woke us up
      // It's our turn to write to the terminal
   }

   // Mark terminal as busy (if we just woke up, we take ownership now)
   terminal->in_use = 1;

   // Copy the data from the buffer into the kernel-allocated array in terminal->write_buffer
   TracePrintf(0, "TtyWrite: PID %d acquired terminal %d. Starting copy.\n", curr->pid, tty_id);
   
   // Allocation and memcpy happen here
   int result = BeginTtyTransmit(tty_id, curr, buf, len);
   
   if (result == ERROR) {
      TracePrintf(0, "TtyWrite: Allocation failed for PID %d\n", curr->pid);
      // Release the lock so other processes are not stuck forever
      terminal->in_use = 0;
      
      // If others are waiting, wake the next one immediately
      if (!is_empty(terminal->blocked_writers)) {
          PCB *next_writer = queue_dequeue(terminal->blocked_writers);
          next_writer->state = PROC_READY;
          queue_enqueue(ready_queue, next_writer);
      }
      return ERROR;
   }

   // Wait for I/O completion. We sleep again until the trap handler says "All chunks done".
   TracePrintf(0, "TtyWrite: PID %d waiting for I/O completion...\n", curr->pid);
   
   curr->state = PROC_BLOCKED;
   queue_enqueue(blocked_queue, curr);
   
   PCB *next = (is_empty(ready_queue)) ? idle_proc : queue_dequeue(ready_queue);
   KernelContextSwitch(KCSwitch, curr, next);

   // Done!! The trap handler woke us up.
   TracePrintf(0, "TtyWrite: PID %d write complete.\n", curr->pid);
   return len;
}

/* =============== Helpers =============== */

/**
 * Description: Prepares and starts a terminal transmission by copying the user
 *              buffer into a kernel write buffer, initializing write state, and
 *              issuing the first TtyTransmit chunk.
 * ========= Parameters ========
 * @param tty_id: Terminal ID to write to.
 * @param writer: PCB of the writing process.
 * @param buf: User data to copy into kernel space.
 * @param len: Total number of bytes to transmit.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */

int BeginTtyTransmit(int tty_id, PCB *writer, void *buf, int len) {
   TracePrintf(0, "BeginTtyTransmit: Process PID %d starting to write %d bytes to terminal tty_id %d.\n", writer->pid, len, tty_id);
   
   terminal_t *terminal = &terminals[tty_id];
   terminal->write_buffer = malloc(len);

   if (terminal->write_buffer == NULL) {
      TracePrintf(0, "BeginTtyTransmit: Failed to allocate memory for write buffer for terminal %d and writer process %d.\n", tty_id, writer->pid);
      writer->user_context.regs[0] = ERROR;
      return ERROR;
   }

   // Need to copy the contents of the buffer into the kernel array write_buffer in case a context switch happens
   memcpy(terminal->write_buffer, buf, len);
   terminal->write_buffer_len = len;
   terminal->write_buffer_position = 0;
   terminal->current_writer = writer;

   // Beginning the first write transmit
   int bytes_to_write = (len > TERMINAL_MAX_LINE) ? TERMINAL_MAX_LINE : len;
   TracePrintf(0, "BeginTtyTransmit: Process PID %d starting off by writing %d bytes into terminal tty_id %d.\n", writer->pid, bytes_to_write, tty_id);
   TtyTransmit(tty_id, terminal->write_buffer, bytes_to_write);
   terminal->write_buffer_position = bytes_to_write;
   return SUCCESS;
}