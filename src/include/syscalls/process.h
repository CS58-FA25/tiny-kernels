#ifndef PROCESS_H
#define PROCESS_H

#define SYSCALLS_TRACE_LEVEL    0

int Fork (void);
int Exec (char * file, char ** argvec);
void Exit (int status);
int Wait (int * status_ptr);
int GetPid (void);
int Delay(int clock_ticks);
int Brk(void *addr);


#endif