#include <ylib.h>
#include <yalnix.h>
#include <hardware.h> 

#include "ykernel.h"
#include "kernel.h"
#include "proc.h"
#include "mem.h"
#include "init.h"
#include "program.h"

#include <fcntl.h>
#include <unistd.h>
#include "load_info.h"

int is_vm_enabled;

// Kernel's region 0 sections
int kernel_brk_page;
int text_section_base_page;
int data_section_base_page;
int LoadProgram(char *name, char *args[], PCB *proc);

/* ================== Terminals ================== */
#define NUM_TERMINALS 4
#define TERMINAL_BUFFER_SIZE 1024
typedef struct terminal {
    int id;                        // terminal number (0–3)
    char input_buffer[TERMINAL_BUFFER_SIZE];
    int input_head;
    int input_tail;
    char output_buffer[TERMINAL_BUFFER_SIZE];
    int output_head;
    int output_tail;
    int transmitting;              // flag: is a transmission in progress?
    PCB *waiting_read_proc;      // process blocked on reading
    PCB *waiting_write_proc;     // process blocked on writing
} terminal_t;

/* ==================== Helpers =================== */

void CloneFrame(int pfn1, int pfn2);


void KernelStart(char **cmd_args, unsigned int pmem_size, UserContext *uctxt)
{
    TracePrintf(1, "KernelStart: Entering KernelStart\n");
    TracePrintf(1, "Physical memory has size %d\n", pmem_size);

    kernel_brk_page = _orig_kernel_brk_page;

    TracePrintf(1, "Initializing the free frame array...\n");
    InitializeFreeFrameList(pmem_size);

    TracePrintf(1, "Initializing virtual memory (creating region0 of page table and mapping to physical frames)...\n");
    InitializeVM();

    TracePrintf(1, "Initializing the interrupt vector table....\n");
    InitializeInterruptVectorTable();

    TracePrintf(1, "Initializing the table of free pids to be used by processes....\n");
    InitializeProcTable();

    TracePrintf(1, "Initializing process queues (ready, blocked, zombie)....\n");
    InitializeProcQueues();
    WriteRegister(REG_PTBR0, (unsigned int)pt_region0);
    WriteRegister(REG_PTLR0, MAX_PT_LEN);
    WriteRegister(REG_VM_ENABLE, 1);

 
    idle_proc = CreateIdlePCB(uctxt);
    current_process = idle_proc;
    WriteRegister(REG_PTBR1, (unsigned int)current_process->ptbr);
    WriteRegister(REG_PTLR1, NUM_PAGES_REGION1);
    TracePrintf(0, "Boot sequence till creating Idle process is done!\n");
    memcpy(uctxt, &current_process->user_context, sizeof(UserContext));

    init_proc = getFreePCB();
    if (init_proc == NULL) {
       TracePrintf(0, "Failed to create the init process pdb.\n");
    }
    init_proc->kstack = InitializeKernelStackProcess();
    memcpy(&(init_proc->user_context), uctxt, sizeof(UserContext));

    char *name = (cmd_args[0] == NULL) ? "./user/init" : cmd_args[0];

    TracePrintf(0, "Loading program into init proccess. Switching registers for region1 of page table and flushing TLB entries!\n");
    WriteRegister(REG_PTBR1, (unsigned int)init_proc->ptbr);
    WriteRegister(REG_PTLR1, NUM_PAGES_REGION1);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    // Now, load the program text data and stack into init_proc process control block

    int load_status = LoadProgram(name, cmd_args, init_proc);

    if (load_status != SUCCESS) {
      TracePrintf(0, "KernelStart: Failed to load program %s!\n", name);
      Halt();
    }

    // Switch back to idle proc registers
    TracePrintf(0, "KernelStart: Resetting page table registers to those of idle process.\n");
    WriteRegister(REG_PTBR1, (unsigned int)idle_proc->ptbr);
    WriteRegister(REG_PTLR1, NUM_PAGES_REGION1);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL); // Flushing TLB from any stale mappings

    // Add the init process to the ready queue to be scheduled to run by the scheduler
    queueEnqueue(ready_queue, init_proc);

    // Now copy kernel context into init process
    KernelContextSwitch(KCCopy, init_proc, NULL);
    memcpy(uctxt, &current_process->user_context, sizeof(UserContext));
    return;



