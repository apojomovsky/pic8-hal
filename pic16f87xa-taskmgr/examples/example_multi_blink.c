/**
 * @file    example_multi_blink.c
 * @brief   Four independent LED blinks at distinct rates driven by the
 *          cooperative task manager, one source for the host simulator
 *          and a real XC8 target, with no `#ifdef` in the code.
 *
 * @details
 *   The scheduler is wired to a ~10 ms Timer0 tick on a 20 MHz target
 *   (Fosc/4 = 5 MHz, prescaler 1:256, reload 61 → 195 counts × 51.2 µs ≈
 *   9.98 ms per tick). Four cooperative tasks blink LEDs on PORTB at distinct
 *   rates, showing:
 *
 *     - Periodic tasks at different periods (led_fast / led_med / led_slow).
 *     - Priority ordering: the supervisor (priority 0) runs first each round.
 *     - Runtime task spawning: the periodic supervisor spawns one-shot blip
 *       children at runtime.
 *     - One-shot tasks (period 0): a blip runs once, then its slot is freed.
 *
 *   PORTB is used so the example builds unchanged for every device in the
 *   family (the 28-pin PIC16F873A/876A have no PORTD/PORTE, but all have a
 *   full PORTB). Wiring (real target): an LED and resistor on each of
 *   RB0..RB3 to GND (active-high); a 20 MHz HS crystal on OSC1/OSC2.
 *
 *   On the host the harness bounds the run and the example reports per-LED
 *   toggle counts to stdout; the test passes when the fast LED toggled more
 *   often than the medium, the medium more than the slow, and at least one
 *   blip was spawned. On a real target the loop never returns and the LEDs
 *   blink forever.
 *
 *   The build links the host harness (sim) or the target harness (real time)
 *   via the HAL harness seam; `task_manager_run()` runs bounded on the host
 *   and forever on the target.
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "core/pic16_irq.h"
#include "core/pic8_harness.h"

#include "task_manager.h"

/* ───────────────────────── timing ────────────────────────────────── */

/**
 * @brief  Timer0 reload for a ~10 ms tick on a 20 MHz target.
 *         Fosc/4 = 5 MHz; prescaler 1:256 → a 51.2 µs count.
 *         256 - 195 = 61 → 195 counts × 51.2 µs ≈ 9.98 ms per tick.
 *         The sim reproduces the overflow/IRQ plumbing, not the wall-clock
 *         rate.
 */
#define TICK_RELOAD       61U
#define TICK_PRESCALER    TIMER0_PRESCALER_1_256

/** Host run length: long enough for the period-40 supervisor to fire, its
 *  one-shot blip to land, and the period-20 slow blink to toggle a few times.
 *  Override with -DSIM_CYCLES=. */
#ifndef SIM_CYCLES
#define SIM_CYCLES        4000000UL
#endif

/* Task periods in ticks (~10 ms each). Chosen for clearly distinct visible
 * rates on hardware and comfortable margins on the sim (counts need only be
 * monotonic: fast > med > slow >= 2, plus >= 1 blip). */
#define PERIOD_FAST        5U    /* ~50 ms  → RB0 (fastest) */
#define PERIOD_MED        10U    /* ~100 ms → RB1 */
#define PERIOD_SLOW       20U    /* ~200 ms → RB2 */
#define PERIOD_SUPERVISOR 40U    /* ~400 ms → spawns a blip on RB3 */

/* ───────────────────────── per-task state ─────────────────────────── */

/**
 * @brief  Per-LED state carried by each blink task through its @ref
 *         task_spawn argument. This is the idiomatic cooperative-scheduler
 *         pattern: a task cannot keep state in locals (they don't survive
 *         between calls), so it stores it in a struct it owns and reaches
 *         via `arg`.
 *
 *         Pointer-free so it banks in the 192 B of the 28-pin
 *         PIC16F873A/876A: `pin` is a bit index (0..7), and `count` is a
 *         uint8_t toggle counter.
 */
typedef struct {
    GPIO_TypeDef      port;   /* Which port the LED lives on. */
    uint8_t           pin;    /* Bit index 0..7 of the LED pin. */
    volatile uint8_t  count;  /* Toggle count (host assertion; uint8 is plenty). */
} blink_arg_t;

