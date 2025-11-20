#include "syscalls/synchronization.h"

#include "queue.h"
#include "proc.h"
#include "kernel.h"

Lock_t locks[NUM_LOCKS];
Cvar_t cvars[NUM_CVARS];
Pipe_t pipes[NUM_PIPES];

int num_locks_active = 0;
int num_cvars_active = 0;
int num_pipes_active = 0;

int ReclaimLockHelper(int id);
int ReclaimCvarHelper(int id);
int ReclaimPipeHelper(int id);

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

/* ============ Pipes ============ */

int PipeInit (int * pipe_idp) {
   if (num_pipes_active == NUM_PIPES) {
      TracePrintf(0, "PipeInit_ERROR: Can't initialize another Pipe. Already reached the maximum number of pipes in use.\n");
      return ERROR;
   }
   
   // Look for a free pipe to use
   int index = -1;
   for (int i = 0; i < NUM_PIPES; i++) {
      if (pipes[i].is_active == 0) {
         index = i;
         break;
      }
   }

   // Once we find a free pipe, we create an id and save its value at the memory address pointed at by pipe_idp
   int pipe_id = CREATE_ID(PIPE_TYPE, index);
   *pipe_idp = pipe_id;

   // Initializing the pipe
   pipes[index].id = pipe_id;
   pipes[index].is_active = 1;
   pipes[index].len = 0;

   // Initializing blocking queues
   queue_t *waiting_writers = queueCreate();
   queue_t *waiting_readers = queueCreate();
   if (waiting_writers == NULL || waiting_readers == NULL) {
      TracePrintf(0, "PipeInit_ERROR: Failed to allocate memory for queues for pipe %d!\n", pipe_id);
      return ERROR;
   }
   
   // Saving the queues pointers
   pipes[index].waiting_writers = waiting_writers;
   pipes[index].waiting_readers = waiting_readers;

   num_pipes_active++;
   return SUCCESS;
}

int PipeRead(int pipe_id, void *buf, int len) {
   int pipe_idx = GET_INDEX(pipe_id);

   // routine validating arguments
   if (GET_TYPE(pipe_id) != PIPE_TYPE || pipe_idx < 0 || pipe_idx > NUM_PIPES) {
      TracePrintf(0, "PipeRead_ERROR: Invalid pipe ID %d!\n", pipe_id);
      return ERROR;
   }
   if (!pipes[pipe_idx].is_active) {
      TracePrintf(0, "PipeRead_ERROR: Pipe %d hasn't been initialized!\n", pipe_id);
      return ERROR;
   }
   if (buf == NULL || len <= 0) {
      TracePrintf(0, "PipeRead_ERROR: Invalid arugments passed into PipeRead for pipe %d!\n", pipe_id);
      return ERROR;
   }

   Pipe_t *pipe = &pipes[pipe_idx];
   PCB *curr = current_process;
   while (pipe->len == 0) {
      TracePrintf(0, "PipeRead: Pipe %d is empty. Putting process PID %d to sleep!\n", pipe->id, curr->pid);

      // There's nothing to read in the pipe. Put the process to sleep and put it on the waiting queue
      queueEnqueue(pipe->waiting_readers, curr);

      // Block it
      curr->state = PROC_BLOCKED;
      queueEnqueue(blocked_queue, curr);

      // Dispatch the next ready process of the idle process
      PCB * next = (is_empty(ready_queue)) ? idle_proc : queueDequeue(ready_queue);
      KernelContextSwitch(KCSwitch, curr, next);

      // Got woken up by a writer who finished writing....
      TracePrintf(0, "PipeRead: Process PID %d woken up to read pipe %d. Checking if there are any bytes to read....\n", current_process->pid, pipe->id);

      // While loop beacuase it's possible that another process already consumed the buffer.
   }

   // Awesome, now we are sure that there is some data to read from the buffer
   int bytes_to_read = (len >= pipe->len) ? pipe->len : len;
   TracePrintf(0, "PipeRead: Process PID %d reading %d bytes from pipe %d!\n", curr->pid, bytes_to_read, pipe->id);

   // Copy the data into buffer
   memcpy(buf, pipe->read_buffer, bytes_to_read);
   if (bytes_to_read < pipe->len) {
      memmove(pipe->read_buffer, pipe->read_buffer + bytes_to_read, pipe->len - bytes_to_read); // Shift
   }
   pipe->len -= bytes_to_read;

   // Wake up all waiting writers (It's kind of inefficient?)
   while (!is_empty(pipe->waiting_writers)) {
      PCB *waiting_writer = queueDequeue(pipe->waiting_writers);

      // Unblock it and add it to ready queue
      waiting_writer->state = PROC_READY;
      queueRemove(blocked_queue, waiting_writer);
      queueEnqueue(ready_queue, waiting_writer);
   }

   return bytes_to_read;
}

