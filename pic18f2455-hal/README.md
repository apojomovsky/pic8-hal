# PIC18F2455 family HAL

Hardware abstraction layer for the **PIC18F2455 / 2550 / 4455 / 4550** family,
inspired by the STM32Cube HAL API and structured as a sibling of the
PIC16F87XA HAL. Every constant, register address and behaviour is taken
1-to-1 from the datasheet [DS39632E](https://ww1.microchip.com/downloads/en/DeviceDoc/39632e.pdf).

This tree is the second family under the shared `pic8-common/` layer
(see [docs/multi-family-plan.md](../docs/multi-family-plan.md)). The status
codes, bit helpers, the host/target harness contract, and the shared
interrupt-dispatch name (`pic8_dispatch_all_irqs`) all come from
`pic8-common/` unchanged; only the register-specific parts (SFR map,
BSR/Access-Bank platform, dual-priority interrupt backend, peripheral
drivers) live here.

## Status

**Phase 1: scaffold.** The build seam is proven, no real drivers exist yet.

- ✅ Family header (`pic18f2455.h`): device select for all four parts,
  family capability macros (flash / RAM / EEPROM / I/O / ADC channels /
  PORTD / PORTE / SPP / USB), pulls in the shared status codes and the
  platform layer.
- ✅ Platform layer (`include/host` + `include/target` `pic18_platform.h`):
  the same `pic8_sfr_read8` / `PIC8_REG8` / `PIC8_WEAK` contract as PIC16,
  host version backed by a memory array, target version a volatile
  dereference. The BSR / Access-Bank model is provisional here and is
  finalized in Phase 2.
- ✅ Host simulation backend (`pic18_sim.c`, minimal): reset + step + IRQ
  callback. Phase 2 grows Timer0 stepping and GPIO drive/read.
- ✅ Shared interrupt dispatch (`pic18_irq_dispatch.c`): `pic8_dispatch_all_irqs`,
  empty body in Phase 1 (no handlers yet). Phase 2 fills in the per-source
  fan-out and the two-vector `pic18_isr_vector.c`.
- ✅ Test/firmware harness wired up via the shared `pic8_harness.h`; the
  host implementation is `pic18_harness_sim.c`, the target implementation
  is the family-blind `pic8_harness_target.c` in `pic8-common/`.
- ✅ CMake host build + XC8 Makefile, both thin callers of the shared
  `pic8-common/cmake/pic8_family.cmake` and `pic8-common/mk/pic8_family.mk`.
- ✅ `example_smoke`: proves the shared harness contract links and runs
  against the empty PIC18 backend (the central Phase 1 claim).

**Not yet (Phase 2+):** SFR map, GPIO (write-through-LATx), Timer0,
dual-priority interrupt core, WDT/Sleep, host-sim peripheral stepping,
`example_blink`. See the plan's Phase 2 task list.

## Layout

```
pic18f2455-hal/
├── include/
│   ├── pic18f2455.h              Family header, device selection, platform
│   │                             include; pulls in shared status codes /
│   │                             bit helpers from pic8-common/hal_status.h
│   ├── pic18f2455_sim.h          Simulation backend public API (Phase 1 minimal)
│   ├── host/pic18_platform.h     Host platform: memory-backed SFR + weak attr
│   ├── target/pic18_platform.h   Target platform: volatile-deref SFR
│   ├── core/                     (Phase 2: pic18_irq.h)
│   └── peripherals/              (Phase 2: HAL_GPIO_*, HAL_TIMER0_*, ...)
├── src/
│   ├── core/                     harness_sim, irq_dispatch (Phase 2: isr_vector)
│   ├── peripherals/              (Phase 2)
│   └── sim/                      Host simulation backend
├── tests/                        example_smoke (Phase 2: example_blink, ...)
├── mcu/pic18f2455-mplabx/        XC8 Makefile (thin caller of pic8-common/mk)
└── CMakeLists.txt                Host build (thin caller of pic8-common/cmake)
```

## Build (host simulation)

```sh
cmake -B build -S .
cmake --build build

./build/example_smoke
./build/example_smoke_PIC18F2455
./build/example_smoke_PIC18F2550
./build/example_smoke_PIC18F4455
./build/example_smoke_PIC18F4550
```

Each prints `smoke: 10 ticks, device <PART>` and exits 0. This is the proof
that `pic8_harness_*` is family-blind: PIC18's empty backend links against
the exact same header and contract PIC16 uses.

## Build (real target)

The HAL compiles against XC8 via the same sources, with the host/target
split done at **build time, not with `#ifdef`**: the XC8 Makefile puts
`include/target` ahead of `include` on the include path, so
`pic18_platform.h` resolves to the real-target version (SFR macros =
direct volatile dereference) and the family-blind target harness is
linked. The MPLAB X project lives under `mcu/pic18f2455-mplabx/`; its
Makefile produces `<MCU>-firmware.hex` via `xc8-cc`.

### PIC18F DFP requirement

XC8 v3.x moved device support into Device Family Packs. The **PIC18Fxxxx
DFP is not bundled with XC8** (unlike the PIC16Fxxx DFP); install it once:

- Via MPLAB X: *Tools → Packs → Pack Manager*, search `PIC18Fxxxx_DFP`.
- Or manually: download `Microchip.PIC18Fxxxx_DFP.<ver>.atpack` from
  [packs.download.microchip.com](https://packs.download.microchip.com/)
  and unzip it into
  `/opt/microchip/xc8/v3.10/pic/packs/Microchip.PIC18Fxxxx_DFP/`
  (the Makefile's `DFP_DIR` defaults to the `xc8/` subdir there).

Then:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
make MCU=18F4550    # default; also 18F2455 / 18F2550 / 18F4455
```

The result is `build/18F4550-firmware.hex`. The Phase 1 config-word set
is a conservative default (HS oscillator, WDT on, LVP off, XINST off,
code/write/read protection off); adjust per your hardware. The block-3
code-protection settings (`CP3` / `WRT3` / `EBTR3`) are emitted only for
the 32 KB parts (18F2550 / 18F4550), since the 24 KB parts stop at block 2
(DS39632E §23.1).

## XC8 PIC18 interrupt syntax (resolved, Phase 1 open question)

PIC18F2455/2550/4455/4550 are classic dual-priority-vector PIC18 devices
(vectors at 0008h high, 0018h low; DS39632E §9.0), not the vectored-
interrupt-controller variants. Per the XC8 C Compiler User Guide for PIC
(DS50002737K) §5.9.1.2 "Writing Dual-priority or Legacy Mode ISRs", the
syntax Phase 2 uses for `pic18_isr_vector.c` is:

```c
void __interrupt(high_priority) PIC18_IRQ_HandlerHigh(void) { ... }
void __interrupt(low_priority)  PIC18_IRQ_HandlerLow(void)  { ... }
```

If no priority argument is present the ISR defaults to high priority;
Microchip recommends always specifying it. This is recorded in
[docs/multi-family-plan.md](../docs/multi-family-plan.md).

## API conventions

Same as the PIC16F87XA HAL (mirror STM32Cube): `HAL_PPP_Init/DeInit`,
`HAL_PPP_MspInit` weak override, `GPIOA..` / `GPIO_PIN_*`, `HAL_OK/ERROR/
BUSY/TIMEOUT/INVALID`, `PIC8_BIT*`. The IRQ enum will be `PIC18_IRQ_*`
(Phase 2), taking the per-family `PIC18_IRQn` type, with the priority
contract extension decided and recorded in the plan before the
interrupt core is written.