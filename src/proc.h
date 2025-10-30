#ifndef PROC_H
#define PROC_H

#include "yalnix.h"
#include "hardware.h"
#include "ykernel.h"   /* contains PCB type if you put it there */
#include "queue.h"

#define IDLE_PID         0         /* pid reserved for the kernel idle process */
#define INVALID_PID      (-1)  /* Entries in the processes table that have this value mean that this pid is free to use */

#define USER_MEM_START    VMEM_1_BASE
#define USER_STACK_BASE   VMEM_1_LIMIT

/* process state */
typedef enum {
    PROC_FREE = 0,
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

    /* The MMU registers REG_PTBR1/REG_PTLR1 are set to these during context-switch. */
    pte_t *ptbr;             /* virtual address of page table for region 1 */
    unsigned int ptlr;       /* number of entries in this page table for region 1*/

    UserContext user_context; /* Full user cpu snapshow*/

    /* Kernel context & kernel stack */
    KernelContext kernel_context;
    pte_t *kstack;       /* page table to map to the frames allocated from the process kernel stack*/

    /* User-region memory accounting */
    unsigned int user_heap_start_vaddr; /* page-aligned lowest heap address (inclusive) */
    unsigned int user_heap_end_vaddr;   /* current brk (lowest not-in-use address) */
    unsigned int user_stack_base_vaddr; /* top of user stack (initial) */

    /* Scheduling queue pointers */
    PCB *next;
    PCB *prev;
    PCB *parent;
    queue_t *children_processes;


    /* bookkeeping flags */
    int waiting_for_child_pid;  /* if parent is blocked waiting for child (Wait) */
    int last_run_tick;          /* last tick when this process ran (scheduler info) */

    /* optional: file descriptors, tty state, etc. (omitted for cp1) */
};

extern PCB *idle_proc; // Pointer to the idle process PCB
extern PCB *current_process; // Pointer to the current running process PCB
extern queue_t *ready_queue; // A FIFO queue of processes ready to be executed by the cpu
extern queue_t *blocked_queue; // A queue of processes blocked (either waiting on a lock, cvar or waiting for an I/O to finish)
extern queue_t *zombie_queue; // A queue of processes that have terminated but whose parent has not yet called Wait()

extern PCB **proc_table; // List of processes. Not all of them are actual processes but are pointers to processes that could be initialized by LoadProgram

/**
 * ======================== Description =======================
 * @brief Allocates and initializes a new Process Control Block (PCB).
 * ======================== Behavior ==========================
 * - Allocates memory for the PCB struct and its region 1 page table.
 * - Initializes default fields such as PID, PPID, and state.
 * - Sets up internal queues and zeroes out kernel/user contexts.
 * ======================== Returns ===========================
 * @returns Pointer to a valid PCB on success.
 * @returns NULL if any allocation fails.
 * ======================== Notes =============================
 * - Each PCB has its own children queue to track forked processes.
 * - Does not yet allocate a kernel stack; that’s done separately.
 */
PCB *allocNewPCB(void); // Allocates memory and initializes one PCB (used only during kernel init)

/**
 * ======================== Description =======================
 * @brief Cleans up and frees a PCB and its associated resources.
 * ======================== Behavior ==========================
 * - Orphans any child processes (sets their parent to NULL).
 * - Frees region 1 page table and the children queue.
 * - Removes PCB from any linked list if applicable.
 * ======================== Notes =============================
 * - Does NOT yet free user memory pages (handled separately in VM cleanup).
 * - Should only be called when process is fully terminated.
 */
void deletePCB(PCB *process);

/**
 * ======================== Description =======================
 * @brief Returns a free PCB slot from the global process table.
 * ======================== Behavior ==========================
 * - Iterates over the process table to find a PCB marked `PROC_FREE`.
 * ======================== Returns ===========================
 * @returns Pointer to a free PCB if found, NULL otherwise.
 */
PCB *getFreePCB(void); // Retrieves an unused PCB from proc_table for process creation

/**
 * ======================== Description =======================
 * @brief Initializes the global process queues used by the scheduler.
 * ======================== Behavior ==========================
 * - Allocates and initializes the ready, blocked, and zombie queues.
 * - Each queue is created using `queueCreate()` and stored in global vars.
 * ======================== Notes =============================
 * - Called once during kernel startup before any process is created.
 * - Halts the system if any allocation fails to prevent inconsistent state.
 */
void InitializeProcQueues(void);

/**
 * ======================== Description =======================
 * @brief Creates and initializes the idle process.
 * ======================== Behavior ==========================
 * - Allocates a PCB for the idle process.
 * - Allocates one user stack page in region 1.
 * - Initializes kernel stack using `InitializeKernelStackIdle()`.
 * - Sets up user context to run `DoIdle()`.
 * ======================== Notes =============================
 * - PID = 0, PPID = INVALID_PID.
 * - This process never terminates; it runs when no other is ready.
 */
PCB *CreateIdlePCB(UserContext *uctxt);

/**
 * ======================== Description =======================
 * @brief Entry point for the idle process.
 * ======================== Behavior ==========================
 * - Loops forever, calling `Pause()` to yield CPU when possible.
 * - Used as the background process when no others are runnable.
 */
void DoIdle(void);

#endif /* PROC_H */
