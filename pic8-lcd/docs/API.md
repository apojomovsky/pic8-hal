# `pic8-lcd` API reference

Authoritative declarations: [`include/pic8_lcd.h`](../include/pic8_lcd.h)
(core types and functions) and [`include/pic8_lcd_transport.h`](../include/pic8_lcd_transport.h)
(HAL-dependent transport types).

## Core types (pic8_lcd.h)

### `pic8_lcd_ops_t`

Transport operations, injected at init time. Three function pointers:

| Field | Purpose |
|---|---|
| `send(ctx, rs, byte)` | Write one byte. rs=0 for instruction, rs=1 for data. |
| `delay_us(ctx, us)` | Wait at least `us` microseconds. |
| `delay_ms(ctx, ms)` | Wait at least `ms` milliseconds. |

### `pic8_lcd_config_t`

| Field | Purpose |
|---|---|
| `cols` | Columns per row (e.g. 16 or 20) |
| `rows` | Number of rows (e.g. 2 or 4) |
| `row_addr[4]` | DDRAM base address per row. Set all to 0 for standard HD44780 defaults. |

### `pic8_lcd_t`

LCD instance. Caller-owned storage. Cache fields (`display_ctrl`,
`entry_mode`) are internal; do not modify directly.

## Lifecycle

### `void pic8_lcd_init(pic8_lcd_t *lcd, const pic8_lcd_ops_t *ops, void *ops_ctx, const pic8_lcd_config_t *config)`

Run the full HD44780 init sequence and configure the display. Display on,
cursor off, blink off, entry mode = increment/no-shift. Must be called
once before any other function.

## Display content

### `void pic8_lcd_clear(pic8_lcd_t *lcd)`

Clear the entire display and return cursor to (0, 0). ~1.6 ms.

### `void pic8_lcd_home(pic8_lcd_t *lcd)`

Return cursor to (0, 0) without clearing. Display contents unchanged. ~1.6 ms.

### `void pic8_lcd_set_cursor(pic8_lcd_t *lcd, uint8_t col, uint8_t row)`

Move cursor to (col, row). Row 0 is the top row. Out-of-range values are
clamped to the last valid column/row.

### `void pic8_lcd_write_char(pic8_lcd_t *lcd, char c)`

Write one character at the current position and advance the cursor.

### `void pic8_lcd_write(pic8_lcd_t *lcd, const char *str, size_t len)`

Write `len` characters from `str`.

### `void pic8_lcd_print(pic8_lcd_t *lcd, const char *str)`

Write a null-terminated string.

## Display and cursor control

### `void pic8_lcd_display_on(pic8_lcd_t *lcd, bool on)`

Turn display on/off. Cursor and blink settings preserved; nothing cleared.

### `void pic8_lcd_cursor_on(pic8_lcd_t *lcd, bool on)`

Show/hide the underscore cursor.

### `void pic8_lcd_cursor_blink(pic8_lcd_t *lcd, bool on)`

Enable/disable cursor-position blinking (solid block).

## Scrolling

### `void pic8_lcd_scroll_left(pic8_lcd_t *lcd)`

Shift the entire display one position left.

### `void pic8_lcd_scroll_right(pic8_lcd_t *lcd)`

Shift the entire display one position right.

## Custom characters

### `void pic8_lcd_create_char(pic8_lcd_t *lcd, uint8_t slot, const uint8_t glyph[8])`

Define a custom character in CGRAM slot `slot` (0-7). `glyph` is 8 bytes,
one per pixel row; bottom 5 bits are the pixel pattern (bit 4 = leftmost).
After creation, write character code 0x00-0x07 to display it. Slots > 7
are silently ignored. The cursor returns to DDRAM after this call.

## Low-level access

### `void pic8_lcd_command(pic8_lcd_t *lcd, uint8_t cmd)`

Send a raw instruction byte. For commands not covered by the high-level API.

### `void pic8_lcd_data(pic8_lcd_t *lcd, uint8_t data)`

Send a raw data byte.

## Cheat sheet

| Function | Purpose |
|---|---|
| `pic8_lcd_init` | full init sequence |
| `pic8_lcd_clear` | clear display + home |
| `pic8_lcd_home` | cursor to (0,0), no clear |
| `pic8_lcd_set_cursor` | move cursor |
| `pic8_lcd_write_char` | one character |
| `pic8_lcd_write` | `len` characters |
| `pic8_lcd_print` | null-terminated string |
| `pic8_lcd_display_on` | display on/off |
| `pic8_lcd_cursor_on` | cursor on/off |
| `pic8_lcd_cursor_blink` | blink on/off |
| `pic8_lcd_scroll_left` | shift display left |
| `pic8_lcd_scroll_right` | shift display right |
| `pic8_lcd_create_char` | define custom character |
| `pic8_lcd_command` | raw instruction byte |
| `pic8_lcd_data` | raw data byte |
