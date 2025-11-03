#include <hardware.h>
#include <yuser.h>

int main(int argc, char** argv) {
    while (0) {
	int pid = GetPid();
        TracePrintf(1, "init, my pid is %x\n");
	Pause();
    }
    return 0;
}
