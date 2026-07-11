# Multi-family PIC HAL: refactor plan

Status: **approved, not started**. This document is the plan agreed for
generalizing the PIC16F87XA HAL so it can support additional 8-bit PIC
families (starting with PIC18F2455/2550/4455/4550, DS39632E) without
rewriting the whole tree per chip. Phase 0 is the next work to start.

## Motivation

The HAL and task manager currently assume exactly one architecture:
PIC16F87XA's single interrupt vector, STATUS-bank register addressing, and
register set. A second family was requested (PIC18F2455/2550/4455/4550,
datasheet `39632e.pdf`), and a third and fourth are expected to follow. The
goal is a structure where adding a new family is bounded, mostly-mechanical
work: implement a fixed contract once per chip, not re-derive the whole
harness/build/status-code machinery every time.

## Decision: Option A (shared core, per-family backend)

Considered and rejected: a fully parallel HAL tree per family (no shared
code beyond copy-paste of the pattern). Rejected because the stated goal is
minimizing the diff for chip #3 and #4, and a parallel-tree approach doesn't
shrink that diff, each new family re-derives the same harness contract,
build boilerplate, and status-code enum from scratch.

Chosen instead: extract the parts of the current HAL that are genuinely
architecture-blind into a shared layer (`pic8-common/`), and keep everything
that is genuinely register-specific (SFR maps, bank/BSR addressing, IRQ
vector layout, peripheral register semantics) in per-family trees that all
implement the same contract. See the design writeup in conversation for the
full comparison table; the short version:

**Shared across every family** (extract once, reuse forever):
- Status codes and bit macros (`HAL_StatusTypeDef`, `PIC8_BIT*`)
- The host/target harness contract (`init/tick/running/log`) and its
  target-side no-op implementation (identical on every family, it's four
  functions that do nothing because real silicon needs no harness)
- `HAL_PPP_Init/DeInit/MspInit` naming convention and the weak-ISR-override
  pattern
- Build-system boilerplate (CMake helper functions, Makefile pattern rules)

**Per-family, implementing a fixed contract**:
- SFR address map and bank/BSR addressing scheme
- `platform.h` (how an SFR read/write is spelled, in each of host/target ×
  family)
- IRQ vector layout and priority model (PIC16: one vector, no priority;
  PIC18: two vectors, per-source priority bit)
- Peripheral driver register-level implementation (`HAL_GPIO_*`,
  `HAL_TIMER0_*`, etc. keep the same *names* and *signatures*; the bodies
  differ because the registers differ)
- Config Word generation (PIC16 has one config word; PIC18 has several,
  unrelated fields)

## Target tree layout

```
pic8-common/                         # NEW — thin, stable, rarely touched after Phase 0
  include/core/hal_status.h          # HAL_StatusTypeDef, PIC8_BIT*
  include/core/pic8_harness.h        # the 4-function host/target contract
  src/core/pic8_harness_target.c     # the one shared no-op file
  cmake/pic8_family.cmake            # shared CMake helpers (add_example, etc.)
  mk/pic8_family.mk                  # shared Makefile include (pattern rules, VPATH)

pic16f87xa-hal/                      # EXISTING tree, restructured in place
  include/{host,target}/...          # unchanged pattern; defines PIC8_REG8 etc.
  include/core/pic16_irq.h           # PIC16 IRQn enum + backend (renamed from
                                      # pic16f87xa_interrupt.h's family-specific half)
  include/peripherals/...            # HAL_GPIO_*, HAL_TIMER0_* — names unchanged
  src/...

pic18f2455-hal/                      # NEW — same skeleton as pic16f87xa-hal
  include/{host,target}/...          # PIC18 BSR/Access-Bank addressing, LATx-aware GPIO
  include/core/pic18_irq.h           # PIC18 IRQn enum, 2-vector + priority backend
  include/peripherals/...            # same HAL_GPIO_*/HAL_TIMER0_* names, PIC18-shaped bodies
  src/...
  mcu/pic18f2455-mplabx/Makefile     # includes mk/pic8_family.mk

pic16f87xa-taskmgr/                  # UNCHANGED in scope — must become provably
                                      # family-agnostic (see Phase 3 validation)
```

