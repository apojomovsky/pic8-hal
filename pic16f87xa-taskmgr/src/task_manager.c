/**
 * @file    task_manager.c
 * @brief   Cooperative task scheduler implementation (see task_manager.h).
 *
 * @details
 *   The scheduler keeps a fixed array of task control blocks
 *   (@ref TASK_MGR_MAX_TASKS slots). `task_manager_tick()` (called from a
 *   Timer0 overflow in the usual setup) decrements each enabled task's
 *   countdown and marks due tasks ready. `task_manager_run_once()` then
 *   runs the ready set in priority order, with interrupts re-enabled so a
 *   tick that lands during a long task is not lost.
 *
 *   The only shared state between interrupt context (tick) and main context
 *   (run_once / mutators) is the TCB array. Three rules keep it race-free:
 *     1. run_once snapshots and clears the ready flags under a brief
 *        critical section, then runs the tasks with interrupts on.
 *     2. The mutators (spawn/stop/set_period) fill their slot under a brief
 *        critical section, so a tick that fires mid-spawn never sees a
 *        half-initialised TCB.
 *     3. A one-shot task that has run is freed atomically (its slot's flags
 *        are cleared under a critical section), so a periodic task that
 *        re-spawns one-shots — like the example's supervisor — does not
 *        exhaust the table one slot per spawn.
 *   On the host sim the tick is delivered synchronously from inside
 *   `pic16f87xa_harness_tick()`, so tick and run_once never truly overlap —
 *   the critical sections are a no-op there but remain correct on the target.
 */

#include "task_manager.h"
#include "core/pic16f87xa_interrupt.h"   /* PIC16F87XA_IRQ_Disable/Restore */
#include "core/pic16f87xa_harness.h"      /* harness_tick / harness_running */
#include "core/pic16f87xa_wdt_sleep.h"    /* HAL_WDT_Refresh */

/* ───────────────────────── state ─────────────────────────────────── */

/** The task table. Slot 0 is claimed first by task_spawn. */
static task_t g_tasks[TASK_MGR_MAX_TASKS];

/** Monotonic tick counter since the last init (wraps at 65535). */
static uint16_t g_ticks = 0U;

/**
 * @brief  The countdown value that makes a task fire after exactly `period`
 *         ticks. The tick logic is "if countdown==0 then fire, else
 *         decrement", so a periodic task reloads to @e period-1 (not
 *         period) to keep the spacing constant — otherwise every fire would
 *         take period+1 ticks. A one-shot (period 0) uses 0, so it is ready
 *         on the first tick until @ref task_manager_run_once frees it.
 */
static uint16_t arm_countdown(uint16_t period)
{
    return (period == 0U) ? 0U : (uint16_t)(period - 1U);
}

/** Reload written back into TMR0 on each overflow so every tick has the
 *  same period. Timer0 has no hardware auto-reload (unlike Timer2), so a
 *  fixed tick rate requires reloading in the ISR — also the correct pattern
 *  on a real target. Stored by task_manager_attach_timer0. */
static uint8_t g_tick_reload = 0U;

/* ───────────────────────── lifecycle ─────────────────────────────── */

void task_manager_init(void)
{
    uint8_t prev = PIC16F87XA_IRQ_Disable();
    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        g_tasks[i].fn        = NULL;
        g_tasks[i].arg       = NULL;
        g_tasks[i].period    = 0U;
        g_tasks[i].countdown = 0U;
        g_tasks[i].priority  = 0U;
        g_tasks[i].flags     = 0U;
    }
    g_ticks = 0U;
    PIC16F87XA_IRQ_Restore(prev);
}

task_id_t task_spawn(task_fn_t fn, void *arg, uint16_t period_ticks,
                     uint8_t priority)
{
    if (fn == NULL) {
        return TASK_ID_INVALID;
    }

    /* Claim and fill a free slot under a critical section so a tick ISR
     * can never observe a half-initialised TCB. */
    uint8_t prev = PIC16F87XA_IRQ_Disable();
    task_id_t id = TASK_ID_INVALID;
    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        if (!(g_tasks[i].flags & TM_FLAG_USED)) {
            g_tasks[i].fn        = fn;
            g_tasks[i].arg       = arg;
            g_tasks[i].period    = period_ticks;
            g_tasks[i].countdown = arm_countdown(period_ticks);
            g_tasks[i].priority  = priority;
            g_tasks[i].flags     = TM_FLAG_USED | TM_FLAG_ENABLED;
            id = (task_id_t)i;
            break;
        }
    }
    PIC16F87XA_IRQ_Restore(prev);
    return id;
}

void task_start(task_id_t id)
{
    if (id >= TASK_MGR_MAX_TASKS) return;
    uint8_t prev = PIC16F87XA_IRQ_Disable();
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].countdown = arm_countdown(g_tasks[id].period);
        g_tasks[id].flags |=  TM_FLAG_ENABLED;
        g_tasks[id].flags &= (uint8_t)~TM_FLAG_READY;
    }
    PIC16F87XA_IRQ_Restore(prev);
}

void task_stop(task_id_t id)
{
    if (id >= TASK_MGR_MAX_TASKS) return;
    uint8_t prev = PIC16F87XA_IRQ_Disable();
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].flags &= (uint8_t)~(TM_FLAG_ENABLED | TM_FLAG_READY);
    }
    PIC16F87XA_IRQ_Restore(prev);
}

