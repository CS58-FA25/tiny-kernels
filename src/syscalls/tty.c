#include "syscalls/tty.h"
#include "terminal.h"

#include <ykernel.h>

// A note on TtyPrintf: I see that it's declared in the header that has
// most of the syscalls listed
//
// However, I also see that it's something that is already implemented
// in libyuser.a
// So I didn't include it here
// If I were to incluse it, it would end up being a wrapper function for TtyWrite that handles the arguments first


int TtyRead(int tty_id, void *buf, int len) {
    terminal_t* term = GetTerminal(tty_id);
    if (!term || !ValidateTerminal(term)) {
       return ERROR;
    }
    if (TerminalRead(term, buf, len) > ERROR) {
       return SUCCESS;
    }
    return ERROR;
}

int TtyWrite(int tty_id, void *buf, int len) {
    terminal_t* term = GetTerminal(tty_id);
    if (!term || !ValidateTerminal(term)) {
       return ERROR;
    }
    if (TerminalWrite(term, buf, len) > ERROR) {
       return SUCCESS;
    }
    return ERROR;
}

