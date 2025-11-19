#include <hardware.h>

#include "traps/memory.h"
#include "syscalls/syscall.h"
#include "mem.h"
#include "proc.h"

void MemoryTrapHandler(UserContext* ctx) {
   unsigned int fault_addr = (unsigned int)ctx->addr;
   TracePrintf(0, "Fault address is 0x%x\n", fault_addr);
   unsigned int user_heap_limit_addr = UP_TO_PAGE((unsigned int)(current_process->user_heap_end_vaddr));
   unsigned int user_stack_base_addr = DOWN_TO_PAGE((unsigned int)(current_process->user_stack_base_vaddr));
   if (ctx->code == YALNIX_MAPERR &&
      fault_addr > user_heap_limit_addr &&
      fault_addr < user_stack_base_addr
      )
   {
      TracePrintf(0, "Kernel: Memory Trap Handler growing stack for user process PID %d.\n", current_process->pid);
      int result = growStack(fault_addr);
      if (result == KILL) {
         TracePrintf(0, "Kernel: Memory trap handler killing process PID %d because of a failure at growing user stack!\n", current_process->pid);
         Exit(ERROR);
      }
      return;
   }
   if (ctx -> code == YALNIX_MAPERR)
   if (ctx->code == YALNIX_MAPERR) {
      TracePrintf(0, "Kernel: Error, page is not mapped!\n");
      TracePrintf(0, "Kernel: Killing process PID %d.\n", current_process->pid);
      Exit(ERROR);
   } else if (ctx->code == YALNIX_ACCERR) {
      TracePrintf(0, "Kernel: Invalid memory access for process PID %d!\n", current_process->pid);
      TracePrintf(0, "Kernel: Killing process PID %d.\n", current_process->pid);
      Exit(ERROR);
   }
   return;
}

void IllegalInstructionTrapHandler(UserContext* ctx) {
   void *pc = ctx->pc;
   TracePrintf(0, "Kernel: Process PID %d tried executing an illegal instruction at pc 0x%x. Killing the process!\n", current_process->pid, pc);
   Exit(ERROR); // Killing the process

   TracePrintf(0, "Error: Exit syscall returned in IllegalInstructionTrapHandler! This shouldn't happen. Halting the machine!\n");
   Halt();
}


int growStack(unsigned int addr) {
   unsigned int addr_aligned_to_page_base = DOWN_TO_PAGE(addr);
   unsigned int stack_base_aligned_to_page_base = DOWN_TO_PAGE((unsigned int)(current_process->user_stack_base_vaddr));

   int target_vpn = (addr_aligned_to_page_base - VMEM_1_BASE) >> PAGESHIFT;
   int stack_base_vpn = (stack_base_aligned_to_page_base - VMEM_1_BASE) >> PAGESHIFT;
   int num_pages_requested = stack_base_vpn - target_vpn;

   pte_t *pt_region1 = current_process->ptbr;
   for (int i = 0; i < num_pages_requested; i++) {
      int pfn = allocFrame(FRAME_USER, current_process->pid);
      if (pfn == -1) {
         TracePrintf(0, "Kernel: Ran out of physical frames while trying to grow user stack for process PID %d!\n", current_process->pid);
         // Need to deallocate all the frames that we already allocated
         for (int j = 0; j < i; j++) {
            int pfn_to_free = pt_region1[target_vpn + j].pfn;
            freeFrame(pfn_to_free);
            pt_region1[target_vpn + j].valid = 0;
            // Do i need to zero out the frame?
         }
         return KILL;
      }

      pt_region1[target_vpn + i].valid = 1;
      pt_region1[target_vpn + i].pfn = pfn;
      pt_region1[target_vpn + i].prot = PROT_READ | PROT_WRITE;
   }
   current_process->user_stack_base_vaddr = addr_aligned_to_page_base;
   TracePrintf(0, "Succesfully grew process PID %d stack to %u.\n", current_process->pid, addr);
   return SUCCESS;
}
