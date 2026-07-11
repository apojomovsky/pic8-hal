# pic16f87xa-taskmgr

A tiny **cooperative (non-preemptive) task scheduler** for the
[PIC16F87XA HAL](../pic16f87xa-hal) — "kinda like an RTOS, not an RTOS."

A real preemptive RTOS is a poor fit for a PIC16F87XA: the core has no
software stack and only 192–368 B of RAM, so per-task stacks and context
switching are off the table. This module keeps the part of an RTOS that
*does* make sense there — a scheduler that runs a set of registered tasks
in a cooperative, priority-ordered round on each tick. No task is ever
preempted mid-execution; each runs to completion and returns, exactly like
an interrupt handler. That is what makes it *not* an RTOS.

## Features

- **Spawn tasks** at startup or at runtime (even from inside a running task).
- **Periodic tasks** (run every *N* ticks) and **one-shot tasks** (run once,
  then their slot is freed — so a periodic task that re-spawns one-shots
  never exhausts the table).
- **Priority ordering** within each round (lower number runs first).
- **Start / stop / set-period** at runtime.
- **Build-agnostic**: one source builds for the HAL host simulator *and* a
  real XC8 target with **no `#ifdef`** — it reuses the HAL's harness seam.
- **RAM-aware**: the table defaults to 6 slots on the 192 B
  PIC16F873A/874A and 8 slots on the 368 B PIC16F876A/877A, so it banks
  cleanly into every part of the family. Override with
  `-DTASK_MGR_MAX_TASKS=N`.
- **Race-free**: tick (interrupt context) and `run_once` (main context) share
  the task table through short critical sections; a tick that lands during a
  long task arms it for the next round rather than being lost.

## Layout

```
pic16f87xa-taskmgr/
├── include/
│   └── task_manager.h          Public API (no build branches).
├── src/
│   └── task_manager.c          The scheduler.
├── examples/
│   └── example_multi_blink.c   Four blinks on PORTB — sim + hardware.
├── CMakeLists.txt              Host simulation build (reuses the HAL).
├── mcu/pic16f87xa-taskmgr-mplabx/
│   └── Makefile                Real-silicon build via MPLAB XC8.
└── README.md
```

## The model

1. `task_manager_init()` clears the table.
2. `task_spawn(fn, arg, period_ticks, priority)` registers a task and arms it.
   `period_ticks == 0` means one-shot.
3. A periodic **tick** drives the scheduler. The default helper
   `task_manager_attach_timer0(reload, prescaler)` wires a HAL Timer0
   overflow to `task_manager_tick()` (short ISR: it only decrements counters
   and marks due tasks ready — it never runs user code).
4. The main loop calls `task_manager_run()` (or `task_manager_run_once()`
   inside your own loop), which runs the ready tasks in priority order with
   interrupts re-enabled.

A task is a plain `void fn(void *arg)` that returns. Per-task state must live
in storage reached through `arg` (a struct you own) — locals don't survive
between calls. This is the idiomatic cooperative-scheduler pattern, and it is
why the example carries its LED pin and toggle count in a small `blink_arg_t`.

## Build (host simulation)

Requires only CMake + a C compiler. It pulls the HAL in via
`add_subdirectory(../pic16f87xa-hal)`, so the HAL is built once and linked.

```sh
cmake -B build -S .
cmake --build build

./build/example_multi_blink                 # default device (PIC16F877A)
./build/example_multi_blink_PIC16F873A       # 28-pin, 192 B → 6-slot table
./build/example_multi_blink_PIC16F874A
./build/example_multi_blink_PIC16F876A
./build/example_multi_blink_PIC16F877A
```

Expected output (host only — the target has no stdout and runs forever):

```
multi-blink: fast=12 med=6 slow=3 blips=1 (ticks=61, tasks=4)
```

The test passes when the four tasks ran at four distinct rates (fast > med >
slow ≥ 2) and the supervisor spawned at least one blip. `tasks=4` (not 5)
confirms the one-shot blip freed its slot after running — the same mechanism
that lets the supervisor spawn blips forever on hardware without exhausting
the table.

