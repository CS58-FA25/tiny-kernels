#ifndef kernel.h
#define kernel.h

#include "ykernel.h"
#include "hardware.h"
#include "proc.h"

#define NUM_PAGES_REGION1 (VMEM_1_SIZE / PAGESIZE)
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
KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *unused);


#endif