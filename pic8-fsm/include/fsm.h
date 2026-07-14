/**
 * @file    fsm.h
 * @brief   Vendor-agnostic, table-driven finite state machine engine.
 *
 * @details
 *   A state machine's behavior lives in one `static const` array of
 *   @ref fsm_transition_t rows, sitting in the caller's own file — the
 *   whole machine is readable at a glance, with no per-state `switch` to
 *   trace across functions:
 *
 *       enum { ST_RED, ST_GREEN, ST_YELLOW, ST_FAULT };
 *       enum { EV_TIMER, EV_FAULT };
 *
 *       static const fsm_transition_t light_transitions[] = {
 *           { ST_RED,        EV_TIMER, NULL, NULL,       ST_GREEN  },
 *           { ST_GREEN,      EV_TIMER, NULL, on_caution, ST_YELLOW },
 *           { ST_YELLOW,     EV_TIMER, NULL, NULL,       ST_RED    },
 *           { FSM_ANY_STATE, EV_FAULT, NULL, on_fault,   ST_FAULT  },
 *       };
 *
 *       fsm_t light;
 *       FSM_INIT(&light, light_transitions, ST_RED, &light_ctx);
 *       fsm_dispatch(&light, EV_TIMER);
 *
 *   This library has no hardware dependency at all: it includes only
 *   <stdint.h> and <stdbool.h>, never a HAL header. It composes with
 *   pic8-taskmgr (or anything else) by staying decoupled from it — a task
 *   callback just owns an @ref fsm_t and calls @ref fsm_dispatch; there is
 *   no FSM-specific hook anywhere in task_manager.h. The transition table
 *   is `static const`, so it is placed in flash, not RAM; the only RAM
 *   cost is one @ref fsm_t handle (a pointer, a length byte, a state byte,
 *   and a context pointer) per machine instance.
 *
 *   Because this engine has no per-family backend, the same fsm.c compiles
 *   unchanged for the host, PIC16, and PIC18 — host unit tests exercise the
 *   literal code that ships on-target, not a stand-in reference
 *   implementation.
 *
 * @par Dispatch semantics (read before writing a table)
 *   `fsm_dispatch()` scans the table top-to-bottom for a row whose `state`
 *   matches the machine's current state (or is @ref FSM_ANY_STATE) *and*
 *   whose `event` matches. If that row has a `guard` and it returns false,
 *   the scan **continues** to the next matching row rather than stopping —
 *   this lets one (state, event) pair have several candidate rows
 *   disambiguated by guards, e.g.:
 *
 *       { ST_LOCKED, EV_COIN, has_sufficient_credit, do_unlock,     ST_UNLOCKED },
 *       { ST_LOCKED, EV_COIN, NULL,                  buzz_rejected, ST_LOCKED   },
 *
 *   A `COIN` event with insufficient credit falls through the first row
 *   (guard fails) to the second (no guard, always matches), which buzzes
 *   and stays in `ST_LOCKED`. The first row with a passing guard (or no
 *   guard) wins; its `action` runs (if non-NULL) and the machine moves to
 *   `next_state`. If no row matches at all, @ref fsm_dispatch returns
 *   false and the machine's state is unchanged — the caller decides what
 *   an unhandled event means (ignore it, log it, assert), the library does
 *   not impose a policy.
 *
 * @par Context convention
 *   `ctx` is an opaque `void *`, not a typed pointer — one shared
 *   @ref fsm_dispatch serves every machine in the firmware, rather than a
 *   macro-generated dispatch per context type (which would duplicate the
 *   scan loop and cost flash for each additional FSM type declared). Every
 *   guard/action casts `ctx` back to its real type on its first line:
 *
 *       static bool has_sufficient_credit(void *ctx) {
 *           turnstile_ctx_t *c = ctx;
 *           return c->credit_cents >= TURNSTILE_FARE_CENTS;
 *       }
 */

#ifndef FSM_H
#define FSM_H

#include <stddef.h>   /* NULL — every transition row uses it for guard/action */
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Storage type for states and events. `uint8_t` by default (0..254
 *         usable values; 255 reserved for @ref FSM_ANY_STATE), which is
 *         enough for any state machine likely to run on an 8-bit PIC.
 *         Override by `#define FSM_STATE_TYPE <type>` before including this
 *         header if a machine genuinely needs more than 255 states or
 *         events. This is the *only* build-time knob this library has —
 *         every other property (handle size, table-in-flash placement,
 *         dispatch cost) is fixed-cost enough that no other axis is worth
 *         making tunable.
 */
#ifndef FSM_STATE_TYPE
#define FSM_STATE_TYPE uint8_t
#endif

