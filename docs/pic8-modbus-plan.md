# `pic8-modbus`: Modbus RTU slave, implementation plan

Status: **approved, implementing** — design validated against the existing
HAL (`pic8-serial`, `pic8-tick`, GPIO) before writing code; scope confirmed
with the user (RTU slave only, core function codes). This doc stands next to
the other `docs/*-plan.md` files as the durable design record; see
`pic8-modbus/docs/ARCHITECTURE.md` for the as-built version once complete.

## Why this module, and why from scratch

Nothing in this repo implements Modbus or CRC-16 today. External libraries
were surveyed first (nanoMODBUS, FreeMODBUS, liblightmodbus, PetitModbus,
`pic-modbus`, libmodbus, esp-modbus): none is vendored in. nanoMODBUS (MIT,
static/no-malloc, two-callback transport) and FreeMODBUS (BSD, real bare-metal
AVR port with a `port.c`/`portserial.c`/`porttimer.c` split) are the two worth
reading for *design shape*; GPL-licensed options (liblightmodbus,
PetitModbus, `pic-modbus`) are ruled out outright for a permissively-licensed
HAL. This module is written the way `pic8-math` ported AN526/AN544: read the
prior art for the idea, write idiomatic code for this repo's own HAL
contract.

## Scope (confirmed with the user)

- **Role**: RTU **slave** only. No master/client role.
- **Transport**: **RTU** binary framing + CRC-16 only. No ASCII, no TCP.
- **Function codes**: 01 (Read Coils), 02 (Read Discrete Inputs), 03 (Read
  Holding Registers), 04 (Read Input Registers), 05 (Write Single Coil), 06
  (Write Single Register), 15/0x0F (Write Multiple Coils), 16/0x10 (Write
  Multiple Registers).

## What it builds on (solved)

- **`pic8-serial`**: non-blocking ring-buffered UART
  (`pic8_serial_init/write/read/available/tx_pending/flush`). RTU byte I/O
  and RS-485 DE/RE timing (via `flush`, which blocks until the ring *and*
  shift register are both empty) come straight from here.
- **`pic8-tick`**: 1 ms monotonic timebase
  (`pic8_tick_get/elapsed_since`) drives the T3.5 inter-frame silence
  detection.
- **GPIO HAL** (`HAL_GPIO_WritePin`/`GPIO_TypeDef`, per-family): optional
  RS-485 driver-enable pin.
- **Module template**: `pic8-serial`'s file layout, and `pic8-debounce`'s
  `CMakeLists.txt`/Makefile pattern for linking a sibling `pic8-*` module
  (`pic8-debounce` links `pic8-tick` the same way `pic8-modbus` needs to
  link both `pic8-serial` and `pic8-tick`).
- **Host test pattern**: `pic8-serial/examples/example_serial.c`'s RX
  injection (`pic16f87xa_sim_drive_usart_rx` / PIC18 equivalent) + TX
  draining (`pic8_dispatch_all_irqs` + reading `PIC8_REG8(PIC_REG_TXREG)`),
  combined with `pic8_tick_delay_ms` to advance simulated time past T3.5.
- **Considered and rejected**: `pic8-fsm` for RTU frame assembly. RTU
  framing is silence-delimited, not event-delimited; a direct
  accumulate-then-check-elapsed poll loop is simpler than forcing it through
  transition-table guards/actions, the same call `pic8-debounce` made about
  not using `pic8-fsm` for its own poll loop.

## Design (solved)

### Public API

```c
typedef struct {
    uint8_t        *coils;               /* bit-packed, RW */
    uint16_t        num_coils;
    const uint8_t  *discrete_inputs;     /* bit-packed, read-only */
    uint16_t        num_discrete_inputs;
    uint16_t       *holding_regs;        /* RW */
    uint16_t        num_holding_regs;
    const uint16_t *input_regs;          /* read-only */
    uint16_t        num_input_regs;
} pic8_modbus_slave_map_t;

void pic8_modbus_slave_init(uint32_t fosc_hz, uint32_t baud,
                             uint8_t slave_addr,
                             const pic8_modbus_slave_map_t *map);
void pic8_modbus_slave_set_rs485_dir_pin(GPIO_TypeDef port, uint16_t pins);
void pic8_modbus_slave_poll(void);
```

