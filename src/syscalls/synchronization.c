#include "syscalls/synchronization.h"

#include "queue.h"
#include "proc.h"
#include "kernel.h"

Lock_t locks[NUM_LOCKS];
Cvar_t cvars[NUM_CVARS];
int num_locks_active = 0;
int num_cvars_active = 0;

int ReclaimLockHelper(int id);
int ReclaimCvarHelper(int id);

int LockInit(int *lock_idp) {
   if (num_locks_active == NUM_LOCKS) {
      TracePrintf(0, "LockInit_ERROR: Can't initialize another lock. Already reached the maximum number of locks in use.\n");
      return ERROR;
   }

   int index = -1;
   for (int i = 0; i < NUM_LOCKS; i++) {
      if (!locks[i].is_active) {
         index = i;
         break;
      }
   }

   int id = CREATE_ID(LOCK_TYPE, index);
   *lock_idp = id;
   locks[index].id = id;
   locks[index].is_active = 1;
   locks[index].locked = 0;
   locks[index].owner = NULL;
   
   queue_t *waiting_processes = queueCreate();
   if (waiting_processes == NULL) {
      TracePrintf(0, "LockInit_ERROR: Failed to allocate memory for the lock waiting processes queue!\n");
      return ERROR;
   }
   locks[index].waiting_processes = waiting_processes;

   num_locks_active++;
   return SUCCESS;
}

int Acquire (int lock_id) {
   int lock_index = GET_INDEX(lock_id);
   if (lock_index > NUM_LOCKS || lock_index < 0) {
      TracePrintf(0, "Acquire_ERROR: A lock with id %d doesn't exist!\n", lock_id);
      return ERROR;
   }
   Lock_t *lock = &locks[lock_index];
   if (!lock->is_active) {
      TracePrintf(0, "Acquire_ERROR: Lock %d is not initalized yet!\n", lock_id);
      return ERROR;
   }

   PCB *curr = current_process;
   // Check if lock is in use by another process
   if (lock->locked) {
      TracePrintf(0, "Acquire: Lock %d is in use by process PID %d. Putting process PID %d to sleep for now.\n", lock_id, lock->owner->pid, curr->pid);
      // Put the current process in the waiting queue for this lock
      queueEnqueue(lock->waiting_processes, curr);

      // Block the process
      curr->state = PROC_BLOCKED;
      queueEnqueue(blocked_queue, curr);

      // Switch to another available process until the lock is free
      PCB *next = is_empty(ready_queue) ? idle_proc : queueDequeue(ready_queue);
      KernelContextSwitch(KCSwitch, curr, next);

      // If we get here, that means we got woken up by the trap handler
      TracePrintf(0, "Acquire: Process PID %d got woken up by release call!\n", curr->pid);
   }

   // Marking this lock as locked and set the owner to be the current owner
   lock->locked = 1;
   lock->owner = curr;
   TracePrintf(0, "Acquire: Process PID %d has possession of lock %d.\n", curr->pid, lock->id);
   return SUCCESS;
}

int Release (int lock_id) {
   int lock_index = GET_INDEX(lock_id);
   if (lock_index >= NUM_LOCKS || lock_index < 0) {
      TracePrintf(0, "Release_ERROR: A lock with id %d doesn't exist!\n", lock_id);
      return ERROR;
   }
   Lock_t *lock = &locks[lock_index];
   PCB *curr = current_process;

   if (!lock->is_active) {
      TracePrintf(0, "Release_ERROR: the lock with id %d hasn't been initialized!\n", lock_id);
      return ERROR;
   }

   if (lock->owner != curr) {
      TracePrintf(0, "Release_ERROR: Lock %d is not owned by process PID %d.\n", curr->pid);
      return ERROR;
   }

   // Before releasing, first check if there are any waiting processes and pass them the lock
   if(!is_empty(lock->waiting_processes)) {
      PCB *next = queueDequeue(lock->waiting_processes);
      TracePrintf(0, "Release: Process PID %d releasing lock %d and passing it to process PID %d.\n", curr->pid, lock->id, next->pid);

      // Change its state to ready, remove it from blocked queue and put it in ready queue
      next->state = PROC_READY;
      lock->owner = next;
      queueRemove(blocked_queue, next);
      queueEnqueue(ready_queue, next);
   } else {
      // Otherwise, just release the lock.
      lock->locked = 0;
      lock->owner = NULL;
   }
   return SUCCESS;
}

