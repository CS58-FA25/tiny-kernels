#include <hardware.h>
#include <yuser.h>

int main(int argc, char** argv) {
    while (1) {
    TracePrintf(1, "Testing syscall GetPid!\n");
	int pid = GetPid();
    TracePrintf(1, "This is init! My PID is %d\n", pid);
    TracePrintf(1, "Testing syscall Delay!\n");
	Delay(5);
    }
    return 0;
}
