#ifndef TERMINAL_H
#define TERMINAL_H

#include "proc.h"

/* ================== Terminals ================== */
#define NUM_TERMINALS 4
#define TERMINAL_BUFFER_SIZE 1024

typedef struct terminal {
    int id;                        // terminal number (0–3)
    char input_buffer[TERMINAL_BUFFER_SIZE];
    int input_head;
    int input_tail;
    char output_buffer[TERMINAL_BUFFER_SIZE];
    int output_head;
    int output_tail;
    int transmitting;              // flag: is a transmission in progress?
    PCB *waiting_read_proc;      // process blocked on reading
    PCB *waiting_write_proc;     // process blocked on writing
} terminal_t;

static terminal_t DummyTerminal;

static terminal_t* _terminals[NUM_TERMINALS] = {
   &DummyTerminal,
   &DummyTerminal,
   &DummyTerminal,
   &DummyTerminal
};

int InitializeTerminal(int tty);

int WriteTerminal(int tty, void* data, int nbytes);
int ReadTerminal(int tty, void* buf, int nbytes);

#endif // TERMINAL_H
