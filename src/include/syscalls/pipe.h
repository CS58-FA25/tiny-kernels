#ifndef PIPE_H
#define PIPE_H

#define SYSCALLS_TRACE_LEVEL 0

int PipeInit (int * pipe_idp);
int PipeRead(int pipe_id, void *buf, int len);
int PipeWrite(int pipe_id, void *buf, int len);


#endif