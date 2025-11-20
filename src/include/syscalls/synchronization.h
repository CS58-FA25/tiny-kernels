#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include "queue.h"
#include "proc.h"

#define SYSCALLS_TRACE_LEVEL 0

#define NUM_LOCKS 32
#define NUM_CVARS 32
#define NUM_PIPES 32

#define TYPE_MASK  0xF0000 // A mask for type is only going to use the first 4 bits
#define INDEX_MASK 0x0FFFF // A mask for the index is only going to use the bottom 16 bits (makes up to around possible 2^16 indices for each type)

#define CREATE_ID(type, index)  ((type | index))
#define GET_TYPE(ID)            ((TYPE_MASK & ID))
#define GET_INDEX(ID)           ((INDEX_MASK & ID))

#define LOCK_TYPE  0x10000
#define CVAR_TYPE  0x20000
#define PIPE_TYPE  0x30000

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

/* ========== Locks ========== */
int LockInit (int * lock_idp);
int Acquire (int lock_id);
int Release (int lock_id);

/* ======== Condition Variables ====== */
int CvarInit (int * cvar_idp);
int CvarWait (int cvar_id, int lock_id);
int CvarSignal (int cvar_id);
int CvarBroadcast (int cvar_id);

/* ========== Pipes ========== */
int PipeInit (int * pipe_idp);
int PipeRead(int pipe_id, void *buf, int len);
int PipeWrite(int pipe_id, void *buf, int len);

/* ========= Reclaim ======== */
int Reclaim (int id);

extern Lock_t locks[NUM_LOCKS];
extern Cvar_t cvars[NUM_CVARS];
extern Pipe_t pipes[NUM_PIPES];

#endif