/**
 * @file    example_pid_setpoint_step.c
 * @brief   Phase 0 placeholder. Phase 2 replaces this with the real
 *          host-only setpoint-step + manual/auto transfer demo.
 *
 * The Phase 0 stub just initializes and steps a zero-gain PID a few times
 * to prove the example links against the Phase 0 library and pic8-math.
 */

#include "pid.h"

int main(void)
{
    pid_t pid;
    pid_init(&pid, 0, 0, 0, -100, 100);
    for (int i = 0; i < 4; i++) {
        (void)pid_update(&pid, 0, 0);
    }
    return 0;
}