static blink_arg_t arg_fast = { GPIOB, 0U, 0U };   /* RB0 */
static blink_arg_t arg_med  = { GPIOB, 1U, 0U };   /* RB1 */
static blink_arg_t arg_slow = { GPIOB, 2U, 0U };   /* RB2 */
static blink_arg_t arg_blip = { GPIOB, 3U, 0U };   /* RB3 (spawned at runtime) */

/* ───────────────────────── tasks ──────────────────────────────────── */

/** Map a LED's pin index to a short label for the log. The string literals
 *  live in program space (not RAM), so this costs no data memory, important
 *  on the 192 B parts. Padded to 4 chars so the log columns line up. */
static const char *led_name(uint8_t pin)
{
    switch (pin) {
        case 0: return "fast";
        case 1: return "med ";
        case 2: return "slow";
        case 3: return "blip";
        default: return "?   ";
    }
}

/** Periodic blink task: toggle the LED described by @ref blink_arg_t, bump
 *  its toggle count, and log a line so the run is visible as it happens
 *  (a stream of dispatches rather than one summary at the end). The log is
 *  a no-op on a real target (no stdout), so this is free there. The same
 *  function serves all four LEDs, each carries its own arg. */
static void task_blink(void *arg)
{
    blink_arg_t *a = (blink_arg_t *)arg;
    HAL_GPIO_TogglePin(a->port, PIC8_BIT(a->pin));
    a->count++;
    pic8_harness_log("[t=%3u] %s  #%u\n",
                           (unsigned)task_manager_ticks(),
                           led_name(a->pin), (unsigned)a->count);
}

/** Periodic supervisor (priority 0, runs first each round): every
 *  PERIOD_SUPERVISOR ticks, spawn a fresh one-shot blip on RB3. This is a
 *  task spawning another task at runtime, from inside a running task, the
 *  "etc." The blip fires once on the next tick, toggles RB3, and the
 *  scheduler frees its slot (period 0). */
static void task_supervisor(void *arg)
{
    (void)arg;
    task_spawn(task_blink, &arg_blip, 0U, 2U);   /* one-shot (period 0) */
    pic8_harness_log("[t=%3u] super  spawned blip\n",
                           (unsigned)task_manager_ticks());
}

int main(void)
{
    pic8_harness_init(SIM_CYCLES);
    task_manager_init();

    /* 1. RB0..RB3 as outputs, all starting low. */
    HAL_GPIO_Init(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                  GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                      GPIO_PIN_RESET);

    /* 2. Spawn the application tasks. Priority 0 = supervisor runs first
     *    within each round; the three blinks share priority 1 and run in
     *    spawn order. The arg pointer is how each task knows which LED and
     *    counter are its own. */
    task_spawn(task_supervisor, NULL, PERIOD_SUPERVISOR, 0U);
    task_spawn(task_blink, &arg_fast, PERIOD_FAST, 1U);
    task_spawn(task_blink, &arg_med,  PERIOD_MED,  1U);
    task_spawn(task_blink, &arg_slow, PERIOD_SLOW, 1U);

    /* 3. Wire the ~10 ms Timer0 tick to the scheduler. This sets TMR0IE;
     *    arm it on the target by enabling global interrupts (harmless on
     *    the sim, where the IRQ fires regardless). */
    task_manager_attach_timer0(TICK_RELOAD, TICK_PRESCALER);
    HAL_IRQ_Restore(1);

    /* 4. Run the scheduler. On the host the harness bounds the loop to
     *    SIM_CYCLES; on the target it runs forever. */
    task_manager_run();

    /* 5. Host-only epilogue: the verdict after the dispatch stream. On the
     *    target these lines are unreachable (the loop never returns). */
    pic8_harness_log("done: fast=%u med=%u slow=%u blips=%u "
                           "(ticks=%u, tasks=%u)\n",
                           (unsigned)arg_fast.count, (unsigned)arg_med.count,
                           (unsigned)arg_slow.count, (unsigned)arg_blip.count,
                           (unsigned)task_manager_ticks(),
                           (unsigned)task_manager_count());

    /* Pass when the four tasks ran at four distinct rates and the
     * supervisor spawned at least one blip. */
    int ok = (arg_fast.count > arg_med.count) &&
             (arg_med.count  > arg_slow.count) &&
             (arg_slow.count >= 2U) &&
             (arg_blip.count >= 1U);
    return pic8_harness_report(ok);
}