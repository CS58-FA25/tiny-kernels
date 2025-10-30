#ifndef KERNEL_H
#define KERNEL_H

#include "ykernel.h"
#include "hardware.h"
#include "proc.h"

#define NUM_PAGES_REGION1 (VMEM_1_SIZE / PAGESIZE)
#define NUM_PAGES_REGION0      (VMEM_0_SIZE / PAGESIZE)
#define KERNEL_STACK_SIZE (KERNEL_STACK_LIMIT - KERNEL_STACK_BASE)
#define KSTACK_PAGES (KERNEL_STACK_MAXSIZE / PAGESIZE)
#define KSTACK_START_PAGE (KERNEL_STACK_BASE >> PAGESHIFT)

extern PCB pcb_table[MAX_PROCS];
pte_t *ptbr0;

int is_vm_enabled = 0;
extern int kernel_brk_page;
extern int text_section_base_page;
extern int data_section_base_page;

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);

/**
 * ======================== Description =======================
 * @brief Adjusts the kernel break (end of the kernel heap) to the specified address.
 *        If the requested address is higher than the current break, new pages are allocated
 *        and mapped into region 0. If it is lower, pages beyond the target address are
 *        unmapped and their corresponding frames freed.
 * ======================== Parameters ========================
 * @param addr_ptr (void *): The desired end address of the kernel heap. Must not overlap
 *                           with the kernel stack region.
 * ======================== Returns ===========================
 * @returns 0 on success.
 * @returns -1 on failure (e.g., invalid address, out of frames, or collision with stack).
 * 
 * ======================== Notes =============================
 * - Uses region 0 page table entries to manage kernel heap growth and shrinkage.
 * - Ensures that the heap does not grow into the kernel stack area.
 * - Calls MapRegion0() when expanding the heap and UnmapRegion0() when shrinking it.
 * - May require a TLB flush if region 0 mappings change during runtime.
 * 
*/
int SetKernelBrk(void *addr_ptr);

/**
 * ======================== Description =======================
 * @brief Initializes the kernel stack for the idle process.
 *        This function sets aside specific physical frames for the idle
 *        process’s kernel stack that were already mapped in region 0
 *        during virtual memory initialization.
 * ======================== Behavior ==========================
 * - Allocates a page table of size `KSTACK_PAGES` for the idle kernel stack.
 * - For each stack page, allocates a specific frame (using `allocSpecificFrame`)
 *   corresponding to the pre-mapped physical frames starting at `KSTACK_START_PAGE`.
 * - Marks each PTE as valid with read/write protection.
 * ======================== Returns ===========================
 * @returns pte_t* Pointer to the initialized kernel stack page table.
 * @returns NULL if memory allocation for the PTE array fails.
 * ======================== Notes =============================
 * - Unlike regular processes, the idle process must use the exact frames
 *   mapped during `initializeVM()` for its stack.
 * - These frames are marked as used only at this stage (not during boot).
 * - Intended to be called once during system startup.
 */
pte_t *InitializeKernelStackIdle(void);

/**
 * ======================== Description =======================
 * @brief Initializes the kernel stack for a newly created process.
 *        This function allocates fresh physical frames for the process’s
 *        kernel stack and maps them with standard read/write permissions.
 * ======================== Behavior ==========================
 * - Allocates a page table of size `KSTACK_PAGES` for the process’s kernel stack.
 * - For each stack page, allocates a new physical frame using `allocFrame(FRAME_KERNEL)`.
 * - Marks each PTE as valid and sets read/write protection bits.
 * ======================== Returns ===========================
 * @returns pte_t* Pointer to the initialized kernel stack page table.
 * @returns NULL if memory allocation for the PTE array fails.
 * ======================== Notes =============================
 * - Unlike `InitializeKernelStackIdle()`, this does *not* rely on specific
 *   physical frames that were pre-mapped in region 0.
 * - Used for user processes (non-idle) created after system initialization.
 * - If frame allocation fails, the system halts to prevent inconsistent state.
 */
pte_t *InitializeKernelStackProcess(void);

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *unused);

#endif