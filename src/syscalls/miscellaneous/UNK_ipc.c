// Marking this file as unknown (UNK)
// -----
// There is no guidance for this in the Yalnix documentation that we have
// However after some research it looks like these are IPC related functions

int Register (unsigned int);
int Send (void *, int);
int Receive (void *);
int ReceiveSpecific (void *, int);
int Reply (void *, int);
int Forward (void *, int, int);

