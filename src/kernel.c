#include <ylib.h>
#include <yalnix.h>
#include <hardware.h> 
#include "ykernel.h"   

/* =============== Configuration Constants ================ */

#define MAX_PROCS        64        /* size of proc table/array */
#define IDLE_PID         0         /* pid reserved for the kernel idle process */
#define INVALID_PID      (-1)  /* Entries in the processes table that have this value mean that this pid is free to use */

#define KERNEL_STACK_PAGES   2     /* kernel stack size in pages */
#define KERNEL_STACK_SIZE    (KERNEL_STACK_PAGES * PAGESIZE)

/* Kernel return codes */
#define SUCCESS  (0)
#define ERROR    (-1)
#define KILL     (-2)



/* ============= Process Control Block (PCB) ================ */

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

extern PCB *current_proc;     /* currently running process (NULL if kernel idle) */

/* Idle PCB convenience */
extern PCB *idle_proc;

/* Allocate / free PCBs (helper prototypes) */
PCB *AllocPCB(void);   /* returns pointer to free PCB, or NULL if none */
void FreePCB(PCB *p);  /* free data structures; for exit/reap */


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

void KernelStart(char **cmd_args, unsigned int pmem_size, UserContext *uctxt)
{
    // TracePrintf(KERNEL_TRACE_LEVEL, "Entering KernelStart");

    // InitializeKernelDataStructures();
    // InitializeFreeFrameList(pmem_size);
    // InitializeInterruptVectorTable()
    // InitializeVM();            // Build Region 0 & 1 page tables

    // EnableVM();                // Turn on virtual memory

    // PCB *init_pcb = CreateProcess(INIT_PID);
    // CopyUserContext(uctxt, init_pcb);
    // current = init_pcb;
    // Enqueue(ready_queue, init_pcb);

    // PCB *idle_pcb = CreateIdleProcess();

    // Run Init process
    // return
}


KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    // curr = (PCB *)curr_pcb_p
    // next = (PCB *)next_pcb_p

    // // Save kernel context of current process
    // memcpy(&curr->kctx, kc_in, sizeof(KernelContext))

    // // Switch page tables for region 1
    // WriteRegister(REG_PTBR1, (unsigned int) next->ptbr1)
    // WriteRegister(REG_PTLR1, MAX_PT_LEN)
    // WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1)

    // current = next
    // return &(next->kctx)
}

KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *unused){
    // newpcb = (PCB *) new_pcb_p

    // // Copy kernel context
    // memcpy(&newpcb->kctx, kc_in, sizeof(KernelContext))

    // // Allocate kernel stack for new process
    // CopyKernelStack(newpcb)

    // // Return kc_in (so parent resumes after KernelContextSwitch)
    // return kc_in
}

void scheduler() {
    // while true:
    //     next = Dequeue(ready_queue)
    //     if next == NULL:
    //         next = idle_pcb  // run idle if no ready processes

    //     rc = KernelContextSwitch(KCSwitch, current, next)

    //     if rc == -1:
    //         TracePrintf(0, "Context switch failed")
    //         Halt()

    //     // When we return here, current process has resumed
    //     HandlePendingTrapsOrSyscalls()
}