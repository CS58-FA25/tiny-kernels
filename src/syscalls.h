#ifndef SYSCALLS_H
#define SYSCALLS_H

#define SYSCALLS_TRACE_LEVEL    0

int GetPid(void);
int Brk(void *addr);
int Delay(int clockTicks);

#endif
