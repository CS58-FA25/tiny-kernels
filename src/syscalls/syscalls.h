#ifndef SYSCALLS_H
#define SYSCALLS_H

#define SYSCALLS_TRACE_LEVEL    0

// brk.c
int SysBrk(void *addr);


// pipe.c
int PipeInit (int * pipe_idp);
int PipeRead(int pipe_id, void *buf, int len);
int PipeWrite(int pipe_id, void *buf, int len);

// lock.c
int LockInit (int * lock_idp);
int Acquire (int lock_id);
int Release (int lock_id);
int CvarInit (int * cvar_idp);
int CvarWait (int cvar_id, int lock_id);
int CvarSignal (int cvar_id);
int CvarBroadcast (int cvar_id);
int Reclaim (int id);

// tty.c
int TtyRead(int tty_id, void *buf, int len);
int TtyWrite(int tty_id, void *buf, int len);

// process.c
int Fork (void);
int Exec (char * file, char ** argvec);
void Exit (int status);
int Wait (int * status_ptr);
int SysGetPid (void);
int SysDelay(int clock_ticks);

#endif
