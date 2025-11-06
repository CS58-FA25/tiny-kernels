#include "queue.h"
#include "ykernel.h"
#include "proc.h"

queue_t *queueCreate() {
    queue_t *queue = malloc(sizeof(queue_t));
    if (queue) {
        queue->head = NULL;
        queue->tail = NULL;
    }
    return queue;
}

void queueRemove(queue_t *queue, PCB *process) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't remove process.\n");
        Halt();
    }
    if (process == NULL) {
        TracePrintf(0, "process: process is Null. Can't remove from queue.\n");
        Halt();
    }

    if (process->prev == NULL) queue->head = process->next;
    else process->prev->next = process->next;
    if (process->next == NULL) queue->tail = process->prev;
    else process->next->prev = process->prev;

}

void queueEnqueue(queue_t *queue, PCB *process) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't queueEnqueue.\n");
        Halt();
    }
    
    if (process == NULL) {
        TracePrintf(0, "process: process is Null. Can't queueEnqueue.\n");
        Halt();
    }
    process->next = NULL;
    process->prev = queue->tail;
    if (queue->head == NULL) {
        queue->head = process;
    } else {
        queue->tail->next = process;
    }
    queue->tail = process;
    TracePrintf(1, "queueEnqueued PCB (%d pid)\n", process->pid);
}

PCB *queueDequeue(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't deqeue.\n");
        Halt();
    }
    PCB *p = queue->head;
    if (p == NULL) {
        TracePrintf(0, "queueDequeue: Queue is Empty. Can't deque.\n");
        return NULL;
    }

    queue->head = p->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    } else {
        queue->head->prev = NULL;
    }

    p->next = NULL;
    p->prev = NULL;

    TracePrintf(1, "queueDequeue: Dequeued process (%d pid)\n", p->pid);
    return p;
}

void queueDelete(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queueDelete: queue is null. Can't delete.\n");
        Halt();
    }
    free(queue);
}


int is_in_queue(queue_t * queue, PCB *process) {
    if (queue == NULL) {
        TracePrintf(0, "queue: Queue is empty. Can't look up reqeusted process.\n");
        Halt();
    }
    if (process == NULL) {
        TracePrintf(0, "process: process is Null. Can't look it up in queue.\n");
        Halt();
    }

    PCB* curr = queue->head;
    while (curr != NULL) {
        if (curr == process) return 1;
        curr = curr->next;
    }
    return 0;
}

int is_empty(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queue: Queue is Null. Can't check if it's empty or not.\n");
        Halt();
    }
    return (queue->head == NULL);
}


void print_queue(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "print_queue: Error - Queue pointer is NULL.\n");
        return;
    }

    // Print the queue's address for easy identification
    TracePrintf(0, "--- Printing Queue (addr %p) ---", queue);
    
    PCB *current = queue->head;
    if (current == NULL) {
        TracePrintf(0, "  [ EMPTY ]");
    }

    // Loop through and print each PID
    while (current != NULL) {
        TracePrintf(0, "  PID: %d (at %p)", current->pid, current);
        current = current->next;
    }
    
    TracePrintf(0, "--- End of Queue (addr %p) ---\n", queue);
}