## Litmus test for the whole effort

After Phase 3, the task manager's build can point at `pic18f2455-hal`
instead of `pic16f87xa-hal` with **zero changes to `task_manager.c` or
`task_manager.h`**, and `example_multi_blink` passes on the PIC18 host sim
and on real PIC18 silicon. If that requires touching the task manager, the
contract in `pic8-common/` isn't right yet and Phase 0-2 need revisiting
before peripheral coverage grows further.

## Phases

Each phase has an explicit validation step. A phase is not done until its
validation passes; don't start the next phase on a red validation.

---

### Phase 0 — Extract the shared layer, rename in place (PIC16 only)

No new hardware support in this phase. Pure, behavior-preserving refactor
of the existing PIC16F87XA HAL.

**Tasks**
1. Create `pic8-common/` with `hal_status.h` (`HAL_StatusTypeDef`,
   `HAL_OK/ERROR/BUSY/TIMEOUT/INVALID`, `PIC8_BIT/BIT_SET/BIT_CLR/BIT_TGL/
   BIT_READ`) and `pic8_harness.h` (the existing 4-function contract, moved
   and renamed from `pic16f87xa_harness_*` to `pic8_harness_*`).
2. Move `pic16f87xa_harness_target.c` to `pic8-common/src/core/
   pic8_harness_target.c` unchanged in substance (it's already
   family-blind: four no-ops).
3. Rename in `pic16f87xa-hal`:
   `PIC16F87XA_StatusTypeDef`/`_OK`/... → use the shared `HAL_*` from
   `pic8-common`; `PIC16F87XA_BIT*` → `PIC8_BIT*`; `PIC16F87XA_IRQ_Disable/
   Restore/Enable/DisableSrc/ClearFlag/GetFlag` → `HAL_IRQ_*` (still taking
   a PIC16-defined `IRQn` enum); `PIC16F87XA_WEAK` → `PIC8_WEAK`;
   `PIC16F87XA_REG8`/`_SFR_PTR` → `PIC8_REG8`/`PIC8_SFR_PTR`.
4. Extract shared CMake helper functions (e.g. the `pic16f87xa_add_example`
   pattern) into `pic8-common/cmake/pic8_family.cmake`, parameterized so
   `pic16f87xa-hal/CMakeLists.txt` becomes a thin caller.
5. Extract shared Makefile pattern rules (the `.c` → `.p1` rule, VPATH
   setup, the `.hex` link step) into `pic8-common/mk/pic8_family.mk`, and
   make `pic16f87xa-hal/mcu/pic16f87xa-mplabx/Makefile` include it.
6. Update `pic16f87xa-taskmgr` to link against the renamed symbols
   (`HAL_IRQ_Disable/Restore` instead of `PIC16F87XA_IRQ_Disable/Restore`,
   etc.).
7. Update docs (`pic16f87xa-hal/README.md`, `MANUAL.md`,
   `pic16f87xa-taskmgr/README.md`, `docs/API.md`, `docs/ARCHITECTURE.md`,
   root `README.md`) for the renamed public API surface.

**Explicitly out of scope for Phase 0**: no PIC18 code, no new directory
for a second family, no behavior change of any kind. If a test's *output*
changes, that's a bug in the refactor, not an intended improvement.

**Validation**
- [ ] `pic16f87xa-hal` host-sim build (`cmake --build`) succeeds with zero
      warnings introduced.
- [ ] Every existing HAL example (`example_blink`, `example_idle_blink`,
      `example_timer1/2`, `example_ccp_pwm`, `example_usart`, `example_ssp`,
      `example_adc`, `example_comp_vref`, `example_eeprom`, `example_psp`,
      `example_wdt_sleep`) passes with **identical** stdout/exit-code to
      before the refactor. Diff the captured output pre- and post-refactor,
      don't just check exit codes.