//     // Check if there are no arguments provided
//     // If there are no arguments, lets create a *new* argument array
//     if (!cmd_args || !*cmd_args) {
// 	// _Cmd_Args = cmd_args at point of start
// 	// Initialize a new array for hot-swapped arguments
//         cmd_args = malloc(sizeof(char*) * 1);
//         *cmd_args = "user/init"; // TODO: in CWD..
//         TracePrintf(0, "NO ARGUMENTS PROVIDED!!! Set local `cmd_args` to \"init\".\n");
//         TracePrintf(0, "did not touch global `_Cmd_Args`, do NOT expect a reflection in behavior.\n");
//     }


    
//     int pidx = LoadProgramFromArguments(cmd_args);

//     if (pidx < 0) {
//         TracePrintf(0, "Failed to load program.\n");
// 	// Error!!!! Let's jump out gracefully
//     } else {
// 	// DEBUG CODE WHILE THERE IS PROGRAM TRACKING! This doesn't need to exist later though
// #ifdef DEBUG
// 	Program* prog = GetProgramListing()->progs[pidx];
//         TracePrintf(0, "Loaded program: %d (%p) (%s) \n", pidx, prog, prog->file);
// 	// This run the program with the provided context
// 	RunProgram(pidx, uctxt);
// #endif
//     }


}

KernelContext *KCSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    PCB *curr = (PCB *)curr_pcb_p;
    PCB *next = (PCB *)next_pcb_p;

    if (curr) {
	// If there is no current process, the context doesn't get saved :(
        memcpy(&(curr->kernel_context), kc_in, sizeof(KernelContext));
    }

    // Switch kernel stacks in region 0 from current process to next process
    pte_t *next_kstack = next->kstack;
    if (next_kstack == NULL) {
      TracePrintf(0, "nextKstack is null!\n");
    }
    for(int i = 0; i < KSTACK_PAGES; i++) {
        int vpn = (KERNEL_STACK_BASE >> PAGESHIFT)+ i;
        pt_region0[vpn] = next_kstack[i];
    }

    // Setting the current process to be the new one
    current_process = next;
    next->state = PROC_RUNNING;
    
    // Switch pointers to region 1 page table in CPU registers.
    WriteRegister(REG_PTBR1, (unsigned int) next->ptbr);
    WriteRegister(REG_PTLR1, next->ptlr);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    return &(next->kernel_context);
}

KernelContext *KCCopy(KernelContext *kc_in, void *new_pcb_p, void *unused){
    PCB *pcb_new = (PCB *) new_pcb_p;
    if (pcb_new == NULL) {
        TracePrintf(1, "KCCopy: New PCB is null. Can't clone into new process.\n");
    }
    // Copying kernel context of the parent process into the new child process PCB
    memcpy(&(pcb_new->kernel_context), kc_in, sizeof(KernelContext));

    // Initializing the new child process kernel stack
    pte_t *kstack_new = pcb_new->kstack;
    if (kstack_new == NULL) {
        kstack_new = InitializeKernelStackProcess();
    }

    // Copying the contents of the current process kernel stack into the child process
    for (int i = 0; i < KSTACK_PAGES; i++) {
        int pfn_parent = current_process->kstack[i].pfn;
        int pfn_child = kstack_new[i].pfn;

        CloneFrame(pfn_parent, pfn_child);
    }
    
    // In case the CPU switches to the new process after returning. We don't want it to use old kstack mappings
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_KSTACK); 
    
    return kc_in;

}

