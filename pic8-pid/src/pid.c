/**
 * @file    pid.c
 * @brief   Fixed-point PID controller, see pid.h.
 *
 * @details
 *   Implements the algorithm spec from docs/pic8-pid-plan.md verbatim:
 *   one implementation, no per-family variant, no inline asm. The only
 *   operation that ever benefits from a family-specific backend is the
 *   16x16->32 signed multiply `pic_math_mul_s16`, which `pid.c` calls
 *   without knowing or caring which of pic8-math's three backends
 *   (host reference / PIC16 AN526 shift-add / PIC18 hardware MULWF) the
 *   build linked in.
 *
 *   The Q8.8 `sum_q8 >> 8` truncation relies on signed `>>` being
 *   arithmetic (sign-extending). That precondition is confirmed for
 *   every target this module ships on: gcc/clang on host, XC8 v3.10 on
 *   PIC16F87XA, and XC8 v3.10 on PIC18F2455. See Phase 0's commit
 *   message and docs/ARCHITECTURE.md for the probe detail and the
 *   generated-instruction sequence confirming it.
 *
 *   State invariants the code maintains (the tests assert these):
 *     - `pid->integrator_q8` is always in `[out_min << 8, out_max << 8]`
 *       after `pid_init`, after every `pid_update` (both modes), and
 *       after `pid_reset`. Anti-windup by clamping, not by back-
 *       calculation or conditional integration.
 *     - `pid->have_prev_measurement` is false after `pid_init` /
 *       `pid_reset` and true after the first `pid_update` call. The
 *       derivative-on-measurement term is exactly zero on that first
 *       call.
 *     - The return value of `pid_update` is always in
 *       `[out_min, out_max]`, regardless of which term (P, I, or D)
 *       would otherwise have pushed `sum_q8` outside that range.
 *     - Switching AUTO -> MANUAL -> AUTO is bumpless: if the same
 *       setpoint/measurement is presented before and after the switch,
 *       the output does not change at the switch point.
 */

#include "pid.h"
#include "pic_math.h"

void pid_init(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8,
              int16_t out_min, int16_t out_max)
{
    pid->kp_q8   = kp_q8;
    pid->ki_q8   = ki_q8;
    pid->kd_q8   = kd_q8;
    pid->out_min = out_min;
    pid->out_max = out_max;

    pid->integrator_q8         = 0;
    pid->prev_measurement      = 0;
    pid->have_prev_measurement = false;
    pid->skip_next_i_increment = false;
    pid->mode                  = PID_MODE_AUTO;
    pid->manual_output         = 0;
}

void pid_reset(pid_t *pid)
{
    /* Zero the integrator and the D-term history; keep gains, clamp,
     * and mode untouched (the plan: 'pid_reset' is for fault recovery,
     * not a full re-init). Also clear the MANUAL-bumpless skip flag,
     * since a reset is a fresh start and any prior MANUAL->AUTO
     * handoff is no longer relevant. */
    pid->integrator_q8         = 0;
    pid->have_prev_measurement = false;
    pid->skip_next_i_increment = false;
}

void pid_set_gains(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8)
{
    pid->kp_q8 = kp_q8;
    pid->ki_q8 = ki_q8;
    pid->kd_q8 = kd_q8;
}

void pid_set_mode(pid_t *pid, pid_mode_t mode)
{
    pid->mode = mode;
}

void pid_set_manual_output(pid_t *pid, int16_t value)
{
    pid->manual_output = value;
}

int16_t pid_update(pid_t *pid, int16_t setpoint, int16_t measurement)
{
    /* P term: Kp * error, in Q8.8. The 16-bit Q8.8 * 16-bit signed
     * fits in int32_t (max product ~ 1.07e9, well under INT32_MAX), so
     * the multiply itself does not need an overflow guard, as the
     * plan documents. */
    int16_t error = (int16_t)(setpoint - measurement);
    int32_t p_q8  = pic_math_mul_s16(pid->kp_q8, error);

    /* D term: derivative-on-measurement, negated. Zero on the very
     * first call (no previous measurement) so a setpoint step at
     * boot does not produce a spurious derivative kick. The sign flip
     * (using -d(measurement)/dt instead of d(error)/dt) is what
     * eliminates setpoint-step kick in steady operation. */
    int16_t dmeas;
    if (!pid->have_prev_measurement) {
        dmeas = 0;
        pid->have_prev_measurement = true;
    } else {
        dmeas = (int16_t)(measurement - pid->prev_measurement);
    }
    pid->prev_measurement = measurement;
    int32_t d_q8 = -pic_math_mul_s16(pid->kd_q8, dmeas);

    /* Q8.8 clamp rails for the integrator (the only term that can
     * accumulate across calls; clamping it here is the anti-windup). */
    int32_t out_min_q8 = (int32_t)pid->out_min << 8;
    int32_t out_max_q8 = (int32_t)pid->out_max << 8;

    if (pid->mode == PID_MODE_MANUAL) {
        /* MANUAL: output is the caller's manual_output, clamped to the
         * actuator range. The integrator is back-calculated so the
         * next AUTO call (with the same setpoint/measurement) returns
         * the same clamped manual value, making the AUTO -> MANUAL ->
         * AUTO round-trip bumpless. The same clamp that anti-windups
         * AUTO applies here too: the back-calculated integrator must
         * also stay in [out_min_q8, out_max_q8] so the next AUTO call
         * sees a valid starting state.
         *
         * Set skip_next_i_increment so the very next AUTO call does
         * NOT apply its I increment. That call's output then equals
         * (P + back-calcd_I + D) >> 8, which is exactly the manual
         * value we just held (the integrator was back-calculated to
         * make that so). Without the skip, the integrator would grow
         * by ki*error on the first AUTO call and the output would
         * jump by that amount, breaking the held-output-equals-
         * first-auto-output handoff the plan's test asserts. The flag
         * is single-shot: consumed by the AUTO call that uses it. */
        int16_t output = pid->manual_output;
        if (output < pid->out_min) { output = pid->out_min; }
        if (output > pid->out_max) { output = pid->out_max; }
        pid->integrator_q8 = ((int32_t)output << 8) - p_q8 - d_q8;
        if (pid->integrator_q8 < out_min_q8) { pid->integrator_q8 = out_min_q8; }
        if (pid->integrator_q8 > out_max_q8) { pid->integrator_q8 = out_max_q8; }
        pid->skip_next_i_increment = true;
        return output;
    }

    /* AUTO: accumulate Ki * error into the integrator (unless the
     * previous MANUAL call asked us to skip exactly this step for a
     * bumpless handoff), then sum the three terms and shift back to
     * int16_t with the final output clamp. */
    if (!pid->skip_next_i_increment) {
        pid->integrator_q8 += pic_math_mul_s16(pid->ki_q8, error);
    }
    pid->skip_next_i_increment = false;  /* single-shot: consumed */

    if (pid->integrator_q8 < out_min_q8) { pid->integrator_q8 = out_min_q8; }
    if (pid->integrator_q8 > out_max_q8) { pid->integrator_q8 = out_max_q8; }

    int32_t sum_q8 = p_q8 + pid->integrator_q8 + d_q8;
    int16_t output = (int16_t)(sum_q8 >> 8);
    if (output < pid->out_min) { output = pid->out_min; }
    if (output > pid->out_max) { output = pid->out_max; }
    return output;
}
