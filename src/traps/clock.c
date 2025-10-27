#include "trap.h"

#include <hardware.h> // UserContext

/**
 * TODO:
 * necessary duration of time has passed
 *
 * update processes, timers of the time change
 * update scheduler of the time change
 * ^
 * |_____ these updates will handle the actions on "time passed"
 *
 * context does not need to be modified for this
 */
void ClockTrapHandler(UserContext* ctx) {
   // Checkpoint 2: Temporary code
   TracePrintf(TRAP_TRACE_LEVEL, "[CLOCK_TRAP] Clock trap triggered\n");
}

