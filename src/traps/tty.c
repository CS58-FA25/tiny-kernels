#include "terminal.h"
#include "traps/trap.h"

#include <hardware.h>

static void _WakeProc(PCB*);

void TtyTrapTxHandler(UserContext* ctx) {
   int tty = ctx->code;
   // Tty was written to, inform waiting proc
   if (tty >= NUM_TERMINALS) {
     TracePrintf(TRAP_TRACE_LEVEL, "Attempted to write to invalid terminal id: %d\n", tty);
     return;  
   }


   terminal_t* term = GetTerminal(tty);
   if (term == &DummyTerminal) {
      // SHOULD NOT HAPPEN. Invalid and failed
      TracePrintf(TRAP_TRACE_LEVEL, "Attempted to write to uninitialized terminal id: %d\n", tty);
      return; 
   }

   TracePrintf(TRAP_TRACE_LEVEL, "Starting to write to terminal id: %d\n", tty);

   term->transmitting = 0;
   if (term->waiting_write_proc) {
      _WakeProc(term->waiting_write_proc);
      term->waiting_write_proc = 0; // clear current waiter?
   }
}

void TtyTrapRxHandler(UserContext* ctx) {
   int tty = ctx->code;
   // Tty was read, inform
   if (tty >= NUM_TERMINALS) {
     TracePrintf(TRAP_TRACE_LEVEL, "Attempted to read from invalid terminal id: %d\n", tty);
     return;
   }

   terminal_t* term = GetTerminal(tty);
   if (term == &DummyTerminal) {
      // SHOULD NOT HAPPEN. Invalid and failed
      TracePrintf(TRAP_TRACE_LEVEL, "Attempted to read from uninitialized terminal id: %d\n", tty);
      return;
   }

   TracePrintf(TRAP_TRACE_LEVEL, "Starting to send data to terminal id: %d\n", tty);

   char buf[TERMINAL_MAX_LINE];
   int nbytes = TtyReceive(tty, buf, TERMINAL_MAX_LINE);
   if (nbytes <= 0) {
       // error
       return;
   }

   for (int i = 0; i < nbytes; i++) {
       term->input_buffer[term->input_tail] = buf[i];
       term->input_tail = (term->input_tail + 1) % TERMINAL_BUFFER_SIZE;

       if (term->input_tail == term->input_head) {
           term->input_head = (term->input_head + 1) % TERMINAL_BUFFER_SIZE;
       }
   }


   if (term->waiting_read_proc) {
      _WakeProc(term->waiting_read_proc);
      term->waiting_read_proc = 0;
   }
}

static void _WakeProc(PCB* proc) {
   if (!proc) {
      TracePrintf(TRAP_TRACE_LEVEL, "called _WakeProc with no proc\n");
      return;
   }
   proc->state = PROC_READY;
   queueRemove(blocked_queue, proc);
   queueEnqueue(ready_queue, proc);
   TracePrintf(TRAP_TRACE_LEVEL, "Woke up proc pid: %d\n", proc->pid);
}
