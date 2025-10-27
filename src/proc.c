#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "proc.h"

#define KERNEL_TRACE_LEVEL   3

PCB *proc_table = NULL;
unsigned int proc_table_len = 0;

PCB *current_proc = NULL;
PCB *idle_proc = NULL;

runqueue_t ready_queue = { NULL, NULL };
runqueue_t blocked_queue = { NULL, NULL };
runqueue_t zombie_queue = { NULL, NULL };


/* ---------- Initialization ---------- */

/* InitializeProcSubsystem:
 * - allocates proc_table (MAX_PROCS)
 * - sets pid to INVALID_PID and state to PROC_FREE
 * - sets next = NULL (scheduler owns this field)
 */
void InitializeProcSubsystem(void) {
    proc_table_len = MAX_PROCS;
    proc_table = malloc(sizeof(PCB) * proc_table_len);
    if (!proc_table) {
        TracePrintf(KERNEL_TRACE_LEVEL, "InitializeProcSubsystem: out of memory\n");
        Halt();
    }

    /* Clear and initialize each PCB */
    for (unsigned int i = 0; i < proc_table_len; ++i) {
        PCB *p = &proc_table[i];
        memset(p, 0, sizeof(PCB));
        p->pid = INVALID_PID;
        p->state = PROC_FREE;
        p->ppid = -1;
        p->exit_status = 0;
        p->ptbr = NULL;
        p->ptlr = 0;
        p->kstack_base = NULL;
        p->kstack_npages = 0;
        p->user_heap_start_vaddr = 0;
        p->user_heap_end_vaddr = 0;
        p->user_stack_base_vaddr = 0;
        p->next = NULL;
        p->waiting_for_child_pid = -1;
        p->last_run_tick = 0;
    }

    /* initialize queues */
    ready_queue.head = ready_queue.tail = NULL;
    blocked_queue.head = blocked_queue.tail = NULL;
    zombie_queue.head = zombie_queue.tail = NULL;

    TracePrintf(KERNEL_TRACE_LEVEL, "Process subsystem initialized (MAX_PROCS=%u)\n", proc_table_len);
}

/* ---------- PCB allocation / freeing ---------- */

/* AllocPCB:
 * - finds a PROC_FREE slot in proc_table
 * - initializes fields
 * - returns pointer to PCB or NULL if none available
 */
PCB *AllocPCB(void) {
    for (unsigned int i = 0; i < proc_table_len; ++i) {
        if (proc_table[i].pid == INVALID_PID && proc_table[i].state == PROC_FREE) {
            PCB *p = &proc_table[i];
            /* Clear but preserve struct size */
            memset(p, 0, sizeof(PCB));
            p->pid = (int) i;   /* pid == index (simple scheme) */
            p->state = PROC_READY; /* caller may change to other states */
            p->ppid = -1;
            p->exit_status = 0;
            p->ptbr = NULL;
            p->ptlr = 0;
            p->kstack_base = NULL;
            p->kstack_npages = 0;
            p->user_heap_start_vaddr = 0;
            p->user_heap_end_vaddr = 0;
            p->user_stack_base_vaddr = 0;
            p->next = NULL;
            p->waiting_for_child_pid = -1;
            p->last_run_tick = 0;
            TracePrintf(KERNEL_TRACE_LEVEL,"AllocPCB: created pid=%d\n", p->pid);
            return p;
        }
    }
    TracePrintf(KERNEL_TRACE_LEVEL, "AllocPCB: no free PCB available\n");
    return NULL;
}

/* FreePCB:
 * - free resources owned by PCB (page table, kernel stack frames, etc.)
 * - mark entry free (pid = INVALID_PID, state = PROC_FREE)
 *
 * NOTE: actual freeing of page-table entries and kernel-stack frames depends
 * on your VM and frame allocator. Add code to unmap pages and free frames
 * as needed in the TODO sections below.
 */
