#include <hardware.h>
#include <yuser.h>
/**
 * Description: Tests Fork, GetPid, Delay, Exit and Wait. Machines halts (doesn't switch to idle process
 *              if this program is the init process).
*/

int main(int argc, char** argv) {
    int pid = Fork();
    if (pid == 0) {
        int child_pid = GetPid();
        TracePrintf(1, "Hi this is child process! my pid is %d.\n", child_pid);
        TracePrintf(1, "Child process is going to execute a for loop and then exit!\n");
        for (int i = 0; i < 2; i++) {
            TracePrintf(1, "Process PID %d inside for loop, iteration %d\n", child_pid, i + 1);
            Delay(2);
        }
        TracePrintf(1, "Child process finished iterating! Now exiting!.\n");
        Exit(0);
    } else {
        int parent_pid = GetPid();
        int status;
        int reaped_pid = Wait(&status);
        if (reaped_pid != ERROR) {
            TracePrintf(1, "Parent process PID %d succesfully reaped zombie child process %d! exit status is %d\n", parent_pid, pid, status);
        }
        else {
            TracePrintf(1, "Parent process PID %d failed to reap zombie child process!\n");
        }
        Exit(0);
    }
    return 0;
}
