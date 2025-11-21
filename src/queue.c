#include "queue.h"
#include "ykernel.h"
#include "proc.h"

/* ============ Forward Declarations ============== */
QueueNode_t *new_node();
/* ============ Forward Declarations ============== */


/**
 * ======== queue_create =======
 * See queue.h for more details
*/
queue_t *queue_create() {
    queue_t *queue = malloc(sizeof(queue_t));
    if (queue) {
        queue->head = NULL;
        queue->tail = NULL;
    }
    return queue;
}

/**
 * ======== queue_enqueue =======
 * See queue.h for more details
*/
void queue_enqueue(queue_t *queue, PCB *process) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't queue_enqueue.\n");
        Halt();
    }
    
    if (process == NULL) {
        TracePrintf(0, "process: process is Null. Can't queue_enqueue.\n");
        Halt();
    }
    QueueNode_t *node = new_node();
    if  (node == NULL) {
        TracePrintf(0, "queue_enqueue: Failed to allocate memory for node struct!\n");
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
    TracePrintf(1, "queue_enqueued PCB (%d pid)\n", process->pid);
}

/**
 * ======== queue_dequeue =======
 * See queue.h for more details
*/
PCB *queue_dequeue(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queue: queue is Null. Can't deqeue.\n");
        Halt();
    }
    QueueNode_t *node = queue->head;
    if (node == NULL) {
        TracePrintf(0, "queue_dequeue: Queue is Empty. Can't deque.\n");
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
    TracePrintf(1, "queue_dequeue: Dequeued process (%d pid)\n", p->pid);
    return p;
}

/**
 * ======== queue_iterate =======
 * See queue.h for more details
*/
void queue_iterate(queue_t* queue, void *arg, void (*itemfunc)(void* arg, PCB* process)) {
    if (queue == NULL || itemfunc == NULL) {
        return;
    }

    QueueNode_t *node = queue->head;
    while (node != NULL) {
        QueueNode_t *next_node = node->next;
        (itemfunc)(arg, node->process);
        node = next_node;
    }
}

/**
 * ======== queue_delete =======
 * See queue.h for more details
*/
void queue_delete(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queue_delete: queue is null. Can't delete.\n");
        Halt();
    }
    QueueNode_t *curr = queue->head;
    while (curr != NULL) {
        QueueNode_t *next = curr->next;
        free(curr);
        curr = next;
    }
    free(queue);
}

/**
 * ======== queue_remove =======
 * See queue.h for more details
*/
void queue_remove(queue_t *queue, PCB *process) {
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
            TracePrintf(0, "queue_remove: Removed process PID %d from queue!\n", process->pid);
            free(node);
            return;
        }
        node = node->next;
    }

    TracePrintf(0, "queue_remove: Couldn't find process PID %d in queue!\n", process->pid);

}

/* =========== Helpers ========== */

/**
 * Description: Allocates memory for a new node for a queue.
 * @param void
 * @return A pointer to a new QueueNode_t object.
*/
QueueNode_t *new_node() {
    QueueNode_t *node = malloc(sizeof(QueueNode_t));
    if (node == NULL) {
        return NULL;
    }
    node->process = NULL;
    node->next = NULL;
    node->prev = NULL;
    
    return node;
}

/**
 * ======== is_empty =======
 * See queue.h for more details
*/
int is_empty(queue_t *queue) {
    if (queue == NULL) {
        TracePrintf(0, "queue: Queue is Null. Can't check if it's empty or not.\n");
        Halt();
    }
    return (queue->head == NULL);
}

/**
 * ======== print_queue =======
 * See queue.h for more details
*/
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