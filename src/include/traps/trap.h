#ifndef TRAP_H
#define TRAP_H

#include <hardware.h> // UserContext
#include <stdint.h>

#define TRAP_TRACE_LEVEL 2 /* TODO: (TEMPORARY) - If hardware is 1, traps can be at 2 */

// From libhardware.so
extern uint32_t tick_count;

typedef void (*TrapHandler)(UserContext*); // Page 23

/**
 * Description:
 *    Handles the clock interrupt generated on every tick. Updates the currently
 *    running process's UserContext, updates delay-blocked processes, and performs
 *    round-robin scheduling to switch to the next ready process.
 *
 * ======= Parameters ======
 * @param ctx: Pointer to the UserContext of the process interrupted by the clock tick.
 *
 * ====== Return =======
 * @return void
 */
void ClockTrapHandler(UserContext* ctx);

/**
 * Description:
 *    Handles all system calls triggered from user mode. Dispatches execution to the
 *    corresponding syscall implementation based on ctx->code. For each syscall, the
 *    handler:
 *       - Saves the UserContext of the calling process
 *       - Executes the requested system call (Delay, Brk, Fork, Exec, etc.)
 *       - Restores the updated UserContext
 *       - Places the system call return value in ctx->regs[0]
 *
 * ======= Parameters ======
 * @param ctx: Pointer to the UserContext of the process making the system call.
 *
 * ====== Return =======
 * @return void
 */
void KernelTrapHandler(UserContext* ctx);

/**
 * Description:
 *    Handles mathematical exceptions such as divide-by-zero or invalid arithmetic
 *    operations caused by the user program. The kernel immediately terminates the
 *    offending process with an ERROR status.
 *
 * ======= Parameters ======
 * @param ctx: Pointer to the UserContext at the time of the math exception.
 *
 * ====== Return =======
 * @return void
 */
void MathTrapHandler(UserContext* ctx);

/**
 * Description:
 *    Handles memory access faults such as page faults, invalid addresses, or
 *    access-permission errors. The handler:
 *       - Detects if the fault is due to legitimate stack growth and allocates
 *         additional pages as needed.
 *       - Otherwise terminates the process for illegal memory access.
 *
 *    The handler supports:
 *       - YALNIX_MAPERR: missing page or unmapped region
 *       - YALNIX_ACCERR: access permission violation
 *
 * ======= Parameters ======
 * @param ctx: Pointer to the UserContext containing fault address and fault type.
 *
 * ====== Return =======
 * @return void
 */
void MemoryTrapHandler(UserContext* ctx);

/**
 * Description:
 *    Handles illegal or undefined CPU instructions executed by a user program.
 *    The kernel immediately terminates the offending process with an ERROR code.
 *
 * ======= Parameters ======
 * @param ctx: Pointer to the UserContext at the time the illegal instruction occurred.
 *
 * ====== Return =======
 * @return void
 */
void IllegalInstructionTrapHandler(UserContext* ctx);

/**
 * Description:
 *    Handles terminal transmit interrupts. This trap fires when a terminal finishes
 *    writing previously sent characters. The handler:
 *       - Sends the next chunk of data to the terminal if more bytes remain.
 *       - Otherwise finalizes the write operation, frees the kernel write buffer,
 *         wakes the writing process, and unblocks the next writer waiting on the terminal.
 *
 * ======= Parameters ======
 * @param ctx: UserContext containing the terminal ID in ctx->code.
 *
 * ====== Return =======
 * @return void
 */
void TtyTrapTransmitHandler(UserContext* ctx);

/**
 * Description:
 *    Handles terminal receive interrupts when new input is available. The handler:
 *       - Reads newly received characters into the terminal's kernel buffer.
 *       - Wakes any blocked readers that are waiting for available input.
 *       - Transfers data to each waking process and updates their return values.
 *
 * ======= Parameters ======
 * @param ctx: UserContext containing the terminal ID in ctx->code.
 *
 * ====== Return =======
 * @return void
 */
void TtyTrapReceiveHandler(UserContext* ctx);


/* ========= Unimplemented trap handlers =========== */
void DiskTrapHandler(UserContext* ctx);
void NotImplementedTrapHandler(UserContext* ctx);


#endif // TRAP_H
