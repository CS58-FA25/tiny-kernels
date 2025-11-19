#include "terminal.h"

#include <ylib.h>
#include <stdlib.h>

int InitializeTerminal(int tty) {
   terminal_t* term = _terminals[tty];
   if (term == &DummyTerminal) {
      term = malloc(sizeof(terminal_t));
      term->id = tty;

      memset(term->input_buffer, 0, TERMINAL_BUFFER_SIZE);
      memset(term->output_buffer, 0, TERMINAL_BUFFER_SIZE);

      term->input_head = term->input_tail = 0;
      term->output_head = term->output_tail = 0;
      _terminals[tty] = term;
      return tty;
   }
   return ERROR; // already initialized
}

int ValidateTerminal(terminal_t* term) {
   return term && term != &DummyTerminal;
}

int TerminalWrite(terminal_t* term, void* data, int nbytes) {
   if (nbytes == 0) {
      return nbytes; // It didn't error, but it didn't write
   }

   if (!ValidateTerminal(term)) {
      return ERROR; // Can't use uninitialized terminal
   }

   TracePrintf(1, "Attempting to write nbytes %d to tty %d\n", nbytes, term->id);

   int written = 0;
   while (written < nbytes) {
       while (term->transmitting || term->waiting_write_proc != NULL) {
           term->waiting_write_proc = current_process;
           current_process->state = PROC_BLOCKED;
           if (!is_in_queue(blocked_queue, current_process)) {
               queueEnqueue(blocked_queue, current_process);
           }
       }

       int remaining = nbytes - written;
       int todo = remaining;
       if (remaining >= TERMINAL_MAX_LINE) {
           todo = TERMINAL_MAX_LINE;
       }

       TracePrintf(1, "wrote %d\n", written);
       memcpy(term->output_buffer, data + written, todo);
       TtyTransmit(term->id, term->output_buffer, todo);
       term->transmitting = 1;
       term->waiting_write_proc = current_process;
       current_process->state = PROC_BLOCKED;
       queueEnqueue(blocked_queue, current_process);

       TracePrintf(1, "Transmit %d/%d bytes to tty/%d\n", todo, nbytes, term->id);
       written = written + todo;
   }

   TracePrintf(1, "Completed transmit %d bytes to tty/%d\n", nbytes, term->id);
   return written;
}

int TerminalRead(terminal_t* term, void* buf, int nbytes) {
   if (!ValidateTerminal(term)) {
      return ERROR; // Can't use uninitialized terminal
   }

   TracePrintf(1, "Attempting to read nbytes %d from tty %d\n", nbytes, term->id);

   int read = 0;
   while (read < nbytes) {
       int head = term->input_head;
       int tail = term->input_tail;
       int available = 0;
       if (tail < head) {
          available = (TERMINAL_BUFFER_SIZE - head) + tail;
       } else {
          available = tail - head;
       }

       int todo = available;
       if (available > nbytes) {
          todo = nbytes;
       } 
       
       // Circular read
       for (int i = 0; i < todo; i++) {
          ((char*) buf)[read] = term->input_buffer[term->input_head];
          term->input_head = (term->input_head + 1) % TERMINAL_BUFFER_SIZE;
          read = read + 1;
       }

       // Reset buffer
       if (term->input_head == term->input_tail) {
           term->input_head = term->input_tail = 0;
       }
   }

   TracePrintf(1, "Received %d bytes from tty/%d. Expected %d\n", read, term->id, nbytes);
}


terminal_t* GetTerminal(unsigned int id) {
   if (id < NUM_TERMINALS) {
      return _terminals[id];
   }
   return 0;
}
