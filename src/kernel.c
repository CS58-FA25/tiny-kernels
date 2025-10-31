#include <ylib.h>
#include <yalnix.h>
#include <hardware.h> 
#include "ykernel.h"
#include "kernel.h"
#include "proc.h"
#include "mem.h"



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

    TracePrintf(1, "Initializing virtual memory (creating region0 of page table and mapping to physical frames)...\n");
    initializeVM();

    TracePrintf(1, "Initializing the interrupt vector table....\n");
    initializeInterruptVectorTable();

    TracePrintf(1, "Initializing the table of free pids to be used by processes....\n");
    InitializeProcTable();

    TracePrintf(1, "Initializing process queues (ready, blocked, zombie)....\n");
    InitializeProcQueues();

    idle_proc = CreateIdlePCB(uctxt);
    current_process = idle_proc;
    EnableVM();
    TracePrintf(0, "Boot sequence till creating Idle process is done!\n");
    memcpy(uctxt, &current_process->user_context, sizeof(UserContext));
    return; // returns to user mode.

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
    //     next = queueDequeue(ready_queue)
    //     if next == NULL:
    //         next = idle_pcb  // run idle if no ready processes

    //     rc = KernelContextSwitch(KCSwitch, current, next)

    //     if rc == -1:
    //         TracePrintf(0, "Context switch failed")
    //         Halt()

    //     // When we return here, current process has resumed
    //     HandlePendingTrapsOrSyscalls()
}

pte_t *InitializeKernelStackIdle(void) {
    pte_t* kernel_stack = malloc(sizeof(pte_t) * KSTACK_PAGES);
    if (kernel_stack == NULL) {
        TracePrintf(0, "InitializeKernelStackIdle: Failed to allocate memory for idle process Kernel Stack.\n");
        return NULL;
    }
    for (int i = 0; i < KSTACK_PAGES; i++) {
        int pfn = KSTACK_START_PAGE + i;
        kernel_stack[i].valid = 1;
        kernel_stack[i].pfn = pfn;
        kernel_stack[i].prot = PROT_WRITE | PROT_READ;

        allocSpecificFrame(pfn, FRAME_KERNEL, -1);
    }
    return kernel_stack;
}

pte_t *InitializeKernelStackProcess(void) {
    pte_t* kernel_stack = malloc(sizeof(pte_t) * KSTACK_PAGES);
    if (kernel_stack == NULL) {
        TracePrintf(0, "InitializeKernelStackProcess: Failed to allocate memory for idle process Kernel Stack.\n");
        return NULL;
    }
    for (int i = 0; i < KSTACK_PAGES; i++) {
        int pfn = allocFrame(FRAME_KERNEL, -1);
        if (pfn == -1) {
            TracePrintf(0, "InitializeKernelStackProcess: Failed to allocate frame for kernel stack.\n");
            Halt();
        }
        int pfn = KSTACK_START_PAGE + i;
        kernel_stack[i].valid = 1;
        kernel_stack[i].pfn = pfn;
        kernel_stack[i].prot = PROT_WRITE | PROT_READ;
    }
    return kernel_stack;
}

int SetKernelBrk(void *addr_ptr)
{
    if (addr_ptr == NULL) {
        TracePrintf(0, "SetKernelBrk: NULL addr\n");
        return -1;
    }

    /* convert address to target VPN */
    unsigned int target_vaddr = DOWN_TO_PAGE((unsigned int) addr_ptr);
    unsigned int target_vpn  = target_vaddr >> PAGESHIFT;

    /* bounds: cannot grow into kernel stack (stack base is virtual address) */
    unsigned int stack_base_vpn = DOWN_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT;
    if (target_vpn >= stack_base_vpn) {
        TracePrintf(0, "SetKernelBrk: target collides with kernel stack\n");
        return -1;
    }

    /* current kernel_brk is a page number (vpn) */
    unsigned int cur_vpn = (unsigned int) kernel_brk_page; /* assume kernel_brk is already VPN */

    /* Grow heap: map pages for VPNs [cur_vpn .. target_vpn-1] */
    while (cur_vpn < target_vpn) {
        int pfn = allocFrame(FRAME_KERNEL, -1);

        if (pfn == -1) {
            TracePrintf(0, "SetKernelBrk: out of physical frames\n");
            return -1;
        }

        /* Map physical pfn into region0 VPN = cur_vpn */
        MapRegion0VPN(cur_vpn, pfn);
        cur_vpn++;
    }

    /* Shrink heap: unmap pages for VPNs [target_vpn .. cur_vpn-1] */
    while (cur_vpn > target_vpn) {
        cur_vpn--; /* handle the page at vpn = cur_vpn - 1 */

        if (!vpn_in_region0(cur_vpn)) {
            TracePrintf(0, "SetKernelBrk: shrink vpn out of range %u\n", cur_vpn);
            return -1;
        }

        if (!pt_region0[cur_vpn].valid) {
            TracePrintf(0, "SetKernelBrk: unmapping already-unmapped vpn %u\n", cur_vpn);
            continue;
        }

        int pfn = (int) pt_region0[cur_vpn].pfn;

        /* unmap */
        UnmapRegion0(cur_vpn);
        WriteRegister(REG_TLB_FLUSH, (unsigned int) cur_vpn);


        /* free the physical frame */
        freeFrame(pfn);
    }

    /* update kernel_brk (store vpn) */
    kernel_brk_page = (int) target_vpn;
    return 0;
}