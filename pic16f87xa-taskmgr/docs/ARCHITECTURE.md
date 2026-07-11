# Architecture

How the PIC16F87XA task manager is built — the cooperative model, the tick
source, the concurrency rules, the RAM scaling, and the constraints that come
from running on an 8-bit PIC with banked RAM and an 8-level hardware stack.

## Why cooperative, not preemptive

A real preemptive RTOS needs a separate stack per task (to save registers and
resume mid-execution). The PIC16F87XA has **no software stack** — only an
8-level hardware call stack — and 192–368 B of banked RAM. Per-task stacks and
context switching are therefore off the table.

This module keeps the part of an RTOS that *does* make sense on such a part: a
scheduler that runs a set of registered tasks in a **cooperative,
priority-ordered round** on each tick. A task is a plain `void fn(void *arg)`
that runs to completion and returns. No task is ever preempted mid-execution;
each is called and returns, exactly like an interrupt handler. That is what
makes it “kinda like an RTOS, not an RTOS.”

## The model, step by step

1. `task_manager_init()` clears the task table.
2. `task_spawn(fn, arg, period_ticks, priority)` registers a task and arms it.
   `period_ticks == 0` means **one-shot** (run once, then its slot is freed).
3. A periodic **tick** drives the scheduler. The default helper
   `task_manager_attach_timer0(reload, prescaler)` wires a HAL Timer0 overflow
   to `task_manager_tick()`. The tick ISR is **short**: it only decrements each
   enabled task’s countdown and marks due tasks ready — it never runs user code,
   so it is safe in interrupt context.
4. The main loop calls `task_manager_run()` (or `task_manager_run_once()` inside
   your own loop), which runs the ready tasks in priority order with interrupts
   re-enabled.

A task keeps its state in storage reached through `arg` (a struct you own), not
in locals — locals do not survive between calls. This is the idiomatic
cooperative-scheduler pattern, and it is why the example carries its LED pin
and toggle count in a small `blink_arg_t`.

### Period semantics

The tick logic is “if `countdown == 0` then fire, else decrement.” A periodic
task reloads to **period − 1** (not `period`) on each fire, so the spacing is
constant at exactly `period` ticks — otherwise every fire would take
`period + 1` ticks. A one-shot (`period == 0`) uses `countdown == 0`, so it is
ready on the first tick after it is spawned; once it runs, its slot is freed.

Consequence worth knowing: a periodic task’s **first** fire happens after
`period` ticks (e.g. a period-5 task first runs at tick 5, not tick 0).

### One-shots free their slot

When a one-shot task finishes, `task_manager_run_once()` clears its slot
atomically (under a short critical section). A periodic task that re-spawns
one-shots — like the example’s supervisor — therefore does **not** permanently
consume a slot per spawn; the same slot is recycled. This is what lets the
supervisor spawn blips forever on hardware without exhausting the table.

## Two execution models, one source

The scheduler and its examples build for two execution models with **no `#ifdef`**,
reusing the HAL’s host/target harness seam (`core/pic16f87xa_harness.h`):

- **Host simulator** — a normal program. `task_manager_run()` calls
  `pic16f87xa_harness_tick()` each iteration, which pumps the simulated Timer0;
  its overflow ISR calls `task_manager_tick()` synchronously, marking due tasks
  ready; `task_manager_run_once()` then runs them. The loop is bounded by the
  harness so the test can report pass/fail.
- **Real target** — firmware. The Timer0 overflow fires asynchronously from
  hardware; `task_manager_tick()` runs in interrupt context and marks tasks
  ready; the main loop’s `task_manager_run_once()` runs them. The loop never
  exits (firmware runs forever).

On the host, `pic16f87xa_harness_log` is a `printf`; on the target it is a
no-op (no stdout). That is how the example streams a dispatch log on the host
while staying build-agnostic.

> The sim models the Timer0 overflow/IRQ plumbing but **not** real Fosc timing
> (same simplification as the HAL’s `example_idle_blink`) — so periods are in
> *ticks*, deterministic on the sim and ~wall-clock on the target. On a 20 MHz
> target with prescaler 1:256 and reload 61, one tick is ~9.98 ms.

## Concurrency: keeping the shared table race-free

`task_manager_tick()` runs in interrupt context while `task_manager_run_once()`
and the mutators run in main context. They share one thing: the task table.
Three rules keep it race-free:

1. **`run_once` snapshots the ready set under a brief critical section**, clears
   each ready flag, then runs the tasks with interrupts **re-enabled** — so a
   tick that arrives during a long task arms the task for the next round rather
   than being lost, and the ISR is not blocked by long user code.
2. **The mutators** (`task_spawn`, `task_stop`, `task_set_period`) **fill their
   slot under a brief critical section**, so a tick that fires mid-spawn never
   sees a half-initialised task control block. This makes `task_spawn` safe to
   call from inside a running task (e.g. a supervisor that spawns one-shot
   children at runtime).
3. **A one-shot is freed atomically** after it runs (rule 2’s critical section),
   so the tick ISR never observes a half-freed slot.

The critical sections use the HAL’s `PIC16F87XA_IRQ_Disable/Restore`, which
save/restore the GIE bit. On the host sim the tick is delivered synchronously
from inside `pic16f87xa_harness_tick()`, so tick and `run_once` never truly
overlap — the critical sections are a no-op there but remain correct on the
target. `task_manager_ticks()` also takes a critical section because a 16-bit
read is not atomic on an 8-bit PIC.

## RAM scaling

Each task control block is ~12 B (two 3-byte PIC16 pointers for `fn` and `arg`,
two `uint16` for `period`/`countdown`, a `priority` byte and a packed `flags`
byte). The state bits — `used` / `enabled` / `ready` — are packed into one
`flags` byte rather than three bools, which keeps the TCB small enough to bank
cleanly into the smallest parts.

`TASK_MGR_MAX_TASKS` defaults scale to the part:

| Part | RAM | Default slots | Table size |
|---|---|---|---|
| PIC16F873A / 874A | 192 B | 6 | ~72 B |
| PIC16F876A / 877A | 368 B | 8 | ~96 B |

Override with `-DTASK_MGR_MAX_TASKS=N` before including `task_manager.h`.

> **PIC16 banking gotcha, learned the hard way:** the first cut of the example
> stored a `volatile uint32_t *counter` in its per-task arg struct. Banked
> pointers are 3 bytes on the PIC16, and XC8 could not place one in the 192 B
> of the 873A (“could not find space for variable”). The fix was to keep the
> arg struct **pointer-free** (a pin index + a `uint8_t` counter) — the idiomatic
> shape anyway, since per-task state should live in the arg, not behind a
> pointer to a global.

## Constraints

- **No preemption.** A task that overruns its slot delays the others. Keep tasks
  short — do a little work and return; the scheduler calls you again next
  period. For long work, break it into a small state machine stored in the task’s
  `arg`.
- **One stack.** All tasks share the ordinary call stack, so the PIC16’s 8-level
  hardware stack is the depth limit. The example’s
  supervisor → `task_spawn` path stays well within it; XC8 warns if a deeper
  chain risks overflow.
- **Tick resolution** is set by the Timer0 reload + prescaler (≈10 ms here). The
  finest period is one tick; periods are in **ticks**, not milliseconds.