/**
 * @file    task_manager.h
 * @brief   Cooperative (non-preemptive) task scheduler for the PIC16F87XA
 *          family: a scheduler, not an RTOS.
 *
 * @details
 *   A preemptive RTOS is a poor fit for a PIC16F87XA: the core has no
 *   software stack and only 192-368 B of RAM, so per-task stacks and context
 *   switching are not feasible. This module provides a cooperative scheduler
 *   instead: a single shared stack (the ordinary call stack) and a round of
 *   registered tasks run in priority order on each tick. No task is ever
 *   preempted; each runs to completion and returns, like an interrupt handler.
 *   That makes it a scheduler, not an RTOS.
 *
 *   Two execution models are supported from the same source, with no
 *   `#ifdef`, reusing the HAL's host/target harness seam
 *   (core/pic8_harness.h):
 *
 *     - Host simulator: `task_manager_run()` calls `pic8_harness_tick()`
 *       each iteration, which pumps the simulated Timer0; its overflow ISR
 *       calls `task_manager_tick()` synchronously, marking due tasks ready;
 *       `task_manager_run_once()` then runs them. The loop is bounded by the
 *       harness so the test can report pass/fail.
 *     - Real target: the Timer0 overflow fires asynchronously from hardware;
 *       `task_manager_tick()` runs in interrupt context and marks tasks ready;
 *       the main loop's `task_manager_run_once()` runs them. The loop never
 *       exits (firmware runs forever).
 *
 *   Time base: a single periodic tick drives the scheduler. The helper
 *   `task_manager_attach_timer0()` wires a HAL Timer0 overflow to
 *   `task_manager_tick()`. Tick rate is set by the Timer0 reload and prescaler
 *   (real Fosc/4 timing on target; the sim reproduces the plumbing, not the
 *   wall-clock rate, as in the HAL's example_idle_blink). The tick may instead
 *   come from any other timer ISR; the scheduler is agnostic to its source.
 *
 *   Concurrency: `task_manager_tick()` runs in interrupt context while
 *   `task_manager_run_once()` and the mutators run in main context. The
 *   scheduler disables interrupts only for the brief snapshot of the ready
 *   set, then runs tasks with interrupts re-enabled, so a tick that arrives
 *   during a long task is not lost; it arms the task for the next round. The
 *   mutators (`task_spawn`, `task_stop`, `task_set_period`) take the same
 *   short critical section, so they are safe to call from a running task
 *   (e.g. a supervisor that spawns one-shot children at runtime).
 *
 *   See examples/example_multi_blink.c for a complete, build-agnostic use.
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdint.h>
#include "peripherals/pic16f87xa_timer0.h"   /* for the Timer0 tick source */

/* ───────────────────────── configuration ────────────────────────── */

/**
 * @brief Maximum number of simultaneously registered tasks. Each slot costs
 *        ~12 B of RAM (two 3-byte PIC16 pointers + two uint16 + a flags
 *        byte). The default scales to the part: 6 on the 192 B
 *        PIC16F873A/874A, 8 on the 368 B PIC16F876A/877A, so the scheduler
 *        banks cleanly into every device in the family. Override by defining
 *        TASK_MGR_MAX_TASKS before including this header.
 *
 *        One-shot tasks free their slot after they run (see
 *        @ref task_manager_run_once), so a periodic task that re-spawns
 *        one-shots does not permanently consume a slot per spawn.
 */
#ifndef TASK_MGR_MAX_TASKS
#  if PIC16F87XA_FAMILY_RAM_BYTES <= 192
#    define TASK_MGR_MAX_TASKS  6
#  else
#    define TASK_MGR_MAX_TASKS  8
#  endif
#endif

/* ───────────────────────── types ─────────────────────────────────── */

/** Opaque task identifier returned by @ref task_spawn. */
typedef uint8_t task_id_t;

/** Sentinel for "no task" / an invalid spawn. */
#define TASK_ID_INVALID  ((task_id_t)0xFFU)

/**
 * @brief Task entry point. A task is a plain function that runs to
 *        completion and returns; it is called with the `arg` passed to
 *        @ref task_spawn. Keep it short; nothing else runs while it does.
 *
 *        A periodic task is called every `period_ticks` ticks; a one-shot
 *        task (period 0) is called once and then its slot is freed (so a
 *        periodic task that re-spawns one-shots never exhausts the table).
 *        Persist per-task state in storage reached through `arg` (a struct
 *        you own), not in locals, which do not survive between calls.
 */
typedef void (*task_fn_t)(void *arg);

/**
 * @brief Task control block. Stored in a fixed array of
 *        @ref TASK_MGR_MAX_TASKS slots; `task_spawn` claims a free slot.
 *        All fields are internal; users address tasks by @ref task_id_t.
 *
 *        The state bits (used / enabled / ready) are packed into one
 *        @ref flags byte rather than three bools, which keeps each TCB to
 *        ~12 B so the whole table banks cleanly into the 192 B parts.
 */
typedef struct {
    task_fn_t   fn;          /**< Entry point (NULL in a free slot). */
    void       *arg;         /**< Opaque argument passed to `fn`. */
    uint16_t    period;      /**< Period in ticks; 0 = one-shot. */
    uint16_t    countdown;   /**< Ticks until the next ready. */
    uint8_t     priority;    /**< Lower number = runs first within a round. */
    uint8_t     flags;       /**< Packed @ref task_flags. */
} task_t;

