#ifndef INIT_H
#define INIT_H

#include "hardware.h"
#include "yalnix.h"
#include "ykernel.h"
#include "kernel.h"
#include "mem.h"
#include "traps/trap.h"

/**
 * ======================== Description =======================
 * @brief Initializes the kernel’s virtual memory system.
 * 
 * This function sets up the initial page tables for Region 0 (kernel space),
 * maps kernel text, data, heap, and stack segments, and writes
 * the base and limit registers for the kernel page table.
 * ======================== Parameters ========================
 * @param None
 * ======================== Returns ==========================
 * @returns void
 */
void InitializeVM(void);


/**
 * ======================== Description =======================
 * @brief Enables virtual memory by writing to the VM control register.
 * 
 * After setting up all page tables, this function must be called
 * to activate virtual memory translation in the hardware.
 * ======================== Parameters ========================
 * @param None
 * ======================== Returns ==========================
 * @returns void
 */
void EnableVM(void);


/**
 * ======================== Description =======================
 * @brief Initializes the global frame descriptor table and free frame list.
 * 
 * This function creates and populates the `frame_table`, marking all
 * physical frames according to their initial usage (kernel vs. free).
 * It also tracks the number of free frames available for future allocations.
 * ======================== Parameters ========================
 * @param pmem_size (int): Total size of physical memory in bytes.
 * ======================== Returns ==========================
 * @returns void
 */
void InitializeFreeFrameList(int pmem_size);


/**
 * ======================== Description =======================
 * @brief Initializes the global process table.
 * * Allocates memory for the 'proc_table' array, which holds
 * pointers to all Process Control Blocks (PCBs). All entries
 * in the array are initialized to NULL, signifying that no
 * processes exist yet.
 * ======================== Parameters ========================
 * @param None
 * ======================== Returns ==========================
 * @returns void
 */
void InitializeProcTable(void);


/**
 * ======================== Description =======================
 * @brief Initializes the interrupt vector table.
 * 
 * Registers default trap handlers for kernel and clock interrupts,
 * and sets unimplemented traps to a "not implemented" handler.
 * Writes the trap vector base address into the hardware register.
 * ======================== Parameters ========================
 * @param None
 * ======================== Returns ==========================
 * @returns void
 */
void InitializeInterruptVectorTable(void);


/**
 * ======================== Description =======================
 * @brief Initializes the terminal structures and interrupt handlers.
 * 
 * Sets up terminal buffers, indices, and state fields, and registers
 * TTY receive/transmit handlers in the interrupt vector table.
 * ======================== Parameters ========================
 * @param None
 * ======================== Returns ==========================
 * @returns void
 */
void InitializeTerminals(void);

#endif
