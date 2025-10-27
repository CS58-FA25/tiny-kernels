#include "traps.h" // TrapHandler, NotImplementedTrapHandler, ClockTrapHandler, ...

#include <hardware.h> // yalnix: hardware.h

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
    // TracePrintf(KERNEL_TRACE_LEVEL, "Building initial page tables")

    // // Allocate and zero region 0 page table
    // pt_region0 = AllocPhysicalPage()
    // memset(pt_region0, 0, PAGESIZE)

    // // Map kernel text, data, and stack into region 0
    // for vaddr in [KERNEL_BASE .. KERNEL_STACK_LIMIT):
    //     pfn = vaddr >> PAGESHIFT
    //     pt_region0[vaddr >> PAGESHIFT].valid = 1
    //     pt_region0[vaddr >> PAGESHIFT].prot = PROT_READ | PROT_WRITE
    //     pt_region0[vaddr >> PAGESHIFT].pfn = pfn

    // // Create a blank region 1 page table
    // pt_region1 = AllocPhysicalPage()
    // memset(pt_region1, 0, PAGESIZE)

    // // Write base and limit registers for both regions
    // WriteRegister(REG_PTBR0, (unsigned int) pt_region0)
    // WriteRegister(REG_PTLR0, MAX_PT_LEN)
    // WriteRegister(REG_PTBR1, (unsigned int) pt_region1)
    // WriteRegister(REG_PTLR1, MAX_PT_LEN)

    // TracePrintf(KERNEL_TRACE_LEVEL, "VM initialization complete")
}

void initializeFreeFrameList() {
    // TracePrintf(KERNEL_TRACE_LEVEL, "Initializing free frame list")

    // num_frames = pmem_size / PAGESIZE

    // for i in 0..num_frames-1:
    //     frame_table[i].pfn = i
    //     if i < KERNEL_RESERVED_FRAMES:
    //         frame_table[i].usage = FRAME_KERNEL
    //         frame_table[i].owner_pid = IDLE_PID
    //         frame_table[i].refcount = 1
    //     else:
    //         frame_table[i].usage = FRAME_FREE
    //         frame_table[i].owner_pid = -1
    //         frame_table[i].refcount = 0

    // free_frame_count = num_frames - KERNEL_RESERVED_FRAMES
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
