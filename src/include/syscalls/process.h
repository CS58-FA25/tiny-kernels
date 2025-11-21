#ifndef PROCESS_H
#define PROCESS_H

#define SYSCALLS_TRACE_LEVEL    0


/**
 * Description: Adjusts the end of the user heap to the address provided in `addr`.
 *              Handles heap growth by allocating and mapping new frames, and handles
 *              shrinking by freeing frames and invalidating PTEs. Updates the 
 *              process's user_heap_end_vaddr on success.
 * ========= Parameters ========
 * @param addr: Desired new program break address.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int Brk(void *addr);

/**
 * Description: Creates a new child process by duplicating the current process’s
 *              address space, user context, kernel stack, and PCB metadata.
 *              Schedules the child and returns twice (parent receives child PID,
 *              child receives 0).
 * ========= Parameters ========
 * @param (none)
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int Fork (void);

/**
 * Description: Replaces the current process image with a new program located at
 *              `filename`, loading arguments from `argvec`. If successful, the 
 *              current process continues execution in the new program image.
 * ========= Parameters ========
 * @param filename : Path to the executable file.
 * @param argvec   : Vector of argument strings for the new program.
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int Exec (char * file, char ** argvec);

/**
 * Description: Terminates the calling process, recording its exit status, cleaning
 *              up resources, and placing the PCB into the zombie queue. If the 
 *              process has children, reassigns them to init. Wakes up the parent 
 *              if it is waiting, then triggers a context switch.
 * ========= Parameters ========
 * @param status : Exit status code for the terminating process.
 * ========= Returns ==========
 * @return (void)
 */
void Exit (int status);

/**
 * Description: Blocks the calling process until one of its child processes exits.
 *              If a zombie child already exists, it is immediately reaped. Upon 
 *              reaping, stores the child's exit status (if status_ptr is non-null)
 *              and returns the child's PID.
 * ========= Parameters ========
 * @param status_ptr : Pointer to store the child's exit status (may be NULL).
 * ========= Returns ==========
 * @return Child PID on success, or ERROR if no children or failure occurs.
 */
int Wait (int * status_ptr);

/**
 * Description: Returns the PID of the current process.
 * ========= Parameters ========
 * @param (none)
 * ========= Returns ==========
 * @return PID of the calling process.
 */
int GetPid (void);

/**
 * Description: Blocks the calling process for a specified number of clock ticks.
 *              Moves the process to the blocked queue and schedules a new process.
 * ========= Parameters ========
 * @param clock_ticks : Number of ticks to delay (0 = no delay).
 * ========= Returns ==========
 * @return SUCCESS or ERROR
 */
int Delay(int clock_ticks);




#endif