#ifndef SCHEDULER_H
#define SCHEDULER_H

typedef struct {
  Queue_t* blocked_queue;
  Queue_t* ready_queue;
} Scheduler;

void SchedulerQueueProcess(Scheduler* s, PCB* proc);

void SchedulerRunProcessNow(Scheduler* s, PCB* proc);

void SchedulerRemoveProcess(Scheduler* s, int pid);

#endif // SCHEDULER_H
