// A note on TtyPrintf: I see that it's declared in the header that has
// most of the syscalls listed
//
// However, I also see that it's something that is already implemented
// in libyuser.a
// So I didn't include it here
// If I were to incluse it, it would end up being a wrapper function for TtyWrite that handles the arguments first


int TtyRead(int tty_id, void *buf, int len) {
   // get tty object from a global tty devices map
   // if tty does not exist (out of range), return error
   //
   // acquire access to read from the tty
   //
   // check how much data is in the tty
   // if there is no data, WAIT until there is data (wait until trap read trap has informed our tty object)
   //
   // if data > len, read len amount of data
   // remove that data out of the tty object, as the top of the data buffer is from len->onward
   //
   // if len > data, read data amount. clear tty
   // write the read data into the buf
   // 
   // release the tty so that the tty can be written to
   //
   // return the number of bytes read if the read is successful
   // if the read is not successful, return error
}

int TtyWrite(int tty_id, void *buf, int len) {
   // get tty object from a global tty device map
   // if the tty does not exist (out of range), return error
   // 
   // check that TERMINAL_MAX_LINE is greater than len
   // if len is greater than the max amount of data that can be written (`TERMINAL_MAX_LINE`), return error
   //
   // while the tty is being written to, limit tty access
   // if the tty is in use, wait for the tty
   // 
   // write data of size `len` from `buf` into the tty, to be read by the call `TtyRead`
   // (wait until tty write trap has returned success)
   // return number of bytes written
   //
   // allow tty access for the next read/write call
   //
   // if write was unsuccessful, return error
}

