/**
 * @file    core/pic16f87xa_harness.h
 * @brief   Build-agnostic test/firmware harness — the one seam that lets
 *          the same example source build for the host simulator and a
 *          real XC8 target with no `#ifdef` in the example code.
 *
 * @details
 *   Two execution models meet here:
 *
 *     - Host simulator: a normal program. Simulated time advances only
 *       when pic16f87xa_sim_step() is pumped, the test must terminate,
 *       and it reports pass/fail to stdout.
 *
 *     - Real target: firmware. Hardware time advances on its own, main()
 *       never returns, and there is no stdout.
 *
 *   The harness hides that difference behind four functions, each with
 *   a host implementation (src/core/pic16f87xa_harness_sim.c, linked by
 *   the CMake build) and a target implementation
 *   (src/core/pic16f87xa_harness_target.c, linked by the XC8 Makefile).
 *   The build selects which file links — there is no `#ifdef` anywhere
 *   in the API or in example code that uses it.
 *
 *   Typical example shape (no `#ifdef` around code):
 *
 *       pic16f87xa_harness_init(SIM_CYCLES);
 *       // ... peripheral setup, identical on both builds ...
 *       for (uint32_t i = 0; pic16f87xa_harness_running(i); i++) {
 *           pic16f87xa_harness_tick();
 *           // ... work — same on both builds ...
 *       }
 *       pic16f87xa_harness_log("toggled %u\n", (unsigned)count);
 *       return pic16f87xa_harness_report(count >= 2);
 *
 *   On the target `harness_running` always returns 1, so the loop never
 *   exits and the log/report lines are unreachable. On the host the loop
 *   runs for `cycles` simulated cycles, then logs and returns the exit
 *   code.
 *
 *   Interrupt dispatch is also handled here: the host harness registers
 *   @ref pic16f87xa_dispatch_all_irqs as the single sim IRQ callback, so
 *   examples never call pic16f87xa_sim_set_irq_callback themselves. The
 *   real target gets the equivalent dispatch from its interrupt vector
 *   (src/core/pic16f87xa_isr_vector.c).
 */

#ifndef PIC16F87XA_HARNESS_H
#define PIC16F87XA_HARNESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Harness start-up. On the host this resets the simulated CPU and
 *         wires the sim IRQ callback to the shared dispatcher; `cycles`
 *         bounds the upcoming run. On a real target this is a no-op
 *         (the CPU starts itself; `cycles` is ignored).
 */
void pic16f87xa_harness_init(uint32_t cycles);

/**
 * @brief  Advance simulated time by one instruction cycle. On the host
 *         this pumps the simulator; on a real target time advances on its
 *         own, so this is a no-op. Call it inside any loop that must let
 *         time pass (the idle loop, or a hardware-ready wait).
 */
void pic16f87xa_harness_tick(void);

/**
 * @brief  Loop-continuation test. On the host returns 1 while the
 *         bounded run is in progress, 0 when it is over. On a real
 *         target always returns 1 (firmware runs forever).
 */
int pic16f87xa_harness_running(uint32_t iteration);

/**
 * @brief  printf-style log line. On the host this prints to stdout; on a
 *         real target it is a no-op (no stdout), so examples can log
 *         unconditionally without dragging in <stdio.h> or #ifdef.
 */
void pic16f87xa_harness_log(const char *fmt, ...);

/**
 * @brief  Map a pass/fail flag to a process exit code (0 = pass, 1 =
 *         fail). Identical on both builds — the only shared harness
 *         function — so it is inlined here.
 */
static inline int pic16f87xa_harness_report(int ok)
{
    return ok ? 0 : 1;
}

#ifdef __cplusplus
}
#endif

#endif /* PIC16F87XA_HARNESS_H */