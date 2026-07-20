/**
 * @file    pid.h
 * @brief   Vendor-agnostic, single-loop, fixed-point PID controller with
 *          anti-windup, derivative-on-measurement, and bumpless auto/manual
 *          transfer.
 *
 * @details
 *   One caller-owned `pid_t` instance per control loop, initialized once,
 *   then stepped once per fixed-period control cycle via `pid_update()`. A
 *   PID controller is arithmetic on plain data, not a hardware operation,
 *   so this module needs no per-family backend and no inline asm: `pid.c`
 *   is one file, compiles unchanged for host, PIC16, and PIC18, and the
 *   host unit suite tests the exact code that ships on-target.
 *
 *   The one real dependency is `pic8-math`, for the 16x16->32 signed
 *   multiply (`pic_math_mul_s16`) the Q8.8 fixed-point math needs. On PIC16
 *   (no hardware multiply) that call is AN526's shift-add asm; on PIC18 it
 *   is the hardware `MULWF` path; on host it is the portable-C reference.
 *   `pid.c` never knows or cares which. `pid.h` itself stays a two-`#include`
 *   file (`<stdint.h>`, `<stdbool.h>`), dependency-free.
 *
 *   The control algorithm is fixed-point throughout (Q8.8, `int16_t`/`int32_t`
 *   holding `value * 256`). Gains are caller-pre-scaled by the sample
 *   period `Ts` at configuration time, so `pid_update` needs no division
 *   (division is the most expensive primitive `pic8-math` provides on
 *   PIC16) and the module has zero dependency on `pic8-tick` or any
 *   timebase. See `docs/API.md` for the exact Kp/Ki/Kd/Ts -> kp_q8/ki_q8/kd_q8
 *   conversion.
 *
 *   Anti-windup is clamping: the integrator is hard-capped to
 *   `[out_min << 8, out_max << 8]` after every accumulation, so it can
 *   recover immediately when the error reverses sign (no separate
 *   `Kt`/back-calculation tracking gain, no conditional-integration flag;
 *   the only two design alternatives considered, both rejected as a
 *   gratuitous extra knob). The derivative term is computed from
 *   `-d(measurement)/dt`, not `d(error)/dt`, to eliminate setpoint-step
 *   derivative kick by construction (no mode flag). Bumpless auto/manual
 *   transfer is built in: while in MANUAL, the integrator is back-calculated
 *   to whatever value would have made the AUTO output equal the current
 *   manual output, so resuming AUTO is a no-op.
 *
 *   Preconditions documented (not runtime-guarded, consistent with how
 *   `pic_math` documents its own scaling assumptions): `|setpoint -
 *   measurement| <= 32767` (so `error` fits in `int16_t`); caller calls
 *   `pid_update` at a fixed, known period `Ts`; `out_min <= out_max`.
 */

#ifndef PID_H
#define PID_H

#include <stdint.h>
#include <stdbool.h>

/** Auto / manual mode selector (see @ref pid_set_mode, @ref pid_update). */
typedef enum {
    PID_MODE_MANUAL = 0,
    PID_MODE_AUTO,
} pid_mode_t;

/**
 * @brief  One PID control loop, caller-owned storage.
 *
 * Fields are written by `pid_init` / `pid_set_*` and by `pid_update` (the
 * integrator, the D-term history, and the mode's back-calculation). The
 * caller reads them through the API; reading them directly is unsupported
 * but harmless.
 */
typedef struct {
    int16_t    kp_q8, ki_q8, kd_q8;   /* Q8.8 discrete-time gains; ki_q8 already
                                        * includes *Ts, kd_q8 already includes /Ts */
    int16_t    out_min, out_max;      /* actuator output clamp; out_min <= out_max */

    int32_t    integrator_q8;         /* Q8.8 accumulated integral term, always kept
                                        * clamped to [out_min<<8, out_max<<8] */
    int16_t    prev_measurement;      /* for derivative-on-measurement */
    bool       have_prev_measurement; /* false until the first pid_update() call
                                        * since init/reset -- gates the D term so
                                        * there is no spurious kick on the first call */
    bool       skip_next_i_increment; /* true after a MANUAL pid_update that
                                        * back-calculated the integrator: the
                                        * next AUTO call must NOT apply the I
                                        * increment, so its output equals the
                                        * held manual value exactly (the
                                        * bumpless-transfer property). Cleared
                                        * by the AUTO call that consumes it. */
    pid_mode_t mode;
    int16_t    manual_output;         /* caller-set target output while mode == MANUAL */
} pid_t;

