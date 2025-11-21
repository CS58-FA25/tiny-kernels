#include "hardware.h"
#include "yalnix.h"
#include "ykernel.h"
#include "kernel.h"
#include "mem.h"
#include "traps/trap.h"
#include "syscalls/tty.h"


/**
 * ======== InitializeVM =======
 * See init.h for more details
*/
void InitializeVM() {

    TracePrintf(0, "InitializeVM: Building initial page tables\n");
    memset(pt_region0, 0, sizeof(pte_t) * NUM_PAGES_REGION0); // Makes the whole table clean and invalid

    int stack_limit_pfn = UP_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT;
    int kernel_stack_top = stack_limit_pfn;

    // Mapping the kernel text, data, heap segment. The frames for these are already allocated
    for (int vpn = _first_kernel_text_page; vpn < _first_kernel_data_page; vpn++) {
        pt_region0[vpn].valid = 1;
        pt_region0[vpn].prot = PROT_READ | PROT_EXEC;
        pt_region0[vpn].pfn = vpn;
    }

    for (int vpn = _first_kernel_data_page; vpn < _orig_kernel_brk_page; vpn++) {
        pt_region0[vpn].valid = 1;
        pt_region0[vpn].prot = PROT_READ | PROT_WRITE;
        pt_region0[vpn].pfn = vpn;
    }

    // Mapping the kernel stack in region0. The frames for these are not allocated yet.
    // They are allocated first time when we allocate kernel stack for the first process (idle)
    for (int vpn = KSTACK_START_PAGE; vpn < KSTACK_START_PAGE + KSTACK_PAGES; vpn++) {
        pt_region0[vpn].valid = 1;
        pt_region0[vpn].prot = PROT_READ | PROT_WRITE;
        pt_region0[vpn].pfn = vpn;
    }

    TracePrintf(0, "InitializeVM: VMinitialization complete\n");
}

/**
 * ======== EnableVM =======
 * See init.h for more details
*/
void EnableVM() {
    WriteRegister(REG_PTBR0, (unsigned int) pt_region0);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);
    WriteRegister(REG_PTBR1, (unsigned int) current_process->ptbr);
    WriteRegister(REG_PTLR1, current_process->ptlr);
    WriteRegister(REG_VM_ENABLE, 1);
}

/**
 * ======== InitializeFreeFrameList =======
 * See init.h for more details
*/
void InitializeFreeFrameList(int pmem_size) {
    TracePrintf(0, "Initializing free frame list");
    nframes = pmem_size / PAGESIZE;
    free_nframes = 0;
    frame_table = (frame_desc_t*)malloc(sizeof(frame_desc_t) * nframes);
    int kernel_brk_pfn = kernel_brk_page;
    int stack_base_pfn= DOWN_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT;
    int stack_limit_pfn = UP_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT;

    for (int i = 0; i < nframes; i++) {
        frame_table[i].pfn = i;
        if (i >= text_section_base_page && i < kernel_brk_pfn) {
            frame_table[i].usage = FRAME_KERNEL;
            frame_table[i].owner_pid = IDLE_PID;
        } else {
            frame_table[i].usage = FRAME_FREE;
            frame_table[i].owner_pid = -1;
            free_nframes += 1;
        }
    }

    TracePrintf(0, "Free frame list initialized: %d free / %d total (kernel ends at PFN=%u)\n", free_nframes, nframes, kernel_brk_pfn);
}

/**
 * ======== InitializeProcTable =======
 * See init.h for more details
*/
void InitializeProcTable(void) {
    // Just allocate the array of pointers
    proc_table = malloc(sizeof(PCB*) * MAX_PROCS);
    
    // And set them all to NULL
    memset(proc_table, 0, sizeof(PCB*) * MAX_PROCS);
}

/**
 * ======== InitializeInterruptVectorTable =======
 * See init.h for more details
*/
void InitializeInterruptVectorTable(void) {
    TRAP_VECTOR[TRAP_KERNEL] = &KernelTrapHandler;
    TRAP_VECTOR[TRAP_CLOCK] = &ClockTrapHandler;
    TRAP_VECTOR[TRAP_MEMORY] = &MemoryTrapHandler;
    TRAP_VECTOR[TRAP_TTY_RECEIVE] = &TtyTrapReceiveHandler;
    TRAP_VECTOR[TRAP_TTY_TRANSMIT] = &TtyTrapTransmitHandler;
    TRAP_VECTOR[TRAP_MATH] = &MathTrapHandler;
    TRAP_VECTOR[TRAP_ILLEGAL] = &IllegalInstructionTrapHandler;
    WriteRegister(REG_VECTOR_BASE, (unsigned int) TRAP_VECTOR);
}

/**
 * ======== InitializeTerminals =======
 * See init.h for more details
*/
void InitializeTerminals(void) {
    TracePrintf(0, "Kernel: Initializing terminals....\n");
    for (int i = 0; i < NUM_TERMINALS; i++) {
        terminals[i].tty_id = i;
        terminals[i].blocked_readers = queue_create();
        terminals[i].blocked_writers = queue_create();
        if (terminals[i].blocked_readers == NULL || terminals[i].blocked_writers == NULL) {
            TracePrintf(0, "Kernel: Failed to allocate memory for one of the queues in the %dth terminal.\n", i);
            Halt();
        }

        terminals[i].read_buffer = malloc(TERMINAL_MAX_LINE);
        if (terminals[i].read_buffer == NULL) {
            TracePrintf(0, "Kernel: Failed to allocate memory for read buffer for the %dth terminal.\n", i);
            Halt();
        }

        terminals[i].read_buffer_len = 0;
        terminals[i].write_buffer = NULL;
        terminals[i].write_buffer_position = 0;
        terminals[i].write_buffer_len = 0;
        terminals[i].current_writer = NULL;
        terminals[i].in_use = 0;
        TracePrintf(0, "Kernel: Succesfully initialized the %dth terminal.\n", i);
    }
    TracePrintf(0, "Kernel: Sucessfully initialized all terminals!\n");
}

/**
 * ======== InitializeProcQueues =======
 * See init.h for more details
*/
void InitializeProcQueues(void) {
    ready_queue = queue_create();
    if (ready_queue == NULL) {
        TracePrintf(0, "ready_queue: Couldn't allocate memory for ready queue.\n");
        Halt();
    }

    blocked_queue = queue_create();
    if (blocked_queue == NULL) {
        TracePrintf(0, "blocked_queue: Couldn't allocate memory for blocked queue.\n");
        Halt();
    }

    zombie_queue = queue_create();
    if (zombie_queue == NULL) {
        TracePrintf(0, "zombie_queue: Couldn't allocate memory for zombie queue.\n");
        Halt();
    }

    waiting_parents = queue_create();
    if (waiting_parents == NULL) {
        TracePrintf(0, "waiting_parents: Couldn't allocate memory for waiting parents queue.\n");
        Halt();
    }
}