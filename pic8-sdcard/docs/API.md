# `pic8-sdcard` API reference

Authoritative declarations: [`include/pic8_sdcard.h`](../include/pic8_sdcard.h).
Depends on `pic8_hal.h` (for `GPIO_TypeDef`) and `pic8-tick` (for real
wall-clock timeouts on the target build).

### `bool pic8_sdcard_init(const pic8_sdcard_pins_t *pins, uint32_t fosc_hz)`

Configure the SSP peripheral for SPI mode 0,0, assert CS idle, and run the
SD/MMC card bring-up sequence (CMD0/CMD8/ACMD41/CMD58/CMD9 — the full
`mmc_init_card` sequence). Blocks until the card responds or M-Stack's
internal retry/timeout bounds are hit.

- `pins` — which GPIO pin drives the card's CS line. SCK/SDI/SDO are fixed
  to the SSP peripheral's pins; only CS is board-specific.
- `fosc_hz` — system oscillator frequency in Hz, needed to compute the SPI
  clock divisor for both the slow bring-up speed and the card's negotiated
  max speed. The SSP's fixed divisors (Fosc/4, /16, /64) constrain the
  achievable rates — the actual speed may be slower than the card's max.

Returns `true` if the card initialized and `num_blocks`/`read_block`/
`write_block` are now usable, `false` otherwise.

### `bool pic8_sdcard_ready(void)`

Re-query the card's status (SEND_STATUS-adjacent). Has real SPI traffic
cost — don't call in a tight loop.

### `uint32_t pic8_sdcard_num_blocks(void)`

Number of 512-byte blocks on the card, cached from init. 0 if not
initialized.

### `bool pic8_sdcard_read_block(uint32_t block_addr, uint8_t *data)`

Read one 512-byte block. `data` must point to a buffer of at least 512
bytes. Returns `true` on success (including CRC16 check passing).

### `bool pic8_sdcard_write_block(uint32_t block_addr, const uint8_t *data)`

Write one 512-byte block. `data` must point to exactly 512 bytes. Returns
`true` on success (card accepted the data and reported no write error via
the follow-up SEND_STATUS check).

## Usage

```c
#include "pic8_sdcard.h"

pic8_sdcard_pins_t pins = { .cs_port = GPIOC, .cs_pin = 6 };
if (!pic8_sdcard_init(&pins, 48000000UL)) {
    /* init failed */
}

printf("card: %lu blocks\r\n", (unsigned long)pic8_sdcard_num_blocks());

uint8_t block[512];
if (pic8_sdcard_read_block(0, block)) {
    /* use block[] */
}

uint8_t data[512] = { /* ... */ };
pic8_sdcard_write_block(0, data);
```

## Cheat sheet

| Function | Purpose |
|---|---|
| `pic8_sdcard_init` | configure SSP, bring up card (blocking) |
| `pic8_sdcard_ready` | re-query card status (SPI traffic cost) |
| `pic8_sdcard_num_blocks` | cached block count (0 if not initialized) |
| `pic8_sdcard_read_block` | read one 512-byte block |
| `pic8_sdcard_write_block` | write one 512-byte block |
