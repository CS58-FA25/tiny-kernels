void MemoryTrapHandler(UserContext* ctx) {
   // get the ctx->code to determine what type of memory exception this is
   // based on the exception, report this information to whoever needs to be aware (loggers, kernel)
   //
   // kill the process and inform the scheduler of this change
   // ---> scheduler should be aware of upcoming deletion (post-completion)
}

void IllegalInstructionTrapHandler(UserContext* ctx) {
   // get the offending address, current pc
   // report this information
   //
   // kill the process and inform the scheduler of this change
   // ---> scheduler should be aware of upcoming deletion (post-completion)
}
