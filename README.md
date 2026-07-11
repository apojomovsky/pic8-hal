# PIC16F87XA HAL & Task Manager

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Language: C](https://img.shields.io/badge/C-99-555.svg)](https://en.cppreference.com/w/c)
[![Toolchain: MPLAB XC8](https://img.shields.io/badge/toolchain-MPLAB%20XC8-green.svg)](https://www.microchip.com/mpgb/xc8.html)
[![Platform: PIC16F87XA](https://img.shields.io/badge/platform-PIC16F87XA-red.svg)](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf)
[![Runs on: host & silicon](https://img.shields.io/badge/runs%20on-host%20%26%20silicon-orange.svg)](#quick-start)

A datasheet-faithful **hardware abstraction layer** and a tiny **cooperative
task scheduler** for the PIC16F873A / 874A / 876A / 877A family. Every
register, bit name and reset value is taken 1-to-1 from the Microchip
datasheet [DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf),
and the same source builds unchanged for a host **simulator** and real
**silicon** — with no `#ifdef` in the code.

```
┌──────────────────────────────────────────────────────────────────────┐
│  pic16f87xa-hal        STM32Cube-style HAL: GPIO, Timers, CCP, USART, │
│  (the foundation)      MSSP, ADC, Comparator, Vref, EEPROM, PSP, WDT   │
│                              │                                        │
│                              ▼ built on                               │
│  pic16f87xa-taskmgr    Cooperative (non-preemptive) scheduler: spawn  │
│  (the scheduler)       periodic & one-shot tasks, priority-ordered    │
└──────────────────────────────────────────────────────────────────────┘
```

## Components

| | Component | What it is | Docs |
|---|---|---|---|
| 🧩 | **[pic16f87xa-hal](pic16f87xa-hal/)** | STM32Cube-style HAL for every peripheral on the part, with a host simulation backend so firmware runs on a PC before it touches hardware. | [README](pic16f87xa-hal/README.md) · [MANUAL](pic16f87xa-hal/MANUAL.md) |
| 🗓️ | **[pic16f87xa-taskmgr](pic16f87xa-taskmgr/)** | A tiny cooperative scheduler that builds on the HAL — spawn tasks, periodic + one-shot, priority-ordered, race-free. | [README](pic16f87xa-taskmgr/README.md) · [Architecture](pic16f87xa-taskmgr/docs/ARCHITECTURE.md) · [API](pic16f87xa-taskmgr/docs/API.md) |

## Quick start

Build and run the multi-blink example on the host simulator (needs only CMake
3.16+ and a C99 compiler — it pulls the HAL in automatically):

```sh
cmake -B build -S pic16f87xa-taskmgr
cmake --build build
./build/example_multi_blink
```

You should see the scheduler dispatch tasks tick by tick:

```
[t=  5] fast  #1
[t= 10] fast  #2
[t= 10] med   #1
[t= 20] fast  #4
[t= 20] med   #2
[t= 20] slow  #1
[t= 40] super  spawned blip
[t= 40] fast  #8
[t= 41] blip  #1
[t= 60] slow  #3
done: fast=12 med=6 slow=3 blips=1 (ticks=61, tasks=4)
```

…four blinks at clearly different rates on RB0–RB3, plus a priority-0
supervisor task that **spawns a one-shot blip on the fly** at t=40 (it runs
at t=41, then frees its slot).

### Real hardware

For a real PIC, use the MPLAB XC8 toolchain:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
cd pic16f87xa-taskmgr/mcu/pic16f87xa-taskmgr-mplabx
make MCU=16F877A        # also 873A / 874A / 876A
```

This produces `build/<MCU>-multi-blink.hex`. Wire LEDs on RB0–RB3 and a 20 MHz
crystal, then program with MPLAB X or any programmer. See the task manager
[README](pic16f87xa-taskmgr/README.md) for the full wiring and the HAL
[MANUAL](pic16f87xa-hal/MANUAL.md) for peripheral bring-up.

## Documentation

- **HAL**
  - [README](pic16f87xa-hal/README.md) — overview, build, the simulation middleware
  - [MANUAL.md](pic16f87xa-hal/MANUAL.md) — the full human-readable manual (architecture, both builds, per-peripheral reference)
- **Task manager**
  - [README](pic16f87xa-taskmgr/README.md) — overview, quick start, demo
  - [docs/ARCHITECTURE.md](pic16f87xa-taskmgr/docs/ARCHITECTURE.md) — the cooperative model, tick source, concurrency, RAM scaling, constraints
  - [docs/API.md](pic16f87xa-taskmgr/docs/API.md) — full API reference
- **Datasheet** — [DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf) (also included locally as `39582b.pdf`)

## Repository layout

```
.
├── pic16f87xa-hal/                 # the HAL
│   ├── include/                    # public headers (core/ + peripherals/)
│   ├── src/                        # drivers + host sim backend
│   ├── tests/                      # end-to-end examples (blink, USART, ADC, …)
│   ├── mcu/pic16f87xa-mplabx/      # XC8 Makefile for real silicon
│   ├── README.md  MANUAL.md
│   └── CMakeLists.txt              # host simulation build
│
├── pic16f87xa-taskmgr/             # the cooperative scheduler
│   ├── include/  src/  examples/   # library + multi-blink example
│   ├── docs/                      # ARCHITECTURE.md, API.md
│   ├── mcu/pic16f87xa-taskmgr-mplabx/  # XC8 Makefile
│   ├── README.md
│   └── CMakeLists.txt              # host simulation build (reuses the HAL)
│
├── 39582b.pdf                      # Microchip datasheet (DS39582B)
├── LICENSE                          # MIT
└── README.md                       # this file
```

## Requirements

- **Host simulation** — CMake ≥ 3.16 and any C99 compiler (gcc/clang).
- **Real target** — MPLAB X IDE v6.x and MPLAB XC8 v3.x (`xc8-cc`). Tested on a
  PIC16F877A; the examples use PORTB only, so they run on all four parts of
  the family.

## License

MIT — see [LICENSE](LICENSE).

The Microchip datasheet DS39582B (included as `39582b.pdf`) is © 2003 Microchip
Technology Inc.; register and bit names cited throughout the code follow it
directly. The datasheet is provided here for convenience — remove it from your
fork if you prefer not to redistribute vendor documentation.