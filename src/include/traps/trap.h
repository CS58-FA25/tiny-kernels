#ifndef TRAP_H
#define TRAP_H

#include <hardware.h> // UserContext
#include <stdint.h>

#define TRAP_TRACE_LEVEL 2 /* TODO: (TEMPORARY) - If hardware is 1, traps can be at 2 */

// From libhardware.so
extern uint32_t tick_count;

typedef void (*TrapHandler)(UserContext*); // Page 23
void ClockTrapHandler(UserContext* ctx);
void MathTrapHandler(UserContext* ctx);
void MemoryTrapHandler(UserContext* ctx);
void IllegalInstructionTrapHandler(UserContext* ctx);
void TtyTrapTransmitHandler(UserContext* ctx);
void TtyTrapReceiveHandler(UserContext* ctx);
void DiskTrapHandler(UserContext* ctx);
void NotImplementedTrapHandler(UserContext* ctx);
void KernelTrapHandler(UserContext* ctx);

#endif // TRAP_H
