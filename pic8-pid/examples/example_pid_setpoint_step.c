/**
 * @file    example_pid_setpoint_step.c
 * @brief   Host-only setpoint-step + manual/auto transfer demo for pic8-pid.
 *
 * @details
 *   Pure-host, zero HAL dependency, no XC8 build (the example's purpose
 *   is to make the algorithm's behavior visible in logged output, not
 *   to be flashed). Mirrors pic8-fsm's example_traffic_light in scope
 *   and shape: a single-file demo of one module's central use case.
 *
 *   The setup:
 *
 *     - A simple integer first-order-lag "plant":
 *         measurement += (output - measurement) / N   (N = 4)
 *       no floats anywhere, consistent with this repo's no-float
 *       convention even though the example itself is host-only.
 *       N=4 makes the plant time constant about 4 control steps.
 *
 *     - A PID loop with Kp, Ki, Kd chosen so the step response is
 *       underdamped and visibly oscillates if anti-windup is not
 *       doing its job. Output clamp is deliberately tight (e.g.
 *       [-150, 150]) so anti-windup engages on a setpoint jump
 *       that the unclamped integral would otherwise overshoot by.
 *
 *     - A setpoint step at the start, then a switch to MANUAL
 *       mid-run (operator wants to take over), then a switch back
 *       to AUTO (bumpless handoff). The log shows the integrator
 *       wind down naturally during AUTO, the held manual value
 *       during MANUAL, and the no-jump AUTO resume after.
 *
 *   The numbers in the log are not pinned to specific values
 *   (different Kp/Ki/Kd choices make the example easy to retune);
 *   the test of correctness is that anti-windup engages (the
 *   output saturates at the clamp during the step) and that the
 *   MANUAL->AUTO handoff is continuous (the first AUTO output
 *   after MANUAL matches the last MANUAL output exactly).
 */

#include "pid.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Q8.8 conversion helpers (host-side convenience, not part of the
 * library: the library takes pre-scaled Q8.8 gains, the example
 * picks them from a continuous-time design here). */
static int16_t q8(float x) { return (int16_t)(x * 256.0f); }

