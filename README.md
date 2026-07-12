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
**silicon**, with no `#ifdef` in the code.

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
| 🗓️ | **[pic16f87xa-taskmgr](pic16f87xa-taskmgr/)** | A cooperative scheduler built on the HAL: periodic and one-shot tasks, priority-ordered, race-free. | [README](pic16f87xa-taskmgr/README.md) · [Architecture](pic16f87xa-taskmgr/docs/ARCHITECTURE.md) · [API](pic16f87xa-taskmgr/docs/API.md) |
| 🆕 | **[pic18f2455-hal](pic18f2455-hal/)** | Second family under the shared layer: PIC18F2455/2550/4455/4550. **Phase 1 scaffold** (build seam + harness proven, drivers land in Phase 2). | [README](pic18f2455-hal/README.md) |
| 🧱 | **[pic8-common](pic8-common/)** | The shared layer every family reuses: status codes, the host/target harness contract, CMake/Make fragments. | [README](pic8-common/README.md) |
| 📐 | **[docs/multi-family-plan](docs/multi-family-plan.md)** | The refactor plan: extract `pic8-common`, add families behind a fixed contract. Phase 0–1 done, Phase 2 next. | — |

## Quick start

Build and run the multi-blink example on the host simulator (CMake 3.16+ and a
C99 compiler; the HAL is pulled in automatically):

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

Four blinks at distinct rates on RB0-RB3, plus a priority-0 supervisor that
**spawns a one-shot blip at runtime** at t=40 (it runs at t=41, then frees its
slot).

### Real hardware

For a real PIC, use the MPLAB XC8 toolchain:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
cd pic16f87xa-taskmgr/mcu/pic16f87xa-taskmgr-mplabx
make MCU=16F877A        # also 873A / 874A / 876A
```

This produces `build/<MCU>-multi-blink.hex`. Wire LEDs on RB0-RB3 and a 20 MHz
crystal, then program with MPLAB X or any programmer. See the task manager
[README](pic16f87xa-taskmgr/README.md) for the full wiring and the HAL
[MANUAL](pic16f87xa-hal/MANUAL.md) for peripheral bring-up.

## Documentation

- **HAL**
  - [README](pic16f87xa-hal/README.md): overview, build, simulation middleware
  - [MANUAL.md](pic16f87xa-hal/MANUAL.md): the full manual (architecture, both builds, per-peripheral reference)
- **Task manager**
  - [README](pic16f87xa-taskmgr/README.md): overview, quick start, demo
  - [docs/ARCHITECTURE.md](pic16f87xa-taskmgr/docs/ARCHITECTURE.md): cooperative model, tick source, concurrency, RAM scaling, constraints
  - [docs/API.md](pic16f87xa-taskmgr/docs/API.md): full API reference
- **Multi-family**
  - [docs/multi-family-plan.md](docs/multi-family-plan.md): the refactor plan that extracted `pic8-common/` and is adding the PIC18F2455 family behind a fixed contract (Phase 0–1 done)
  - [pic18f2455-hal/README.md](pic18f2455-hal/README.md): the second family (Phase 1 scaffold)
- **Datasheets**: [DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf) PIC16F87XA (also locally as `39582b.pdf`), [DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf) PIC18F2455 family (also locally as `39632e.pdf`)

## Repository layout

```
.
├── pic8-common/                    # shared layer (every family reuses this)
│   ├── include/core/              #   hal_status.h (HAL_* / PIC8_BIT*), pic8_harness.h
│   ├── src/core/                  #   pic8_harness_target.c (family-blind no-ops)
│   ├── cmake/  mk/                #   shared pic8_family.cmake / pic8_family.mk
│   └── README.md
│
├── pic16f87xa-hal/                 # the HAL (PIC16F87XA family)
│   ├── include/                    # public headers (core/ + peripherals/); core/
│   │                               #   holds pic16_irq.h + wdt_sleep, the shared
│   │                               #   harness header lives in pic8-common/
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
├── pic18f2455-hal/                 # second family (PIC18F2455/2550/4455/4550)
│   ├── include/  src/  tests/      # Phase 1 scaffold; drivers land in Phase 2
│   ├── mcu/pic18f2455-mplabx/      # XC8 Makefile (needs PIC18Fxxxx DFP, see README)
│   ├── README.md
│   └── CMakeLists.txt              # host simulation build (reuses pic8-common)
│
├── docs/multi-family-plan.md       # the refactor plan (Phase 0–1 done)
├── 39582b.pdf                      # Microchip datasheet (DS39582B, PIC16F87XA)
├── 39632e.pdf                      # Microchip datasheet (DS39632E, PIC18F2455 family)
├── LICENSE                          # MIT
└── README.md                       # this file
```

## Requirements

- **Host simulation**, CMake ≥ 3.16 and any C99 compiler (gcc/clang).
- **Real target**, MPLAB X IDE v6.x and MPLAB XC8 v3.x (`xc8-cc`). Tested on a
  PIC16F877A; the examples use PORTB only, so they run on all four parts of
  the family.

## License

MIT; see [LICENSE](LICENSE).

The Microchip datasheet DS39582B (included as `39582b.pdf`) is © 2003 Microchip
Technology Inc.; register and bit names in the code follow it directly. Remove
the PDF from your fork if you do not want to redistribute vendor documentation.