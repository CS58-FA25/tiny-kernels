
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