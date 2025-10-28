#include "traps/trap.h" // TrapHandler, NotImplementedTrapHandler, ClockTrapHandler, ...
#include "ykernel.h"

#include <hardware.h> // yalnix: hardware.h
#include <stdlib.h>
#include <stdio.h>
#include <yalnix.h>
#include <hardware.h> 
#include "ykernel.h"
#include "kernel.h"
#include "mem.h"

// Create trap handlers, set them all to not implemented.
// Since this is still being developed and all of them might not be implemented,
// it's easier to "set them as we go"
static TrapHandler TRAP_VECTOR[TRAP_VECTOR_SIZE] = {
   // Kernel, Clock, Illegal, Memory (0-3)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,

   // Math, Tty Rx, Tty Tx, Disk (4-7)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler, 

   // Unused (8-11)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,

   // EXTRA (12-15)
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
   NotImplementedTrapHandler,
};

void initializeVM() {

    TracePrintf(0, "Building initial page tables");

    pt_region0 = malloc(sizeof(pte_t) * MAX_PT_LEN);
    memset(pt_region0, 0, sizeof(pte_t) * MAX_PT_LEN); // Makes the whole table clean and invalid

    int stack_limit_pfn = UP_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT;
    int kernel_stack_top = stack_limit_pfn;

    for (int vpn = 0; vpn < kernel_stack_top; vpn++) {
        if ((vpn >= text_section_base_page && vpn < kernel_brk_page) || (vpn >= KERNEL_STACK_BASE && vpn < stack_limit_pfn)) {
            pt_region0[vpn].valid = 1;
            pt_region0[vpn].prot = PROT_READ | PROT_WRITE;
            pt_region0[vpn].pfn = vpn;
        }
    }

    WriteRegister(REG_PTBR0, (unsigned int) pt_region0);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);
    TracePrintf(0, "VM initialization complete");

}

void initializeFreeFrameList(int pmem_size) {
    TracePrintf(0, "Initializing free frame list");
    nframes = pmem_size / PAGESIZE;
    free_nframes = 0;
    frame_table = (frame_desc_t*)malloc(sizeof(frame_desc_t) * nframes);
    int kernel_brk_pfn = kernel_brk_page;
    int stack_base_pfn= DOWN_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT;
    int stack_limit_pfn = UP_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT;

    for (int i = 0; i < nframes; i++) {
        frame_table[i].pfn = i;
        if (i >= text_section_base_page && i < kernel_brk_pfn || (i >= stack_base_pfn && i < stack_limit_pfn)) {
            frame_table[i].usage = FRAME_KERNEL;
            frame_table[i].owner_pid = IDLE_PID;
            frame_table[i].refcount = 1;
        } else {
            frame_table[i].usage = FRAME_FREE;
            frame_table[i].owner_pid = -1;
            frame_table[i].refcount = 0;
            free_nframes += 1;
        }
    }

    TracePrintf(0, "Free frame list initialized: %d free / %d total (kernel ends at PFN=%u)\n", free_nframes, nframes, kernel_brk_pfn);
}

void InitializeProcTable(void) {
    proc_table_len = MAX_PROCS;
    available_pids = malloc(sizeof(int) * proc_table_len);
    for (int i = 0; i < proc_table_len; i++) {
        available_pids[i] = 1;
    }
}

void InitializeInterruptVectorTable() {
    TRAP_VECTOR[TRAP_KERNEL] = &KernelTrapHandler;
    TRAP_VECTOR[TRAP_CLOCK] = &ClockTrapHandler;
    // TODO:
    // These are currently unimplemented (Checkpoint 2)
    // Add these in as they are implemented
    // TRAP_ILLEGAL,TRAP_MEMORY,TRAP_MATH,TRAP_TTY_RECEIVE,TRAP_TTY_TRANSMIT
    WriteRegister(REG_VECTOR_BASE, &TRAP_VECTOR);
}

void InitializeTerminals() {
    // TracePrintf(KERNEL_TRACE_LEVEL, "Initializing terminals...\n");

    // for (int i = 0; i < NUM_TERMINALS; i++) {
    //     terminals[i].id = i;
    //     terminals[i].input_head = terminals[i].input_tail = 0;
    //     terminals[i].output_head = terminals[i].output_tail = 0;
    //     terminals[i].transmitting = 0;
    //     terminals[i].waiting_read_proc = NULL;
    //     terminals[i].waiting_write_proc = NULL;

    //     memset(terminals[i].input_buffer, 0, TERMINAL_BUFFER_SIZE);
    //     memset(terminals[i].output_buffer, 0, TERMINAL_BUFFER_SIZE);
    // }

    // // Register the TTY interrupt handlers in the vector table
    // vector_table[TRAP_TTY_RECEIVE]  = TTYReceiveHandler;
    // vector_table[TRAP_TTY_TRANSMIT] = TTYTransmitHandler;

    // WriteRegister(REG_VECTOR_BASE, (unsigned int) vector_table);

    // TracePrintf(KERNEL_TRACE_LEVEL, "Terminal initialization complete.\n");
}
