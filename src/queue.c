#include "queue.h"
#include "ykernel.h"
#include "proc.h"

queue_t *create_queue() {
    queue_t *queue = malloc(sizeof(queue_t));
    if (queue) {
        queue->head = NULL;
        queue->tail = NULL;
    }
    return queue;
}

void queue_remove(queue_t *queue, PCB *process) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't remove process.\n");
        Halt();
    }
    if (process == NULL) {
        TracePrintf(0, "process: process is Null. Can't remove from queue.\n");
        Halt();
    }

    if (process->prev = NULL) queue->head = process->next;
    else process->prev->next = process->next;
    if (process->next = NULL) queue->tail = process->prev;
    else process->next->prev = process->prev;

}

void enqueue(queue_t *queue, PCB *process) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't enqueue.\n");
        Halt();
    }
    
    if (process == NULL) {
        TracePrintf(0, "process: process is Null. Can't enqueue.\n");
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
    TracePrintf(1, "Enqueued PCB (%d pid)", process->pid);
}

PCB *dequeue(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't deqeue.\n");
        Halt();
    }
    PCB *p = queue->head;
    if (p == NULL) {
        TracePrintf(0, "Dequeue: Queue is Empty. Can't deque.\n");
        return NULL;
    }
    
    queue->head = p->next;
    TracePrtinf(1, "Dequeue: Dequeued process (%d pid)\n", p->pid);
    return p;
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
    if (queue->head == NULL) return 0;
    else return 1;
}

void delete_queue(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "delete_queue: queue is null. Can't delete.\n");
        Halt();
    }
    free(queue);
}