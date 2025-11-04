// // This one get's a special file, because although it is under Process Coordination
// // in Yalnix, I do not believe it is in Linux
// // Also my mental association with brk is with sbrk and malloc, so this is just easier for
// // peace of mind

// int Brk(void *addr) {
//     // prev_brk = store previous brk number
//     // if addr % PAGESIZE == 0
//     // new_brk = addr
//     // else
//     // set lowest memory location to next multiple
//     // new_brk = (int) ((addr / PAGESIZE) + 1) * PAGESIZE)
//     // if prev_brk < new_brk
//     // allocate prev_brk to new_brk
//     // else
//     // deallocate prev_brk to new_brk
//     // return 0 if works, error if there was an error (not enough memory, etc)
// }
