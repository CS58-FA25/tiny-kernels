#ifndef CLOCK_H
#define CLOCK_H

// From libhardware.so
extern uint32_t tick_count;

void ClockTrapHandler(UserContext* ctx);

#endif // CLOCK_H
