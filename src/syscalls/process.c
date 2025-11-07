#include "queue.h"
#include "proc.h"
#include "kernel.h"
#include "mem.h"
#include "syscalls/process.h"
#include <hardware.h>
#include <ykernel.h>


int Brk(void *addr) {
    if (addr == NULL) {
        TracePrintf(SYSCALLS_TRACE_LEVEL, "Brk: Address passed to Brk is NULL.\n");
        return ERROR;
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
            return ERROR;
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
    return SUCCESS;

}

int Fork (void) {
    PCB *child = getFreePCB();   // Create new process: pid, process control block
    if (child == NULL) {
        TracePrintf(0, "Fork: Failed to find a free PCB for the child process!\n");
        return ERROR;
    }
    TracePrintf(0, "Fork1: Current process pid %d\n", current_process->pid);
    PCB *parent = current_process;
    child->ppid = parent->pid; // Mapping the pid of the parent to the ppid in the child

    // Copy current `UserContext` from parent process PCB to child process's PCB
    memcpy(&child->user_context, &parent->user_context, sizeof(UserContext));
    TracePrintf(0, "Fork2: Current process pid %d\n", current_process->pid);
    TracePrintf(0, "1Printing ready queue inside fork\n");
    print_queue(ready_queue);
    // Now need to copy region1 pagetable
    TracePrintf(0, "Fork3: Current process pid %d\n", current_process->pid);
    int result = CopyPT(parent, child);
    if (result == ERROR) {
        TracePrintf(0, "Fork: Failed to clone region 1 memory into child process!\n");
        return ERROR;
    }
    TracePrintf(0, "2Printing ready queue inside fork\n");
    print_queue(ready_queue);
    // Copy kernel stack and kernel context from parent process into child process.
    int rc = KernelContextSwitch(KCCopy, child, NULL);
    //TracePrintf(0, "Fork4: Current process pid %d\n", current_process->pid);
    if (rc == -1) {
        TracePrintf(0, "Fork: Kernel Context Switch failed while copying kernel stack!\n");
        return ERROR;
    }
    if (current_process->pid == parent->pid) {
        child->state = PROC_READY;
        queueEnqueue(ready_queue, child);
        (&current_process->user_context)->regs[0] = child->pid;
    } else {
        (&current_process->user_context)->regs[0] = 0;
    }
    // TracePrintf(0, "Fork5: Current process pid %d\n", current_process->pid);
    // TracePrintf(0, "3Printing ready queue inside fork\n");
    // print_queue(ready_queue);
    // Marking the child process as ready and adding it to the ready queue
    // child->state = PROC_READY;
    // queueEnqueue(ready_queue, child);
    //queueEnqueue(parent->children_processes, child); // Putting the child process in the parent's children processes queue
    TracePrintf(0, "4Printing ready queue inside fork\n");
    print_queue(ready_queue);
    // Return value register for each process is going to be different
    // (&child->user_context)->regs[0] = 0;
    // (&parent->user_context)->regs[0] = child->pid;
    TracePrintf(0, "Fork6: Current process pid %d\n", current_process->pid);
    return SUCCESS;

    // if (current_process->pid == child->pid) {
    //     TracePrintf(0, "Fork: Child process PID %d returning from fork!\n", child->pid);
    //     child->state = PROC_RUNNING; // Marking it as running

    //     WriteRegister(REG_PTBR1, (unsigned int) child->ptbr);
    //     WriteRegister(REG_PTLR1, child->ptlr);
    //     WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    //     return 0;
    // } else {
    //     TracePrintf(0, "Fork: Parent process PID %d returning from fork!\n");
    //     queueEnqueue(parent->children_processes, child); // Putting the child process in the parent's children processes queue
        
    //     // Marking the child process as ready and adding it to the ready queue
    //     child->state = PROC_READY; 
    //     queueEnqueue(ready_queue, child);
    //     return child->pid;
    // }
}


int Exec (char * file, char ** argvec) {
   // Open file
   // Read header, find entry
   // Load file
   // Create argc, argv
   // Create process, place start to location of entry, load argc and argv into stack
   // If anything above fails, return identifiable error for it
   // Otherwise return ok
   // Thought: If execution fails immediately, it could be worth returning that value instead
}

void Exit (int status) {
   // if this is pid 1, do not allow
   // alternatively if this has no parent (ppid == -1, though this should mean this is also pid 1)
   // if this is not the upmost parent, check if the parent is waiting for this process
   // if so, inform parent at end
   // if this parent has children, rehome them to the parent
   // close open file descriptors
   // get pcb, set exit status
   // set status to zombie
   // if this pcb has a pointer to the next item in the scheduler queue, inform the scheduler to go here next
   // trigger scheduler
}


int Wait (int * status_ptr) {
   // if the caller has no children, return error
   // infinite loop, until you find a zombie child
   // the infinite loop will require a pause in execution. however, if there is already a zombie child, then this returns immediately
   // IF status_ptr is not null, this will be filled with child exit status
}

int GetPid (void) {
   // Get current process PCB and return the PID
   return current_process->pid;
}

int Delay(int clock_ticks) {
    if (clock_ticks == 0) {
        return SUCCESS;
    }
    if (clock_ticks < -1) {
        TracePrintf(SYSCALLS_TRACE_LEVEL, "Delay: Can't delay for a negative number of ticks.\n");
        return ERROR;
    }

    // Get the current running process to delay
    PCB *curr = current_process;
    curr->delay_ticks = clock_ticks;
    TracePrintf(0, "Printing ready queue inside delay before setting current to blocked and dequeuing next ready\n");
    print_queue(ready_queue);
    // Change its status to blocked and add it to blocked queue
    curr->state = PROC_BLOCKED;
    queueEnqueue(blocked_queue, curr);
    
    TracePrintf(0, "!!!!!!!Printing ready queue inside delay before dequeing next ready\n");
    print_queue(ready_queue);
    // Get the next ready process to run
    PCB *next_proc = queueDequeue(ready_queue);
    if (next_proc == NULL) {
        next_proc = idle_proc;
    }
    TracePrintf(0, "!!!!!!!Printing ready queue inside delay after dequeing next ready\n");
    print_queue(ready_queue);
    
    TracePrintf(SYSCALLS_TRACE_LEVEL, "Delay: Process PID %d is delayed. Switching to process PID %d...\n", curr->pid, next_proc->pid);
    KernelContextSwitch(KCSwitch, curr, next_proc);

    return SUCCESS;
}

