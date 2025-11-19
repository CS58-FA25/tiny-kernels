#ifndef MEM_H
#define MEM_H

#include "hardware.h"
#include "ykernel.h"
#include "kernel.h"


// #define SCRATCH_ADDR (KERNEL_STACK_BASE - PAGESIZE)
#define SCRATCH_ADDR_DST (KERNEL_STACK_BASE - PAGESIZE) // e.g., 0xFA000
#define SCRATCH_ADDR_SRC (KERNEL_STACK_BASE - (PAGESIZE*2)) // e.g., 0xF8000

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
extern int nframes;        /* number of frames available */
extern int free_nframes;

// Kernel region 0 page table
extern pte_t pt_region0[MAX_PT_LEN];

/**
 * ======================== Description =======================
 * @brief Iterates through the free frames list, finds the first free frame, marks its usage with usage, and returns
 *        the physical frame number (pfn).
 * ======================== Parameters ========================
 * @param usage (frame_usage_t): The usage we want to set to the frame we are allocating.
 * ======================== returns ==========================
 * @returns pfn (int), the physical frame number we allocated for this usage.
 * 
*/
int allocFrame(frame_usage_t usage, int owner_pid);

/**
 * ======================== Description =======================
 * @brief Tries to allocate a specific frame number.
 * ======================== Parameters ========================
 * @param pfn (int): The physical frame number we want to allocate.
 * @param usage (frame_usage_t): The usage we want to set to the frame we are allocating.
 * ======================== returns ==========================
 * @returns -1 if the frame is not free or not found, pfn if allocation succeeded
 * 
*/
int allocSpecificFrame(int pfn, frame_usage_t usage, int owner_pid);

/**
 * ======================== Description =======================
 * @brief Frees a specific frame
 * ======================== Parameters ========================
 * @param pfn (int): The physical frame number we want to free.
 * ======================== returns ==========================
 * @returns Nothing
 * 
*/
void freeFrame(int pfn);

/**
 * ======================== Description =======================
 * @brief Maps a virtual page number (vpn) to a physical frame number (pfn) in the given page table (ptbr),
 *        sets the protection bits, and marks the page as valid.
 * ======================== Parameters ========================
 * @param ptbr (pte_t *): The base address of the page table where the mapping will be created.
 * @param vpn (int): The virtual page number to map.
 * @param pfn (int): The physical frame number to associate with the virtual page.
 * @param prot (int): The protection bits to apply to the mapping.
 * ======================== returns ==========================
 * @returns Nothing
 * 
*/
void MapPage(pte_t *ptbr, int vpn, int pfn, int prot);


/**
 * ======================== Description =======================
 * @brief Checks whether a given physical frame number (pfn) is valid.
 * ======================== Parameters ========================
 * @param pfn (int): The physical frame number to validate.
 * ======================== Returns ===========================
 * @returns 1 if valid, 0 otherwise.
 * 
*/
int valid_pfn(int pfn);

/**
 * ======================== Description =======================
 * @brief Checks whether a given virtual page number (vpn) lies within region 0.
 * ======================== Parameters ========================
 * @param vpn (unsigned int): The virtual page number to validate.
 * ======================== Returns ===========================
 * @returns 1 if the vpn is within region 0, 0 otherwise.
 * 
*/
int vpn_in_region0(unsigned int vpn);

/**
 * ======================== Description =======================
 * @brief Maps a physical frame (pfn) to a region 0 virtual page (vpn) with read/write protection.
 * ======================== Parameters ========================
 * @param vpn (unsigned int): The virtual page number within region 0.
 * @param pfn (int): The physical frame number to map.
 * ======================== Returns ===========================
 * @returns Nothing.
 * 
*/
void MapRegion0(unsigned int vpn, int pfn);

/**
 * ======================== Description =======================
 * @brief Unmaps a virtual page (vpn) in region 0 by invalidating its PTE entry.
 * ======================== Parameters ========================
 * @param vpn (unsigned int): The virtual page number within region 0 to unmap.
 * ======================== Returns ===========================
 * @returns Nothing.
 * 
*/
void UnmapRegion0(unsigned int vpn);
void CloneFrame(int pfn_src, int pfn_dst);
int CopyPT(PCB *src, PCB *dst);


#endif