> The sim models the Timer0 overflow/IRQ plumbing but not real Fosc timing
> (same simplification as the HAL's `example_idle_blink`), so periods are in
> *ticks*, deterministic on the sim and ~wall-clock on the target.

## Build (real target)

Uses MPLAB XC8 v3.x (`xc8-cc`). The Makefile mirrors the HAL's: it puts
`include/target` ahead of `include` on the include path so
`pic16f87xa_platform.h` resolves to the real volatile-dereference version,
and links the target-side harness / interrupt vector. The host-only sim
backend is simply absent from the source list.

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
cd mcu/pic16f87xa-taskmgr-mplabx

make MCU=16F877A          # default; also 873A / 874A / 876A
make MCU=16F873A          # 192 B part — 6-slot table, ~156 B used
make clean
```

This produces `build/<MCU>-multi-blink.hex`. Program it with MPLAB X's
"Make and Program" button or any external programmer (PICkit, ICD, IPE).

### Wiring (real target)

- **LEDs**: an LED + current-limiting resistor on each of **RB0, RB1, RB2,
  RB3** to GND (active-high).
- **Clock**: a 20 MHz HS crystal on OSC1/OSC2 (→ Fosc/4 = 5 MHz). With the
  default Timer0 prescaler 1:256 and reload 61, the tick is ~9.98 ms.
- The config word (auto-generated by the Makefile) sets `FOSC=HS, WDTE=ON,
  PWRTE=ON, BOREN=ON, LVP=OFF`. The scheduler refreshes the WDT each loop.

You should see four blinks at clearly different rates: RB0 fastest (~50 ms),
RB1 (~100 ms), RB2 (~200 ms), and RB3 blipping briefly every ~400 ms (each
blip is a freshly-spawned one-shot task).

## Using the scheduler in your own firmware

```c
#include "task_manager.h"

static void my_task(void *arg) {
    /* ...do a small amount of work, then return... */
}

int main(void) {
    task_manager_init();
    task_spawn(my_task, NULL, /*period_ticks=*/10, /*priority=*/1);

    task_manager_attach_timer0(61, TIMER0_PRESCALER_1_256);  /* ~10 ms tick */
    PIC16F87XA_IRQ_Restore(1);   /* arm the Timer0 interrupt */

    task_manager_run();          /* never returns on target */
}
```

For a custom main loop (e.g. to interleave other work), call
`task_manager_run_once()` yourself instead of `task_manager_run()`. To drive
the tick from a different timer, call `task_manager_tick()` from that timer's
ISR instead of using `task_manager_attach_timer0()` — the scheduler does not
care where the tick comes from.

## API cheat sheet

| Function | Purpose |
|---|---|
| `task_manager_init()` | Clear the table; call once at startup. |
| `task_spawn(fn, arg, period, prio)` | Register + arm a task; returns its id (or `TASK_ID_INVALID`). `period==0` → one-shot. |
| `task_start(id)` / `task_stop(id)` | Enable / disable a task at runtime. |
| `task_set_period(id, period)` | Change a task's period. |
| `task_manager_tick()` | Advance one tick — call from a timer ISR. |
| `task_manager_run_once()` | Run all due tasks once (priority order); returns count run. |
| `task_manager_run()` | Canonical loop (bounded on sim, forever on target). |
| `task_manager_ticks()` | Tick counter since init (wraps at 65535). |
| `task_manager_count()` | Number of registered tasks. |
| `task_manager_attach_timer0(reload, prescaler)` | Wire a HAL Timer0 to the tick. |

## Why cooperative, and constraints

- **No preemption**: a task that overruns its slot delays the others. Keep
  tasks short — do a little work and return; the scheduler calls you again
  next period. For long work, break it into a small state machine stored in
  the task's `arg`.
- **One stack**: all tasks share the ordinary call stack, so the PIC16's
  8-level hardware stack is the depth limit. The example's
  supervisor→`task_spawn` path stays well within it; XC8 warns if a deeper
  chain risks overflow.
- **Tick resolution** is set by the Timer0 reload + prescaler (≈10 ms here).
  The finest period is one tick; periods are in ticks, not milliseconds.