int main(void)
{
    /* Sample period Ts = 1 control step. Plant time constant ~ N=4
     * steps, so an aggressive design (visibly saturating the output
     * and engaging anti-windup) is:
     *   Kp ~ 2.0   (big step response, P alone would saturate)
     *   Ki ~ 0.5   (steady-state error gone in ~10 steps after
     *               anti-windup releases the integrator)
     *   Kd ~ 0.0   (the integer plant already gives some natural
     *               smoothing; Kd only sharpens noise here)
     * Output clamp tight on purpose so the integrator windup
     * (and anti-windup) is clearly visible during the initial step. */
    const int16_t kp_q8 = q8(2.0f);
    const int16_t ki_q8 = q8(0.5f);
    const int16_t kd_q8 = q8(0.0f);
    const int16_t out_min = -200;
    const int16_t out_max = 200;

    pid_t pid;
    pid_init(&pid, kp_q8, ki_q8, kd_q8, out_min, out_max);

    int16_t measurement = 0;
    int16_t setpoint    = 0;

    /* Phase 1: setpoint step from 0 to 100. Watch the plant rise,
     * the output saturate against the upper clamp briefly, and the
     * anti-windup keep the integrator in check so it doesn't have
     * to unwind once the plant catches up. With the integer plant's
     * N=4 lag, a steady-state output of 100 drives the measurement
     * to about 97 (the integer-quantization asymptote), and the
     * integrator winds up to remove the residual error. */
    setpoint = 100;
    printf("== Setpoint step 0 -> 100 (AUTO) ==\n");
    printf("step | setpoint | measurement | output | integrator_q8 | mode\n");
    for (int step = 0; step < 30; step++) {
        int16_t output = pid_update(&pid, setpoint, measurement);
        /* Plant: integer first-order lag, no floats. */
        int16_t plant_delta = (int16_t)((output - measurement) / 4);
        measurement = (int16_t)(measurement + plant_delta);
        printf("%4d | %8d | %11d | %6d | %13d | %s\n",
               step, setpoint, measurement, output,
               pid.integrator_q8,
               pid.mode == PID_MODE_AUTO ? "AUTO" : "MANUAL");
    }

    /* Phase 2: switch to MANUAL, set the operator's target. The PID
     * loop stops integrating; the operator drives the plant directly
     * via pid_set_manual_output each cycle. The integrator is
     * back-calculated to make the eventual AUTO resume bumpless. */
    printf("\n== Switch to MANUAL, operator takes over (target 50) ==\n");
    pid_set_mode(&pid, PID_MODE_MANUAL);
    pid_set_manual_output(&pid, 50);
    for (int step = 30; step < 40; step++) {
        int16_t output = pid_update(&pid, setpoint, measurement);
        int16_t plant_delta = (int16_t)((output - measurement) / 4);
        measurement = (int16_t)(measurement + plant_delta);
        printf("%4d | %8d | %11d | %6d | %13d | %s\n",
               step, setpoint, measurement, output,
               pid.integrator_q8,
               pid.mode == PID_MODE_AUTO ? "AUTO" : "MANUAL");
    }

    /* Bumpless-equivalence demonstration: with the plant frozen (no
     * step in between), a MANUAL call followed by an AUTO call at
     * the same setpoint/measurement returns the exact same output.
     * This is the "held output equals first AUTO output" property
     * the plan's test_bumpless_transfer asserts; the loop above
     * doesn't show it directly because the plant moves between
     * the MANUAL and AUTO calls, so the AUTO call sees a different
     * measurement. */
    int16_t frozen_setpoint = setpoint;
    int16_t frozen_measurement = measurement;
    int16_t new_manual = 75;
    printf("\n== Bumpless-equivalence demo (plant frozen) ==\n");
    printf("  setpoint=%d  measurement=%d\n", frozen_setpoint, frozen_measurement);
    pid_set_mode(&pid, PID_MODE_MANUAL);
    pid_set_manual_output(&pid, new_manual);
    int16_t held_out = pid_update(&pid, frozen_setpoint, frozen_measurement);
    printf("  MANUAL output = %d  (operator's target was %d)\n", held_out, new_manual);
    pid_set_mode(&pid, PID_MODE_AUTO);
    int16_t first_auto = pid_update(&pid, frozen_setpoint, frozen_measurement);
    printf("  first AUTO output = %d  %s\n", first_auto,
           (first_auto == held_out) ? "(matches MANUAL exactly -- bumpless)"
                                    : "(differs -- check the test suite's assert)");
    /* Second AUTO call: integrator resumes evolving, output diverges. */
    int16_t second_auto = pid_update(&pid, frozen_setpoint, frozen_measurement);
    printf("  second AUTO output = %d  (integrator resumes evolving)\n", second_auto);

    /* Phase 3: switch back to AUTO. The bumpless-transfer property
     * means the integrator was back-calculated during the last MANUAL
     * call so that, *if* the same setpoint/measurement were presented
     * to the first AUTO call, its output would equal the held manual
     * value exactly. In this run, the plant advances between the
     * MANUAL call and the first AUTO call, so the measurement is
     * slightly different and the AUTO output is too -- the
     * controller's internal state is continuous (no jump in the
     * integrator), but the new measurement changes P and therefore
     * the output. The same-property-at-same-input behavior is what
     * test_bumpless_transfer in tests/test_pid.c asserts. */
    printf("\n== Switch back to AUTO (controller state is continuous, "
           "but the plant moved one step) ==\n");
    pid_set_mode(&pid, PID_MODE_AUTO);
    for (int step = 40; step < 70; step++) {
        int16_t output = pid_update(&pid, setpoint, measurement);
        int16_t plant_delta = (int16_t)((output - measurement) / 4);
        measurement = (int16_t)(measurement + plant_delta);
        printf("%4d | %8d | %11d | %6d | %13d | %s\n",
               step, setpoint, measurement, output,
               pid.integrator_q8,
               pid.mode == PID_MODE_AUTO ? "AUTO" : "MANUAL");
    }

    return 0;
}