- [ ] `pic16f87xa-taskmgr` host-sim build succeeds; `example_multi_blink`
      produces identical output to the pre-refactor baseline (`fast=12
      med=6 slow=3 blips=1 (ticks=61, tasks=4)`).
- [ ] XC8 real-target build succeeds for all four devices (873A/874A/876A/
      877A) with no new warnings; `.hex` output size is unchanged (a size
      change would indicate the rename affected codegen, which it must
      not).
- [ ] `grep -r PIC16F87XA_BIT\|PIC16F87XA_StatusTypeDef\|PIC16F87XA_IRQ_`
      the tree, expect zero hits outside of comments/docs explaining the
      old name for migration context (if any are kept).
- [ ] Root README and both component READMEs build/link-check clean (no
      broken relative links after any file moves).

**Exit criterion**: all boxes checked, changes committed. This is the
foundation every later phase depends on; do not proceed to Phase 1 with a
partially-done rename.

---

### Phase 1 — Scaffold `pic18f2455-hal/` (empty backend)

Stand up the skeleton so the family-selection seam can be exercised before
any real PIC18 register code exists.

**Tasks**
1. Create `pic18f2455-hal/` mirroring `pic16f87xa-hal/`'s directory shape:
   `include/{host,target}/`, `include/core/`, `include/peripherals/`,
   `src/{core,peripherals,sim}/`, `tests/`, `mcu/pic18f2455-mplabx/`.
2. Device-select header (`pic18f2455.h` analog to `pic16f87xa.h`): defines
   exactly one of `PIC18F2455/2550/4455/4550`, sets family capability
   macros (flash size, RAM, ADC channel count, has-SPP, etc., mirroring
   `PIC16F87XA_FAMILY_HAS_*`).
3. `include/host/pic18_platform.h` and `include/target/pic18_platform.h`
   stubs: host version backed by a memory array (size TBD once the BSR
   model is mapped in Phase 2), target version a placeholder volatile
   dereference. Both can be minimal/incomplete at this stage, they exist so
   the build wires up, not so registers work yet.
4. `CMakeLists.txt` and `mcu/pic18f2455-mplabx/Makefile`, both including
   `pic8-common`'s shared CMake/Makefile fragments from Phase 0.
5. One trivial smoke source (`tests/example_smoke.c`) that does nothing but
   call `pic8_harness_init/tick/running/log/report`, to prove the harness
   contract links against an empty family backend.

**Explicitly out of scope**: no real SFR addresses, no working GPIO or
Timer0 yet. This phase proves the *build* seam, not the *hardware*.

**Validation**
- [ ] `cmake -B build -S pic18f2455-hal && cmake --build build` succeeds
      and produces `example_smoke`.
- [ ] `./build/example_smoke` runs and reports pass via the shared harness
      (proves `pic8_harness_*` really is family-blind, since PIC18's
      backend links against the exact same header/contract PIC16 uses).
- [ ] XC8 build of the skeleton (empty peripheral set) produces a `.hex`
      for at least one device (e.g. PIC18F4550) with no link errors, proving
      the Makefile fragment + config-word generation plumbing works end to
      end even before real drivers exist.
- [ ] Confirm the XC8 v3.10 PIC18 interrupt syntax to use in Phase 2
      (`#pragma interrupt` vs `__interrupt(high_priority)` vs something
      else) against the actual XC8 compiler documentation, not assumption.
      Record the answer in this document before Phase 2 starts.

**Exit criterion**: an empty but buildable-on-both-targets PIC18 tree
exists, and the interrupt-syntax open question is resolved and recorded.

---

### Phase 2 — Port the MVP vertical slice from DS39632E

The smallest slice that lets the task manager run: GPIO, Timer0, interrupts
(two vectors + priority), WDT/sleep. Chosen because it's exactly the task
manager's entire HAL dependency surface (confirmed by grepping
`pic16f87xa-taskmgr/src/task_manager.c` and the example), so finishing it
is both useful on its own and sets up Phase 3 directly.

