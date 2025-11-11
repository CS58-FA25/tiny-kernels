#include <ylib.h>
#include <yalnix.h>
#include <hardware.h> 

#include "include/program.h"
#include "ykernel.h"
#include "include/kernel.h"
#include "include/proc.h"
#include "include/mem.h"
#include "include/init.h"

#include <fcntl.h>
#include <unistd.h>
#include "load_info.h"

int is_vm_enabled;

// Kernel's region 0 sections
int kernel_brk_page;
int text_section_base_page;
int data_section_base_page;

/* ==================== Helpers =================== */

void CloneFrame(int pfn1, int pfn2);


void KernelStart(char **cmd_args, unsigned int pmem_size, UserContext *uctxt)
{
    TracePrintf(1, "KernelStart: Entering KernelStart\n");
    TracePrintf(1, "Physical memory has size %d\n", pmem_size);

    kernel_brk_page = _orig_kernel_brk_page;

    TracePrintf(1, "Initializing the free frame array...\n");
    InitializeFreeFrameList(pmem_size);

    TracePrintf(1, "Initializing virtual memory (creating region0 of page table and mapping to physical frames)...\n");
    InitializeVM();

    TracePrintf(1, "Initializing the terminals....\n");
    InitializeTerminals();

    TracePrintf(1, "Initializing the interrupt vector table....\n");
    InitializeInterruptVectorTable();

    TracePrintf(1, "Initializing the table of free pids to be used by processes....\n");
    InitializeProcTable();

    TracePrintf(1, "Initializing process queues (ready, blocked, zombie)....\n");
    InitializeProcQueues();
    WriteRegister(REG_PTBR0, (unsigned int)pt_region0);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);
    WriteRegister(REG_VM_ENABLE, 1);

 
    idle_proc = CreateIdlePCB(uctxt);
    current_process = idle_proc;
    WriteRegister(REG_PTBR1, (unsigned int)current_process->ptbr);
    WriteRegister(REG_PTLR1, NUM_PAGES_REGION1);
    TracePrintf(0, "Boot sequence till creating Idle process is done!\n");
    memcpy(uctxt, &current_process->user_context, sizeof(UserContext));

    if (!cmd_args || !*cmd_args) {
	// _Cmd_Args = cmd_args at point of start
	// Initialize a new array for hot-swapped arguments
        cmd_args = malloc(sizeof(char*) * 1);
	*cmd_args = "user/init"; // TODO: in CWD..
	TracePrintf(0, "NO ARGUMENTS PROVIDED!!! Set local `cmd_args` to \"init\".\n");
	TracePrintf(0, "did not touch global `_Cmd_Args`, do NOT expect a reflection in behavior.\n");
    }

    int pidx = LoadProgramFromArguments(cmd_args);

    if (pidx < 0) {
        TracePrintf(0, "Failed to load program.\n");
	// Error!!!! Let's jump out gracefully
    } else {
	// DEBUG CODE WHILE THERE IS PROGRAM TRACKING! This doesn't need to exist later though
#ifdef DEBUG
	Program* prog = GetProgramListing()->progs[pidx];
        TracePrintf(0, "Loaded program: %d (%p) (%s) \n", pidx, prog, prog->file);
	// This run the program with the provided context
	RunProgram(pidx, uctxt);
        // Add the init process to the ready queue to be scheduled to run by the scheduler
	queueEnqueue(ready_queue, prog->_pcb);
        // Now copy kernel context into init process
	KernelContextSwitch(KCCopy, prog->_pcb, NULL);
	memcpy(uctxt, &current_process->user_context, sizeof(UserContext));
#endif
    }
    // Switch back to idle proc registers
    // TracePrintf(0, "KernelStart: Resetting page table registers to those of idle process.\n");
    // WriteRegister(REG_PTBR1, (unsigned int)idle_proc->ptbr);
    // WriteRegister(REG_PTLR1, NUM_PAGES_REGION1);
    // WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL); // Flushing TLB from any stale mappings

   //  memcpy(uctxt, &current_process->user_context, sizeof(UserContext));
    // Still pid 0 running and only pid 1 in there
    return;
}

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    PCB *curr = (PCB *)curr_pcb_p;
    PCB *next = (PCB *)next_pcb_p;

    if (curr) {
        // If there is no current process, the context doesn't get saved :(
        memcpy(&(curr->kernel_context), kc_in, sizeof(KernelContext));
    }

    // Switch kernel stacks in region 0 from current process to next process
    pte_t *next_kstack = next->kstack;
    if (next_kstack == NULL) {
      TracePrintf(0, "nextKstack is null!\n");
    }

    for(int i = 0; i < KSTACK_PAGES; i++) {
        int vpn = (KERNEL_STACK_BASE >> PAGESHIFT) + i;
        pt_region0[vpn] = next_kstack[i];
    }

    // Setting the current process to be the new one
    current_process = next;
    next->state = PROC_RUNNING;
    
    // Switch pointers to region 1 page table in CPU registers.
    WriteRegister(REG_PTBR1, (unsigned int) next->ptbr);
    WriteRegister(REG_PTLR1, next->ptlr);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    return &(next->kernel_context);
}

KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *unused){
    PCB *pcb_new = (PCB *) new_pcb_p;
    if (pcb_new == NULL) {
        TracePrintf(1, "KCCopy: New PCB is null. Can't clone into new process.\n");
    }
    // Copying kernel context of the parent process into the new child process PCB
    memcpy(&(pcb_new->kernel_context), kc_in, sizeof(KernelContext));

    // Initializing the new child process kernel stack
    pte_t *kstack_new = pcb_new->kstack;
    if (kstack_new == NULL) {
        kstack_new = InitializeKernelStackProcess();
        pcb_new->kstack = kstack_new;
    }

    // Copying the contents of the current process kernel stack into the child process
    for (int i = 0; i < KSTACK_PAGES; i++) {
        int pfn_parent = current_process->kstack[i].pfn;
        int pfn_child = kstack_new[i].pfn;

        CloneFrame(pfn_parent, pfn_child);
    }
    
    // In case the CPU switches to the new process after returning. We don't want it to use old kstack mappings
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_KSTACK); 
    
    return kc_in;

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
        allocSpecificFrame(pfn, FRAME_KERNEL, -1);

        kernel_stack[i].valid = 1;
        kernel_stack[i].pfn = pfn;
        kernel_stack[i].prot = PROT_WRITE | PROT_READ;

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
    unsigned int target_vaddr = UP_TO_PAGE((unsigned int) addr_ptr);
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
        MapRegion0(cur_vpn, pfn);
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


