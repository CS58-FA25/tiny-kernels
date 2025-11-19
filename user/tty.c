#include "terminal.h"
#include "syscalls/syscall.h"
#include <yuser.h>
#include <yalnix.h>



int main(int argc, char **argv){
   TracePrintf(0, "hi\n");
   TracePrintf(0, "argc: %p, argv: %p\n", argc, argv);
   char* arg = argv[0];
   TracePrintf(0, "arg: %s\n", arg);
   int tty = arg[0] - 0x30;
   TracePrintf(0, "Starting on TTY/%d\n", tty);
   TtyWrite(tty, "Hello from TTY/%d\n", tty);
   char buf[TERMINAL_MAX_LINE];
   while (1) {
      TtyWrite(tty, ">>> ", 7);

      int rval = TtyRead(tty, buf, TERMINAL_MAX_LINE - 1);
      TracePrintf(0, "Read returned %d\n", n);
      if (rval <= 0) {
          continue;
      }

      TracePrintf(0, "Echo terminal input: %s\n", buf);
   }

   return 0;
} 
  
		
  
  
  


