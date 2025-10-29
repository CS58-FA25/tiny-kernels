#ifndef TRAP_H
#define TRAP_H

#include <hardware.h> // UserContext

#define TRAP_TRACE_LEVEL 2 /* TODO: (TEMPORARY) - If hardware is 1, traps can be at 2 */

typedef void (*TrapHandler)(UserContext*); // Page 23

// clock.c
void ClockTrapHandler(UserContext* ctx);

// math.c
void MathTrapHandler(UserContext* ctx);

// memory.c
void MemoryTrapHandler(UserContext* ctx);
void IllegalInstructionTrapHandler(UserContext* ctx);

// tty.c
void TtyTrapTxHandler(UserContext* ctx);
void TtyTrapRxHandler(UserContext* ctx);

// disk.c 
void DiskTrapHandler(UserContext* ctx);

// nop.c
void NotImplementedTrapHandler(UserContext* ctx);

// kernel.c
void KernelTrapHandler(UserContext* ctx);

#endif // TRAP_H
