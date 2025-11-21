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
int AllocFrame(frame_usage_t usage, int owner_pid);

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
int AllocSpecificFrame(int pfn, frame_usage_t usage, int owner_pid);

/**
 * ======================== Description =======================
 * @brief Frees a specific frame
 * ======================== Parameters ========================
 * @param pfn (int): The physical frame number we want to free.
 * ======================== returns ==========================
 * @returns Nothing
 * 
*/
void FreeFrame(int pfn);

/**
 * Description:
 *    CloneFrame copies the contents of one physical frame into another by temporarily
 *    mapping both frames into two scratch virtual addresses inside Region 0. This avoids
 *    using kernel stack-based copying and ensures the kernel can read and write the frames
 *    safely. The function maps the source frame read-only, maps the destination frame
 *    read-write, flushes the TLB, performs a memcpy through the scratch virtual addresses,
 *    and then unmaps both pages.
 *
 * ======= Parameters =======
 * @param pfn_src : Physical frame number of the source frame to be copied.
 * @param pfn_dst : Physical frame number of the destination frame to write into.
 *
 * ====== Return =======
 * @return void
 */
void CloneFrame(int pfn_src, int pfn_dst);

/**
 * Description:
 *    CopyPT clones the page table of one process (src) into another process (dst).
 *    For every valid entry in the source page table, the function allocates a new
 *    physical frame for the destination process, clones the contents of the source
 *    frame into the new frame using CloneFrame, and initializes the corresponding
 *    destination PTE to match the source's permissions. The function returns ERROR
 *    if any frame allocation fails; otherwise, it returns SUCCESS on completion.
 *
 * ======= Parameters =======
 * @param src : Pointer to the PCB of the source process whose page table is copied.
 * @param dst : Pointer to the PCB of the destination process that receives the cloned page table.
 *
 * ====== Return =======
 * @return int :
 *      - SUCCESS (0) if the page table was fully and correctly copied.
 *      - ERROR (-1) if a frame allocation failed.
 */

int CopyPT(PCB *src, PCB *dst);

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



#endif