void scheduler() {
    // while true:
    //     next = queueDequeue(ready_queue)
    //     if next == NULL:
    //         next = idle_pcb  // run idle if no ready processes

    //     rc = KernelContextSwitch(KCSwitch, current, next)

    //     if rc == -1:
    //         TracePrintf(0, "Context switch failed")
    //         Halt()

    //     // When we return here, current process has resumed
    //     HandlePendingTrapsOrSyscalls()
}

pte_t *InitializeKernelStackIdle(void) {
    pte_t* kernel_stack = malloc(sizeof(pte_t) * KSTACK_PAGES);
    if (kernel_stack == NULL) {
        TracePrintf(0, "InitializeKernelStackIdle: Failed to allocate memory for idle process Kernel Stack.\n");
        return NULL;
    }
    for (int i = 0; i < KSTACK_PAGES; i++) {
        int pfn = KSTACK_START_PAGE + i;
        allocSpecificFrame(pfn, FRAME_KERNEL, -1);

        kernel_stack[i].valid = 1;
        kernel_stack[i].pfn = pfn;
        kernel_stack[i].prot = PROT_WRITE | PROT_READ;

    }
    return kernel_stack;
}

pte_t *InitializeKernelStackProcess(void) {
    pte_t* kernel_stack = malloc(sizeof(pte_t) * KSTACK_PAGES);
    if (kernel_stack == NULL) {
        TracePrintf(0, "InitializeKernelStackProcess: Failed to allocate memory for idle process Kernel Stack.\n");
        return NULL;
    }
    for (int i = 0; i < KSTACK_PAGES; i++) {
        int pfn = allocFrame(FRAME_KERNEL, -1);
        if (pfn == -1) {
            TracePrintf(0, "InitializeKernelStackProcess: Failed to allocate frame for kernel stack.\n");
            Halt();
        }
        kernel_stack[i].valid = 1;
        kernel_stack[i].pfn = pfn;
        kernel_stack[i].prot = PROT_WRITE | PROT_READ;
    }
    return kernel_stack;
}

int SetKernelBrk(void *addr_ptr)
{
    if (addr_ptr == NULL) {
        TracePrintf(0, "SetKernelBrk: NULL addr\n");
        return -1;
    }

    /* convert address to target VPN */
    unsigned int target_vaddr = UP_TO_PAGE((unsigned int) addr_ptr);
    unsigned int target_vpn  = target_vaddr >> PAGESHIFT;

    /* bounds: cannot grow into kernel stack (stack base is virtual address) */
    unsigned int stack_base_vpn = DOWN_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT;
    if (target_vpn >= stack_base_vpn) {
        TracePrintf(0, "SetKernelBrk: target collides with kernel stack\n");
        return -1;
    }

    /* current kernel_brk is a page number (vpn) */
    unsigned int cur_vpn = (unsigned int) kernel_brk_page; /* assume kernel_brk is already VPN */

    /* Grow heap: map pages for VPNs [cur_vpn .. target_vpn-1] */
    while (cur_vpn < target_vpn) {
        int pfn = allocFrame(FRAME_KERNEL, -1);

        if (pfn == -1) {
            TracePrintf(0, "SetKernelBrk: out of physical frames\n");
            return -1;
        }

        /* Map physical pfn into region0 VPN = cur_vpn */
        MapRegion0(cur_vpn, pfn);
        cur_vpn++;
    }

    /* Shrink heap: unmap pages for VPNs [target_vpn .. cur_vpn-1] */
    while (cur_vpn > target_vpn) {
        cur_vpn--; /* handle the page at vpn = cur_vpn - 1 */

        if (!vpn_in_region0(cur_vpn)) {
            TracePrintf(0, "SetKernelBrk: shrink vpn out of range %u\n", cur_vpn);
            return -1;
        }

        if (!pt_region0[cur_vpn].valid) {
            TracePrintf(0, "SetKernelBrk: unmapping already-unmapped vpn %u\n", cur_vpn);
            continue;
        }

        int pfn = (int) pt_region0[cur_vpn].pfn;

        /* unmap */
        UnmapRegion0(cur_vpn);
        WriteRegister(REG_TLB_FLUSH, (unsigned int) cur_vpn);


        /* free the physical frame */
        freeFrame(pfn);
    }

    /* update kernel_brk (store vpn) */
    kernel_brk_page = (int) target_vpn;
    return 0;
}

