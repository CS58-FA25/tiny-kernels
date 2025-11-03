#ifndef PROGRAM_H
#define PROGRAM_H

#include "proc.h"
#include <load_info.h>

#define MAX_PROGRAMS 1

typedef struct _program {
   PCB* _pcb;
   char* file;
   int argc;
   char** argv;
   unsigned long base; /* Address */
   struct load_info li;
} Program;

#ifdef DEBUG

// TODO: This exists for debugging right now
typedef struct _program_listing {
   Program ** progs;
   unsigned int nprogs;
   unsigned int max_progs;
} ProgramListing;

ProgramListing* GetProgramListing();

// Function is split out during debug
int RunProgram(int, UserContext*);
#else

static int _RunProgram(Program*, UserContext*);

#endif


int LoadProgram(char*, int, char**);
int LoadProgramFromArguments(char**);

#endif
