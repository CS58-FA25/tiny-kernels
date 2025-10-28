#ifndef MEM_H
#define MEM_H

#include "hardware.h"
#include "ykernel.h"

typedef enum {
    FRAME_FREE = 0,
    FRAME_KERNEL,
    FRAME_USER
} frame_usage_t;

/* Descriptors for each physical frame in memory */
typedef struct frame_desc {
    unsigned int pfn;        /* physical frame number */
    frame_usage_t usage;     /* usage type */
    int owner_pid;           /* owning pid if usage == FRAME_USER (or -1) */
    unsigned int last_used_tick; /* for eviction heuristics if needed */
    int refcount;
} frame_desc_t;

extern frame_desc_t *frame_table;   /* allocated during InitMemory */
extern unsigned int nframes;        /* number of frames available */
extern int free_nframes;

extern pte_t *pt_region0;
extern pte_t *pt_region1;

int allocFrame(frame_usage_t usage);
void freeFrame(int pfn);
int SetKernelBrk(void *addr_ptr);

#endif