void FreePCB(PCB *p) {
    if (!p) return;

    TracePrintf(KERNEL_TRACE_LEVEL, "FreePCB: freeing pid=%d\n", p->pid);

    /* TODO: free user-region page table (ptbr) and associated frames */
    if (p->ptbr) {
        /* If ptbr points to a dynamically allocated pte array for this process,
           free it here and free any PFNs it used (walk ptbr entries). */
        free(p->ptbr);
        p->ptbr = NULL;
    }

    /* TODO: free kernel stack pages allocated for this process.
    free them here (call freeFrame for each page) and unmap region0 VPNs. */
    if (p->kstack_base) {
        /* Example placeholder: you should implement actual unmap + free logic */
        /* UnmapRegion0Range((unsigned int) p->kstack_base, p->kstack_npages); */
        p->kstack_base = NULL;
        p->kstack_npages = 0;
    }

    /* reset bookkeeping */
    p->pid = INVALID_PID;
    p->state = PROC_FREE;
    p->ppid = -1;
    p->exit_status = 0;
    p->ptlr = 0;
    p->user_heap_start_vaddr = p->user_heap_end_vaddr = p->user_stack_base_vaddr = 0;
    p->next = NULL;
    p->waiting_for_child_pid = -1;
    p->last_run_tick = 0;
}

/* ---------- Queue primitives ---------- */

/* Enqueue: add process to tail of queue */
void Enqueue(runqueue_t *q, PCB *p) {
    if (!q || !p) return;
    p->next = NULL;
    if (q->tail == NULL) {
        q->head = q->tail = p;
    } else {
        q->tail->next = p;
        q->tail = p;
    }
}

/* Dequeue: pop head from queue and return it (or NULL if empty) */
PCB *Dequeue(runqueue_t *q) {
    if (!q) return NULL;
    PCB *p = q->head;
    if (!p) return NULL;
    q->head = p->next;
    if (q->head == NULL) q->tail = NULL;
    p->next = NULL;
    return p;
}

/* RemoveFromQueue: remove a specific PCB from a queue.
 * Returns 0 if removed, -1 if not found.
 */
