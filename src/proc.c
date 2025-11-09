#include "proc.h"
#include "ykernel.h"
#include "kernel.h"
#include "hardware.h"
#include "queue.h"
#include "mem.h"


PCB *idle_proc; // Pointer to the idle process PCB
PCB *init_proc;
PCB *current_process; // Pointer to the current running process PCB
queue_t *ready_queue; // A FIFO queue of processes ready to be executed by the cpu
queue_t *blocked_queue; // A queue of processes blocked (either waiting on a lock, cvar or waiting for an I/O to finish)
queue_t *zombie_queue; // A queue of processes that have terminated but whose parent has not yet called Wait()
queue_t *waiting_parents; // A queue of processes blocked waiting for a child to exit;

PCB **proc_table;

PCB *allocNewPCB() {
    PCB *process = (PCB *)malloc(sizeof(PCB));
    if (process == NULL) {
        TracePrintf(0, "allocNewPCB: Failed to allocate memory for PCB.\n");
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
    process->next = NULL;
    process->prev = NULL;
    process->parent = NULL;
    process->children_processes = queueCreate();
    if (process->children_processes == NULL) {
        TracePrintf(0, "allocNewPCB: Failed to create children queue.\n");
        free(process->ptbr);
        free(process);
        return NULL;
    }
    // Bookkeeping
    process->waiting_for_child_pid = INVALID_PID;
    process->last_run_tick = 0;
    process->delay_ticks = 0;

    TracePrintf(1, "allocNewPCB: New PCB created at %p\n", process);
    return process;

}

void deletePCBHelper(void *arg, PCB* process) {
    process->parent = NULL;
    process->state = PROC_ORPHAN;
}

void deletePCB(PCB *process) {
    if (process == NULL) {
        TracePrintf(0, "delete_PCB: Process is null.\n");
        Halt();
    }
    if (process->children_processes != NULL) {
        queueIterate(process->children_processes, NULL, deletePCBHelper);
    }
    // Free up the queue created for children processes
    queueDelete(process->children_processes);

    // Free memory allocated for region 1 (make sure we freed the frames used up)
    free(process->ptbr);
    // Free memory allocated for kstack (make sure we freed the frames used up)
    free(process->kstack);

    // finally free up the pcb struct allocated for this process
    free(process);
}

PCB *getFreePCB(void) {
    if (proc_table == NULL) {
        TracePrintf(0, "getFreePCB: The process table is not initialized.\n");
        return NULL;
    }
    
    // Look for an empty (NULL) slot
    for (int i = 0; i < MAX_PROCS; i++) {
        if (proc_table[i] == NULL) {
            
            // Found one! Allocate a new PCB for it.
            PCB *pcb = allocNewPCB();
            
            // Put it in the table
            proc_table[i] = pcb; 
            
            // Set its initial state and return it
            pcb->state = PROC_RUNNING; // Or whatever state you use for "new"
            pcb->pid = helper_new_pid(pcb->ptbr);
            
            return pcb;
        }
    }
    
    // No NULL slots found, table is full
    return NULL;
}

void CloneRegion1(PCB *pcb_from, PCB *pcb_to) {
    pte_t *pt_region1_from = pcb_from->ptbr;
    pte_t *pt_region1_to = pcb_to->ptbr;
    for (int i = 0; i < NUM_PAGES_REGION1; i++) {
        if (pt_region1_from[i].valid == 1) {
            int pfn_to = allocFrame(FRAME_USER, pcb_to->pid);
            int pfn_from = pt_region1_from[i].pfn;
            CloneFrame(pfn_from, pfn_to);

            pt_region1_to[i].pfn = pfn_to;
            pt_region1_to[i].prot = pt_region1_from[i].prot;
            pt_region1_to[i].valid = 1;
        }
    }
}

void InitializeProcQueues(void) {
    ready_queue = queueCreate();
    if (ready_queue == NULL) {
        TracePrintf(0, "ready_queue: Couldn't allocate memory for ready queue.\n");
        Halt();
    }

    blocked_queue = queueCreate();
    if (blocked_queue == NULL) {
        TracePrintf(0, "blocked_queue: Couldn't allocate memory for blocked queue.\n");
        Halt();
    }

    zombie_queue = queueCreate();
    if (zombie_queue == NULL) {
        TracePrintf(0, "zombie_queue: Couldn't allocate memory for zombie queue.\n");
        Halt();
    }

    waiting_parents = queueCreate();
    if (waiting_parents == NULL) {
        TracePrintf(0, "waiting_parents: Couldn't allocate memory for waiting parents queue.\n");
        Halt();
    }
}

PCB *CreateIdlePCB(UserContext *uctxt) {
    idle_proc = getFreePCB();
    if (idle_proc == NULL) {
        TracePrintf(0, "CreateIdlePCB: Failed to create the idle process pcb.\n");
        Halt();
    }
    // Allocating memory for user stack. Initially it's one page and going to be at the top of region 1
    int ustack_vpn = (VMEM_1_LIMIT - PAGESIZE - VMEM_1_BASE) >> PAGESHIFT;
    int ustack_pfn = allocFrame(FRAME_USER, 0);
    MapPage(idle_proc->ptbr, ustack_vpn, ustack_pfn, PROT_READ | PROT_WRITE);
    memset(&idle_proc->user_context, 0, sizeof(UserContext));

    idle_proc->user_context.sp = (void *)(VMEM_1_LIMIT - 4);
    idle_proc->kstack = InitializeKernelStackIdle();
    (&(idle_proc->user_context))->pc = (void *)DoIdle;

    idle_proc->state = PROC_RUNNING;

    TracePrintf(1, "Created Idle process with %d PID.\n", idle_proc->pid);
    return idle_proc;
}

void DoIdle(void) {
    while (1) {
        TracePrintf(0, "Running Idle Process...\n");
        Pause();
    }
}

