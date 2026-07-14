/**
 * @file    debounce.h
 * @brief   Vendor-agnostic, instantiable digital-input debouncer.
 *
 * @details
 *   A debounce instance for one digital input (a button, a limit switch,
 *   anything read as a boolean): given a raw, possibly-bouncy pin read, decide
 *   when the *stable* state has actually changed and emit a press/release edge
 *   event. Multiple instances, each independent plain data, cover multiple
 *   inputs, there is no global "the button" anywhere in this design.
 *
 *   Vendor-agnostic: the caller supplies a small read callback
 *   (`debounce_read_fn`) returning `true` when the pin currently reads
 *   "active", the callback itself resolves active-high vs. active-low, so the
 *   debounce core never needs to know or care which convention a given input
 *   uses. This makes the module equally useful over a HAL GPIO pin, an
 *   I2C-expander bit (`pic8-bus`), or a mock pin in a host test.
 *
 *   The implementation depends on `pic8-tick` for its timebase (calling
 *   `pic8_tick_get()` / `pic8_tick_elapsed_since()` directly, not an injected
 *   clock, so the host test suite exercises genuinely real timing semantics
 *   under simulated tick advancement). Only `src/debounce.c` includes
 *   `pic8_tick.h`; this header stays a two-`#include` file
 *   (`<stdint.h>`, `<stdbool.h>`), dependency-free.
 *
 *   Poll-driven: call `debounce_poll()` once per scheduler tick or main-loop
 *   iteration. No per-family interrupt-on-change wiring needed.
 */

#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Pin-read callback. Returns `true` when the pin currently reads
 *         "active." The callback resolves active-high vs. active-low.
 */
typedef bool (*debounce_read_fn)(void *ctx);

/** Debounce edge events emitted by `debounce_poll`. */
typedef enum {
    DEBOUNCE_EVENT_NONE     = 0,  /**< no state change committed this poll */
    DEBOUNCE_EVENT_PRESSED,       /**< just became stably active */
    DEBOUNCE_EVENT_RELEASED,      /**< just became stably inactive */
} debounce_event_t;

/** Bit flags packed into `debounce_t.flags`, mirroring pic8-taskmgr's
 *  `task_t.flags` convention. */
#define DEBOUNCE_FLAG_STABLE     0x01U  /**< current committed (debounced) state */
#define DEBOUNCE_FLAG_CANDIDATE  0x02U  /**< last raw read being watched for stability */

/** One debounce instance, plain data, no hidden global state. One per input. */
typedef struct {
    debounce_read_fn read;           /**< pin-read callback               */
    void            *read_ctx;       /**< opaque context for the callback */
    uint16_t         debounce_ms;    /**< stability window, e.g. 20-50 ms */
    uint32_t         candidate_since; /**< pic8_tick_get() timestamp of last raw change */
    uint8_t          flags;          /**< DEBOUNCE_FLAG_*                 */
} debounce_t;

/**
 * @brief  Initialize a debounce instance.
 * @param  db          the instance (caller-owned storage).
 * @param  read        pin-read callback (returns true = active).
 * @param  read_ctx    opaque context passed to `read` (may be NULL).
 * @param  debounce_ms stability window in ms (e.g. 20-50).
 * @note   Reads the pin once at init time and sets both the stable and
 *         candidate state to that initial reading, so a button already held
 *         down at boot does NOT spuriously fire a PRESSED event once the
 *         window elapses.
 */
void debounce_init(debounce_t *db, debounce_read_fn read, void *read_ctx,
                   uint16_t debounce_ms);

/**
 * @brief  Poll the input once. Call once per scheduler tick or main-loop
 *         iteration. Reads the pin via the callback, applies the debounce
 *         algorithm, and returns an edge event if the stable state just
 *         committed a transition.
 * @return `DEBOUNCE_EVENT_PRESSED`, `DEBOUNCE_EVENT_RELEASED`, or
 *         `DEBOUNCE_EVENT_NONE`.
 */
debounce_event_t debounce_poll(debounce_t *db);

/**
 * @brief  Query the last committed (debounced) state.
 * @return `true` if the stable state is "active," `false` if "inactive."
 * @note   Reflects the committed stable state, not the raw/candidate state.
 */
bool debounce_is_active(const debounce_t *db);

#endif /* DEBOUNCE_H */