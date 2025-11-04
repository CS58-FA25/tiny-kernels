int Fork (void) {
   // Create new process: pid, control block
   // Copy current `UserContext` from current process memory to new process's memory
   // Note: worth creating explicit memory for new process that parent does not touch
   // Copy kernel stack from (`KernelContext`)
   // If anything about fails, return -1 else
   // Set return value for child to be 0 in `UserContext`
   // Return child pid to parent
}


int Exec (char * file, char ** argvec) {
   // Open file
   // Read header, find entry
   // Load file
   // Create argc, argv
   // Create process, place start to location of entry, load argc and argv into stack
   // If anything above fails, return identifiable error for it
   // Otherwise return ok
   // Thought: If execution fails immediately, it could be worth returning that value instead
}

void Exit (int status) {
   // if this is pid 1, do not allow
   // alternatively if this has no parent (ppid == -1, though this should mean this is also pid 1)
   // if this is not the upmost parent, check if the parent is waiting for this process
   // if so, inform parent at end
   // if this parent has children, rehome them to the parent
   // close open file descriptors
   // get pcb, set exit status
   // set status to zombie
   // if this pcb has a pointer to the next item in the scheduler queue, inform the scheduler to go here next
   // trigger scheduler
}


int Wait (int * status_ptr) {
   // if the caller has no children, return error
   // infinite loop, until you find a zombie child
   // the infinite loop will require a pause in execution. however, if there is already a zombie child, then this returns immediately
   // IF status_ptr is not null, this will be filled with child exit status
}

// int GetPid (void) {
//    // Get current process PCB and return the PID
// }

// int Delay (int clock_ticks) {
//    // get start tick
//    // halt execution until current tick = start tick + clock_ticks
//    // if clock_ticks is 0, return immedately
//    // if clock ticks is less than 0, return error -> "time travel is not carried out" :P
//    // returns 0
// }