/**
 * @brief  Initialize a PID instance.
 *
 * Stores the gains and the output clamp range, sets `mode = PID_MODE_AUTO`,
 * zeroes the integrator, clears the D-term history, and zeroes
 * `manual_output`. The first `pid_update()` call therefore behaves like
 * the first call on a freshly-initialized loop: P+I from zero, D zeroed
 * by the no-prev-measurement gate.
 *
 * @param  pid       the instance (caller-owned storage).
 * @param  kp_q8     Q8.8 proportional gain (= round(Kp * 256)).
 * @param  ki_q8     Q8.8 *integral* gain, already pre-multiplied by the
 *                   sample period Ts (= round(Ki * Ts * 256)).
 * @param  kd_q8     Q8.8 *derivative* gain, already pre-divided by the
 *                   sample period Ts (= round(Kd / Ts * 256)).
 * @param  out_min   minimum allowed output (clamp rail; precond: out_min <= out_max).
 * @param  out_max   maximum allowed output (clamp rail).
 */
void pid_init(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8,
              int16_t out_min, int16_t out_max);

/**
 * @brief  Zero the integrator and clear the D-term history.
 *
 * For recovering from an external fault (e-stop, sensor dropout) without
 * losing tuning. Keeps `kp_q8`/`ki_q8`/`kd_q8`/`out_min`/`out_max`/`mode`
 * untouched. The next `pid_update()` call behaves like the very first call
 * on a freshly-`pid_init`'d instance despite the gains being unchanged
 * from before the reset.
 */
void pid_reset(pid_t *pid);

/**
 * @brief  Replace the three gains, leaving the integrator, D-term history,
 *         and mode untouched.
 */
void pid_set_gains(pid_t *pid, int16_t kp_q8, int16_t ki_q8, int16_t kd_q8);

/**
 * @brief  Switch between AUTO and MANUAL. Does NOT reset the integrator
 *         or D-term history; switching mode is not a fault, and bumpless
 *         transfer depends on integrator state carrying across the switch.
 */
void pid_set_mode(pid_t *pid, pid_mode_t mode);

/**
 * @brief  Set the target output used while `mode == PID_MODE_MANUAL`.
 *         Only consulted by `pid_update()` while in MANUAL; ignored in AUTO.
 *         Call every cycle the operator/supervisor wants a new manual
 *         output in effect.
 */
void pid_set_manual_output(pid_t *pid, int16_t value);

/**
 * @brief  Step the controller once. Call once per fixed control-loop
 *         period, in EITHER mode; this is the single per-cycle entry
 *         point, there is no separate "manual step" function.
 *
 * In AUTO: integrates `ki_q8 * error`, adds the P, I, and -d(measurement)/dt
 * terms, and returns `clamp((P+I+D) >> 8, out_min, out_max)`. The integrator
 * is itself clamped to `[out_min << 8, out_max << 8]` so it can recover
 * immediately when the error reverses sign.
 *
 * In MANUAL: returns `clamp(manual_output, out_min, out_max)`, and
 * back-calculates the integrator so that resuming AUTO is bumpless (the
 * next AUTO `pid_update` returns the same output this MANUAL call did,
 * given the same setpoint/measurement).
 *
 * Precondition: `|setpoint - measurement| <= 32767` (so `error` fits in
 * `int16_t`). Not runtime-checked; violating it indicates a misconfigured
 * caller, not a normal runtime condition.
 *
 * @return  the clamped output, always in `[out_min, out_max]`.
 */
int16_t pid_update(pid_t *pid, int16_t setpoint, int16_t measurement);

#endif /* PID_H */