int PipeWrite(int pipe_id, void *buf, int len) {
   int pipe_idx = GET_INDEX(pipe_id);

   // validating arguments
   if (GET_TYPE(pipe_id) != PIPE_TYPE || pipe_idx < 0 || pipe_idx >= NUM_PIPES) {
      TracePrintf(0, "PipeWrite_ERROR: Invalid pipe ID %d!\n", pipe_id);
      return ERROR;
   }
   if (!pipes[pipe_idx].is_active) {
      TracePrintf(0, "PipeWrite_ERROR: Pipe %d hasn't been initialized!\n", pipe_id);
      return ERROR;
   }
   if (buf == NULL || len < 0) {
      TracePrintf(0, "PipeWrite_ERROR: Invalid arguments!\n");
      return ERROR; 
   }

   Pipe_t *pipe = &pipes[pipe_idx];
   PCB *curr = current_process;
   char *user_buffer = (char *)buf;
   int total_written = 0;

   // The chunking loop
   while (total_written < len) {
      
      // free space in the buffer currently available to write
      int free_space = PIPE_BUFFER_LEN - pipe->len;
      // if buffer is full just block
      if (free_space == 0) {
         TracePrintf(0, "PipeWrite: Pipe %d full. PID %d sleeping.\n", pipe->id, curr->pid);
         queueEnqueue(pipe->waiting_writers, curr);
         
         // Block
         curr->state = PROC_BLOCKED;
         queueEnqueue(blocked_queue, curr);

         // Dipsatch next ready process or idle
         PCB *next = (is_empty(ready_queue)) ? idle_proc : queueDequeue(ready_queue);
         KernelContextSwitch(KCSwitch, curr, next);

         // When we wake up, we continue the loop to recalculate free_space
         TracePrintf(0, "PipeWrite: Process PID %d woken up by a reader to write into pipe %d. Checking if there's any free space to write...\n", curr->pid, pipe->id);
         continue; 
      }

      // writing a chunk
      int bytes_left = len - total_written;
      int chunk_size = (bytes_left < free_space) ? bytes_left : free_space;

      TracePrintf(0, "PipeWrite: PID %d writing chunk of %d bytes. Total written so far: %d out of %d\n", curr->pid, chunk_size, total_written, len);

      // FIFO, append to the buffer. Also, adjust pointer of buf to point to the start of the new chunk
      memcpy(pipe->read_buffer + pipe->len, user_buffer + total_written, chunk_size);

      // Updating state
      pipe->len += chunk_size;
      total_written += chunk_size;

      // Wake up all readers (this is highly inefficient again)
      if (!is_empty(pipe->waiting_readers)) {
         TracePrintf(0, "PipeWrite: Waking up waiting readers.\n");
         while (!is_empty(pipe->waiting_readers)) {
            PCB *reader = queueDequeue(pipe->waiting_readers);
            reader->state = PROC_READY;
            queueRemove(blocked_queue, reader);
            queueEnqueue(ready_queue, reader);
         }
      }
   }

   return total_written;
}


/* =========== Condition Variables ============== */

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
      case PIPE_TYPE:
         rc = ReclaimPipeHelper(id);
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

int ReclaimPipeHelper(int id) {
   int pipe_idx = GET_INDEX(id);
   PCB *curr = current_process;
   if (pipe_idx >= NUM_PIPES || pipe_idx < 0) {
      TracePrintf(0, "Reclaim_ERROR: A pipe with id %d doesn't exist!\n", id);
      return ERROR;
   }

   Pipe_t *pipe =  &pipes[pipe_idx];
   if (!pipe->is_active) {
      TracePrintf(0, "Reclaim_ERROR: The pipe with id %d hasn't been initialized yet!\n", id);
      return ERROR;
   }
   if (!is_empty(pipe->waiting_readers) || !is_empty(pipe->waiting_writers) || pipe->len > 0) {
      TracePrintf(0, "Reclaim_ERROR: The pipe with id %d is busy! Can't reclaim now.\n", id);
      return ERROR;
   }

   // Clean Up
   queueDelete(pipe->waiting_readers);
   queueDelete(pipe->waiting_writers);
   pipe->waiting_readers = NULL;
   pipe->waiting_writers = NULL;
   pipe->is_active = 0;
   num_pipes_active--;
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