int RemoveFromQueue(runqueue_t *q, PCB *p) {
    if (!q || !p) return -1;
    PCB *prev = NULL;
    PCB *cur = q->head;
    while (cur) {
        if (cur == p) {
            if (prev) prev->next = cur->next;
            else q->head = cur->next;
            if (q->tail == cur) q->tail = prev;
            cur->next = NULL;
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    return -1;
}

/* Convenience wrappers for ready-queue */
void EnqueueReady(PCB *p) {
    if (!p) return;
    p->state = PROC_READY;
    Enqueue(&ready_queue, p);
}

PCB *DequeueReady(void) {
    PCB *p = Dequeue(&ready_queue);
    if (p) p->state = PROC_RUNNING;
    return p;
}

/* ---------- Process lifecycle helpers ---------- */

/* CreateIdleProcess:
 * - Allocates a PCB for the kernel idle process and sets appropriate fields.
 * - The caller should set up the kernel context (kctx) and kernel stack if needed.
 */
PCB *CreateIdleProcess(void) {
    PCB *p = AllocPCB();
    if (!p) {
        TracePrintf(KERNEL_TRACE_LEVEL, "CreateIdleProcess: failed to alloc PCB\n");
        return NULL;
    }

    p->state = PROC_IDLE;
    p->ppid = -1;
    /* idle pid is typically IDLE_PID (0) in your constants; override pid if desired */
    p->pid = IDLE_PID;

    /* TODO: allocate kernel stack for idle process and initialize p->kctx */
    /* Example:
       AllocateKernelStack(p);
       InitializeIdleKernelContext(&p->kctx, p->kstack_base, ...);
    */

    idle_proc = p;
    TracePrintf("CreateIdleProcess: pid=%d\n", p->pid);
    return p;
}

/* MakeProcessZombie: mark a process as zombie and move to zombie queue.
 * Returns 0 on success.
 */
int MakeProcessZombie(PCB *p, int exit_status) {
    if (!p) return -1;
    /* remove p from any queue it may be on (ready/blocked) */
    RemoveFromQueue(&ready_queue, p);
    RemoveFromQueue(&blocked_queue, p);

    p->state = PROC_ZOMBIE;
    p->exit_status = exit_status;
    p->next = NULL;
    Enqueue(&zombie_queue, p);

    TracePrintf(KERNEL_TRACE_LEVEL,"MakeProcessZombie: pid=%d status=%d\n", p->pid, exit_status);
    return 0;
}

/* ReapZombieByPid: find zombie with pid and free it.
 * Returns 0 on success, -1 if not found.
 */
int ReapZombieByPid(int pid) {
    /* find and remove from zombie queue */
    PCB *prev = NULL;
    PCB *cur = zombie_queue.head;
    while (cur) {
        if (cur->pid == pid) {
            /* unlink */
            if (prev) prev->next = cur->next;
            else zombie_queue.head = cur->next;
            if (zombie_queue.tail == cur) zombie_queue.tail = prev;
            cur->next = NULL;
            /* free resources */
            FreePCB(cur);
            TracePrintf(KERNEL_TRACE_LEVEL, "ReapZombieByPid: reaped pid=%d\n", pid);
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    return -1;
}

/* WaitForChild: parent blocks waiting for a specific child pid (or -1 for any)
 *
 * - If child already a zombie, reap immediately and return child pid.
 * - Otherwise, mark parent as blocked and set parent->waiting_for_child_pid.
 * - The caller (syscall handler) should arrange to schedule another process.
 *
 * NOTE: This helper only sets up waiting state; actual blocking & scheduling
 *       must be done by syscall/trap handler. Here we return:
 *       0 on success (parent now waiting), -1 on error, or child's pid if immediate.
 */
int WaitForChild(PCB *parent, int child_pid) {
    if (!parent) return -1;

    /* Search zombie queue for matching child */
    PCB *z = zombie_queue.head;
    while (z) {
        if (child_pid == -1 || z->pid == child_pid) {
            int found_pid = z->pid;
            /* remove & reap */
            ReapZombieByPid(found_pid);
            return found_pid;
        }
        z = z->next;
    }

    /* Not found -> block parent */
    parent->waiting_for_child_pid = child_pid;
    parent->state = PROC_BLOCKED;
    /* remove from ready queue if present */
    RemoveFromQueue(&ready_queue, parent);
    Enqueue(&blocked_queue, parent);

    TracePrintf(KERNEL_TRACE_LEVEL, "WaitForChild: parent pid=%d waiting for child pid=%d\n", parent->pid, child_pid);
    return 0;
}

/* Unblock processes waiting for a child: called when a child is made a zombie.
 * If a parent was waiting for that child's PID (or waiting for any child), wake it.
 * This traverses blocked_queue and moves matching parents back to ready_queue.
 */
void WakeParentsWaitingFor(int child_pid) {
    PCB *cur = blocked_queue.head;
    while (cur) {
        PCB *next = cur->next; /* save next because cur may be moved */
        if (cur->waiting_for_child_pid == child_pid || cur->waiting_for_child_pid == -1) {
            /* remove from blocked queue and enqueue ready */
            RemoveFromQueue(&blocked_queue, cur);
            cur->waiting_for_child_pid = -1;
            cur->state = PROC_READY;
            Enqueue(&ready_queue, cur);
            TracePrintf(KERNEL_TRACE_LEVEL,"WakeParentsWaitingFor: woken parent pid=%d for child pid=%d\n", cur->pid, child_pid);
            /* Note: do not break; there could be multiple parents waiting for -1 */
        }
        cur = next;
    }
}

/* ---------- End of proc.c ---------- */
