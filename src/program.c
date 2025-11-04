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

   for (int idx = 0; idx < pcb->ptlr; idx++) {
       if (pcb->ptbr[idx].valid) {
           freeFrame(pcb->ptbr[idx].pfn);
           pcb->ptbr[idx].valid = 0;
       }
       MapPage(pcb->ptbr, idx, 0, 0);
   }
   WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

   unsigned int size = 0;
   for (int i = 0; i < program->argc; i++) {
      size = size + strlen(program->argv[i]) + 1;
   }

   char* cp = ((char *)VMEM_1_LIMIT) - size;
   char** cpp = (char **) (((int) cp - ((program->argc + 3 + POST_ARGV_NULL_SPACE) * sizeof (void *))) & ~7);
   char* sp = (caddr_t)cpp - INITIAL_STACK_FRAME_SIZE;

   int text_pg1 = (program->li.t_vaddr - VMEM_1_BASE) >> PAGESHIFT;
   int data_pg1 = (program->li.id_vaddr - VMEM_1_BASE) >> PAGESHIFT;
   int data_npg = program->li.id_npg + program->li.ud_npg;
   int stack_npg = (VMEM_1_LIMIT - DOWN_TO_PAGE(sp)) >> PAGESHIFT;

   if (stack_npg + data_pg1 + data_npg + text_pg1 >= MAX_PT_LEN) {
       // too big
       return -1;
   }

   for (int pgno = 0; pgno < program->li.t_npg; pgno++) {
       int pfn = allocFrame(FRAME_USER, pcb->pid);
       MapPage(pcb->ptbr, text_pg1 + pgno, pfn, PROT_READ | PROT_WRITE);
   }

   for (int pgno = 0; pgno < data_npg; pgno++) {
       int pfn = allocFrame(FRAME_USER, pcb->pid);
       MapPage(pcb->ptbr, data_pg1 + pgno, pfn, PROT_READ | PROT_WRITE);
   }

   for (int pgno = 0; pgno < stack_npg; pgno++) {
       int pfn = allocFrame(FRAME_USER, pcb->pid);
       MapPage(pcb->ptbr, (pcb->ptlr - stack_npg) + pgno, pfn, PROT_READ | PROT_WRITE);
   }

   WriteRegister(REG_PTBR1, pcb->ptbr);
   WriteRegister(REG_PTLR1, pcb->ptlr);
   WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

   int fd = open(program->file, O_RDONLY);
   if (fd < 0) {
       return -1;
   }

   size_t textsz = program->li.t_npg << PAGESHIFT;
   lseek(fd, program->li.t_faddr, SEEK_SET);
   if (read(fd, (void*)program->li.t_vaddr, textsz) != textsz) { 
       close(fd);
   }

   size_t datasz = program->li.id_npg << PAGESHIFT;
   lseek(fd, program->li.id_faddr, SEEK_SET);
   if (read(fd, (void*)program->li.id_vaddr, datasz) != datasz) { 
       close(fd);
   }

   TracePrintf(0, "Copied over program data\n");

   bzero((void*) program->li.id_end, program->li.ud_end - program->li.id_end);

   pcb->user_context.sp = sp;
   sp = malloc(size);

   cpp = cpp + 1;
   *(int*)cpp = program->argc;
   for (int i = 0; i < program->argc; i++) {
      char* arg = program->argv[i];
      int len = strlen(arg);
      cpp = cpp + 1;
      *cpp = cp;
      memcpy(sp, arg, len);
      memcpy(cp, sp, len);
      cp = cp + len + 1;
      sp = sp + len + 1;
   }
   cpp = cpp + 1;
   *cpp = 0;
   cpp = cpp + 1;
   *cpp = 0;

   for (int pgno = 0; pgno < program->li.t_npg; pgno++) {
       int pfn = pcb->ptbr[pgno].pfn;
       MapPage(pcb->ptbr, pgno, pfn, PROT_READ | PROT_EXEC);
   }
   WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

   pcb->user_context.pc = program->li.entry;
   program->_pcb = pcb;
   program->_pcb->kstack = InitializeKernelStackProcess();
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
