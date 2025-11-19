#include <hardware.h>
#include <yuser.h>
/**
 * Description: Program to be loaded by executed by Exec call in test2.c
*/

int main(int argc, char *argv[]) {
    TracePrintf(0, "Hello! This is the new program now! Completely replaced the old program!\n");
    TracePrintf(0, "I'm going to print the word that the user passed into the argument!\n");
    char *message = argv[1];
    TracePrintf(0, "It seems like it's: %s!\n", message);
    TracePrintf(0, "Going to exit now (exit status reaped should be 1)!\n");
    return 1;
}