#include "trap.h"

void ClockTrapHandler(UserContext* ctx) {
   // necessary duration of time has passed
   //
   // update processes, timers of the time change
   // update scheduler of the time change
   // ^
   // |_____ these updates will handle the actions on "time passed"
   // 
   // context does not need to be modified for this
}

