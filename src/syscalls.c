#include "queue.h"
#include "proc.h"
#include "kernel.h"
#include "syscalls.h"
#include "hardware.h"
#include "ykernel.h"
#include "mem.h"

int Delay(int clock_ticks) {
    if (clock_ticks == 0) {
        return 0;
    }
    if (clock_ticks < -1) {
        TracePrintf(SYSCALLS_TRACE_LEVEL, "Delay: Can't delay for a negative number of ticks.\n");
        return -1;
    }

    // Get the current running process to delay
    PCB *curr = current_process;
    curr->delay_ticks = clock_ticks;

    // Change its status to blocked and add it to blocked queue
    curr->state = PROC_BLOCKED;
    queueEnqueue(blocked_queue, curr);

    // Get the next ready process to run
    PCB *next_proc = queueDequeue(ready_queue);
    if (next_proc == NULL) {
        next_proc = idle_proc;
    }

    TracePrintf(SYSCALLS_TRACE_LEVEL, "Delay: Process PID %d is delayed. Switching to process PID %d...\n", curr->pid, next_proc->pid);
    KernelContextSwitch(KCSwitch, curr, next_proc);

    return 0;
}

int GetPid(void) {
    return current_process->pid;
}

int Brk(void *addr) {
    if (addr == NULL) {
        TracePrintf(SYSCALLS_TRACE_LEVEL, "Brk: Address passed to Brk is NULL.\n");
        return -1;
    }
    unsigned int aligned_addr = UP_TO_PAGE((unsigned int) addr);
    unsigned int aligned_user_heap_brk = UP_TO_PAGE(current_process->user_heap_end_vaddr);
    
    unsigned int target_vpn  = (aligned_addr - VMEM_1_BASE) >> PAGESHIFT;
    unsigned int user_heap_brk_vpn = (aligned_user_heap_brk - VMEM_1_BASE) >> PAGESHIFT;

    pte_t *pt_region1 = current_process->ptbr;
    // In case of growing the heap
    while (user_heap_brk_vpn < target_vpn) {
        if (pt_region1[user_heap_brk_vpn].valid == 1) {
            user_heap_brk_vpn++;
            continue;
        }
        // Allocate new frame
        int pfn = allocFrame(FRAME_USER, current_process->pid);
        if (pfn == -1) {
            TracePrintf(SYSCALLS_TRACE_LEVEL, "Brk: Failed to allocate frames to expand user heap for process PID %d!\n", current_process->pid);
            // TODO: If we already allocated frames for this, free them
            return -1;
        }
        // Map the frame
        pt_region1[user_heap_brk_vpn].pfn = pfn;
        pt_region1[user_heap_brk_vpn].prot = PROT_READ | PROT_WRITE;
        pt_region1[user_heap_brk_vpn].valid = 1;

        // Moving on onto the new page
        user_heap_brk_vpn++;
    }

    while (user_heap_brk_vpn > target_vpn) {
        if (pt_region0[user_heap_brk_vpn].valid == 0) {
            user_heap_brk_vpn--;
            continue;
        }
        freeFrame(pt_region1[user_heap_brk_vpn].pfn);
        pt_region1[user_heap_brk_vpn].valid = 0; // Mark this mapping as invalid
        WriteRegister(REG_TLB_FLUSH, ((user_heap_brk_vpn) << PAGESHIFT) + VMEM_1_BASE); // Flushing it out from the TLB
        user_heap_brk_vpn++;
    }

    current_process->user_heap_end_vaddr = (user_heap_brk_vpn << PAGESHIFT) + VMEM_1_BASE;
    return 0;

}