#include <hardware.h>
#include <yuser.h>
#include <string.h>

/*
 * This function forces the stack to grow by allocating a large
 * array on the stack. According to the Yalnix spec, this should
 * trigger a memory trap, and the kernel should map all the
 * pages from the old stack base down to the new one.
 */
void TestStackGrowth() {
    TracePrintf(0, "User: Starting Stack Growth Test\n");
    TracePrintf(0, "User: Allocating a 3000-int array on the stack.\n");
    
    // This array is 3000 * 4 = 12000 bytes.
    // A page is 4096 bytes. This will require mapping
    // at least 3 new pages for the stack.
    volatile int big_array[3000];

    TracePrintf(0, "User: Writing to the start of the array...\n");
    big_array[0] = 1; // Write to the "middle" of the new stack area

    TracePrintf(0, "User: Writing to the end of the array...\n");
    big_array[2999] = 2; // Write to the lowest part of the new stack area

    if (big_array[0] == 1 && big_array[2999] == 2) {
        TracePrintf(0, "User: SUCCESS: Wrote and read from large stack array.\n");
    } else {
        TracePrintf(0, "User: FAILED: Data corruption in stack array.\n");
    }
    TracePrintf(0, "User: Stack Growth Test Complete \n");
}

/*
 * This function tests a fatal NULL pointer access.
 * The kernel should kill this process.
 */
void TestNullPtr() {
    TracePrintf(0, "User: Starting NULL Pointer Test \n");
    TracePrintf(0, "User: Attempting to write to address 0x0...\n");
    
    // This will fault. The kernel should see this is not
    // a valid stack growth and kill the process.
    int *ptr = NULL;
    *ptr = 42;

    // This line should NEVER be reached
    TracePrintf(0, "User: FAILED: NULL pointer write did not kill process!\n");
}

/*
 * This function tests a fatal write to read-only memory.
 * This should trigger YALNIX_ACCERR.
 * The kernel should kill this process.
 */
void TestReadOnlyWrite() {
    TracePrintf(0, "User: Starting Read-Only Write Test\n");
    TracePrintf(0, "User: Attempting to write to .text segment...\n");

    // String literals are in the .text segment
    char *text = "This is read-only text";
    text[0] = 'X'; // This should fault with YALNIX_ACCERR

    // This line should NEVER be reached
    TracePrintf(0, "User: FAILED: Write to .text did not kill process!\n");
}

/*
 * This function tests a fatal access below the heap.
 * The kernel should kill this process.
 */
void TestBelowHeap() {
    TracePrintf(0, "User: Starting Below-Heap Access Test\n");
    TracePrintf(0, "User: Attempting to write to address 0x1000...\n");

    // Address 0x1000 is in Region 1 but well below the heap
    // and stack. This should be a fatal YALNIX_MAPERR.
    int *ptr = (int *)0x1000;
    *ptr = 1337;

    // This line should NEVER be reached
    TracePrintf(0, "User: FAILED: Write to 0x1000 did not kill process!\n");
}

/*
 * Main test runner.
 * Use command-line arguments to select which test to run.
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        TracePrintf(0, "Usage: test_mem [stack | null | text | heap]\n");
        Exit(1);
    }

    if (strcmp(argv[1], "stack") == 0) {
        TestStackGrowth();
    } 
    else if (strcmp(argv[1], "null") == 0) {
        TestNullPtr();
    } 
    else if (strcmp(argv[1], "text") == 0) {
        TestReadOnlyWrite();
    } 
    else if (strcmp(argv[1], "heap") == 0) {
        TestBelowHeap();
    } 
    else {
        TracePrintf(0, "Unknown test: %s\n", argv[1]);
        TracePrintf(0, "Usage: test_mem [stack | null | text | heap]\n");
        Exit(1);
    }

    // If we get here, the test completed without being killed.
    TracePrintf(0, "User: Test program exiting normally.\n");
    Exit(0);
}