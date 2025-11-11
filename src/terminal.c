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
      return tty;
   }
   return ERROR; // already initialized
}

int WriteTerminal(int tty, void* data, int nbytes) {
   if (tty >= NUM_TERMINALS) {
      return ERROR; // Can't use non existent terminal
   }
   terminal_t* term = _terminals[tty];
   if (term == &DummyTerminal) {
      return ERROR; // Can't use uninitialized terminal
   }

   // TODO: Blocking code, copy to the output buffer, etc

   TtyTransmit(tty, term->output_buffer, nbytes);
}

int ReadTerminal(int tty, void* buf, int nbytes) {
   if (tty >= NUM_TERMINALS) {
      return ERROR; // Can't use non existent terminal
   }
   terminal_t* term = _terminals[tty];
   if (term == &DummyTerminal) {
      return ERROR; // Can't use uninitialized terminal
   }
   // TODO Actual implementation, dummy code right now..
   // TODO is nbytes needed?
   TtyReceive(tty, term->input_buffer, TERMINAL_MAX_LINE);
}

