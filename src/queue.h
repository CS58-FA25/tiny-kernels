#ifndef QUEUE_H
#define QUEUE_H

#include "proc.h"

typedef struct queue {
    PCB* head;
    PCB* tail;
} queue_t;

queue_t *create_queue();
void queue_remove(queue_t *queue, PCB *process);
void enqueue(queue_t *queue, PCB *process);
PCB *dequeue(queue_t *queue);
int is_in_queue(queue_t *queue, PCB *process);
int is_empty(queue_t *queue);
void delete_queue(queue_t *queue);

#endif