#include "queue.h"
#include "proc.h"
#include "kernel.h"
#include "mem.h"
#include "syscalls/process.h"
#include <hardware.h>
#include <ykernel.h>

/* ============ Forward Declarations =========== */
void OrphanAdoptionHelper(void *arg, PCB *child);
/* ============ Forward Declarations =========== */


/**
 * ======= Brk =======
 * See process.h for more details.
*/
int Brk(void *addr) {
    if (addr == NULL) {
        TracePrintf(SYSCALLS_TRACE_LEVEL, "Brk: Address passed to Brk is NULL.\n");
        return ERROR;
    }
    TracePrintf(0, "current user heap brk before brk is %u\n", current_process->user_heap_end_vaddr);
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
        int pfn = AllocFrame(FRAME_USER, current_process->pid);
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
        FreeFrame(pt_region1[user_heap_brk_vpn].pfn);
        pt_region1[user_heap_brk_vpn].valid = 0; // Mark this mapping as invalid
        WriteRegister(REG_TLB_FLUSH, ((user_heap_brk_vpn) << PAGESHIFT) + VMEM_1_BASE); // Flushing it out from the TLB
        user_heap_brk_vpn++;
    }

    current_process->user_heap_end_vaddr = (user_heap_brk_vpn << PAGESHIFT) + VMEM_1_BASE;
    TracePrintf(0, "current user heap brk after brk is %u\n", current_process->user_heap_end_vaddr);
    return SUCCESS;

}

/**
 * ======= Fork =======
 * See process.h for more details.
*/
int Fork (void) {
    PCB *child = getFreePCB();   // Create new process: pid, process control block
    if (child == NULL) {
        TracePrintf(0, "Fork: Failed to find a free PCB for the child process!\n");
        return ERROR;
    }
    PCB *parent = current_process;
    child->ppid = parent->pid; // Mapping the pid of the parent to the ppid in the child

    // Copy current `UserContext` from parent process PCB to child process's PCB
    memcpy(&child->user_context, &parent->user_context, sizeof(UserContext));

    // Now need to copy region1 pagetable
    int result = CopyPT(parent, child);
    if (result == ERROR) {
        TracePrintf(0, "Fork: Failed to clone region 1 memory into child process!\n");
        return ERROR;
    }
    // Copy heap brk and all the user stuff

    // Copy kernel stack and kernel context from parent process into child process.
    int rc = KernelContextSwitch(KCCopy, child, NULL); // Child process resumes executing from here. Caused so many issues

    if (rc == -1) {
        TracePrintf(0, "Fork: Kernel Context Switch failed while copying kernel stack!\n");
        return ERROR;
    }

    // Because both parent and child execute this part after returning from context switch
    if (current_process->pid == parent->pid) {
        // If its the parent, set the child ready for scheduling
        child->state = PROC_READY;
        queue_enqueue(ready_queue, child);
        queue_enqueue(parent->children_processes, child); // Also add it to the child processes queue of the parent
        (&current_process->user_context)->regs[0] = child->pid; // Return value for Fork for the parent (child's pid)
    } else {
        (&current_process->user_context)->regs[0] = 0; // Return value for Fork for the child (0)
        current_process->parent = parent;
    }

    return SUCCESS;
}


/**
 * ======= Exec =======
 * See process.h for more details.
*/
int Exec(char *filename, char **argvec) {
    PCB *curr = current_process;
    int load_status = LoadProgram(filename, argvec, curr);
    if (load_status == ERROR) {
        TracePrintf(0, "Exec: Process PID %d failed to execute %s.\n", curr->pid, filename);
        return ERROR;
    }

    TracePrintf(0, "Exec: Succeded in executing %s for process PID %d.\n", filename, curr->pid);
    return SUCCESS;
}


/**
 * ======= Exit =======
 * See process.h for more details.
*/
void Exit (int status) {
    // Need to handle orphan processes somehow
    PCB* curr = current_process;
    if (curr->pid == 1) {
        deletePCB(curr);
        Halt();
    }
    
    // Handling the orphan adoption for this process if it has any children
    if (!is_empty(curr->children_processes)) {
        queue_iterate(curr->children_processes, (void *)init_proc, OrphanAdoptionHelper);
    }

    queue_enqueue(zombie_queue, curr);
    curr->exit_status = status;
    curr->state = PROC_ZOMBIE;

    PCB *parent = curr->parent;
    if (parent != NULL && parent->waiting_for_child_pid == 1) {
        parent->state = PROC_READY;
        parent->waiting_for_child_pid = 0;
        queue_remove(blocked_queue, parent);
        queue_enqueue(ready_queue, parent);
    }

    TracePrintf(0, "Exiting process PID %d and switching to a different process...\n", curr->pid);
    PCB *next = is_empty(ready_queue) ? idle_proc : queue_dequeue(ready_queue);
    int rc = KernelContextSwitch(KCSwitch, curr, next);
    if (rc == -1) {
        TracePrintf(0, "Exit: Failed to switch context inside syscall Exit!\n");
        Halt();
    }
}

