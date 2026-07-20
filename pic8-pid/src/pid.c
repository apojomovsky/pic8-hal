/**
 * @file    pid.c
 * @brief   Fixed-point PID controller, see pid.h.
 *
 * @details
 *   The control algorithm and its rationale live in pid.h's file-level doc
 *   comment and in docs/ARCHITECTURE.md. Phase 0 of the implementation
 *   plan is scaffolding only -- the real algorithm lands in Phase 1.
 *   This stub exists so the CMake host build (lib + test + example) can
 *   wire up and the signed-shift probe can run; the real body replaces
 *   every function in Phase 1.
 */

#include "pid.h"

void pid_init(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8,
              int16_t out_min, int16_t out_max)
{
    (void)pid; (void)kp_q8; (void)ki_q8; (void)kd_q8;
    (void)out_min; (void)out_max;
}

void pid_reset(pid_t *pid)
{
    (void)pid;
}

void pid_set_gains(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8)
{
    (void)pid; (void)kp_q8; (void)ki_q8; (void)kd_q8;
}

void pid_set_mode(pid_t *pid, pid_mode_t mode)
{
    (void)pid; (void)mode;
}

void pid_set_manual_output(pid_t *pid, int16_t value)
{
    (void)pid; (void)value;
}

int16_t pid_update(pid_t *pid, int16_t setpoint, int16_t measurement)
{
    (void)pid; (void)setpoint; (void)measurement;
    return 0;
}
