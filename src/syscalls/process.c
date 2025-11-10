#include "queue.h"
#include "proc.h"
#include "kernel.h"
#include "mem.h"
#include "syscalls/process.h"
#include "program.h"
#include <hardware.h>
#include <ykernel.h>

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
        queueEnqueue(ready_queue, child);
        queueEnqueue(parent->children_processes, child); // Also add it to the child processes queue of the parent
        (&current_process->user_context)->regs[0] = child->pid; // Return value for Fork for the parent (child's pid)
    } else {
        (&current_process->user_context)->regs[0] = 0; // Return value for Fork for the child (0)
        current_process->parent = parent;
    }

    return SUCCESS;
}


int Exec(char *filename, char **argvec) {
    PCB *curr = current_process;
    extern int (*_GetStringArrayLength)(char**); // hotpatch
    int load_status = LoadProgram(filename, _GetStringArrayLength(argvec),argvec);
    if (load_status == ERROR) {
        TracePrintf(0, "Exec: Process PID %d failed to execute %s.\n", curr->pid, filename);
        return ERROR;
    }

    TracePrintf(0, "Exec: Succeded in executing %s for process PID %d.\n", filename, curr->pid);
    return SUCCESS;
}

void Exit (int status) {
    // Need to handle orphan processes somehow
    PCB* curr = current_process;
    if (curr->pid == 1) {
        deletePCB(curr);
        Halt();
    }

    queueEnqueue(zombie_queue, curr);
    curr->exit_status = status;
    curr->state = PROC_ZOMBIE;

    PCB *parent = curr->parent;
    if (parent && parent->waiting_for_child_pid) {
        parent->state = PROC_READY;
        parent->waiting_for_child_pid = 0;
        queueEnqueue(ready_queue, parent);
    }

    TracePrintf(0, "Exiting process PID %d and switching to a different process...\n", curr->pid);
    PCB *next = is_empty(ready_queue) ? idle_proc : queueDequeue(ready_queue);
    int rc = KernelContextSwitch(KCSwitch, curr, next);
    if (rc == -1) {
        TracePrintf(0, "Exit: Failed to switch context inside syscall Exit!\n");
        Halt();
    }
}


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
            queueRemove(curr->children_processes, zombie_process);
            queueRemove(zombie_queue, zombie_process);
            deletePCB(zombie_process);
            return pid;
        }
        zombie_node = zombie_node->next;
    }

    curr->state = PROC_BLOCKED;
    curr->waiting_for_child_pid = 1;
    PCB *next = is_empty(ready_queue) ? idle_proc : queueDequeue(ready_queue);
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
            queueRemove(curr->children_processes, zombie_process);
            queueRemove(zombie_queue, zombie_process);
            deletePCB(zombie_process);
            return pid;
        }
        zombie_node = zombie_node->next;
    }
    return ERROR;

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

    return SUCCESS;
}