/**
 * ======= Wait =======
 * See process.h for more details.
*/
int Wait (int * status_ptr) {
    PCB *curr = current_process;
    if (curr->children_processes->head == NULL) {
        TracePrintf(0, "Wait: Error! No children to wait on!\n");
        return ERROR;
    }

    QueueNode_t *zombie_node = zombie_queue->head;
    while (zombie_node != NULL) {
        PCB *zombie_process = zombie_node->process;
        if (zombie_process->ppid == curr->pid) {
            TracePrintf(0, "Parent process PID %d reaping child zombie process PID %d\n", curr->pid, zombie_process->pid);
            if (status_ptr != NULL) *status_ptr = zombie_process->exit_status;
            int pid = zombie_process->pid;
            queue_remove(curr->children_processes, zombie_process);
            queue_remove(zombie_queue, zombie_process);
            deletePCB(zombie_process);
            return pid;
        }
        zombie_node = zombie_node->next;
    }

    curr->state = PROC_BLOCKED;
    curr->waiting_for_child_pid = 1;
    queue_enqueue(blocked_queue, curr);
    PCB *next = is_empty(ready_queue) ? idle_proc : queue_dequeue(ready_queue);
    int rc = KernelContextSwitch(KCSwitch, curr, next);
    if (rc == -1) {
        TracePrintf(0, "Wait: Failed to switch context inside syscall Wait!\n");
        Halt();
    }

    zombie_node = zombie_queue->head;
    while (zombie_node != NULL) {
        PCB *zombie_process = zombie_node->process;
        if (zombie_process->ppid == curr->pid) {
            TracePrintf(0, "Parent process PID %d reaping child zombie process PID %d\n", curr->pid, zombie_process->pid);
            if (status_ptr != NULL) *status_ptr = zombie_process->exit_status;
            int pid = zombie_process->pid;
            queue_remove(curr->children_processes, zombie_process);
            queue_remove(zombie_queue, zombie_process);
            deletePCB(zombie_process);
            return pid;
        }
        zombie_node = zombie_node->next;
    }
    return ERROR;

}

/**
 * ======= GetPid =======
 * See process.h for more details.
*/
int GetPid (void) {
   // Get current process PCB and return the PID
   return current_process->pid;
}

/**
 * ======= Delay =======
 * See process.h for more details.
*/
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

    // Change its status to blocked and add it to blocked queue
    curr->state = PROC_BLOCKED;
    queue_enqueue(blocked_queue, curr);
    
    // Get the next ready process to run
    PCB *next_proc = (is_empty(ready_queue)) ? idle_proc : queue_dequeue(ready_queue);
    
    TracePrintf(SYSCALLS_TRACE_LEVEL, "Delay: Process PID %d is delayed. Switching to process PID %d...\n", curr->pid, next_proc->pid);
    KernelContextSwitch(KCSwitch, curr, next_proc);

    return SUCCESS;
}


/* =============== Helpers =============== */

/**
 * Description: Helper used when a process exits to reassign its children to the
 *              init process (PID 1). If a reparented child is already a zombie,
 *              wakes init so it can reap the zombie.
 * ========= Parameters ========
 * @param arg   : Pointer to the init process PCB.
 * @param child : Child process being reparented.
 * ========= Returns ==========
 * @return (void)
 */
void OrphanAdoptionHelper(void *arg, PCB *child) {
    PCB *init_proc = (PCB *)arg;

    // Reparenting
    child->ppid = 1;
    child->parent = init_proc;

    // Adding it to its queue of children
    queue_t *init_proc_children = init_proc->children_processes;
    queue_enqueue(init_proc_children, child);
    
    if (child->state == PROC_ZOMBIE) {
        TracePrintf(0, "OrphanAdoption: Child %d is a zombie. Waking up Init to reap it.\n", child->pid);
        // Check if Init is actually blocked and waiting for a zombie child to reap(presumably in Wait())
        if (init_proc->waiting_for_child_pid) {
            init_proc->state = PROC_READY;
            init_proc->waiting_for_child_pid = 0;

            queue_remove(blocked_queue, init_proc);
            queue_enqueue(ready_queue, init_proc);
        }
    }

}
