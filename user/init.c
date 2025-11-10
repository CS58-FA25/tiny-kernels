#include <hardware.h>
#include <yuser.h>

int main(int argc, char** argv) {
    while (1) {
	int pid = GetPid();
        TracePrintf(1, "init, my pid is %x\n");
	Delay(5);
    }
    return 0;
}
