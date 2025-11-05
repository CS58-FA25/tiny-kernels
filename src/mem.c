
#include "hardware.h"
#include "mem.h"
#include "kernel.h"

// Defining variables for the free frames list
int nframes;
int free_nframes;
frame_desc_t *frame_table;

// Defining the global region 0 page table array
pte_t pt_region0[MAX_PT_LEN];

int allocFrame(frame_usage_t usage, int owner_pid) {
    for (int i = 0; i < (int)nframes; i++) {
        if (frame_table[i].usage == FRAME_FREE) {
            frame_table[i].usage = usage;
            frame_table[i].owner_pid = -1;
            if (free_nframes > 0) free_nframes--;
            return i;
        }
    }
    return -1;
}

int allocSpecificFrame(int pfn, frame_usage_t usage, int owner_pid) {
    if (pfn >= nframes) {
        TracePrintf(0, "allocSpecificFrame: This frame number is out of index");
        return -1;
    }
    if (frame_table[pfn].usage != FRAME_FREE) {
        TracePrintf(0, "allocSpecificFrame: This frame is already in use.\n");
        return -1;
    }
    frame_table[pfn].usage = usage;
    frame_table[pfn].owner_pid = -1;
    if (free_nframes > 0) free_nframes--;
    
    return pfn;
}

void freeFrame(int pfn) {
    if (!valid_pfn(pfn)) {
        TracePrintf(0, "freeFrame: invalid pfn %d\n", pfn);
        return;
    }
    frame_table[pfn].usage = FRAME_FREE;
    frame_table[pfn].owner_pid = -1;
    free_nframes++;
}

void MapPage(pte_t *ptbr, int vpn, int pfn, int prot) {
    ptbr[vpn].pfn = pfn;
    ptbr[vpn].prot = prot;
    ptbr[vpn].valid = 1;
}

int valid_pfn(int pfn) {
    return (pfn >= 0 && (unsigned)pfn < nframes);
}

int vpn_in_region0(unsigned int vpn) {
    return (vpn < NUM_PAGES_REGION0);
}

void MapRegion0(unsigned int vpn, int pfn) {
    if (!vpn_in_region0(vpn)) {
        TracePrintf(0, "MapRegion0: vpn %u out of range\n", vpn);
        return;
    }

    pt_region0[vpn].valid = 1;
    pt_region0[vpn].prot  = PROT_READ | PROT_WRITE;
    pt_region0[vpn].pfn   = (unsigned int)pfn;

    /* Optionally flush TLB if needed: WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0); */
}

void UnmapRegion0(unsigned int vpn) {
    if (!vpn_in_region0(vpn)) {
        TracePrintf(0, "UnmapRegion0: vpn %u out of range\n", vpn);
        return;
    }

    pt_region0[vpn].valid = 0;
    pt_region0[vpn].prot  = 0;
    pt_region0[vpn].pfn   = 0;
}

void CloneFrame(int pfn_src, int pfn_dst) {
    // Map a scratch page to pfn_dst
    int scratch_page = SCRATCH_ADDR >> PAGESHIFT;
    pt_region0[scratch_page].pfn = pfn_dst;
    pt_region0[scratch_page].prot = PROT_READ | PROT_WRITE;
    pt_region0[scratch_page].valid = 1;
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR); // In case the CPU already has a mapping in the TLB

    // Copy the contents of pfn_src to the frame mapped by the page table to pfn_dst
    unsigned int pfn_src_addr = pfn_src << PAGESHIFT;
    memcpy((void *)(SCRATCH_ADDR), (void *)pfn_src_addr, PAGESIZE);

    // Now that the frame pfn_dst has the contents of the frame pfn_src, unmap it from the scratch address in pt_region0
    pt_region0[scratch_page].valid = 0;
    pt_region0[scratch_page].prot = 0;
    pt_region0[scratch_page].pfn = 0;
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR);
}

int copyPT(PCB *src, PCB *dst) {
    /**
     * For each valid page table entry i in pt_src, allocate a frame for pt_dst
     * Copy the content of the frame pt_src[i].pfn to pt_dst[i].pfn.
     * Set protections to be the same as thos of pt_src[i]
     * 
    */

    pte_t *pt_src = src->ptbr;
    pte_t *pt_dst = dst->ptbr;
    for (int i = 0; i < MAX_PT_LEN; i++) {
        if (pt_src[i].valid == 1) {
            // Allocate a frame to mape the destination page table entry to
            int dst_frame = allocFrame(FRAME_USER, dst->pid);
            if (dst_frame == -1) {
                TracePrintf(0, "CopyPT: Failed to allocate a frame for the destination process PID %d pagetable!\n", dst->pid);
                return ERROR;
            }

            int src_frame = pt_src[i].pfn;
            // Copy the contents of the src frame to the dst frame
            CloneFrame(src_frame, dst_frame);

            // Filling in the pagetable entry
            pt_dst[i].pfn = dst_frame;
            pt_dst[i].valid = 1;
            pt_dst[i].prot = pt_src[i].prot;
        }
   }
   TracePrintf(0, "CopyPT: Succesfully copied pagetable from process PID %d to process PID %d!\n");
   return SUCCESS;
}