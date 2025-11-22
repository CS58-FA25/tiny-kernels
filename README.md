# Final Submission
We implemented the full features of the yalnix kernel. The kernel's entry point is `KernelStart`. When it's booted, the kernel initializes a free frame list to keep track of free frames in the physical memory. It then initializes virtual memory and does an identity mapping for the kernel's region 0 (virtual page number `x` gets mapped to physical frame number `x`). After that, it goes ahead to initialize the rest of the kernel data structures like:
- Process Queues
- Interrupt vector table
- Terminals
- Process Table (to keep track of free slots that we can use to run a process).

After initializing these data structures, it goes ahead to enable VM and writes its region 0 PT to the register `REG_PTBR0`.
The second stage of the program involves creating the idle process PCB. We also initalize its kernel stack (this is the only process that has a special initialize kernel stack function. Because, we want to make sure that the kernel stack has an identity mapping similar to the identity mapping we had earlier for the kernel heap, data and BSS. For other processes, we use whichever frame to allocate memory for their kernel stack.)
The third stage involves loading the init process. We create PCB, allocate memory for its kernel stack and then copy idle into init (Init takes the kernel context and the contents of the kernel stack for idle.)
Finally, kernel start returns and the CPU switches back to user-mode to run idle process first. The schdeuler (round-robin), handles switching processes at every clock tick.
## System Architecture
The kernel implementes a two-region virtual memory system:
- Region 0 belongs to the kernel and its data structures. It's text, data, BSS, and heap remain the same across all processes. The kernel stack might change, however. It goes from `VMEM_0_BASE = VMEM_BASE = 0x0` to `VMEM_1_LIMIT`
- Region 1 belongs to user-space. Each process has a PT referring to region 1, which holds the process text, data, BSS, heap and stack. Goes from `VMEM_1_BASE = VMEM_0_LIMIT` to `VMEM_1_LIMIT = VMEM_LIMIT`.
```
                                    Memory Layout:
                                    ┌───────────────┐VMEM_1_LIMIT
                                    │   User Stack  │
                                    │   User Heap   │
                                    │   User BSS    │
                                    │   User Data   │
                                    │   User Text   │
                                    └───────────────┘ 
                                    ┌───────────────┐VMEM_1_BASE
                                    │  Kernel Stack │
                                    │  Kernel Heap  │
                                    │  Kernel Data  │
                                    │  Kernel Text  │
                                    └───────────────┘0x00000000
```
### Process Management
The data structure called `PCB` keeps track of all the information relevant to a process. This includes:
- Its current state (ready, blocked, running, zombie,...etc)
- Its region 1 page table.
- Its children processes.
- Region 1 accounting information (stack pointer, heap_brk, ...etc).
- User-context information.
A support of 4 other queues coordinate for scheduling all the processes. The queues are:
- `ready_queue`: Processes on this queue are ready to be dispatched to run on the CPU. Just waiting for their turn for their next CPU burst.
- `blocked_queue`: Processes on this queue are blocked in general (Waiting for a child, waiting for a lock, waiting for terminal ready, write...etc).
- `zombie_queue`: This queue holds the processes that exited and waiting to be reaped.
- `waiting_parents`: A queue of parents who got blocked and waiting for a child to exit (useful for when a process exits, it can check if its parent waiting on it by traversing this queue.)
Scheduling is done in a round-robin fashion where each clock tick, if the ready queue is not empty, the clock trap handler deques a process and context switches to it. If not, it just runs the idle process to keep the CPU busy.
### Synchronization
The kernel also provides support for synchronization primitives (Check `synchronization.h` for more details):
- Locks: Mutual exclusion locks with waiting queues for processes waiting on the lock to be free. It implementes a 'pass the baton' logic to avoid starvation, where when a process calls `Release`, it first checks if there is any process waiting for the lock. If so, it passes the lock along to it. Otherwise, it just sets the locked state to zero. `lock->locked = 0`
- Condition Variables: Supports `Wait`, `Signal`, and `Broadcast`.
    - Wait: Atomically releases the given lock and puts the calling process to sleep on the condition variable. When woken, the process tries to reacquire the lock.
    - Signal: Wakes one process waiting on the condition variable, if any.
    - BroadCast: Wakes all processes currently waiting on the condition variable.
- Pipes: Supports interprocess communication via pipes. Read consumes bytes from the pipe buffer. Writing can happen in chunk if the message to be written is longer than te space available in the pipe.

We also made use of bitmasking to multiplex IDs of the locks, cvars, and the pipes. Check `synchronization.h` for more details.
### I/O terminal subsystem
The kernel also implements a tty subsystem where processes coordinate to write to a terminal. It supports:
- Buffered reads and writes.
- Process blocking when resources unavailable.
- Interrupt-driven I/O.
### Security Features
- Memory Protection: Strict validation of user pointers and buffer addresses to prevent processes from trying to access/edit pages they are not allowed to. Separation of region 1 and 0 for user-space and kernel-space addresses also protects the kernel memory from being tampered with. Pagetables also ensure that processes only have access to their own memory space and only their own memory space.
- Buffer Validation: For example, when the user passes along a buffer address to read from a terminal, we ensure that we are copying data into user-space memory.
# Team
- Isabella Fusari
- Ahmed Al Sunbati

## Docker (if not on thayer machine)
### Build
docker buildx build --platform linux/amd64 .docker -t yalnix

### Run
./.docker/run.sh

checking