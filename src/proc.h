#ifndef PROC_H
#define PROC_H

#include "yalnix.h"
#include "hardware.h"
#include "ykernel.h"   /* contains PCB type if you put it there */

#define MAX_PROCS        64        /* size of proc table/array */
#define IDLE_PID         0         /* pid reserved for the kernel idle process */
#define INVALID_PID      (-1)  /* Entries in the processes table that have this value mean that this pid is free to use */

/* process state */
typedef enum {
    PROC_FREE = 0,
    PROC_IDLE,     /* the kernel's initial idle process */
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,  /* waiting for I/O or wait() */
    PROC_ZOMBIE
} proc_state_t;

/* Process Control Block */
typedef struct pcb {
    int pid;                 /* process id (unique) */
    proc_state_t state;      /* current state */
    int ppid;                /* parent pid (-1 if none) */
    int exit_status;         /* status for Wait; valid if PROC_ZOMBIE */

    /* Region 1 page table: kernel stores a pointer to an in-memory page table.
     * The MMU registers REG_PTBR1/REG_PTLR1 are set to these during context-switch. */
    pte_t *ptbr;             /* virtual address of page table for region 1 */
    unsigned int ptlr;       /* length (number of entries) */

    /* Full user CPU state snapshot.  The handout requires storing the full
     * UserContext in the PCB (not just a pointer) so it can be restored later. */
    UserContext uctxt;

    /* Kernel context & kernel stack - used by KernelContextSwitch routines.
     * The type KernelContext is provided in hardware.h and is opaque. */
    KernelContext kctx;

    /* Kernel stack bookkeeping: kernel allocates kernel stack frames in Region 0 */
    void *kstack_base;       /* base virtual address of the kernel stack (Region 0) */
    unsigned int kstack_npages;

    /* User-region memory accounting */
    unsigned int user_heap_start_vaddr; /* page-aligned lowest heap address (inclusive) */
    unsigned int user_heap_end_vaddr;   /* current brk (lowest not-in-use address) */
    unsigned int user_stack_base_vaddr; /* top of user stack (initial) */

    /* Scheduling queue pointers (singly linked for simplicity) */
    struct pcb *next;

    /* bookkeeping flags */
    int waiting_for_child_pid;  /* if parent is blocked waiting for child (Wait) */
    int last_run_tick;          /* last tick when this process ran (scheduler info) */

    /* optional: file descriptors, tty state, etc. (omitted for cp1) */
} PCB;

/* Global process table and runqueues */
extern PCB *proc_table;       /* array of MAX_PROCS PCBs allocated at kernel startup */
extern unsigned int proc_table_len;
extern int pid_free_head;

extern PCB *current_proc;     /* currently running process (NULL if kernel idle) */

/* Idle PCB convenience */
extern PCB *idle_proc;

/* Exported globals */
extern PCB *proc_table;             /* array of MAX_PROCS PCBs */
extern unsigned int proc_table_len; /* equals MAX_PROCS */

extern PCB *current_proc;           /* currently running process (NULL if kernel idle) */
extern PCB *idle_proc;

/* Run / other queues */
typedef struct runqueue {
    PCB *head;
    PCB *tail;
} runqueue_t;

extern runqueue_t ready_queue;     /* processes ready to run */
extern runqueue_t blocked_queue;   /* generic blocked processes (optional) */
extern runqueue_t zombie_queue;    /* zombies waiting to be reaped */

/* Initialization */
void InitializeProcSubsystem(void);

/* PCB alloc/free */
PCB *AllocPCB(void);
void FreePCB(PCB *p);

/* Queue ops */
void Enqueue(runqueue_t *q, PCB *p);
PCB *Dequeue(runqueue_t *q);
int RemoveFromQueue(runqueue_t *q, PCB *p); /* returns 0 if removed, -1 if not found */

/* Convenience queue ops */
void EnqueueReady(PCB *p);
PCB *DequeueReady(void);

/* Process lifecycle helpers */
PCB *CreateIdleProcess(void);
int MakeProcessZombie(PCB *p, int exit_status); /* mark as zombie */
int ReapZombieByPid(int pid); /* frees resources, returns 0 on success, -1 if none */
int WaitForChild(PCB *parent, int child_pid); /* parent blocks until child gone or returns -1 */

#endif /* PROC_H */
