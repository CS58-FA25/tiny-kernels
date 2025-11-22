#ifndef KERNEL_H
#define KERNEL_H

#include "ykernel.h"
#include "hardware.h"
#include "proc.h"

#define NUM_PAGES_REGION1     (MAX_PT_LEN)
#define NUM_PAGES_REGION0      (MAX_PT_LEN)
#define KERNEL_STACK_SIZE (KERNEL_STACK_LIMIT - KERNEL_STACK_BASE)
#define KSTACK_PAGES (KERNEL_STACK_MAXSIZE / PAGESIZE)
#define KSTACK_START_PAGE (KERNEL_STACK_BASE >> PAGESHIFT)

extern int kernel_brk_page;
extern int text_section_base_page;
extern int data_section_base_page;

/**
 * ======================== Description =======================
 * @brief Bootstraps the Yalnix kernel, sets up all core subsystems, 
 *        creates the Idle and Init processes, and prepares the system
 *        to transition into user mode. This routine initializes memory
 *        management, interrupt handling, process bookkeeping, terminals,
 *        and the scheduler queues before loading the first user program.
 *
 * ======================== Parameters ========================
 * @param cmd_args: Command-line arguments passed to the kernel, used to determine the initial user program to load.
 * @param pmem_size: Size of available physical memory in bytes. Used to initialize the free-frame allocator
 * @param uctxt: Pointer to the bootstrap UserContext provided at kernel entry.
 * ======================== Returns ===========================
 * @returns Nothing. The function completes kernel initialization and
 *          returns into user mode using the user
 *          context of the Idle process.
 */
void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);

/**
 * ======================== Description =======================
 * @brief Adjusts the kernel break (end of the kernel heap) to the specified address.
 *        If the requested address is higher than the current break, new pages are allocated
 *        and mapped into region 0. If it is lower, pages beyond the target address are
 *        unmapped and their corresponding frames freed.
 * ======================== Parameters ========================
 * @param addr_ptr: The desired end address of the kernel heap. Must not overlap
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
 * @brief Performs a low-level context switch between two processes.
 *
 *        Saves the outgoing process’s kernel context (if any), installs
 *        the incoming process’s kernel stack into Region 0, updates the
 *        active PCB, and switches Region 1 page tables so that the CPU
 *        executes in the new process’s address space. Returns a pointer
 *        to the new process’s saved kernel context so the machine
 *        context-switch trampoline can resume execution there.
 *
 * ======================== Parameters ========================
 * @param kc_in: Pointer to the outgoing process’s saved KernelContext.
 * @param curr_pcb_p: Pointer to the PCB of the currently running process.
 * @param next_pcb_p: Pointer to the PCB of the process that is being switched to.
 * ======================== Returns ===========================
 * @returns Pointer to the incoming process’s KernelContext structure.
 *          The context-switch trampoline loads this context to resume
 *          execution in the next process.
 */

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);

/**
 * ======================== Description =======================
 * @brief Clones the current process's kernel context and kernel stack
 *        into the child process PCB during Fork().
 *
 * ======================== Parameters ========================
 * @param kc_in: Pointer to parent's saved kernel context.
 * @param new_pcb_p: Pointer to the child PCB receiving the copy.
 * @param unused: Required by the callback signature (unused).
 *
 * ======================== Returns ===========================
 * @returns `kc_in` — makes parent execute first; child returns later
 *          from same point with copied state.
 */
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *unused);

/**
 * ========================= Description =========================
 * @brief Loads an executable into the given process’s Region 1 address
 *        space. Replaces any existing mappings, allocates frames for
 *        text/data/stack, copies in program segments, builds the initial
 *        user stack with argc/argv, and sets the process’s entry point
 *        and stack pointer.
 *
 * ========================= Parameters ==========================
 * @param name: Path to the executable to load.
 * @param args: NULL-terminated argument vector passed to the program.
 * @param proc: PCB of the process whose address space is being replaced.
 *
 * ========================= Returns ==============================
 * @returns SUCCESS on successful load; ERROR or KILL on failure.
 */

int LoadProgram(char *name, char *args[], PCB *proc);

#endif
