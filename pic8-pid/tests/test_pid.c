/**
 * @file    test_pid.c
 * @brief   Phase 0 placeholder. Phase 1 replaces this with the real
 *          host tests covering the algorithm cases in the plan.
 */

#include "pid.h"

int main(void)
{
    /* Phase 0: prove the header and the empty pic8-pid library link
     * together against the pic8-math host reference build. The real
     * test cases land in Phase 1. */
    pid_t pid;
    pid_init(&pid, 0, 0, 0, 0, 0);
    pid_set_mode(&pid, PID_MODE_AUTO);
    (void)pid_update(&pid, 0, 0);
    return 0;
}
