# 8-bit PIC HAL & Task Manager

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Language: C](https://img.shields.io/badge/C-99-555.svg)](https://en.cppreference.com/w/c)
[![Toolchain: MPLAB XC8](https://img.shields.io/badge/toolchain-MPLAB%20XC8-green.svg)](https://www.microchip.com/mpgb/xc8.html)
[![Platform: 8-bit PIC](https://img.shields.io/badge/platform-8--bit%20PIC-red.svg)](docs/multi-family-plan.md)
[![Runs on: host & silicon](https://img.shields.io/badge/runs%20on-host%20%26%20silicon-orange.svg)](#quick-start)

A datasheet-faithful **hardware abstraction layer** and a tiny **cooperative
task scheduler** for 8-bit PIC microcontrollers. The HAL was born on the
PIC16F873A / 874A / 876A / 877A family
([DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf))
and now also covers PIC18F2455 / 2550 / 4455 / 4550
([DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf))
under a shared `pic8-common` layer; the **same task-manager source builds
against either family**. Every register, bit name and reset value is taken
1-to-1 from the Microchip datasheets, and the same source builds unchanged
for a host **simulator** and real **silicon**, with no `#ifdef` in the code.

```
                       pic8-taskmgr  (family-agnostic cooperative scheduler)
                            │   builds against either family
              ┌─────────────┴─────────────┐
        pic16f87xa-hal               pic18fxx5x-hal
        (PIC16F87XA family)           (PIC18F2455 family)
              └─────────────┬─────────────┘
                            │   both reuse
                       pic8-common  (shared layer: status, harness, build)
```

## Components

| | Component | What it is | Docs |
|---|---|---|---|
| 🧩 | **[pic16f87xa-hal](pic16f87xa-hal/)** | STM32Cube-style HAL for every peripheral on the part (GPIO, Timers, CCP, MSSP, ADC, Comparator, Vref, EEPROM, PSP, WDT), with a host simulation backend so firmware runs on a PC before it touches hardware. | [README](pic16f87xa-hal/README.md) · [MANUAL](pic16f87xa-hal/MANUAL.md) |
| 🆕 | **[pic18fxx5x-hal](pic18fxx5x-hal/)** | Second family under the shared layer: PIC18F2455/2550/4455/4550. **Full peripheral coverage** (Phase 4 done): GPIO, Timer0-3, ECCP1/CCP2, MSSP, EUSART, Comparator, EEPROM, ADC, SPP, all register-level, cited to DS39632E, with a host sim backend. | [README](pic18fxx5x-hal/README.md) · [MANUAL](pic18fxx5x-hal/MANUAL.md) |
| 🗓️ | **[pic8-taskmgr](pic8-taskmgr/)** | A cooperative scheduler built on the HAL: periodic and one-shot tasks, priority-ordered, race-free. **Family-agnostic**, same `task_manager.c`/`.h` builds against `pic16f87xa-hal` or `pic18fxx5x-hal` (`-DHAL_FAMILY=PIC18`). | [README](pic8-taskmgr/README.md) · [Architecture](pic8-taskmgr/docs/ARCHITECTURE.md) · [API](pic8-taskmgr/docs/API.md) |
| 🚦 | **[pic8-fsm](pic8-fsm/)** | A vendor-agnostic, table-driven finite state machine engine: the whole machine is one `static const fsm_transition_t[]`, readable at a glance. **No HAL dependency, no per-family backend**, one `fsm.c` compiles unchanged for host, PIC16, and PIC18; composes with `pic8-taskmgr` by staying decoupled from it. | [README](pic8-fsm/README.md) · [Architecture](pic8-fsm/docs/ARCHITECTURE.md) · [API](pic8-fsm/docs/API.md) |
| 🔢 | **[pic8-math](pic8-math/)** | Fixed-point math utility library ported from AN526/AN544: multiply, divide, add/sub, BCD, sqrt, numerical diff/integration, RNGs. One family-agnostic API, three backends (host reference, PIC16 shift-add inline asm, PIC18 hardware-`MULWF` inline asm); derived routines are one portable-C body. **Exhaustive host tests** + an on-target self-test. | [README](pic8-math/README.md) · [Architecture](pic8-math/docs/ARCHITECTURE.md) · [API](pic8-math/docs/API.md) |
| ⏱️ | **[pic8-tick](pic8-tick/)** | A 1 ms timebase, the STM32Cube `HAL_GetTick`/`HAL_Delay` equivalent: `pic8_tick_get`/`pic8_tick_delay_ms`/`pic8_tick_elapsed_since` on a Timer2 auto-reload ISR. **Family-agnostic** (same source builds against `pic16f87xa-hal` or `pic18fxx5x-hal`); atomic 32-bit tick read; runs on the host sim and real silicon. | [README](pic8-tick/README.md) · [Architecture](pic8-tick/docs/ARCHITECTURE.md) · [API](pic8-tick/docs/API.md) |
| 📡 | **[pic8-serial](pic8-serial/)** | Interrupt-driven ring-buffered UART + `printf` retarget, the non-blocking serial layer Cube's `HAL_UART_Transmit_DMA`/`_IT` gives: `pic8_serial_write`/`read`/`available`/`flush` + `putch` for XC8 `printf`. **Family-agnostic**; TX is demand-driven, RX always-on; runs on the host sim and real silicon. | [README](pic8-serial/README.md) · [Architecture](pic8-serial/docs/ARCHITECTURE.md) · [API](pic8-serial/docs/API.md) |
| 🔌 | **[pic8-bus](pic8-bus/)** | I2C/SPI "MEM" register-access idiom, Cube's `HAL_I2C_Mem_Read`/`Mem_Write` + the SPI register-transaction pattern: `pic8_bus_i2c_mem_read`/`write`, `pic8_bus_spi_mem_read`/`write` on the MSSP/SSP HAL (adds the ACKDT-NACK + SSPIF-wait the HAL lacks). **Family-agnostic**; host-testable via an injectable ops seam (mock MEM device) since the sim has no SSP slave model. | [README](pic8-bus/README.md) · [Architecture](pic8-bus/docs/ARCHITECTURE.md) · [API](pic8-bus/docs/API.md) |
| 🛰️ | **[pic8-modbus](pic8-modbus/)** | Modbus RTU slave built on `pic8-serial` + `pic8-tick`: core function codes (01/02/03/04/05/06/15/16) over a plain-array register map, T3.5 silence-delimited framing, CRC-16, optional RS-485 driver-enable pin. **Family-agnostic** (no `#if` at all, GPIO is already neutral through the HAL contract). | [README](pic8-modbus/README.md) · [Architecture](pic8-modbus/docs/ARCHITECTURE.md) · [API](pic8-modbus/docs/API.md) |
| 🔘 | **[pic8-debounce](pic8-debounce/)** | Vendor-agnostic, instantiable digital-input debouncer: one `debounce_t` per input, a read callback (any GPIO / I2C bit / mock), poll-driven press/release edges on `pic8-tick`'s real timebase. **One implementation** (no per-family backend, no asm); host tests exercise real simulated timing. | [README](pic8-debounce/README.md) · [Architecture](pic8-debounce/docs/ARCHITECTURE.md) · [API](pic8-debounce/docs/API.md) |
| 📈 | **[pic8-adcfilter](pic8-adcfilter/)** | Vendor-agnostic ADC oversampling + moving average: oversample-and-decimate through a read callback plus an O(1) running-average filter over caller-owned storage. **Zero core HAL dependency**; the exact shipped code is host-tested directly. | [README](pic8-adcfilter/README.md) · [Architecture](pic8-adcfilter/docs/ARCHITECTURE.md) · [API](pic8-adcfilter/docs/API.md) |
| 💾 | **[pic8-settings](pic8-settings/)** | EEPROM-backed settings blobs with CRC-16 validation: save/load an opaque struct, detect blank/corrupt data, and optionally apply+persist defaults on first boot. **Family-agnostic** over the neutral `HAL_EEPROM_*` API, with its own safe multi-byte write sequencing. | [README](pic8-settings/README.md) · [Architecture](pic8-settings/docs/ARCHITECTURE.md) · [API](pic8-settings/docs/API.md) |
| 🖥️ | **[pic8-console](pic8-console/)** | Line-based serial command dispatcher layered over `pic8-serial`: fixed-size line buffer, in-place tokenization, table-driven dispatch, echo/backspace editing, and explicit opt-in help printing. **Family-agnostic**; works against the same UART ring-buffer layer on host and target. | [README](pic8-console/README.md) · [Architecture](pic8-console/docs/ARCHITECTURE.md) · [API](pic8-console/docs/API.md) |
| 🔛 | **[pic8-usb](pic8-usb/)** | USB CDC-ACM (virtual serial port) for PIC18F2455/2550/4455/4550, wrapping the vendored M-Stack USB device stack. **PIC18Fxx5x-only** (PIC16F87XA has no USB peripheral); mirrors `pic8_serial_*`'s API shape; polling-based (`pic8_usb_service()`); host stub tests the ring-buffer + connection-state contract only. | [README](pic8-usb/README.md) · [Architecture](pic8-usb/docs/ARCHITECTURE.md) · [API](pic8-usb/docs/API.md) |
| 💽 | **[pic8-sdcard](pic8-sdcard/)** | SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550, wrapping the vendored M-Stack storage driver. **PIC18Fxx5x-only** (RAM constraint — PIC16F87XA parts can't hold a 512-byte block buffer); binds `mmc.c`/`crc.c` to the SSP/GPIO HAL and `pic8-tick`; host tests exercise the real vendored protocol logic against a mock SPI slave. | [README](pic8-sdcard/README.md) · [Architecture](pic8-sdcard/docs/ARCHITECTURE.md) · [API](pic8-sdcard/docs/API.md) |
| 🖥️ | **[pic8-lcd](pic8-lcd/)** | HD44780-compatible character LCD driver (16x2, 20x4, etc.) with configurable transport: 4-bit GPIO, 8-bit GPIO, or SPI via 74HC595 shift register. **Family-agnostic** (transport-agnostic core with injectable ops); timed delays (no busy-flag polling); host tests verify command logic against the HD44780 spec via mock transport. | [README](pic8-lcd/README.md) · [Architecture](pic8-lcd/docs/ARCHITECTURE.md) · [API](pic8-lcd/docs/API.md) |
| 🎛️ | **[pic8-pid](pic8-pid/)** | Vendor-agnostic, single-loop, fixed-point PID controller with anti-windup, derivative-on-measurement, and bumpless auto/manual transfer. **One implementation** (no per-family backend, no asm), Q8.8 throughout, host-tested directly; 21 B per instance on PIC16 and PIC18. | [README](pic8-pid/README.md) · [Architecture](pic8-pid/docs/ARCHITECTURE.md) · [API](pic8-pid/docs/API.md) |
| 🎯 | **[pic8-encoder](pic8-encoder/)** | Vendor-agnostic, interrupt-driven x4 quadrature decoder for incremental encoders, instantiable (one `encoder_t` per A/B pair), built on the HAL's RB<7:4> interrupt-on-change (the GPIO change-interrupt hook this module motivated adding to both HALs). Push not poll, Gray-code transition table, two separate diagnostic counters (impossible transitions vs. glitch-gate rejections), atomic position read; composes with `pic8-pid` (`encoder_get_position` -> `pid_update` measurement). **One implementation** (no per-family backend, no asm, no `pic8-math` link), host-tested directly; 17 B per instance on PIC16 and PIC18; PORTB only, at most two encoders per port. | [README](pic8-encoder/README.md) · [Architecture](pic8-encoder/docs/ARCHITECTURE.md) · [API](pic8-encoder/docs/API.md) |
| 🧱 | **[pic8-common](pic8-common/)** | The shared layer every family reuses: status codes, the host/target harness contract, CMake/Make fragments. | [README](pic8-common/README.md) · [MANUAL](pic8-common/MANUAL.md) |
| 📐 | **[docs/multi-family-plan](docs/multi-family-plan.md)** | The refactor plan: extract `pic8-common`, add families behind a fixed contract. **Phases 0–3 done** (litmus test met). |, |

## Quick start

Build and run the multi-blink example on the host simulator (CMake 3.16+ and a
C99 compiler; the HAL is pulled in automatically). The default targets the
PIC16F87XA family:

```sh
cmake -B build -S pic8-taskmgr
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
slot). Point the same task manager at the PIC18 family instead with
`-DHAL_FAMILY=PIC18` (see [pic8-taskmgr/README.md](pic8-taskmgr/README.md)).

### Real hardware

For a real PIC, use the MPLAB XC8 toolchain. There is one Makefile per
family:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin

# PIC16F87XA family:
cd pic8-taskmgr/mcu/pic16f87xa-taskmgr-mplabx
make MCU=16F877A        # also 873A / 874A / 876A

# PIC18F2455 family (needs the PIC18Fxxxx DFP installed):
cd ../pic18fxx5x-taskmgr-mplabx
make MCU=18F4550        # also 2455 / 2550 / 4455
```

This produces `build/<MCU>-multi-blink.hex`. Wire LEDs on RB0-RB3 and a 20 MHz
crystal, then program with MPLAB X or any programmer. See the task manager
[README](pic8-taskmgr/README.md) for the full wiring and the HAL
[MANUAL](pic16f87xa-hal/MANUAL.md) for peripheral bring-up.

## Documentation

- **HAL**
  - [pic8-common/MANUAL.md](pic8-common/MANUAL.md): family-agnostic conventions, the harness, the handle pattern, the shared interrupt model — read this first
  - [pic16f87xa-hal/README.md](pic16f87xa-hal/README.md) / [MANUAL.md](pic16f87xa-hal/MANUAL.md): PIC16F87XA build + per-peripheral reference
  - [pic18fxx5x-hal/README.md](pic18fxx5x-hal/README.md) / [MANUAL.md](pic18fxx5x-hal/MANUAL.md): PIC18F2455 family build + per-peripheral reference
- **Task manager**
  - [README](pic8-taskmgr/README.md): overview, quick start, demo
  - [docs/ARCHITECTURE.md](pic8-taskmgr/docs/ARCHITECTURE.md): cooperative model, tick source, concurrency, RAM scaling, constraints
  - [docs/API.md](pic8-taskmgr/docs/API.md): full API reference
- **Multi-family**
  - [docs/multi-family-plan.md](docs/multi-family-plan.md): the refactor plan that extracted `pic8-common/` and added the PIC18F2455 family behind a fixed contract (**Phases 0–3 done**, litmus test met, real-silicon deferred)
  - [docs/hal-manual-plan.md](docs/hal-manual-plan.md): the doc-side split that gave every family the same `MANUAL.md` shape, sharing conventions once in `pic8-common/MANUAL.md`
- **Datasheets** (not vendored in this repo — Microchip vendor documentation, linked instead): [DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf) PIC16F87XA, [DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf) PIC18F2455 family

## Repository layout

```
.
├── pic8-common/                    # shared layer (every family reuses this)
│   ├── include/core/              #   hal_status.h (HAL_* / PIC8_BIT*), pic8_harness.h,
│   │                              #   pic8_irq.h (HAL_IRQ_Priority)
│   ├── src/core/                  #   pic8_harness_target.c (family-blind no-ops)
│   ├── cmake/  mk/                #   shared pic8_family.cmake / pic8_family.mk
│   └── README.md  MANUAL.md
│
├── pic16f87xa-hal/                 # the HAL (PIC16F87XA family), full peripheral coverage
│   ├── include/                    # public headers (core/ + peripherals/); core/
│   │                               #   holds pic16_irq.h + wdt_sleep, the shared
│   │                               #   harness header lives in pic8-common/
│   ├── src/                        # drivers + host sim backend
│   ├── tests/                      # end-to-end examples (blink, USART, ADC, …)
│   ├── mcu/pic16f87xa-mplabx/      # XC8 Makefile for real silicon
│   ├── README.md  MANUAL.md
│   └── CMakeLists.txt              # host simulation build
│
├── pic18fxx5x-hal/                 # second family (PIC18F2455/2550/4455/4550)
│   ├── include/  src/  tests/      # MVP: GPIO, Timer0, dual-priority IRQ, WDT/sleep
│   ├── mcu/pic18fxx5x-mplabx/      # XC8 Makefile (needs PIC18Fxxxx DFP, see README)
│   ├── README.md  MANUAL.md
│   └── CMakeLists.txt              # host simulation build (reuses pic8-common)
│
├── pic8-taskmgr/                   # the cooperative scheduler (family-agnostic)
│   ├── include/  src/  examples/   # library + multi-blink example
│   ├── docs/                      # ARCHITECTURE.md, API.md
│   ├── mcu/pic16f87xa-taskmgr-mplabx/   # XC8 Makefile (PIC16F87XA)
│   ├── mcu/pic18fxx5x-taskmgr-mplabx/   # XC8 Makefile (PIC18F2455 family)
│   ├── README.md
│   └── CMakeLists.txt              # host sim build, -DHAL_FAMILY=PIC16|PIC18
│
├── docs/multi-family-plan.md       # the refactor plan (Phases 0–3 done)
├── LICENSE                          # MIT
└── README.md                       # this file
```

## Requirements

- **Host simulation**, CMake ≥ 3.16 and any C99 compiler (gcc/clang).
- **Real target**, MPLAB X IDE v6.x and MPLAB XC8 v3.x (`xc8-cc`). Tested on a
  PIC16F877A and PIC18F4550; the examples use PORTB only, so they run on all
  parts of both families. The PIC18 XC8 build needs the PIC18Fxxxx Device
  Family Pack installed (the PIC16Fxxx DFP ships with XC8; the PIC18Fxxxx DFP
  does not, see [pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md](pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md)).

## License

MIT; see [LICENSE](LICENSE).

The Microchip datasheets [DS39582B](https://ww1.microchip.com/downloads/en/DeviceDoc/39582b.pdf)
(© 2003 Microchip Technology Inc.) and [DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf)
(© 2009 Microchip Technology Inc.) are vendor documentation; register and
bit names in the code follow them directly. They are not vendored in this
repo — follow the links above to Microchip's own hosted copies.