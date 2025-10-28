#include <ylib.h>
#include <yalnix.h>
#include <hardware.h> 
#include "ykernel.h"
#include "kernel.h"
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

void KernelStart(char **cmd_args, unsigned int pmem_size, UserContext *uctxt)
{
    TracePrintf(1, "KernelStart: Entering KernelStart\n");
    TracePrintf(1, "Physical memory has size %d\n", pmem_size);

    kernel_brk_page = _orig_kernel_brk_page;

    TracePrintf(1, "Initializing the free frame array...\n");
    initializeFreeFrameList(pmem_size);

    TracePrintf(1, "Initializing and enabling virtual memory...\n");
    initializeVM();

    TracePrintf(1, "Initializing the interrupt vector table....\n");
    initializeInterruptVectorTable();

    TracePrintf(1, "Initializing the table of free pids to be used by processes....\n");
    InitializeProcTable();

    TracePrintf(1, "Initializing process queues (ready, blocked, zombie)....\n");
    InitializeProcQueues();
    idle_proc = CreateIdlePCB(uctxt);
    current_proc = idle_proc;
    TracePrintf(0, "Boot sequence till creating Idle process is done!\n");

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