#include "proc.h"
#include "ykernel.h"
#include "kernel.h"
#include "hardware.h"
#include "queue.h"
#include "mem.h"


PCB *idle_proc; // Pointer to the idle process PCB
PCB *init_proc; // A pointer to holde the init process PCB when loaded
PCB *current_process; // Pointer to the current running process PCB
queue_t *ready_queue; // A FIFO queue of processes ready to be executed by the cpu
queue_t *blocked_queue; // A queue of processes blocked (either waiting on a lock, cvar or waiting for an I/O to finish)
queue_t *zombie_queue; // A queue of processes that have terminated but whose parent has not yet called Wait()
queue_t *waiting_parents; // A queue of processes blocked waiting for a child to exit;

/* ============= Start of Forward Declarations ============== */
void deletePCBHelper(void *arg, PCB *process);
void DoIdle(void);
/* ============= End of Forward Declarations ============= */

PCB **proc_table;

/**
 * ======== AllocNewPCB =======
 * See proc.h for more details
*/
PCB *AllocNewPCB() {
    PCB *process = (PCB *)malloc(sizeof(PCB));
    if (process == NULL) {
        TracePrintf(0, "AllocNewPCB: Failed to allocate memory for PCB.\n");
        return NULL;
    }

    // Zero out the struct to avoid garbage values
    memset(process, 0, sizeof(PCB));

    process->ppid = INVALID_PID;      // no parent yet
    process->state = PROC_FREE;
    process->exit_status = 0;
    process->ptbr = (pte_t *)malloc(NUM_PAGES_REGION1 * sizeof(pte_t));

    if (process->ptbr == NULL) {
        TracePrintf(0, "create_PCB: Failed to allocate memory for region page table.\n");
        free(process);
        return NULL;
    }
    memset(process->ptbr, 0, NUM_PAGES_REGION1 * sizeof(pte_t)); // All entries are invalid
    process->ptlr = NUM_PAGES_REGION1;
    
    process->user_heap_start_vaddr = USER_MEM_START; // Reassign after loadProgram is called
    process->user_heap_end_vaddr = USER_MEM_START; // Reassign after loadProgram is called
    process->user_stack_base_vaddr = USER_STACK_BASE;

    process->kstack = NULL;

    // Initialize context structs (no garbage)
    memset(&process->user_context, 0, sizeof(UserContext));
    memset(&process->kernel_context, 0, sizeof(KernelContext));
    // Queue links and relationships
    process->parent = NULL;
    process->children_processes = queue_create();
    if (process->children_processes == NULL) {
        TracePrintf(0, "AllocNewPCB: Failed to create children queue.\n");
        free(process->ptbr);
        free(process);
        return NULL;
    }
    // Bookkeeping
    process->waiting_for_child_pid = INVALID_PID;
    process->last_run_tick = 0;
    process->delay_ticks = 0;

    TracePrintf(1, "AllocNewPCB: New PCB created at %p\n", process);
    return process;

}

/**
 * ======== getFreePCB =======
 * See proc.h for more details
*/
PCB *getFreePCB(void) {
    if (proc_table == NULL) {
        TracePrintf(0, "getFreePCB: The process table is not initialized.\n");
        return NULL;
    }
    
    // Look for an empty (NULL) slot
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i] == NULL) {
            
            // Found one! Allocate a new PCB for it.
            PCB *pcb = AllocNewPCB();
            
            // Put it in the table
            proc_table[i] = pcb; 
            pcb->proc_table_idx = i;
            
            // Set its initial state and return it
            pcb->state = PROC_RUNNING; // Or whatever state you use for "new"
            pcb->pid = helper_new_pid(pcb->ptbr);
            
            return pcb;
        }
    }
    
    // No NULL slots found, table is full
    return NULL;
}

/**
 * ======== deletePCB =======
 * See proc.h for more details
*/
void deletePCB(PCB *process) {
    if (process == NULL) {
        TracePrintf(0, "delete_PCB: Process is null.\n");
        Halt();
    }
    if (process->children_processes != NULL) {
        queue_iterate(process->children_processes, NULL, deletePCBHelper);
    }
    // Free up the queue created for children processes
    queue_delete(process->children_processes);

    // Free memory allocated for region 1 (make sure we freed the frames used up)
    free(process->ptbr);
    // Free memory allocated for kstack (make sure we freed the frames used up)
    free(process->kstack);

    // finally free up the pcb struct allocated for this process
    free(process);
}

/**
 * ======== CreateIdlePCB =======
 * See proc.h for more details
*/
PCB *CreateIdlePCB(UserContext *uctxt) {
    idle_proc = getFreePCB();
    if (idle_proc == NULL) {
        TracePrintf(0, "CreateIdlePCB: Failed to create the idle process pcb.\n");
        Halt();
    }
    // Allocating memory for user stack. Initially it's one page and going to be at the top of region 1
    int ustack_vpn = (VMEM_1_LIMIT - PAGESIZE - VMEM_1_BASE) >> PAGESHIFT;
    int ustack_pfn = AllocFrame(FRAME_USER, 0);
    MapPage(idle_proc->ptbr, ustack_vpn, ustack_pfn, PROT_READ | PROT_WRITE);
    memset(&idle_proc->user_context, 0, sizeof(UserContext));

    idle_proc->user_context.sp = (void *)(VMEM_1_LIMIT - 4);
    (&(idle_proc->user_context))->pc = (void *)DoIdle;

    idle_proc->state = PROC_RUNNING;

    TracePrintf(1, "Created Idle process with %d PID.\n", idle_proc->pid);
    return idle_proc;
}


/* ============== Helper and Idle =============== */

/**
 * Description: itemfunc function passed to queryIterate to iterate over a process children's and
 *              make them orphans (setting their parent to null and changing their status to orphan)
 * =========== Parameters ==========
 * @param arg: NULL.
 * @param process: A pointer to the PCB of the child process to be made an orphan.
 * =========== Returns ===========
 * @return void
*/
void deletePCBHelper(void *arg, PCB* process) {
    process->parent = NULL;
    process->state = PROC_ORPHAN;
}

/**
 * Description: Routine to be run while the ready_queue is empty (no process to run on the cpu)
 *              to keep the cpu busy
 * =========== Parameters ==========
 * @param void
 * =========== Returns ===========
 * @return void
*/
void DoIdle(void) {
    while (1) {
        TracePrintf(0, "Running Idle Process...\n");
        Pause();
    }
}

