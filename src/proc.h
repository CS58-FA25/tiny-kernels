#ifndef PROC_H
#define PROC_H

#include "yalnix.h"
#include "hardware.h"
#include "ykernel.h"   /* contains PCB type if you put it there */
#include "queue.h"

#define MAX_PROCS        64        /* size of proc table/array */
#define IDLE_PID         0         /* pid reserved for the kernel idle process */
#define INVALID_PID      (-1)  /* Entries in the processes table that have this value mean that this pid is free to use */

#define USER_MEM_START    VMEM_1_BASE
#define USER_STACK_BASE   VMEM_1_LIMIT

/* process state */
typedef enum {
    PROC_FREE = 0,
    PROC_IDLE,     /* the kernel's initial idle process */
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,  /* waiting for I/O or wait() */
    PROC_ZOMBIE,
    PROC_ORPHAN,
} proc_state_t;

typedef struct pcb PCB;

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
    UserContext user_context;

    /* Kernel context & kernel stack - used by KernelContextSwitch routines.
     * The type KernelContext is provided in hardware.h and is opaque. */
    KernelContext kernel_context;

    /* Kernel stack bookkeeping: kernel allocates kernel stack frames in Region 0 */
    void *kstack_base;       /* base virtual address of the kernel stack (Region 0) */

    /* User-region memory accounting */
    unsigned int user_heap_start_vaddr; /* page-aligned lowest heap address (inclusive) */
    unsigned int user_heap_end_vaddr;   /* current brk (lowest not-in-use address) */
    unsigned int user_stack_base_vaddr; /* top of user stack (initial) */

    /* Scheduling queue pointers (singly linked for simplicity) */
    PCB *next;
    PCB *prev;
    PCB *parent;
    queue_t *children_processes;


    /* bookkeeping flags */
    int waiting_for_child_pid;  /* if parent is blocked waiting for child (Wait) */
    int last_run_tick;          /* last tick when this process ran (scheduler info) */

    /* optional: file descriptors, tty state, etc. (omitted for cp1) */
};


extern PCB *idle_proc;
extern PCB *current_proc;
extern queue_t *ready_queue;
extern queue_t *blocked_queue;
extern queue_t *zombie_queue;



PCB *create_PCB(void);
void InitializeProcQueues(void);
void delete_pcb(PCB *process);
void copy_pt(PCB *parent, PCB *child);

#endif /* PROC_H */
