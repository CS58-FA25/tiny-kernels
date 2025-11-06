#include <hardware.h>
#include <yuser.h>

int main(int argc, char** argv) {
    int pid = Fork();
    if (pid == 0) {
        int child_pid = GetPid();
        while (1) {
            TracePrintf(1, "Hi this is child process! my pid is %d.\n", child_pid);
            Delay(3);
        }
    } else {
        int parent_pid = GetPid();
        while (1) {
            TracePrintf(1, "Hi, this is parent process! My pid is %d. My child's pid is %d.\n", parent_pid, pid);
            Delay(5);
        }
    }
    // while (1) {
    // TracePrintf(1, "Testing syscall GetPid!\n");
	// int pid = GetPid();
    // TracePrintf(1, "This is init! My PID is %d\n", pid);
    // TracePrintf(1, "Testing syscall Delay!\n");
	// Delay(5);
    // }
    return 0;
}