/** @name Task state flags (packed into @ref task_t.flags). @{ */
#define TM_FLAG_USED     0x01U   /**< Slot is allocated. */
#define TM_FLAG_ENABLED  0x02U   /**< Slot is scheduled. */
#define TM_FLAG_READY    0x04U   /**< Due this round. */
/** @} */

/* ───────────────────────── lifecycle ─────────────────────────────── */

/**
 * @brief  Initialise the scheduler. Clears every task slot and zeroes the
 *         tick counter. Call once before spawning tasks or attaching a
 *         tick source. Idempotent.
 */
void task_manager_init(void);

/**
 * @brief  Register a task and arm it.
 *
 * @param  fn            Entry point (must not be NULL).
 * @param  arg          Opaque pointer passed back to `fn` (may be NULL).
 * @param  period_ticks  Call interval in scheduler ticks. 0 = one-shot
 *                       (called once on the next tick, then its slot is freed).
 *                       For periodic tasks the first call happens after
 *                       `period_ticks` ticks.
 * @param  priority      Scheduling priority within a round; lower numbers
 *                       run first. Ties break by spawn order.
 *
 * @return The new task id, or @ref TASK_ID_INVALID if `fn` is NULL or all
 *         @ref TASK_MGR_MAX_TASKS slots are in use.
 *
 * @note   Safe to call from within a running task (e.g. a supervisor that
 *         spawns one-shot children). The slot fill is a short critical
 *         section, so it does not race with the tick ISR.
 */
task_id_t task_spawn(task_fn_t fn, void *arg, uint16_t period_ticks,
                     uint8_t priority);

/** Enable a previously stopped task. Its countdown is reset to its period. */
void task_start(task_id_t id);

/** Disable a task so the scheduler skips it until @ref task_start. */
void task_stop(task_id_t id);

/** Change a task's period at runtime. Takes effect on the next arming. */
void task_set_period(task_id_t id, uint16_t period_ticks);

/* ───────────────────────── the scheduler ─────────────────────────── */

/**
 * @brief  Advance the scheduler by one tick. For each enabled task, decrement
 *         its countdown; when it reaches zero, mark the task ready and (for
 *         periodic tasks) reload the countdown to the period. One-shot tasks
 *         are marked ready immediately on their first tick.
 *
 *         Call this from a timer interrupt service routine, typically the
 *         Timer0 overflow wired by @ref task_manager_attach_timer0. It is
 *         short (O(n), n ≤ @ref TASK_MGR_MAX_TASKS) and never runs user code,
 *         so it is safe in interrupt context.
 */
void task_manager_tick(void);

/** Current tick counter since @ref task_manager_init. Wraps at 65535. */
uint16_t task_manager_ticks(void);

/**
 * @brief  Run every task that is ready *now*, in priority order, then return.
 *         Each ready task is snapshotted and its ready flag cleared under a
 *         short critical section, then run with interrupts re-enabled, so a
 *         tick arriving during a long task arms the task for the next round
 *         rather than being lost. Non-blocking: if nothing is ready, returns
 *         immediately. Call repeatedly from your main loop.
 *
 * @return Number of tasks actually run this round.
 */
uint8_t task_manager_run_once(void);

/**
 * @brief  The canonical scheduler loop. Equivalent to:
 *
 *             for (;;) {
 *                 pic8_harness_tick();   // pump sim / no-op on target
 *                 task_manager_run_once();
 *                 HAL_WDT_Refresh();
 *             }
 *
 *         On the host the harness bounds the loop (so the test can report
 *         pass/fail and return); on a real target it runs forever. Use this
 *         for the common case, or call @ref task_manager_run_once directly
 *         inside your own loop if you need to do per-iteration work.
 */
void task_manager_run(void);

/** Number of tasks currently registered (used slots), any state. */
uint8_t task_manager_count(void);

/* ───────────────────────── optional tick source ──────────────────── */

/**
 * @brief  Wire a HAL Timer0 overflow to @ref task_manager_tick and start it.
 *         Configures Timer0 for internal Fosc/4, the given prescaler assigned
 *         to Timer0, the given reload, and `task_manager_tick` as the
 *         overflow callback. The TMR0 interrupt enable is set; arm it for
 *         real by calling `HAL_IRQ_Restore(1)` afterwards.
 *
 * @param  reload      TMR0 reload value (0..255). On a 20 MHz target,
 *                     prescaler 1:256, reload 61 → ~10 ms per tick.
 * @param  prescaler   A @ref TIMER0_PrescalerTypeDef (1:2 .. 1:256).
 *
 * @note   The sim does not model real Fosc timing, it reproduces the
 *         overflow/IRQ plumbing, not the wall-clock period (same caveat as
 *         the HAL's example_idle_blink). Periods are therefore in *ticks*,
 *         which are deterministic on the sim and ~wall-clock on the target.
 */
void task_manager_attach_timer0(uint8_t reload, TIMER0_PrescalerTypeDef prescaler);

#endif /* TASK_MANAGER_H */