typedef FSM_STATE_TYPE fsm_state_t;
typedef FSM_STATE_TYPE fsm_event_t;

/**
 * @brief  Wildcard: a transition row with `state == FSM_ANY_STATE` matches
 *         regardless of the machine's current state. All-ones under
 *         whatever @ref FSM_STATE_TYPE resolves to, so it stays out of the
 *         way of ordinary state numbering (which starts at 0) whether the
 *         type is the default `uint8_t` or a wider override.
 */
#define FSM_ANY_STATE  ((fsm_state_t)-1)

/**
 * @brief  Optional predicate gating a transition row. Return true to allow
 *         the row to fire, false to let @ref fsm_dispatch keep scanning for
 *         another matching row. A NULL guard always allows the row.
 */
typedef bool (*fsm_guard_fn)(void *ctx);

/**
 * @brief  Optional side effect run when a transition row fires, before the
 *         state changes. A NULL action means "transition with no side
 *         effect."
 */
typedef void (*fsm_action_fn)(void *ctx);

/**
 * @brief  One row of a transition table: "when in `state` (or any state)
 *         and `event` arrives, if `guard` allows it, run `action` and move
 *         to `next_state`." A whole machine is a `static const` array of
 *         these, declared once in the caller's file.
 */
typedef struct {
    fsm_state_t   state;       /**< Row applies from this state, or @ref FSM_ANY_STATE. */
    fsm_event_t   event;       /**< Row applies to this event. */
    fsm_guard_fn  guard;       /**< NULL = always allowed. */
    fsm_action_fn action;      /**< NULL = no side effect, just move state. */
    fsm_state_t   next_state;  /**< State to move to when this row fires. */
} fsm_transition_t;

/**
 * @brief  A running machine instance: which table it's using, its current
 *         state, and the opaque context passed to every guard/action.
 *         Multiple instances (even sharing the same table) are fully
 *         independent — there is no shared mutable state anywhere in this
 *         library, so instances never interfere with each other.
 */
typedef struct {
    const fsm_transition_t *table;      /**< The machine's transition table (lives in flash). */
    uint8_t                 table_len;  /**< Number of rows in `table`. */
    fsm_state_t             state;      /**< Current state. */
    void                   *ctx;        /**< Opaque, passed to every guard/action. */
} fsm_t;

/**
 * @brief  Initialize a machine instance.
 *
 * @param  fsm            Instance to initialize.
 * @param  table           The transition table (must outlive `fsm`; a
 *                        `static const` array is the normal case).
 * @param  table_len       Number of rows in `table`. Prefer @ref FSM_INIT,
 *                        which computes this for you.
 * @param  initial_state   The machine's starting state.
 * @param  ctx            Opaque pointer passed to every guard/action;
 *                        may be NULL if none of them need it.
 */
void fsm_init(fsm_t *fsm, const fsm_transition_t *table, uint8_t table_len,
              fsm_state_t initial_state, void *ctx);

/**
 * @brief  Convenience wrapper over @ref fsm_init that computes `table_len`
 *         via `sizeof(table) / sizeof(table[0])` at the call site, so a
 *         row added to the table can never silently fall outside the
 *         length passed in.
 *
 * @warning `table` MUST be the actual array here, not a pointer to it (a
 *          function parameter of array type has decayed to a pointer, and
 *          `sizeof` on it would silently give the pointer's size instead of
 *          the array's). Call this where the array is declared or still in
 *          scope by its array type.
 */
#define FSM_INIT(fsm, table, initial_state, ctx) \
    fsm_init((fsm), (table), (uint8_t)(sizeof(table) / sizeof((table)[0])), \
             (initial_state), (ctx))

/**
 * @brief  Feed one event to the machine. Scans the table top-to-bottom for
 *         the first row whose state matches (current state, or
 *         @ref FSM_ANY_STATE) and whose event matches, skipping over rows
 *         whose guard rejects the event (see the "Dispatch semantics"
 *         section in the file-level doc comment for the fall-through
 *         behavior this enables). If a row fires, its action (if any) runs
 *         and the machine moves to that row's `next_state`.
 *
 * @return true if a row fired (the action ran, if any, and the state may
 *         have changed — including to the same state, which is a valid,
 *         explicit self-transition); false if no row matched, in which
 *         case the machine's state is unchanged. The caller decides what
 *         an unhandled event means; this library imposes no policy (no
 *         logging, no assert) so it stays usable in a plain firmware image
 *         with no logging facility at all.
 */
bool fsm_dispatch(fsm_t *fsm, fsm_event_t event);

/** Current state of the machine. */
fsm_state_t fsm_state(const fsm_t *fsm);

#endif /* FSM_H */
