#include "trap.h"
#include "clock.h"

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
   TracePrintf(0, "[CLOCK_TRAP] Clock trap triggered. Ticks: 0x%x\n", tick_count);
}

