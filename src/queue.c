#include "queue.h"
#include "ykernel.h"
#include "proc.h"


QueueNode_t *newNode() {
    QueueNode_t *node = malloc(sizeof(QueueNode_t));
    if (node == NULL) {
        return NULL;
    }
    node->process = NULL;
    node->next = NULL;
    node->prev = NULL;
    
    return node;
}

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

    QueueNode_t *node = queue->head;
    while (node != NULL) {
        if (node->process == process) {
            if (node->prev == NULL) queue->head = node->next;
            else node->prev->next = node->next;
            if (node->next == NULL) queue->tail = node->prev;
            else node->next->prev = node->prev;
            TracePrintf(0, "queueRemove: Removed process PID %d from queue!\n", process->pid);
            free(node);
            return;
        }
        node = node->next;
    }

    TracePrintf(0, "queueRemove: Couldn't find process PID %d in queue!\n", process->pid);

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
    QueueNode_t *node = newNode();
    if  (node == NULL) {
        TracePrintf(0, "queueEnqueue: Failed to allocate memory for node struct!\n");
        return;
    }
    node->process = process;
    node->next = NULL;
    node->prev = queue->tail;
    if (queue->head == NULL) {
        queue->head = node;
    } else {
        queue->tail->next = node;
    }
    queue->tail = node;
    TracePrintf(1, "queueEnqueued PCB (%d pid)\n", process->pid);
}

PCB *queueDequeue(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't deqeue.\n");
        Halt();
    }
    QueueNode_t *node = queue->head;
    if (node == NULL) {
        TracePrintf(0, "queueDequeue: Queue is Empty. Can't deque.\n");
        return NULL;
    }

    PCB *p = node->process;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    } else {
        queue->head->prev = NULL;
    }

    free(node);
    TracePrintf(1, "queueDequeue: Dequeued process (%d pid)\n", p->pid);
    return p;
}

void queueIterate(queue_t* queue, void *arg, void (*itemfunc)(void* arg, PCB* process)) {
    if (queue == NULL || itemfunc == NULL) {
        return;
    }

    QueueNode_t *node = queue->head;
    while (node != NULL) {
        // 1. Get the *next* node *before* calling the function
        QueueNode_t *next_node = node->next;
        
        // 2. Call the function with the *current* node's process
        (itemfunc)(arg, node->process);
        
        // 3. Move to the *next* node (which we safely stored)
        node = next_node;
    }
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

    QueueNode_t *curr_node = queue->head;
    while (curr_node != NULL) {
        if (curr_node->process == process) return 1;
        curr_node = curr_node->next;
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
    QueueNode_t *curr_node = queue->head;
    if (curr_node == NULL) {
        TracePrintf(0, "  [ EMPTY ]");
    }

    while (curr_node != NULL) {
        TracePrintf(0, "  PID: %d (at %p)", curr_node->process->pid, curr_node->process);
        curr_node = curr_node->next;
    }
    TracePrintf(0, "--- End of Queue (addr %p) ---\n", queue);
}
