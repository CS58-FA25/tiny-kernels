#include <stdlib.h>
#include <stdio.h>
#include "mem.c"
#include <yalnix.h>
#include <hardware.h> 
#include "ykernel.h"
#include "kernel.h"


void initializeVM() {

    TracePrintf(KERNEL_TRACE_LEVEL, "Building initial page tables");

    pt_region0 = malloc(sizeof(pte_t) * MAX_PT_LEN);
    pt_region1 = malloc(sizeof(pte_t) * MAX_PT_LEN);
    memset(pt_region0, 0, sizeof(pte_t) * MAX_PT_LEN); // Makes the whole table clean and invalid
    memset(pt_region1, 0, sizeof(pte_t) * MAX_PT_LEN);

    int stack_limit_pfn = UP_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT;
    int kernel_stack_top = stack_limit_pfn;

    for (int vpn = 0; vpn < kernel_stack_top; vpn++) {
        pt_region0[vpn].valid = 1;
        pt_region0[vpn].prot = PROT_READ | PROT_WRITE;
        pt_region0[vpn].pfn = vpn;
    }

    WriteRegister(REG_PTBR0, (unsigned int) pt_region0);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);
    WriteRegister(REG_PTBR1, (unsigned int) pt_region1);
    WriteRegister(REG_PTLR1, MAX_PT_LEN);
    TracePrintf(KERNEL_TRACE_LEVEL, "VM initialization complete");

}

void initializeFreeFrameList(int pmem_size) {
    TracePrintf(KERNEL_TRACE_LEVEL, "Initializing free frame list");
    nframes = pmem_size / PAGESIZE;
    free_nframes = 0;
    frame_table = (frame_desc_t*)malloc(sizeof(frame_desc_t) * nframes);
    int kernel_brk_pfn = kernel_brk_page;
    int stack_base_pfn= DOWN_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT;
    int stack_limit_pfn = UP_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT;

    for (int i = 0; i < nframes; i++) {
        frame_table[i].pfn = i;
        if (i < kernel_brk_pfn || (i >= stack_base_pfn && i <= stack_limit_pfn)) {
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

    TracePrintf(KERNEL_TRACE_LEVEL, "Free frame list initialized: %d free / %d total (kernel ends at PFN=%u)\n", free_nframes, nframes, kernel_brk_pfn);
}

void InitializeProcTable(void) {
    proc_table_len = MAX_PROCS;
    available_pids = malloc(sizeof(int) * proc_table_len);
    for (int i = 0; i < proc_table_len; i++) {
        available_pids[i] = 1;
    }
}

void InitializeInterruptVectorTable() {
    // interruptVector[TRAP_KERNEL] = &trap_kernel_handler
    // interruptVector[TRAP_CLOCK] = &trap_clock_handler
    // interruptVector[TRAP_ILLEGAL] = &trap_illegal_handler
    // interruptVector[TRAP_MEMORY] = &trap_memory_handler
    // interruptVector[TRAP_MATH] = &trap_math_handler
    // interruptVector[TRAP_TTY_RECEIVE] = &trap_tty_receive_handler
    // interruptVector[TRAP_TTY_TRANSMIT] = &trap_tty_transmit_handler
    // WriteRegister(REG_VECTOR_BASE, &interruptVector)
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