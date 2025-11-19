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
   TracePrintf(0, "tty %d\n", tty);
   TtyWrite(tty, "Hi from tty %d\n", tty);
   char buf[TERMINAL_MAX_LINE];
   while (1) {
      TtyWrite(tty, "input:\t", 7);

      int n = TtyRead(tty, buf, TERMINAL_MAX_LINE - 1);
      TracePrintf(0, "Read rval %d\n", n);
      if (n <= 0) {
          continue;
      }

      TtyWrite(tty, "echo: %s\n", buf);
   }

   return 0;
} 
  
		
  
  
  


