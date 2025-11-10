
int PipeInit (int * pipe_idp) {
   // get a pipe id from a list of pipes based on the max and current number of pipes
   // if cannot create pipe, return error
    //
   // create a pipe structure
   // pipe buffer is a predefined size, use the value stored in that macro to allocate
   // pipe->buf (char buf[MAX_PIPE_DATA_LEN])
   // set pipe state to open
   // set any other attributes (TBD based on implementation later)
   // create a read/write lock for the pipe
   //
   // set pipe_idp to the pipe id
   // return 0 on success
}

int PipeRead(int pipe_id, void *buf, int len) {
   // find pipe from pipe id
   // if pipe does not exist, return error
   //
   // Acquire(pipe->lock);
   // 
   // FOR READS:
   // handled in a while loop, read char by char to be aware of pipe, process, state
   // read location & write location are used to keep the buffer cyclical (avoids shifting the data in the buffer after operations as well)
   // awareness ex: if the process dies, so should the pipe. the pipe needs to be aware
   //
   // if len > pipe->buf_size
   // read entire pipe buffer
   // else
   // read len from pipe
   //
   // set current read location
   // 
   // Release(pipe->lock);
   // return nbytes (number of bytes read)
   // 
}

int PipeWrite(int pipe_id, void *buf, int len) {
   // find pipe from pipe id
   // if pipe does not exist, return error
   //
   // Acquire(pipe->lock);
   //
   // FOR WRITES:
   // also handled in a while loop, written char by char to be aware of state, likeness of stream (like above, in PipeRead)
   //
   // if len > pipe->buf_size
   // write pipe->buf_size of data buf (cuts off all of the request data to be written!)
   // else
   // write len of data buf (return value will match len)
   //
   // set current write location
   //
   // Release(pipe->lock); 
   // return nbytes
}
