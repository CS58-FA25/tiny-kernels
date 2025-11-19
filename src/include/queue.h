#ifndef QUEUE_H
#define QUEUE_H

typedef struct pcb PCB;

typedef struct QueueNode {
    PCB* process;
    struct QueueNode *next;
    struct QueueNode *prev;
} QueueNode_t;
typedef struct queue {
    QueueNode_t* head;
    QueueNode_t* tail;
} queue_t;

QueueNode_t *newNode();

/**
 * ======================== Description =======================
 * @brief Creates and initializes a new empty queue.
 * ======================== Returns ==========================
 * @returns Pointer to a newly allocated queue_t structure, or NULL if memory allocation fails.
 */
queue_t *queueCreate();

/**
 * ======================== Description =======================
 * @brief Removes a specific PCB from the given queue.
 * 
 * This function adjusts the linked list pointers to unlink
 * the target process from the queue.
 * ======================== Parameters ========================
 * @param queue (queue_t*): Pointer to the queue to remove from.
 * @param process (PCB*): Pointer to the PCB to remove.
 * ======================== Returns ==========================
 * @returns void. Halts the system if queue or process is NULL.
 */
void queueRemove(queue_t *queue, PCB *process);

/**
 * ======================== Description =======================
 * @brief Adds a PCB to the end (tail) of the queue.
 * 
 * If the queue is empty, the new process becomes both the head and tail.
 * ======================== Parameters ========================
 * @param queue (queue_t*): Pointer to the queue to enqueue into.
 * @param process (PCB*): Pointer to the PCB to add.
 * ======================== Returns ==========================
 * @returns void. Halts the system if queue or process is NULL.
 */
void queueEnqueue(queue_t *queue, PCB *process);

/**
 * ======================== Description =======================
 * @brief Removes and returns the PCB at the head of the queue.
 * 
 * Used for scheduling or resource management where FIFO order is desired.
 * ======================== Parameters ========================
 * @param queue (queue_t*): Pointer to the queue to dequeue from.
 * ======================== Returns ==========================
 * @returns Pointer to the dequeued PCB, or NULL if the queue is empty.
 */
PCB *queueDequeue(queue_t *queue);

/**
 * ======================== Description =======================
 * @brief Frees the memory allocated for the queue structure.
 * 
 * Note: This does not free the PCBs inside the queue — only the queue object itself.
 * ======================== Parameters ========================
 * @param queue (queue_t*): Pointer to the queue to delete.
 * ======================== Returns ==========================
 * @returns void. Halts the system if queue is NULL.
 */
void queueDelete(queue_t *queue);

/**
 * ======================== Description =======================
 * @brief Checks if a specific PCB is present in the queue.
 * 
 * Iterates through the queue to find a matching process pointer.
 * ======================== Parameters ========================
 * @param queue (queue_t*): Pointer to the queue to search.
 * @param process (PCB*): Pointer to the PCB to find.
 * ======================== Returns ==========================
 * @returns 1 if the process is found in the queue, 0 otherwise.
 */
int is_in_queue(queue_t *queue, PCB *process);

/**
 * ======================== Description =======================
 * @brief Checks whether the queue is empty.
 * ======================== Parameters ========================
 * @param queue (queue_t*): Pointer to the queue to check.
 * ======================== Returns ==========================
 * @returns 1 if the queue is empty, 0 otherwise.
 */
int is_empty(queue_t *queue);

void print_queue(queue_t *queue);
void queueIterate(queue_t *queue, void *arg, void (*itemfunc)(void *arg, PCB *process));

#endif
