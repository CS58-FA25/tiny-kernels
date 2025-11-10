#include <hardware.h>

void MathTrapHandler(UserContext* ctx) {
   // if it's possible to get information outside of offending address
   // out of the context, then use this here to inform loggers/kernel/listeners/etc.
   // of the exception
   //
   // kill the offending process
   // notify scheduler that it was killed, do not switch back to it
   // instead, scheduler should handle necessary cleanup (outside of initial cleanup handled here)
}

