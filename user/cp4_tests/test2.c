#include <hardware.h>
#include <yuser.h>

/**
 * Description: Tests GetPid, Fork, Exec and Wait.
*/

int main(int argc, char* argv[]) {
    int pid = Fork();
    if (pid == 0) {
        int child_pid = GetPid();
        TracePrintf(0, "Hi this is child process! my pid is %d.\n", child_pid);
        TracePrintf(0, "Child process is going to be replaced by the program ./user/test_program_exec!\n");
        char *message = "quack!";
        char *program_name = "./user/cp4_tests/test_program_exec";
        char *argvec[] = {program_name, message, NULL};
        Exec(program_name, argvec);
        TracePrintf(0, "If Exec succeded, we shouldn't see this!.\n");
    } else {
        int parent_pid = GetPid();
        int status;
        int reaped_pid = Wait(&status);
        if (reaped_pid != ERROR) {
            TracePrintf(0, "Parent process PID %d succesfully reaped zombie child process %d! exit status is %d\n", parent_pid, pid, status);
        }
        else {
            TracePrintf(0, "Parent process PID %d failed to reap zombie child process!\n");
        }
    }
    return 0;

}