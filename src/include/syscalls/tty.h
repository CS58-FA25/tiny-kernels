#ifndef TTY_H
#define TTY_H

#include "queue.h"
#include "proc.h"

/* ============== Types and Global Variables ============= */
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
/* ============== Types and Global Variables ============= */

/**
 * Description: Reads up to len bytes from the terminal. If data is already
 *              available, returns immediately; otherwise the calling process
 *              blocks until input arrives and is later copied into user buf.
 * ========= Parameters ========
 * @param tty_id: Terminal ID to read from.
 * @param buf: User buffer where data will be placed when unblocked.
 * @param len: Maximum number of bytes to read.
 * ========= Returns ==========
 * @return Number of bytes read, or ERROR
 */
int TtyRead(int tty_id, void *buf, int len);

/**
 * Description: Writes len bytes to the terminal. If the terminal is busy, the
 *              caller blocks until it acquires the write lock, then begins a
 *              transmit and sleeps again until all bytes are written.
 * ========= Parameters ========
 * @param tty_id : Terminal ID to write to.
 * @param buf    : User buffer containing data to write.
 * @param len    : Number of bytes to write.
 * ========= Returns ==========
 * @return len on success, or ERROR
 */

int TtyWrite(int tty_id, void *buf, int len);


#endif