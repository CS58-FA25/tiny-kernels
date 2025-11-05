#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#define SYSCALLS_TRACE_LEVEL 0


int LockInit (int * lock_idp);
int Acquire (int lock_id);
int Release (int lock_id);
int CvarInit (int * cvar_idp);
int CvarWait (int cvar_id, int lock_id);
int CvarSignal (int cvar_id);
int CvarBroadcast (int cvar_id);
int Reclaim (int id);


#endif