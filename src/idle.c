void RunIdle(void) {
    while (1) {
        TracePrintf(1, "Running Idle Process...\n");
        Pause();
    }
}