**Tasks**, each cited against DS39632E the way the PIC16 drivers cite
DS39582B:
1. SFR map subset: `STATUS`, `BSR`, `PORTB`/`LATB`/`TRISB`,
   `INTCON`/`INTCON2`/`INTCON3`, `PIR1`/`PIE1`/`IPR1`, `TMR0L`/`TMR0H`/
   `T0CON`.
2. `platform.h` (host + target): BSR-aware access replacing PIC16's
   `pic_select_bank()` scheme. Confirm whether the Access Bank shortcut is
   worth modeling in the host sim or whether a flat addressable array
   (sized for the banks actually implemented, per Figure 5-5) is simpler
   and sufficient for now.
3. GPIO driver: **write through LATx, not PORTx.** This is a real
   correctness improvement PIC18 provides natively (DS39632E §10.0), not a
   workaround, unlike PIC16 where the driver conventionally treats PORTx
   writes as read-modify-write of the latch, PIC18 exposes the latch as its
   own mapped register.
4. Timer0 driver: PIC18's T0CON adds an 8/16-bit mode bit PIC16 doesn't
   have; decide the default and document the citation.
5. Interrupt core: two vectors (0008h high-priority, 0018h low-priority),
   `IPEN` (RCON<7>) enabling the priority scheme, per-source priority bits
   in `INTCON2`/`INTCON3`/`IPR1`/`IPR2`. Design the `HAL_IRQ_*` contract
   extension needed to express priority without breaking PIC16 callers,
   options: an optional priority parameter PIC16's implementation ignores,
   or a separate `HAL_IRQ_SetPriority()` that's a no-op on PIC16. Pick one,
   document why, in this file, before implementing.
6. ISR vector entry file(s): using the syntax confirmed in Phase 1, one
   entry point per vector, both delegating to the shared dispatch pattern
   already used by PIC16 (`pic8_dispatch_all_irqs` per vector, or per
   priority level, decide during implementation and record the choice).
7. WDT/Sleep driver: port `HAL_WDT_Refresh`/`HAL_Sleep_Enter`/BOR/POR
   status against PIC18's equivalent registers (RCON's `TO`/`PD`/`BOR`/`POR`
   are laid out differently than PIC16's `PCON`, confirm exact bits against
   DS39632E before porting).
8. Host sim backend (`pic18_sim.c`): Timer0 stepping + GPIO drive/read
   hooks, mirroring `pic16f87xa_sim.c`'s approach and public API shape
   (`pic8_sim_reset/step/drive_input/read_output/set_irq_callback`, moved
   to the shared naming from Phase 0 where the API is actually
   family-blind).
9. Example: a PIC18 `example_blink` analog exercising Timer0 + GPIO +
   interrupts, buildable on host sim and XC8.

**Validation**
- [ ] Host-sim `example_blink` (PIC18) toggles the target pin and reports
      pass, mirroring the PIC16 example's structure and pass criteria.
