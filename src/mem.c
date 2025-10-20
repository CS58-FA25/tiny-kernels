/* =============== Frames Bookeeping =============== */

/* Categorizes each frame as either free, used by kernel or used by user */
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
} frame_desc_t;

extern frame_desc_t *frame_table;   /* allocated during InitMemory */
extern unsigned int nframes;        /* number of frames available */


int allocFrame(frame_desc_t usage) {
    // for f in frame_table:
    //     if f.usage == FREE:
    //         f.usage = usage
    //         f.refcount = 1
    //         return f.pfn
    // Panic("Out of physical frames")
}

void freeFrame(int pfn) {
    // frame_table[pfn].usage = FREE
    // frame_table[pfn].refcount = 0
}



int SetKernelBrk(void *addr)
{
    // if (addr == NULL)
    //     return (int) kernel_brk;

    // // Bounds check: don’t grow past kernel stack base
    // if (addr >= (void *) KERNEL_STACK_BASE)
    //     return ERROR;

    // // Grow the kernel heap upward
    // while (kernel_brk < addr) {
    //     int frame = AllocFrame(FRAME_KERNEL);
    //     if (frame == ERROR)
    //         return ERROR;

    //     MapPage(region0_pt, kernel_brk, frame, PROT_READ | PROT_WRITE);
    //     kernel_brk += PAGESIZE;
    // }

    // // Shrink the kernel heap downward
    // while (kernel_brk > addr) {
    //     kernel_brk -= PAGESIZE;
    //     int frame = GetFrameAtAddr(kernel_brk);
    //     UnmapPage(region0_pt, kernel_brk);
    //     FreeFrame(frame);
    // }

    // return SUCCESS;
}
