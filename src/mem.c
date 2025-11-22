
#include "hardware.h"
#include "mem.h"
#include "kernel.h"

/* ======== Global Variables ======= */
int nframes;
int free_nframes;
frame_desc_t *frame_table;
pte_t pt_region0[MAX_PT_LEN];

/* ========== Forward Declarations of helper functions ========= */
int valid_pfn(int pfn);
int vpn_in_region0(unsigned int vpn);
/* ========== End of Forward Declarations of helper functions ========= */


/**
 * ======== AllocFrame =======
 * See mem.h for more details
*/
int AllocFrame(frame_usage_t usage, int owner_pid) {
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


/**
 * ======== AllocSpecificFrame =======
 * See mem.h for more details
*/
int AllocSpecificFrame(int pfn, frame_usage_t usage, int owner_pid) {
    if (pfn >= nframes) {
        TracePrintf(0, "AllocSpecificFrame: This frame number is out of index");
        return -1;
    }
    if (frame_table[pfn].usage != FRAME_FREE) {
        TracePrintf(0, "AllocSpecificFrame: This frame is already in use.\n");
        return -1;
    }
    frame_table[pfn].usage = usage;
    frame_table[pfn].owner_pid = -1;
    if (free_nframes > 0) free_nframes--;
    
    return pfn;
}


/**
 * ======== FreeFrame =======
 * See mem.h for more details
*/
void FreeFrame(int pfn) {
    if (!valid_pfn(pfn)) {
        TracePrintf(0, "FreeFrame: invalid pfn %d\n", pfn);
        return;
    }
    frame_table[pfn].usage = FRAME_FREE;
    frame_table[pfn].owner_pid = -1;
    free_nframes++;
}

/**
 * ======== CopyPT =======
 * See mem.h for more details
*/
int CopyPT(PCB *src, PCB *dst) {
    pte_t *pt_src = src->ptbr;
    pte_t *pt_dst = dst->ptbr;
    for (int i = 0; i < MAX_PT_LEN; i++) {
        if (pt_src[i].valid == 1) {
            // Allocate a frame to mape the destination page table entry to
            int dst_frame = AllocFrame(FRAME_USER, dst->pid);
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
   TracePrintf(0, "CopyPT: Succesfully copied pagetable from process PID %d to process PID %d!\n", src->pid, dst->pid);
   return SUCCESS;
}

/**
 * ======== CloneFrame =======
 * See mem.h for more details
*/
void CloneFrame(int pfn_src, int pfn_dst) {
    // Get VPNs for both scratch pages
    int scratch_vpn_dst = SCRATCH_ADDR_DST >> PAGESHIFT;
    int scratch_vpn_src = SCRATCH_ADDR_SRC >> PAGESHIFT;

    // Map the source frame to the source scratch page
    pt_region0[scratch_vpn_src].pfn = pfn_src;
    pt_region0[scratch_vpn_src].prot = PROT_READ; // Read-only
    pt_region0[scratch_vpn_src].valid = 1;

    // Map the destination frame to the destination scratch page
    pt_region0[scratch_vpn_dst].pfn = pfn_dst;
    pt_region0[scratch_vpn_dst].prot = PROT_READ | PROT_WRITE;
    pt_region0[scratch_vpn_dst].valid = 1;

    // Flush the TLB for both virtual addresses
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR_SRC);
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR_DST);

    // Now, memcpy between the two valid virtual addresses
    memcpy((void *)SCRATCH_ADDR_DST, (void *)SCRATCH_ADDR_SRC, PAGESIZE);

    // Unmap both scratch pages
    pt_region0[scratch_vpn_src].valid = 0;
    pt_region0[scratch_vpn_dst].valid = 0;

    // Flush the TLB again
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR_SRC);
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR_DST);
}


/**
 * ======== MapPage =======
 * See mem.h for more details
*/
void MapPage(pte_t *ptbr, int vpn, int pfn, int prot) {
    ptbr[vpn].pfn = pfn;
    ptbr[vpn].prot = prot;
    ptbr[vpn].valid = 1;
}


/**
 * ======== MapRegion0 =======
 * See mem.h for more details
*/
void MapRegion0(unsigned int vpn, int pfn) {
    if (!vpn_in_region0(vpn)) {
        TracePrintf(0, "MapRegion0: vpn %u out of range\n", vpn);
        return;
    }
    pt_region0[vpn].valid = 1;
    pt_region0[vpn].prot  = PROT_READ | PROT_WRITE;
    pt_region0[vpn].pfn   = (unsigned int)pfn;

    WriteRegister(REG_TLB_FLUSH, (unsigned int)(vpn << PAGESHIFT));
}

/**
 * ======== UnmapRegion0 =======
 * See mem.h for more details
*/
void UnmapRegion0(unsigned int vpn) {
    if (!vpn_in_region0(vpn)) {
        TracePrintf(0, "UnmapRegion0: vpn %u out of range\n", vpn);
        return;
    }
    pt_region0[vpn].valid = 0;
    pt_region0[vpn].prot  = 0;
    pt_region0[vpn].pfn   = 0;
    WriteRegister(TLB_FLUSH_0, (unsigned int)(vpn << PAGESHIFT));
}

/* ============= Helper Functions ============== */

/**
 * Description : Checks whether a given physical frame number (pfn) is valid.
 * ======= Parameters =========
 * @param pfn: The physical frame number to validate.
 * ======= Returns ==========
 * @returns 1 if valid, 0 otherwise.
 * 
*/
int valid_pfn(int pfn) {
    return (pfn >= 0 && (unsigned)pfn < nframes);
}


/**
 * Description: Checks whether a given virtual page number (vpn) lies within region 0.
 * ======= Parameters =======
 * @param vpn: The virtual page number to validate.
 * ======= Returns =========
 * @returns 1 if the vpn is within region 0, 0 otherwise.
 * 
*/
int vpn_in_region0(unsigned int vpn) {
    return (vpn < NUM_PAGES_REGION0);
}