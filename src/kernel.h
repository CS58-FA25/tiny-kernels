#ifndef kernel.h
#define kernel.h

#include "ykernel.h"
#include "hardware.h"
#include "proc.h"

#define NUM_PAGES_REGION1 (VMEM_1_SIZE / PAGESIZE)


extern PCB pcb_table[MAX_PROCS];
pte_t *ptbr0;

int is_vm_enabled = 0;
extern int kernel_brk_page;

void KernelStart(char *cmd_args[], unsigned int pmem_size, UserContext *uctxt);


#endif