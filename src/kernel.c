#include <ylib.h>
#include <yalnix.h>
#include <hardware.h> 
#include "ykernel.h"
#include "proc.h"

/* ================ Global Variables ================= */
#define KERNEL_TRACE_LEVEL   3
extern int kernel_brk;

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
    TracePrintf(KERNEL_TRACE_LEVEL, "Entering KernelStart");
    kernel_brk = _orig_kernel_brk_page;
    initializeFreeFrameList(pmem_size);
    initializeInterruptVectorTable();
    initializeVM();
    // InitializeKernelDataStructures();
    // InitializeInterruptVectorTable()

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