- [ ] XC8 build produces a `.hex` for at least PIC18F4550 (40-pin, matches
      the family's most-featured part) with the interrupt vectors correctly
      placed at 0008h/0018h (verify via the `.hex`/`.lst`/map output, not
      just "it compiled").
- [ ] On real PIC18 hardware (or, if hardware isn't available yet, flag
      this explicitly as deferred rather than skipped silently): LED blinks
      at the expected rate, confirming the Timer0 + interrupt + GPIO chain
      actually works end to end, not just that it links.
- [ ] `HAL_WDT_Refresh` / `HAL_Sleep_Enter` compile and, on host sim, behave
      as documented no-ops exactly like the PIC16 versions.
- [ ] Re-run all of Phase 0's PIC16 validation checks, this phase must not
      have touched anything under `pic16f87xa-hal/` or `pic8-common/` in a
      way that regresses PIC16. If it did, that's a sign the contract
      extension (priority, etc.) leaked family-specific assumptions into
      the shared layer, fix before continuing.

**Exit criterion**: a working PIC18 blink example on host sim, with the
XC8 build producing correct vector placement, and zero PIC16 regression.

---

### Phase 3 — Point the task manager at PIC18 (the litmus test)

**Tasks**
1. Add a `pic16f87xa-taskmgr` build variant (or a build-time switch) that
   links against `pic18f2455-hal` instead of `pic16f87xa-hal`.
2. Do **not** modify `task_manager.c`/`task_manager.h` to make this work.
   If a modification seems necessary, stop and treat it as a Phase 0-2 gap:
   the shared contract is missing something, go fix the contract, not the
   consumer.
3. Adjust only `example_multi_blink.c`'s device-specific bits if any exist
   (there shouldn't be many, it was already written against PORTB generically
   for HAL portability across PIC16 family members).

**Validation**
- [ ] `example_multi_blink` builds against `pic18f2455-hal` on host sim
      with zero changes to `task_manager.c`/`task_manager.h` (diff those
      two files against Phase 2's end state, expect empty diff).
- [ ] Host-sim run produces the same *shape* of output (four distinct
      blink rates, one spawned one-shot, slot reuse confirmed via
      `tasks=N` not growing) as the PIC16 run, exact tick counts may differ
      if Timer0 timing constants differ between families, but the pass/fail
      logic and behavioral guarantees must hold identically.
- [ ] XC8 build succeeds and produces a `.hex` for the PIC18 target.
- [ ] On real PIC18 hardware (or explicitly flagged as deferred): four LEDs
      blink at four distinct rates, matching the PIC16 hardware behavior
      described in the task manager's README.

**Exit criterion**: the litmus test in this document's introduction passes.
This is the point where "the refactor worked" becomes demonstrated, not
just designed.

---

### Phase 4+ — Broaden PIC18 peripheral coverage

Not blocking for the multi-family architecture claim, this is ordinary HAL
growth at the established one-peripheral-at-a-time pace: Timer1/2/3, ECCP
(including dead-band/shutdown, PIC18-only vs. PIC16's plain CCP), MSSP,
ADC, comparator, EEPROM, USART. USB and the extended instruction set are
large enough to scope as separate future efforts, not part of "general
8-bit PIC support."

**Validation per peripheral** (repeat of the existing HAL's established
pattern, nothing new to invent): datasheet-cited driver, host-sim example,
XC8 build, and where hardware is available, a real-hardware confirmation,
matching the rigor already applied to every PIC16 peripheral driver.

---

## How to add family #3 (and #4), after this refactor lands

1. New sibling tree (`<partno>-hal/`), skeleton copied from whichever
   existing family tree is architecturally closer. Note: closeness is
   about addressing model and interrupt architecture, not pin count or
   peripheral list, e.g. PIC16F1xxx (enhanced midrange) has its own
   BSR-like scheme distinct from both classic PIC16 and PIC18, so expect
   real driver work regardless of which tree is copied as a starting point.
2. Implement that family's platform header, SFR map, IRQ backend, and
   peripheral drivers, citing its datasheet the way both existing trees do.
3. Everything under `pic8-common/` is reused untouched, this is the payoff
   of Phase 0. If it turns out something needs to change in
   `pic8-common/` to fit family #3, that's a signal the contract was
   accidentally PIC16/PIC18-specific somewhere and needs a small follow-up
   fix, not a sign the whole approach is wrong.
4. Run the same Phase 3-style litmus test: point the task manager (or
   whatever else consumes the HAL by then) at the new family, expect zero
   changes to the consumer.

## Open questions (resolve during the phase noted)

- **XC8 v3.10 PIC18 interrupt pragma/attribute syntax** — resolve in
  Phase 1, record the answer in this document.
- **How much of PIC18's BSR/Access-Bank addressing to model in the host
  sim** (full 16-bank fidelity vs. a simpler flat array sized to what's
  actually implemented) — resolve during Phase 2 task 2.
- **Shape of the `HAL_IRQ_*` priority contract extension** (optional
  parameter vs. separate setter) — resolve during Phase 2 task 5, before
  writing the interrupt core.
- **Hardware availability for real-silicon validation** — if PIC18
  hardware isn't on hand when Phase 2/3 validation is reached, flag it
  explicitly in the validation checklist as deferred rather than silently
  skipping the check.
