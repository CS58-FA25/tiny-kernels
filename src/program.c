#include "program.h"
#include "proc.h"
#include "kernel.h" // InitializeKernelStackProcess
#include "mem.h"

#include <hardware.h> // TracePrintf
#include <ylib.h> // malloc, memcpy
#include <fcntl.h>
#include <load_info.h>

static Program _empty_program;

#ifdef DEBUG
static ProgramListing _program_listing;

ProgramListing* GetProgramListing() {
    return &_program_listing;
}
#endif

int _GetStringArrayLength(char** array) {
   char** ptr = array;
   unsigned int count = 0;
   while (*ptr != 0x0) {
       count = count + 1;
       ptr = ptr + 1;
   }
   return count;
}

static void AllocateProgramStack(Program* program) {
}

#ifdef DEBUG

int RunProgram(int idx, UserContext* ctx) {
   if (idx > _program_listing.max_progs || idx > _program_listing.nprogs) {
      return -1;
   }

   Program* program = _program_listing.progs[idx];
   if (program == 0x0) {
       return -1;
   }

   PCB* pcb;
   if ((pcb = getFreePCB()) == 0x0) {
        return -1;
   }

   // If there is anything else in the PCB, let's clear it now
   for (int idx = 0; idx < pcb->ptlr; idx++) {
       if (pcb->ptbr[idx].valid) {
	   freeFrame(pcb->ptbr[idx].pfn);
           pcb->ptbr[idx].valid = 0;
       }
       pcb->ptbr[idx].prot = 0;
       pcb->ptbr[idx].pfn = 0;
   }

   WriteRegister(REG_TLB_FLUSH,TLB_FLUSH_0);

  /*
   * Figure out in what region 1 page the different program sections
   * start and end
   */
   int text_pg1 = (program->li.t_vaddr - VMEM_1_BASE) >> PAGESHIFT;
   int data_pg1 = (program->li.id_vaddr - VMEM_1_BASE) >> PAGESHIFT;
   int data_npg = program->li.id_npg + program->li.ud_npg;
   int stack_npg = (VMEM_1_LIMIT - DOWN_TO_PAGE(cp2)) >> PAGESHIFT;
   
   char* cp = ((char *)VMEM_1_LIMIT) - size;
   char** cpp = (char **) (((int)cp - ((argcount + 3 + POST_ARGV_NULL_SPACE) * sizeof (void *))) & ~7);

   pcb->stack = ;

   // (addr >> PAGESHIFT) - stack_npg;
   // Set PC
   pcb->user_context.pc = program->li.entry;
   // Set PCB
   program->_pcb = pcb;

   // Zero out uninitialized data
   bzero(program->li.id_end, program->li.ud_end - program->li.id_end);
   // Migrate the args over

   // TODO: Since there is no scheduler, we are just going to call the switch now.
   // This is not a good idea and we should properly track the kernel context in the future..
   // // KCCopy(kctxt, program->_pcb, 0);
   // TODO: There should also be process tracking here (when there is a scheduler, of course)
   program->_pcb->kstack = InitializeKernelStackProcess();
   KernelContextSwitch(KCSwitch, current_process, program->_pcb);
   memcpy(&(program->_pcb->user_context), ctx, sizeof(UserContext));
   return 0;
}

#else
static inline int _RunProgram(Program* program, UserContext* ctx) {
   // Non-debug mode is NOT IMPLEMENTED YET!!!
   //
   //PCB* pcb;
   // if ((pcb = getFreePCB()) == 0x0) {
	// return -1;
   // }

   // program->_pcb = pcb;
   // KCCopy(kctxt, program->_pcb, 0);
   // memcpy(&(program->_pcb->user_context), ctx, sizeof(UserContext));
   return -1;
}

#endif

int LoadProgram(char* program, int argc, char** argv) {
    int fd;
    struct load_info li;

#ifdef DEBUG
    if (!_program_listing.progs) {
	_program_listing.max_progs = MAX_PROGRAMS;
        _program_listing.progs = malloc(sizeof(Program*) * _program_listing.max_progs);
	_program_listing.nprogs = 0;

	// Initializing this here for now, but this will have to eventually become a resizeable structure.
	for (int i = 0; i < _program_listing.max_progs; i++) {
            _program_listing.progs[i] = &_empty_program;
	}
    }
#endif

//  typedef struct Program {
//      PCB* _pcb;
//      char* file;
//      int argc;
//      char** argv;
//      unsigned long base; // Address
//      struct load_info li;
//  };
  
    if ((fd = open(program, O_RDONLY)) < 0) {
	TracePrintf(0, "[PROGRAM: LOAD]: Could not open file \"%s\"\n", program);
        return -1;
    }

    if (LoadInfo(fd, &li) != LI_NO_ERROR) {
	// LoadInfo looks for a valid ELF, in this case
	TracePrintf(0, "[PROGRAM: LOAD]: Program \"%s\" not in valid format!!\n", program);
        return -1;
    }

    if (li.entry < VMEM_1_BASE) {
        TracePrintf(0, "[PROGRAM: LOAD]: Program \"%s\" is not linked for Yalnix!\n", program);
	return -1;
    }

    Program* p = (Program*) malloc(sizeof(Program));
    p->file = program;
    p->argc = argc;
    p->argv = argv;
    memcpy(&p->li, &li, sizeof(li));

#ifdef DEBUG
    if (_program_listing.progs[_program_listing.nprogs] == &_empty_program) {
       _program_listing.progs[_program_listing.nprogs] = p;
       _program_listing.nprogs = _program_listing.nprogs + 1;
       return _program_listing.nprogs - 1;
    }

    // Should not reach here
    free(p);
    return -1;
#else
    extern UserContext fake_frame_global; // Stealing this,a gain
    return _RunProgram(p, &fake_frame_global);
#endif
}

int LoadProgramFromArguments(char** args) {
   unsigned int charsz = sizeof(char*);
   int count = _GetStringArrayLength(args);
   int argc = count - 1;
   char* prog = args[0];
   char** argv = (char**) malloc(charsz * argc);
   memcpy(argv, args + 1, charsz * argc);
   return LoadProgram(prog, argc, argv);
}
