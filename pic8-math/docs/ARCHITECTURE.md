# `pic8-math` architecture

> Status: **Phase 0 scaffold.** The inline-asm operand-binding convention
> (the XC8 round-trip probe) and the full per-family backend write-up land
> here before Phase 1's arithmetic bodies — see the "Inline-asm binding"
> section below (added by the probe). The rest of this document is filled
> out through Phase 5.

## What it is

`pic8-math` is a stateless fixed-point math utility library for the 8-bit
PIC families this repo already supports (PIC16F87XA, PIC18F2455/2550/4455/
4550), ported from Microchip application notes AN526 and AN544 and
modernized into one family-agnostic C library with a hand-written
inline-asm core. See `docs/pic8-math-plan.md` (repo root) for the source
material and the full implementation plan.

## Backend split (one API, three implementations)

| Layer | Source | Built by | Notes |
|---|---|---|---|
| Leaf primitives (mul/div/addsub/bcd) — **host** | `src/host/` | CMake | Portable C; the Tier-1 oracle |
| Leaf primitives — **PIC16** | `src/pic16/` | XC8 Makefile | shift-add (no HW multiply) |
| Leaf primitives — **PIC18** | `src/pic18/` | XC8 Makefile | hardware `MULWF`, `addwfc`/`subfwb` |
| Derived routines (sqrt/numeric/rand) — **all** | `src/common/` | both | One portable-C body, no asm |

The build selects the backend by source-list, never `#ifdef` — the same
pattern the HALs use for their host/target split.

## Why the host build links no HAL

Unlike `pic8-taskmgr` (which `add_subdirectory()`s a HAL), `pic8-math`'s
CMake host build links only `src/host/` + `src/common/`. The public header
needs only `<stdint.h>`/`<stdbool.h>` and the library is stateless
computation, so the Tier-1 host tests need no peripheral model. The family
dimension (and the HAL the Tier-3 on-target self-test needs for USART)
lives entirely in the XC8 Makefile targets under `mcu/`. This is the
plan-consistent reading of Tier 1 ("build `src/host/` + `src/common/` as a
host static lib and test it directly").

## Inline-asm binding

_Populated by the Phase-0→1 XC8 round-trip probe (task #2): the exact
operand-binding pattern for getting a C parameter into a register and a
computed result back out via XC8's `asm()`, and the PIC18 `WREG`/`PRODL`/
`PRODH` SFR additions to `pic18fxx5x_sfr.h`._

## Testing tiers

- **Tier 1 — host unit tests** (automated, every CTest run): exhaustive
  and randomized coverage of the portable-C oracle under `tests/`.
- **Tier 2 — asm regression** (optional, best-effort): `gpsim` replaying
  `golden_vectors.h` against the PIC16 inline asm. Feature-detected,
  skipped if `gpsim` is absent (it is, in this environment). No PIC18
  equivalent — relies on Tier 3 + code review.
- **Tier 3 — on-target validation** (manual/deferred): `target_selftest.c`
  runs `golden_vectors.h` through the real asm on silicon and streams
  PASS/FAIL over `HAL_USART_*`. The acceptance step before any firmware
  ships depending on this library.