void task_set_period(task_id_t id, uint16_t period_ticks)
{
    if (id >= TASK_MGR_MAX_TASKS) return;
    uint8_t prev = PIC16F87XA_IRQ_Disable();
    if (g_tasks[id].flags & TM_FLAG_USED) {
        g_tasks[id].period = period_ticks;
    }
    PIC16F87XA_IRQ_Restore(prev);
}

/* ───────────────────────── the scheduler ─────────────────────────── */

void task_manager_tick(void)
{
    g_ticks++;

    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        task_t *t = &g_tasks[i];
        uint8_t  f = t->flags;
        if (!(f & TM_FLAG_USED) || !(f & TM_FLAG_ENABLED)) {
            continue;
        }
        if (t->countdown == 0U) {
            /* Due now. Periodic tasks re-arm to period-1 so the next fire is
             * exactly `period` ticks later (not period+1). One-shots
             * (period 0) leave countdown 0 — ready every tick until
             * task_manager_run_once frees the slot. */
            t->flags |= TM_FLAG_READY;
            if (t->period != 0U) {
                t->countdown = arm_countdown(t->period);
            }
        } else {
            t->countdown--;
        }
    }
}

uint16_t task_manager_ticks(void)
{
    /* 16-bit read is not atomic on an 8-bit PIC; take a critical section so
     * a tick ISR landing between the byte reads can't tear the value. */
    uint8_t  prev = PIC16F87XA_IRQ_Disable();
    uint16_t v    = g_ticks;
    PIC16F87XA_IRQ_Restore(prev);
    return v;
}

uint8_t task_manager_run_once(void)
{
    /* Snapshot the ready set in priority order and clear each ready flag
     * under a critical section, then run the tasks with interrupts enabled
     * so a tick during a long task arms the task for the next round. */
    task_id_t order[TASK_MGR_MAX_TASKS];
    uint8_t   n = 0U;

    uint8_t prev = PIC16F87XA_IRQ_Disable();
    for (;;) {
        /* Pick the lowest-numbered-priority ready task (ties: lowest slot). */
        int      best      = -1;
        uint8_t  best_prio = 0xFFU;
        for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
            task_t *t = &g_tasks[i];
            if ((t->flags & (TM_FLAG_USED | TM_FLAG_ENABLED | TM_FLAG_READY))
                == (TM_FLAG_USED | TM_FLAG_ENABLED | TM_FLAG_READY) &&
                t->priority < best_prio) {
                best      = (int)i;
                best_prio = t->priority;
            }
        }
        if (best < 0) {
            break;
        }
        g_tasks[best].flags &= (uint8_t)~TM_FLAG_READY;  /* a new tick re-arms */
        order[n] = (task_id_t)best;
        n++;
    }
    PIC16F87XA_IRQ_Restore(prev);

    /* Run with interrupts enabled. */
    for (uint8_t k = 0; k < n; k++) {
        task_t *t = &g_tasks[order[k]];
        t->fn(t->arg);
        if (t->period == 0U) {
            /* One-shot: free the slot atomically so a periodic task that
             * re-spawns one-shots does not exhaust the table. */
            uint8_t p = PIC16F87XA_IRQ_Disable();
            t->flags = 0U;
            t->fn    = NULL;
            PIC16F87XA_IRQ_Restore(p);
        }
    }
    return n;
}

void task_manager_run(void)
{
    for (uint32_t i = 0; pic16f87xa_harness_running(i); i++) {
        pic16f87xa_harness_tick();    /* host: pumps sim → Timer0 ISR → tick */
        (void)task_manager_run_once();
        HAL_WDT_Refresh();            /* no-op on the host */
    }
}

uint8_t task_manager_count(void)
{
    uint8_t count = 0U;
    uint8_t prev  = PIC16F87XA_IRQ_Disable();
    for (uint8_t i = 0; i < TASK_MGR_MAX_TASKS; i++) {
        if (g_tasks[i].flags & TM_FLAG_USED) {
            count++;
        }
    }
    PIC16F87XA_IRQ_Restore(prev);
    return count;
}

/* ───────────────────────── optional tick source ──────────────────── */

/** Trampoline matching the HAL's `void (*)(void)` overflow callback shape.
 *  Reloads TMR0 first so every tick has the same period (Timer0 has no
 *  auto-reload), then advances the scheduler. */
static void task_manager_on_timer0_overflow(void)
{
    /* Reload for a constant period (Timer0 has no auto-reload), then
     * advance the scheduler. Writing TMR0 also clears the prescaler
     * (DS39582B §5.3) — desirable here: every tick starts a clean count. */
    HAL_TIMER0_WriteCounter(g_tick_reload);
    task_manager_tick();
}

void task_manager_attach_timer0(uint8_t reload, TIMER0_PrescalerTypeDef prescaler)
{
    g_tick_reload = reload;
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = prescaler;
    h.PrescalerAssigned = true;
    h.ReloadValue       = reload;
    h.OverflowCallback  = task_manager_on_timer0_overflow;
    HAL_TIMER0_Init(&h);
    HAL_TIMER0_Start(&h);
}