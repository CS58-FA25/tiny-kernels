#include <hardware.h>

void TtyTrapTxHandler(UserContext* ctx) {
   int tty = ctx->code;

   // get whether this is a tx/rx operation

   //
   // if it's read, then a complete line of input is ready to be read
   // inform anyone waiting on a line of input from the tty
   //
   // if it's a write, then a complete line of input has been written to the tty
   // inform anyone waiting (such as a write handler) that this operation has succeeded
   //
   // update scheduler on fulfillment of requirements for waiters
}

void TtyTrapRxHandler(UserContext* ctx) {
   int tty = ctx->code;
}

