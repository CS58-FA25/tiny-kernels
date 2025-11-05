#ifndef TTY_H
#define TTY_H

#define SYSCALLS_TRACE_LEVEL 0

int TtyRead(int tty_id, void *buf, int len);
int TtyWrite(int tty_id, void *buf, int len);

#endif