int CvarInit (int * cvar_idp) {
   if (num_cvars_active == NUM_CVARS) {
      TracePrintf(0, "CvarInit_ERROR: Can't initialize another cvar. Already reached the maximum number of cvars in use.\n");
      return ERROR;
   }

   int index = -1;
   for (int i = 0; i < NUM_CVARS; i++) {
      if (!cvars[i].is_active) {
         index = i;
         break;
      }
   }

   int id = CREATE_ID(CVAR_TYPE, index);
   *cvar_idp = id;
   cvars[index].id = id;
   cvars[index].is_active = 1;
   
   queue_t *waiting_processes = queueCreate();
   if (waiting_processes == NULL) {
      TracePrintf(0, "CvarInit_ERROR: Failed to allocate memory for the cvar waiting processes queue!\n");
      return ERROR;
   }
   cvars[index].waiting_processes = waiting_processes;

   num_cvars_active++;
   return SUCCESS;
}

int CvarWait (int cvar_id, int lock_id) {
   int cvar_idx = GET_INDEX(cvar_id);
   int lock_idx = GET_INDEX(lock_id);
   PCB *curr = current_process;

   if (cvar_idx >= NUM_CVARS || cvar_idx < 0 || !cvars[cvar_idx].is_active) {
      TracePrintf(0, "CvarWait_ERROR: No such condition variable with id %d exists!\n", cvar_id);
      return ERROR;
   }
   if (lock_idx >= NUM_LOCKS || lock_idx < 0 || !locks[lock_idx].is_active) {
      TracePrintf(0, "CvarWait_ERROR: No such lock with id %d exists!\n", lock_id);
      return ERROR;
   }

   if (locks[lock_idx].owner != curr) {
      TracePrintf(0, "CvarWait_ERROR: The lock %d is not owned by the process PID %d!\n", lock_id, curr->pid);
      return ERROR;
   }

   Lock_t *lock = &locks[lock_idx];
   Cvar_t *cvar = &cvars[cvar_idx];
   // Add this process to list of cvar waiters
   TracePrintf(0, "CvarWait: Process PID %d sleeping on cvar %d.\n", curr->pid, cvar_id);
   queueEnqueue(cvar->waiting_processes, curr);

   // Need to release the lock
   Release(lock_id);

   // Block the current process
   curr->state = PROC_BLOCKED;
   queueEnqueue(blocked_queue, curr);

   // Find the next process to run, or run idle if none exists
   
   PCB *next = (is_empty(ready_queue)) ? idle_proc : queueDequeue(ready_queue);
   TracePrintf(0, "CvarWait: Process PID %d blocked and switching to process PID %d.\n", curr->pid, next->pid);
   KernelContextSwitch(KCSwitch, curr, next);

   // If we get here, it means that someone signaled the cvar
   TracePrintf(0, "CvarWait: Process PID %d woken up by a signal for cvar %d! Must compete for acquiring lock %d again.\n", current_process->pid, cvar_id, lock_id);
   Acquire(lock_id);

   return SUCCESS;
}

int CvarSignal (int cvar_id) {
   int cvar_idx = GET_INDEX(cvar_id);
   PCB *curr = current_process;
   if (cvar_idx >= NUM_CVARS || cvar_idx < 0 || !cvars[cvar_idx].is_active) {
      TracePrintf(0, "CvarSignal_ERROR: No such condition variable with id %d exists!\n", cvar_id);
      return ERROR;
   }

   Cvar_t *cvar = &cvars[cvar_idx];
   // Check if there are any waiting processes
   if (!is_empty(cvar->waiting_processes)) {
      PCB *next_waiting_cvar_process = queueDequeue(cvar->waiting_processes);
      TracePrintf(0, "CvarSignal: Process PID %d waking up process PID %d on condition variable %d.\n", curr->pid, next_waiting_cvar_process->pid, cvar_id);

      next_waiting_cvar_process->state = PROC_READY;
      queueRemove(blocked_queue, next_waiting_cvar_process);
      queueEnqueue(ready_queue, next_waiting_cvar_process);
   }

   return SUCCESS;
}

