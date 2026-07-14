# `pic8-tick` API reference

Authoritative declarations: [`include/pic8_tick.h`](../include/pic8_tick.h).

### `void pic8_tick_init(uint32_t fosc_hz)`
Start the 1 ms timebase. Computes the Timer2 PR2/prescaler/postscaler closest
to 1 ms from `fosc_hz` (exact for common Fosc: 4/8/16/20/32/48 MHz), installs
the tick ISR via the Timer2 `OverflowCallback`, and enables the timer. Call
once at startup. The Timer2 ISR increments the internal counter in the
background from then on.

### `uint32_t pic8_tick_get(void)`
Milliseconds since `pic8_tick_init`. Monotonic; wraps every ~49.7 days
(2³² ms). The 32-bit read is atomic against the ISR (interrupts disabled
around the read), so no torn read.

### `void pic8_tick_delay_ms(uint32_t ms)`
Block for `ms` milliseconds. Guarantees at least `ms` (may overshoot by up to
~1 tick). On the host sim it pumps `pic8_harness_tick()` so simulated time
advances; on a real target it spins while the Timer2 ISR advances the counter.

### `uint32_t pic8_tick_elapsed_since(uint32_t t0)`
`pic8_tick_get() - t0` — the non-blocking idiom
`if (pic8_tick_elapsed_since(t0) >= timeout)` instead of blocking.
Wraparound-safe (unsigned subtraction), so it stays correct across the
~49.7-day roll.

## Usage

```c
pic8_tick_init(FOSC_HZ);            /* once */

pic8_tick_delay_ms(100);            /* blocking 100 ms */

/* non-blocking timeout */
uint32_t t0 = pic8_tick_get();
do_something_chunk();
if (pic8_tick_elapsed_since(t0) >= 50u) { /* took longer than 50 ms */ }
```

## Cheat sheet

| Function | Purpose |
|---|---|
| `pic8_tick_init` | start the 1 ms Timer2 timebase |
| `pic8_tick_get` | monotonic ms count (atomic read) |
| `pic8_tick_delay_ms` | blocking delay in ms |
| `pic8_tick_elapsed_since` | non-blocking ms-since-t0 |