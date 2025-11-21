#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include "queue.h"
#include "proc.h"

#define SYSCALLS_TRACE_LEVEL 0

#define NUM_LOCKS 32
#define NUM_CVARS 32
#define NUM_PIPES 32

/* =========== Bitmasks to multiplex to types =========== */
#define TYPE_MASK  0xF0000 // A mask for type is only going to use the first 4 bits
#define INDEX_MASK 0x0FFFF // A mask for the index is only going to use the bottom 16 bits (makes up to around possible 2^16 indices for each type)

#define CREATE_ID(type, index)  ((type | index))
#define GET_TYPE(ID)            ((TYPE_MASK & ID))
#define GET_INDEX(ID)           ((INDEX_MASK & ID))

#define LOCK_TYPE  0x10000
#define CVAR_TYPE  0x20000
#define PIPE_TYPE  0x30000
/* =========== Bitmasks to multiplex to types =========== */

/* =========== Types and global variables ========== */
typedef struct Lock {
    int id;                     
    int is_active; // 1 if LockInit has been called, 0 otherwise
    int locked; // 1 if a processs has the lock, 0 otherwise

    PCB *owner; // NULL if free, ptr to PCB if held
    queue_t *waiting_processes; // Processes waiting to acquire this lock
} Lock_t;

typedef struct Cvar {
    int id;
    int is_active; // 1 if CvarInit has been called, 0 otherwise

    queue_t *waiting_processes; // Processes waiting on this condition
} Cvar_t;

typedef struct Pipe {
    int id;
    int is_active;
    queue_t *waiting_writers; // Queue of processes waiting for buffer to free up to write
    queue_t *waiting_readers; // Queue of processes waiting for buffer to have data to read

    char read_buffer[PIPE_BUFFER_LEN];
    int len; // Number of bytes in the buffer
} Pipe_t;

extern Lock_t locks[NUM_LOCKS];
extern Cvar_t cvars[NUM_CVARS];
extern Pipe_t pipes[NUM_PIPES];
/* =========== Types and global variables ========== */


/* ========== Locks ========== */

/**
 * Description: Initializes a lock and returns its ID through lock_idp. Allocates a slot,
 *              sets initial fields, and creates the waiting queue.
 * ========= Parameters ========
 * @param lock_idp : Pointer where the new lock ID will be stored.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int LockInit (int * lock_idp);

/**
 * Description: Attempts to acquire the lock. If it's already held, the
 *              calling process is blocked and placed on the lock's wait queue.
 * ========= Parameters ========
 * @param lock_id : ID of the lock to acquire.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int Acquire (int lock_id);

/**
 * Description: Releases a lock held by the calling process. If other
 *              processes are waiting, wakes one and hands the lock off.
 * ========= Parameters ========
 * @param lock_id : ID of the lock to release.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int Release (int lock_id);

/* ======== Condition Variables ====== */

/**
 * Description: Initializes a condition variable and returns its ID through
 *              cvar_idp. Sets up a new waiting queue.
 * ========= Parameters ========
 * @param cvar_idp : Pointer where the cvar ID will be stored.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int CvarInit (int * cvar_idp);

/**
 * Description: Atomically releases the given lock and puts the calling
 *              process to sleep on the condition variable. When woken,
 *              the process tries to reacquire the lock.
 * ========= Parameters ========
 * @param cvar_id : Condition variable ID to wait on.
 * @param lock_id : Lock the caller must hold before waiting.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int CvarWait (int cvar_id, int lock_id);

/**
 * Description: Wakes one process waiting on the condition variable, if any.
 * ========= Parameters ========
 * @param cvar_id : Condition variable ID to signal.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int CvarSignal (int cvar_id);

/**
 * Description: Wakes all processes currently waiting on the condition variable.
 * ========= Parameters ========
 * @param cvar_id : Condition variable ID to broadcast on.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int CvarBroadcast (int cvar_id);

/* ========== Pipes ========== */

/**
 * Description: Initializes a pipe and returns its ID through pipe_idp.
 *              Allocates a free pipe slot and sets up its read/write queues.
 * ========= Parameters ========
 * @param pipe_idp : Pointer where the pipe ID will be written.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int PipeInit (int * pipe_idp);

/**
 * Description: Reads up to len bytes from the pipe into buf. If the pipe
 *              is empty, the caller blocks until data becomes available.
 *              Wakes up all writers upon finishing reading
 * ========= Parameters ========
 * @param pipe_id: ID of the pipe to read from.
 * @param buf: Buffer to write data into.
 * @param len: Max number of bytes to read.
 * ========= Returns ==========
 * @return Number of bytes read, or ERROR
 */
int PipeRead(int pipe_id, void *buf, int len);

/**
 * Description: Writes len bytes from buf into the pipe. If the pipe buffer
 *              is full, the caller blocks until space frees up. Writes in chunks
 *              and wakes up all readers once done with writing.
 * ========= Parameters ========
 * @param pipe_id : ID of the pipe to write to.
 * @param buf     : Data to write.
 * @param len     : Number of bytes to write.
 * ========= Returns ==========
 * @return Number of bytes written, or ERROR
 */
int PipeWrite(int pipe_id, void *buf, int len);

/* ========= Reclaim ======== */
int Reclaim (int id);



#endif