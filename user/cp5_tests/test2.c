#include <hardware.h>
#include <yuser.h>
#include <string.h>

// Helper macro to print results
#define ASSERT_EQ(val, expected, msg) \
    if ((val) != (expected)) { \
        TracePrintf(0, "FAIL: %s. Got %d, expected %d\n", msg, val, expected); \
    } else { \
        TracePrintf(0, "PASS: %s\n", msg); \
    }

/*
 * TestBasicWrite
 * Simply writes a short string to Terminal 0.
 */
void TestBasicWrite() {
    TracePrintf(0, "User: Starting Basic Write Test\n");
    char *str = "Testing Basic Write: Hello World!\n";
    int len = strlen(str);
    
    int rc = TtyWrite(0, str, len);
    ASSERT_EQ(rc, len, "Basic TtyWrite bytes written");
}

/*
 * TestBasicRead
 * Prompts the user to type something, blocks, then echos it back.
 * Verifies: Read Blocking, TrapReceiveHandler, Read Buffer shifting.
 */
void TestBasicRead() {
    TracePrintf(0, "User: Starting Basic Read Test\n");
    char buf[100];
    char *prompt = ">>> Type something (max 99 chars) and press Enter: ";
    
    TtyWrite(0, prompt, strlen(prompt));

    memset(buf, 0, 100);
    
    TracePrintf(0, "User: Calling TtyRead (should block until you type)...\n");
    int rc = TtyRead(0, buf, 99);
    
    TracePrintf(0, "User: Read returned %d bytes. Content: '%s'\n", rc, buf);
    
    char *echo = ">>> You typed: ";
    TtyWrite(0, echo, strlen(echo));
    TtyWrite(0, buf, rc);
    TtyWrite(0, "\n", 1);
}

/*
 * TestLongWrite
 * Writes a buffer much larger than TERMINAL_MAX_LINE.
 * Verifies: TrapTransmitHandler looping (Chunking logic) and I/O Waiting.
 */
void TestLongWrite() {
    TracePrintf(0, "User: Starting Long Write Test\n");
    int size = 1024;
    char big_buf[1025];
    
    // Fill buffer with a visible pattern
    for (int i = 0; i < size; i++) {
        big_buf[i] = 'A' + (i % 26); // ABC...ABC...
    }
    big_buf[size] = '\n'; // End with newline
    
    TracePrintf(0, "User: Writing %d bytes...\n", size);
    int rc = TtyWrite(0, big_buf, size);
    
    ASSERT_EQ(rc, size, "Long TtyWrite full length");
}

/*
 * TestBadArgs
 * Sends invalid arguments to TtyRead/Write.
 * Verifies: KernelTrapHandler validation logic (CheckBuffer).
 */
void TestBadArgs() {
    TracePrintf(0, "User: Starting Bad Arguments Test\n");
    char buf[10];
    
    // Invalid TTY ID
    TracePrintf(0, "User: Testing Invalid TTY ID (NUM_TERMINALS)...\n");
    int rc = TtyWrite(NUM_TERMINALS, buf, 5);
    if (rc == ERROR) TracePrintf(0, "PASS: Invalid TTY ID rejected.\n");
    else TracePrintf(0, "FAIL: Invalid TTY ID accepted (rc=%d)\n", rc);

    // NULL Buffer
    TracePrintf(0, "User: Testing NULL Buffer...\n");
    rc = TtyWrite(0, NULL, 5);
    if (rc == ERROR) TracePrintf(0, "PASS: NULL buffer rejected.\n");
    else TracePrintf(0, "FAIL: NULL buffer accepted (rc=%d)\n", rc);

    // Invalid Length
    TracePrintf(0, "User: Testing Negative Length...\n");
    rc = TtyWrite(0, buf, -5);
    if (rc == ERROR) TracePrintf(0, "PASS: Negative length rejected.\n");
    else TracePrintf(0, "FAIL: Negative length accepted (rc=%d)\n", rc);
}

/*
 * TestConcurrency
 * Forks a child process. Both Parent and Child try to write simultaneously.
 * Verifies: Terminal Locking (terminal->in_use) and Queueing (blocked_writers).
 * Output should NOT be interleaved characters.
 * It should be full blocks ("Parent... Child...").
 */
void TestConcurrency() {
    TracePrintf(0, "User: Starting Concurrency Test\n");
    
    int pid = Fork();
    if (pid == 0) {
        // Child Process
        char *c_msg = "CHILD: Writing line 1... (Waiting for lock?)\n";
        TtyWrite(0, c_msg, strlen(c_msg));
        
        char *c_msg2 = "CHILD: Writing line 2... (Holding lock?)\n";
        TtyWrite(0, c_msg2, strlen(c_msg2));
        
        Exit(0);
    } else {
        // Parent Process
        char *p_msg = "PARENT: Writing line A... (Waiting for lock?)\n";
        TtyWrite(0, p_msg, strlen(p_msg));
        
        char *p_msg2 = "PARENT: Writing line B... (Holding lock?)\n";
        TtyWrite(0, p_msg2, strlen(p_msg2));
        
        Wait(NULL);
        TracePrintf(0, "User: Concurrency Test Done.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        TracePrintf(0, "Usage: test_tty [write | read | long | bad | concurrent]\n");
        Exit(1);
    }

    if (strcmp(argv[1], "write") == 0) {
        TestBasicWrite();
    } 
    else if (strcmp(argv[1], "read") == 0) {
        TestBasicRead();
    } 
    else if (strcmp(argv[1], "long") == 0) {
        TestLongWrite();
    } 
    else if (strcmp(argv[1], "bad") == 0) {
        TestBadArgs();
    } 
    else if (strcmp(argv[1], "concurrent") == 0) {
        TestConcurrency();
    } 
    else {
        TracePrintf(0, "Unknown test: %s\n", argv[1]);
        TracePrintf(0, "Usage: test_tty [write | read | long | bad | concurrent]\n");
        Exit(1);
    }

    TracePrintf(0, "User: Test program exiting normally.\n");
    Exit(0);
}