int CvarBroadcast (int cvar_id) {
   int cvar_idx = GET_INDEX(cvar_id);
   if (cvar_idx >= NUM_CVARS || cvar_idx < 0 || !cvars[cvar_idx].is_active) {
      TracePrintf(0, "CvarBroadcast_ERROR: Invalid Cvar ID %d\n", cvar_id);
      return ERROR;
   }
   Cvar_t *cvar = &cvars[cvar_idx];
   // Wake everyone up
   // Loop until the queue is empty
   while (!is_empty(cvar->waiting_processes)) {
      PCB *proc = queueDequeue(cvar->waiting_processes);
        
      TracePrintf(0, "CvarBroadcast: Waking up PID %d from Cvar %d\n", proc->pid, cvar_id);

      // Move from Blocked -> Ready
      proc->state = PROC_READY;
      queueRemove(blocked_queue, proc);
      queueEnqueue(ready_queue, proc);
   }

   return SUCCESS;
}

int Reclaim (int id) {
   int type = GET_TYPE(id);
   int rc = ERROR;
   switch (type) {
      case LOCK_TYPE:
         rc = ReclaimLockHelper(id);
         break;
      case CVAR_TYPE:
         rc = ReclaimCvarHelper(id);
         break;
   }
   return rc;
}

int ReclaimLockHelper(int id) {
   int lock_index = GET_INDEX(id);
   PCB *curr = current_process;
   if (lock_index >= NUM_LOCKS || lock_index < 0) {
      TracePrintf(0, "Reclaim_ERROR: A lock with id %d doesn't exist!\n", id);
      return ERROR;
   }

   Lock_t *lock = &locks[lock_index];
   if (!lock->is_active) {
      TracePrintf(0, "Reclaim_ERROR: The lock with id %d hasn't been initialized yet!\n", id);
      return ERROR;
   }
   if (lock->locked || !is_empty(lock->waiting_processes)) {
      TracePrintf(0, "Reclaim_ERROR: The lock %d is busy! Can't reclaim now.\n", id);
      return ERROR;
   }

   // Clean Up
   queueDelete(lock->waiting_processes);
   lock->waiting_processes = NULL;
   lock->is_active = 0;
   lock->locked = 0;
   num_locks_active--;
   return SUCCESS;
}

int ReclaimCvarHelper(int id) {
   int cvar_index = GET_INDEX(id);
   PCB *curr = current_process;
   if (cvar_index >= NUM_CVARS || cvar_index < 0) {
      TracePrintf(0, "Reclaim_ERROR: A cvar with id %d doesn't exist!\n", id);
      return ERROR;
   }

   Cvar_t *cvar = &cvars[cvar_index];
   if (!cvar->is_active) {
      TracePrintf(0, "Reclaim_ERROR: The cvar with id %d hasn't been initialized yet!\n", id);
      return ERROR;
   }
   if (!is_empty(cvar->waiting_processes)) {
      TracePrintf(0, "Reclaim_ERROR: The cvar %d is busy! Can't reclaim now.\n", id);
      return ERROR;
   }

   // Clean Up
   queueDelete(cvar->waiting_processes);
   cvar->waiting_processes = NULL;
   cvar->is_active = 0;
   num_cvars_active--;
   return SUCCESS;

}



// OPTIONAL START
//
// This section is marked as optional within the yalnix guide, though might be worth implementing
// later..
//
int SemInit (int *, int);
int SemUp (int);
int SemDown (int);
// OPTIONAL END

// A thought I'm having here that will be implementation dependent:
// A desire to move the atomic operations w/ the internal lock into it's own structure
// - This makes "lock" essentially a wrapper-like structure with information about the usage of the lock (diagnostic?)
