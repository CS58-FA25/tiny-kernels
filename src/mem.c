
#include "hardware.h"
#include "mem.h"
#include "kernel.h"
/* =============== Frames Bookeeping =============== */
#define KERNEL_TRACE_LEVEL   3
/* Categorizes each frame as either free, used by kernel or used by user */


static inline int valid_pfn(int pfn) {
    return (pfn >= 0 && (unsigned)pfn < nframes);
}
static inline int vpn_in_region0(unsigned int vpn) {
    return (vpn < (VMEM_REGION_SIZE / PAGESIZE));
}

/* --- map/unmap helpers for region0 (vpn is region0 virtual page number) --- */
static inline void MapRegion0VPN(unsigned int vpn, int pfn) {
    if (!vpn_in_region0(vpn)) {
        TracePrintf(KERNEL_TRACE_LEVEL, "MapRegion0VPN: vpn %u out of range\n", vpn);
        return;
    }
    pt_region0[vpn].valid = 1;
    pt_region0[vpn].prot  = PROT_READ | PROT_WRITE;
    pt_region0[vpn].pfn   = (unsigned int)pfn;
    /* If TLB invalidation required: WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0); */
}

static inline void UnmapRegion0VPN(unsigned int vpn) {
    if (!vpn_in_region0(vpn)) {
        TracePrintf(KERNEL_TRACE_LEVEL, "UnmapRegion0VPN: vpn %u out of range\n", vpn);
        return;
    }
    pt_region0[vpn].valid = 0;
    pt_region0[vpn].prot  = 0;
    pt_region0[vpn].pfn   = 0;
}

int allocFrame(frame_usage_t usage) {
    for (int i = 0; i < (int)nframes; i++) {
        if (frame_table[i].usage == FRAME_FREE) {
            frame_table[i].usage = usage;
            frame_table[i].refcount = 1;
            frame_table[i].owner_pid = -1;
            if (free_nframes > 0) free_nframes--;
            return i; /* pfn */
        }
    }
    return -1;
}

void freeFrame(int pfn) {
    if (!valid_pfn(pfn)) {
        TracePrintf(KERNEL_TRACE_LEVEL, "freeFrame: invalid pfn %d\n", pfn);
        return;
    }

    if (frame_table[pfn].refcount > 1) {
        frame_table[pfn].refcount--;
        return;
    }
    /* refcount == 1 -> free it */
    frame_table[pfn].refcount = 0;
    frame_table[pfn].usage = FRAME_FREE;
    frame_table[pfn].owner_pid = -1;
    free_nframes++;
}


int SetKernelBrk(void *addr_ptr)
{
    if (addr_ptr == NULL) {
        TracePrintf(KERNEL_TRACE_LEVEL, "SetKernelBrk: NULL addr\n");
        return -1;
    }

    /* convert address to target VPN */
    unsigned int target_vaddr = DOWN_TO_PAGE((unsigned int) addr_ptr);
    unsigned int target_vpn  = target_vaddr >> PAGESHIFT;

    /* bounds: cannot grow into kernel stack (stack base is virtual address) */
    unsigned int stack_base_vpn = DOWN_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT;
    if (target_vpn >= stack_base_vpn) {
        TracePrintf(KERNEL_TRACE_LEVEL, "SetKernelBrk: target collides with kernel stack\n");
        return -1;
    }

    /* current kernel_brk is a page number (vpn) */
    unsigned int cur_vpn = (unsigned int) kernel_brk_page; /* assume kernel_brk is already VPN */

    /* Grow heap: map pages for VPNs [cur_vpn .. target_vpn-1] */
    while (cur_vpn < target_vpn) {
        int pfn = allocFrame(FRAME_KERNEL);

        if (pfn == -1) {
            TracePrintf(KERNEL_TRACE_LEVEL, "SetKernelBrk: out of physical frames\n");
            return -1;
        }

        /* Map physical pfn into region0 VPN = cur_vpn */
        MapRegion0VPN(cur_vpn, pfn);
        cur_vpn++;
    }

    /* Shrink heap: unmap pages for VPNs [target_vpn .. cur_vpn-1] */
    while (cur_vpn > target_vpn) {
        cur_vpn--; /* handle the page at vpn = cur_vpn - 1 */

        if (!vpn_in_region0(cur_vpn)) {
            TracePrintf(KERNEL_TRACE_LEVEL, "SetKernelBrk: shrink vpn out of range %u\n", cur_vpn);
            return -1;
        }

        if (!pt_region0[cur_vpn].valid) {
            TracePrintf(KERNEL_TRACE_LEVEL, "SetKernelBrk: unmapping already-unmapped vpn %u\n", cur_vpn);
            continue;
        }

        int pfn = (int) pt_region0[cur_vpn].pfn;

        /* unmap */
        UnmapRegion0VPN(cur_vpn);

        /* free the physical frame */
        freeFrame(pfn);
    }

    /* update kernel_brk (store vpn) */
    kernel_brk_page = (int) target_vpn;
    return 0;
}

void MapPage(pte_t *ptbr, int vpn, int pfn, int prot) {
    ptbr[vpn].pfn = pfn;
    ptbr[vpn].prot = prot;
    ptbr[vpn].valid = 1;  // <-- This is the missing, essential line
}
