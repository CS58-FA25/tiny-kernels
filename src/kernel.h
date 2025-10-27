#ifndef kernel.h
#define kernel.h

#include "ykernel.h"
#include "hardware.h"

#define NUM_PAGES_REGION1 (VMEM_1_SIZE / PAGESIZE)


extern int kernel_brk;

#endif