/**
 * Description: Copy the contents of the frame pfn1 to the frame pfn2
*/
void CloneFrame(int pfn_from, int pfn_to) {
    // Map a scratch page to pfn_to
    int scratch_page = SCRATCH_ADDR >> PAGESHIFT;
    pt_region0[scratch_page].pfn = pfn_to;
    pt_region0[scratch_page].prot = PROT_READ | PROT_WRITE;
    pt_region0[scratch_page].valid = 1;
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR); // In case the CPU already has a mapping in the TLB

    // Copy the contents of pfn_from to the frame mapped by the page table to pfn_to
    unsigned int pfn_from_addr = pfn_from << PAGESHIFT;
    memcpy((void *)(SCRATCH_ADDR), (void *)pfn_from_addr, PAGESIZE);

    // Now that the frame pfn_to has the contents of the frame pfn_from, unmap it from the scratch address in pt_region0
    pt_region0[scratch_page].valid = 0;
    pt_region0[scratch_page].prot = 0;
    pt_region0[scratch_page].pfn = 0;
    WriteRegister(REG_TLB_FLUSH, SCRATCH_ADDR);
}


int
LoadProgram(char *name, char *args[], PCB *proc) 

{
  int fd;
  int (*entry)();
  struct load_info li;
  int i;
  char *cp;
  char **cpp;
  char *cp2;
  int argcount;
  int size;
  int text_pg1;
  int data_pg1;
  int data_npg;
  int stack_npg;
  long segment_size;
  char *argbuf;

  
  /*
   * Open the executable file 
   */
  if ((fd = open(name, O_RDONLY)) < 0) {
    TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
    return ERROR;
  }

  if (LoadInfo(fd, &li) != LI_NO_ERROR) {
    TracePrintf(0, "LoadProgram: '%s' not in Yalnix format\n", name);
    close(fd);
    return (-1);
  }

  if (li.entry < VMEM_1_BASE) {
    TracePrintf(0, "LoadProgram: '%s' not linked for Yalnix\n", name);
    close(fd);
    return ERROR;
  }

  /*
   * Figure out in what region 1 page the different program sections
   * start and end
   */
  text_pg1 = (li.t_vaddr - VMEM_1_BASE) >> PAGESHIFT;
  data_pg1 = (li.id_vaddr - VMEM_1_BASE) >> PAGESHIFT;
  data_npg = li.id_npg + li.ud_npg;
  /*
   *  Figure out how many bytes are needed to hold the arguments on
   *  the new stack that we are building.  Also count the number of
   *  arguments, to become the argc that the new "main" gets called with.
   */
  size = 0;
  for (i = 0; args[i] != NULL; i++) {
    TracePrintf(3, "counting arg %d = '%s'\n", i, args[i]);
    size += strlen(args[i]) + 1;
  }
  argcount = i;

  TracePrintf(2, "LoadProgram: argsize %d, argcount %d\n", size, argcount);
  
  /*
   *  The arguments will get copied starting at "cp", and the argv
   *  pointers to the arguments (and the argc value) will get built
   *  starting at "cpp".  The value for "cpp" is computed by subtracting
   *  off space for the number of arguments (plus 3, for the argc value,
   *  a NULL pointer terminating the argv pointers, and a NULL pointer
   *  terminating the envp pointers) times the size of each,
   *  and then rounding the value *down* to a double-word boundary.
   */
  cp = ((char *)VMEM_1_LIMIT) - size;

  cpp = (char **)
    (((int)cp - 
      ((argcount + 3 + POST_ARGV_NULL_SPACE) *sizeof (void *))) 
     & ~7);

  /*
   * Compute the new stack pointer, leaving INITIAL_STACK_FRAME_SIZE bytes
   * reserved above the stack pointer, before the arguments.
   */
  cp2 = (caddr_t)cpp - INITIAL_STACK_FRAME_SIZE;



  TracePrintf(1, "prog_size %d, text %d data %d bss %d pages\n",
	      li.t_npg + data_npg, li.t_npg, li.id_npg, li.ud_npg);


  /* 
   * Compute how many pages we need for the stack */
  stack_npg = (VMEM_1_LIMIT - DOWN_TO_PAGE(cp2)) >> PAGESHIFT;

  TracePrintf(1, "LoadProgram: heap_size %d, stack_size %d\n",
	      li.t_npg + data_npg, stack_npg);


  /* leave at least one page between heap and stack */
  if (stack_npg + data_pg1 + data_npg >= MAX_PT_LEN) {
    close(fd);
    return ERROR;
  }

  /*
   * This completes all the checks before we proceed to actually load
   * the new program.  From this point on, we are committed to either
   * loading succesfully or killing the process.
   */

  /*
   * Set the new stack pointer value in the process's UserContext
   */
  
  /* 
   * ==>> (rewrite the line below to match your actual data structure) 
   * ==>> proc->uc.sp = cp2; 
   */
  (proc->user_context).sp = cp2;

  /*
   * Now save the arguments in a separate buffer in region 0, since
   * we are about to blow away all of region 1.
   */
  cp2 = argbuf = (char *)malloc(size);

  /* 
   * ==>> You should perhaps check that malloc returned valid space 
   */
  if (argbuf == NULL) {
    TracePrintf(0, "LoadProgram: Couldn't allocate memory for argbuf.\n");
    return KILL;
  }

  for (i = 0; args[i] != NULL; i++) {
    TracePrintf(3, "saving arg %d = '%s'\n", i, args[i]);
    strcpy(cp2, args[i]);
    cp2 += strlen(cp2) + 1;
  }

  /*
   * Set up the page tables for the process so that we can read the
   * program into memory.  Get the right number of physical pages
   * allocated, and set them all to writable.
   */

  /* ==>> Throw away the old region 1 virtual address space by
   * ==>> curent process by walking through the R1 page table and,
   * ==>> for every valid page, free the pfn and mark the page invalid.
   */
  TracePrintf(0, "LoadProgram: Throwing away old region 1 mappings and freeing allocated frames.\n");
  pte_t *pt_region1 = proc->ptbr;
  for (int vpn = 0; vpn < MAX_PT_LEN; vpn++) {
    if (pt_region1[vpn].valid == 1) {
        freeFrame(pt_region1[vpn].pfn);
        pt_region1[vpn].valid = 0;
    }
  }

  /*
   * ==>> Then, build up the new region1.  
   * ==>> (See the LoadProgram diagram in the manual.)
   *

  /*
   * ==>> First, text. Allocate "li.t_npg" physical pages and map them starting at
   * ==>> the "text_pg1" page in region 1 address space.
   * ==>> These pages should be marked valid, with a protection of
   * ==>> (PROT_READ | PROT_WRITE).
   */
  TracePrintf(0, "LoadProgram: Allocating %d frames for text segment.\n", li.t_npg);
  for (int i = 0; i < li.t_npg; i++) {
    int pfn = allocFrame(FRAME_USER, proc->pid);
    pt_region1[text_pg1 + i].pfn = pfn;
    pt_region1[text_pg1 + i].valid = 1;
    pt_region1[text_pg1 + i].prot = PROT_READ | PROT_WRITE;
    
  }

  /*
   * ==>> Then, data. Allocate "data_npg" physical pages and map them starting at
   * ==>> the  "data_pg1" in region 1 address space.
   * ==>> These pages should be marked valid, with a protection of
   * ==>> (PROT_READ | PROT_WRITE).
   */
  TracePrintf(0, "LoadProgram: Allocating %d frames for data segment.\n", data_npg);
  for (int i = 0; i < data_npg; i++) {
    int pfn = allocFrame(FRAME_USER, proc->pid);
    pt_region1[data_pg1 + i].pfn = pfn;
    pt_region1[data_pg1 + i].valid = 1;
    pt_region1[data_pg1 + i].prot = PROT_READ | PROT_WRITE;
  }
  /* 
   * ==>> Then, stack. Allocate "stack_npg" physical pages and map them to the top
   * ==>> of the region 1 virtual address space.
   * ==>> These pages should be marked valid, with a
   * ==>> protection of (PROT_READ | PROT_WRITE).
   */

  /* Start stack_npg from the top of pt_region1*/
  TracePrintf(0, "LoadProgram: Allocating %d frames for the stack.\n", stack_npg);
  int stack_base = proc->ptlr - stack_npg;
  for (int i = 0; i < stack_npg; i++) {
    int pfn = allocFrame(FRAME_USER, proc->pid);
    pt_region1[stack_base + i].pfn = pfn;
    pt_region1[stack_base + i].valid = 1;
    pt_region1[stack_base + i].prot = PROT_READ | PROT_WRITE;
  }

  /*
   * ==>> (Finally, make sure that there are no stale region1 mappings left in the TLB!)
   */
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

  /*
   * All pages for the new address space are now in the page table.  
   */

  /*
   * Read the text from the file into memory.
   */
  lseek(fd, li.t_faddr, SEEK_SET);
  segment_size = li.t_npg << PAGESHIFT;
  if (read(fd, (void *) li.t_vaddr, segment_size) != segment_size) {
    close(fd);
    return KILL;   // see ykernel.h
  }

  /*
   * Read the data from the file into memory.
   */
  lseek(fd, li.id_faddr, 0);
  segment_size = li.id_npg << PAGESHIFT;

  if (read(fd, (void *) li.id_vaddr, segment_size) != segment_size) {
    close(fd);
    return KILL;
  }


  close(fd);			/* we've read it all now */


  /*
   * ==>> Above, you mapped the text pages as writable, so this code could write
   * ==>> the new text there.
   *
   * ==>> But now, you need to change the protections so that the machine can execute
   * ==>> the text.
   *
   * ==>> For each text page in region1, change the protection to (PROT_READ | PROT_EXEC).
   * ==>> If any of these page table entries is also in the TLB, 
   * ==>> you will need to flush the old mapping. 
   */
  for (int i = 0; i < li.t_npg; i++) {
    pt_region1[text_pg1 + i].prot = PROT_READ | PROT_EXEC;
  }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

  
  /*
   * Zero out the uninitialized data area
   */
  bzero(li.id_end, li.ud_end - li.id_end);


  /*
   * Set the entry point in the process's UserContext
   */
  /* 
   * ==>> (rewrite the line below to match your actual data structure) 
   * ==>> proc->uc.pc = (caddr_t) li.entry;
   */
    (proc->user_context).pc = (caddr_t) li.entry;


    proc->user_heap_start_vaddr = (unsigned int) ((data_pg1 << PAGESHIFT) + VMEM_1_BASE); // Heap starts after end of data segment
    proc->user_heap_end_vaddr = (unsigned int)  ((data_pg1 << PAGESHIFT) + VMEM_1_BASE);
    proc->user_stack_base_vaddr = (unsigned int)((stack_base << PAGESHIFT) + VMEM_1_BASE);

  /*
   * Now, finally, build the argument list on the new stack.
   */


  memset(cpp, 0x00, VMEM_1_LIMIT - ((int) cpp));

  *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
  cp2 = argbuf;
  for (i = 0; i < argcount; i++) {      /* copy each argument and set argv */
    *cpp++ = cp;
    strcpy(cp, cp2);
    cp += strlen(cp) + 1;
    cp2 += strlen(cp2) + 1;
  }
  free(argbuf);
  *cpp++ = NULL;			/* the last argv is a NULL pointer */
  *cpp++ = NULL;			/* a NULL pointer for an empty envp */

  TracePrintf(0, "LoadProgram: Success in loading process PID %d!\n", proc->pid);

  return SUCCESS;
}

