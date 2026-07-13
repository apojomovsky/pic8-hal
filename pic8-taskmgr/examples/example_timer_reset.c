/**
 * @file    example_timer_reset.c
 * @brief   Verify @ref task_reset re-arms a task from the full period.
 *
 * @details
 *   A periodic "marker" task (period 10) records the scheduler tick at each
 *   fire. A priority-0 supervisor (period 25) calls `task_reset(marker)` on
 *   its first fire and records the reset tick. The assertion:
 *
 *     - Before the reset the marker fires every 10 ticks (fires at 10, 20).
 *     - The reset lands at tick 25.
 *     - The first fire *after* the reset is at tick 35 = reset_tick + period,
 *       NOT at the originally-scheduled 30 — i.e. the reset restarted the
 *       countdown from the full period, pushing the fire out. The gap across
 *       the reset widens to 15 (> 10), then resumes at 10.
 *
 *   This is the "re-triggerable timeout" pattern (debounce / feed-the-WDT):
 *   `task_reset` on each event pushes the fire out by a full period.
 *
 *   Uses a fast tick (reload 253, prescaler 1:2 → ~6 sim cycles/tick) so the
 *   run is short. Host sim only; the XC8 target build uses example_multi_blink.
 */

#include "pic8_hal.h"
#include "core/pic8_harness.h"
#include "task_manager.h"

#define CHECK(cond, msg) do { \
    if (!(cond)) { pic8_harness_log("FAIL: %s\n", msg); return pic8_harness_report(0); } \
} while (0)

/* Tick config matched to example_multi_blink (reload 61, 1:256): proven on
 * both families. The fire ticks are deterministic in scheduler-tick units
 * (g_ticks = Timer0 overflow count) regardless of cycles-per-tick, so the
 * assertions on tick numbers 10/20/25/35/45 hold on both. SIM_CYCLES=4M
 * yields ~80 scheduler ticks — enough for the marker to fire through 75. */
#define TICK_RELOAD     61U
#define TICK_PRESCALER  TIMER0_PRESCALER_1_256
#define SIM_CYCLES      4000000UL

#define MARKER_PERIOD   10U
#define SUPERVISOR_PERIOD 25U
#define MAX_FIRES       32U

static volatile uint16_t fire_ticks[MAX_FIRES];
static volatile uint8_t  n_fires = 0U;
static volatile uint8_t  did_reset = 0U;
static volatile uint16_t reset_tick = 0U;
static task_id_t g_marker_id = TASK_ID_INVALID;

static void task_marker(void *arg)
{
    (void)arg;
    if (n_fires < MAX_FIRES) {
        fire_ticks[n_fires] = task_manager_ticks();
        n_fires++;
    }
}

static void task_supervisor(void *arg)
{
    (void)arg;
    if (!did_reset) {
        reset_tick = task_manager_ticks();
        task_reset(g_marker_id);
        did_reset = 1U;
        pic8_harness_log("[t=%u] supervisor reset marker\n", (unsigned)reset_tick);
    }
}

int main(void)
{
    pic8_harness_init(SIM_CYCLES);
    task_manager_init();

    g_marker_id = task_spawn(task_marker, NULL, MARKER_PERIOD, 1U);
    (void)task_spawn(task_supervisor, NULL, SUPERVISOR_PERIOD, 0U);

    task_manager_attach_timer0(TICK_RELOAD, TICK_PRESCALER);
    HAL_IRQ_Restore(1);

    task_manager_run();

    /* 1. Enough fires observed to cover: two pre-reset fires (10, 20), the
     *    first post-reset fire (35), and one more to confirm resumed spacing
     *    (45). PIC16 reaches ~61 scheduler ticks in SIM_CYCLES (vs ~80 on
     *    PIC18 — the PIC16 sim's first TMR0 overflow is slower), yielding 5
     *    marker fires; PIC18 yields 7. Either way >= 4. */
    CHECK(n_fires >= 4U, "marker did not fire enough times");
    CHECK(fire_ticks[0] == 10U, "first fire not at tick 10");
    CHECK(fire_ticks[1] == 20U, "second fire not at tick 20");
    CHECK((uint16_t)(fire_ticks[1] - fire_ticks[0]) == MARKER_PERIOD,
          "pre-reset gap != period");

    /* 2. The supervisor reset at tick 25. */
    CHECK(did_reset == 1U, "supervisor never reset the marker");
    CHECK(reset_tick == 25U, "reset did not land at tick 25");

    /* 3. The first fire after the reset is exactly period ticks after the
     *    reset (35, not 30) — the reset re-armed from the full period. */
    uint8_t j = 0U;
    while (j < n_fires && fire_ticks[j] <= reset_tick) j++;
    CHECK(j < n_fires, "no fire after the reset");
    CHECK((uint16_t)(fire_ticks[j] - reset_tick) == MARKER_PERIOD,
          "first post-reset fire not at reset_tick + period (reset did not re-arm)");
    CHECK((uint16_t)(fire_ticks[j] - fire_ticks[j - 1U]) > MARKER_PERIOD,
          "gap across reset did not widen");
    /* 4. Spacing resumes at the period after the reset. */
    CHECK((j + 1U) < n_fires, "no second post-reset fire to confirm spacing");
    CHECK((uint16_t)(fire_ticks[j + 1U] - fire_ticks[j]) == MARKER_PERIOD,
          "post-reset spacing did not resume at the period");

    pic8_harness_log("OK: task_reset re-arms from the full period "
                     "(fires 10,20 | reset@25 | next@35,45).\n");
    return pic8_harness_report(1);
}