`pic8_modbus_slave_init` calls `pic8_serial_init(fosc_hz, baud)` itself, so
the baud used for UART framing and the baud used for T3.5 timing math can
never drift apart, and precomputes the T3.5 timeout in ms.

Register access is **plain arrays**, not an injected ops-seam (unlike
`pic8-bus`, whose ops seam exists specifically to mock *hardware* the host
sim has no model for). Here the arrays are the actual data on both host and
target, so a callback layer would be pure overhead with no testability
benefit, the "no premature abstraction" rule applies directly.

### RTU framing (poll loop, called from `pic8_modbus_slave_poll`)

1. Drain `pic8_serial_available()`/`read()` into an internal buffer
   (`PIC8_MODBUS_MAX_ADU`, default 64 B, override-able like
   `PIC8_SERIAL_RING_SZ`), stamping `last_rx_tick = pic8_tick_get()` on every
   byte received.
2. Once the buffer is non-empty and
   `pic8_tick_elapsed_since(last_rx_tick) >= t3_5_ms`, the frame is
   considered complete: validate minimum length + CRC-16 (poly 0xA001,
   little-endian on the wire), check address match (exact match or
   broadcast 0), dispatch by function code, build the response PDU, append
   CRC, transmit.
3. Reset the buffer whether or not a response was sent, so the next byte
   starts assembling a fresh frame.
4. Broadcast (address 0): process writes, never respond, per spec.

Exceptions: illegal function (0x01), illegal data address (0x02), illegal
data value (0x03), encoded as `[FC | 0x80][code]`.

### CRC-16

A small bit-loop (poly 0xA001, init 0xFFFF), private to `src/pic8_modbus.c`.
Not table-driven (saves ~512 B flash the frame rate doesn't need back), and
not added to `pic8-math` (that module is fixed-point arithmetic; nothing
else in the repo needs CRC yet).

### RS-485 direction control

Optional. Unconfigured: TX goes out via `pic8_serial_write` untouched (fine
for point-to-point/TTL, or an auto-sensing transceiver). Configured: raise
the pin, write the response, `pic8_serial_flush()` (blocks until ring *and*
shift register are empty), then lower the pin, matching the "hold DE until
the stop bit has actually left the pin" requirement every RS-485
transceiver datasheet states.

## Known limitations (pending, explicitly not fixed in v1)

- **Timing resolution above ~19200 baud.** `pic8_tick`'s 1 ms granularity is
  coarser than the spec's T3.5 above 19200 baud (e.g. T3.5 is ~304 µs at
  115200). Framing still works correctly as long as the poll loop runs
  faster than T3.5, but exact conformance timing is approximate at higher
  baud rates. Acceptable for the 9600/19200 RTU links these parts actually
  see; a hardware-timer-based sub-ms T3.5 would be the fix if a future need
  demands strict high-baud conformance.
- **No T1.5 inter-character corruption detection.** A well-behaved master
  never triggers this; strict conformance would additionally abort a
  frame-in-progress if the gap between two bytes *within* the same frame
  exceeds T1.5. Not implemented in v1.
- **Master/client role, ASCII, TCP**: out of scope per the confirmed scope
  above, not partially started anywhere.

## Files

```
pic8-modbus/
  README.md
  docs/ARCHITECTURE.md
  docs/API.md
  CMakeLists.txt
  include/pic8_modbus.h
  src/pic8_modbus.c
  examples/example_modbus.c         # host ctest
  examples/example_modbus_target.c  # real-target link/init smoke
  mcu/pic16f87xa-modbus-mplabx/Makefile
  mcu/pic18fxx5x-modbus-mplabx/Makefile
```

## Verification

- `cmake -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`, then `cmake -B build18 -DHAL_FAMILY=PIC18 && ctest
  --test-dir build18`, both green.
- Host test asserts exact wire bytes (including computed CRC) for a read FC,
  a write FC, an exception path, and the broadcast no-reply path.
- `make -C mcu/pic16f87xa-modbus-mplabx MCU=16F877A` and the PIC18
  equivalent link cleanly (smoke only, no live transaction, same caveat as
  `pic8-bus`'s target example).
