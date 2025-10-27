#include "proc.h"
#include "ykernel.h"
#include "kernel.h"
#include "hardware.h"
#include "queue.h"

ready_queue = NULL;
blocked_queue = NULL;
zombie_queue = NULL;


void InitializeProcQueues(void) {
    ready_queue = create_queue();
    if (ready_queue == NULL) {
        TracePrintf(0, "ready_queue: Couldn't allocate memory for ready queue.\n");
        Halt();
    }

    blocked_queue = create_queue();
    if (ready_queue == NULL) {
        TracePrintf(0, "blocked_queue: Couldn't allocate memory for blocked queue.\n");
        Halt();
    }

    zombie_queue = create_queue();
    if (zombie_queue == NULL) {
        TracePrintf(0, "zombie_queue: Couldn't allocate memory for zombie queue.\n");
        Halt();
    }
}

PCB *create_PCB() {
    PCB *process = (PCB *)malloc(sizeof(PCB));
    if (process == NULL) {
        TracePrintf(0, "create_PCB: Failed to allocate memory for PCB.\n");
        return NULL;
    }

    // Zero out the struct to avoid garbage values
    memset(process, 0, sizeof(PCB));

    process->pid = INVALID_PID;       // will be assigned later by process table
    process->ppid = INVALID_PID;      // no parent yet
    process->state = PROC_FREE;
    process->exit_status = 0;
    process->ptbr = (pte_t *)malloc(NUM_PAGES_REGION1 * sizeof(pte_t));

    if (process->ptbr == NULL) {
        TracePrintf(0, "create_PCB: Failed to allocate memory for region page table.\n");
        free(process);
        return NULL;
    }
    memset(process->ptbr, 0, NUM_PAGES_REGION1 * sizeof(pte_t));
    process->ptlr = NUM_PAGES_REGION1;
    
    process->user_heap_start_vaddr = USER_MEM_START; // Reassign after loadProgram is called
    process->user_heap_end_vaddr = USER_MEM_START; // Reassign after loadProgram is called
    process->user_stack_base_vaddr = USER_STACK_BASE;

    process->kstack_base = KERNEL_STACK_BASE;

    // Initialize context structs (no garbage)
    memset(&process->user_context, 0, sizeof(UserContext));
    memset(&process->kernel_context, 0, sizeof(KernelContext));
    // Queue links and relationships
    process->next = NULL;
    process->prev = NULL;
    process->parent = NULL;
    process->children_processes = create_queue();
    if (process->children_processes == NULL) {
        TracePrintf(0, "create_PCB: Failed to create children queue.\n");
        free(process->ptbr);
        free(process);
        return NULL;
    }
    // Bookkeeping
    process->waiting_for_child_pid = INVALID_PID;
    process->last_run_tick = 0;

    TracePrintf(1, "create_PCB: New PCB created at %p\n", process);
    return process;

}

void delete_PCB(PCB *process) {
    if (process == NULL) {
        TracePrintf(0, "delete_PCB: Process is null.\n");
        Halt();
    }

    if (process->children_processes != NULL) {
        for (PCB *child = process->children_processes->head; child != NULL; child = child->next) {
            child->parent = NULL;
            child->state = PROC_ORPHAN;
        }
        delete_queue(process->children_processes);
    }

    if (process->next != NULL) {
        process->next->prev = process->prev;
    }
    
    if (process->prev != NULL) {
        process->prev->next = process->next;
    }
    
    for (int i = 0; i < NUM_PAGES_REGION1; i++) {
        if (process->ptbr[i].valid == 1) {
            
        }
    }
    free(process->ptbr);
    delete_queue(process->children_processes);
    
}