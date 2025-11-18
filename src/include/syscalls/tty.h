#ifndef TTY_H
#define TTY_H

#include "queue.h"
#include "proc.h"
typedef struct terminal {
    queue_t *blocked_writers; // Queue of processes waiting to write to the terminal
    queue_t *blocked_readers; // Queue of processes waiting to read from the terminal
    int tty_id;                   // ID identifier for the terminal
    int read_buffer_len;      // Number of bytes available to be read from the read buffer
    int write_buffer_len;     // Total length of write buffer
    char *read_buffer;        // Buffer holding read data
    char *write_buffer;       // Buffer holding data to be written
    int write_buffer_position; // Current position in write buffer
    PCB *current_writer;       // Current process writing  to the buffer
    int in_use;                // Flag indicating if terminal is busy
} terminal_t;

extern terminal_t terminals[NUM_TERMINALS];

int beginWriteTransmit(int tty_id, PCB *writing_process, void *buf, int len);
int TtyRead(int tty_id, void *buf, int len);
int TtyWrite(int tty_id, void *buf, int len);


#endif