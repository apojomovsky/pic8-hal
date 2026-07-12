/**
 * @file    pic18f2455_sim.h
 * @brief   Public API for the PIC18F2455 family host simulation backend.
 *
 * @details
 *   On the host build every SFR access indexes a host-side register file
 *   (the platform header include/host/pic18_platform.h). The hooks declared
 *   here let the host application reset the simulated CPU, advance
 *   simulated time, and register an interrupt callback.
 *
 *   Phase 1 status: this is the minimal backend. `reset` zeroes the
 *   register file, `step` is a no-op (no peripherals are modeled yet), and
 *   `set_irq_callback` stores a callback that nothing fires in Phase 1.
 *   Phase 2 fills in Timer0 stepping, GPIO drive/read, and the
 *   peripheral-event injectors, mirroring `pic16f87xa_sim.h`'s API shape.
 *
 *   The function names mirror `pic16f87xa_sim_*` (family-prefixed). Phase 2
 *   task 8 of the plan moves the family-blind part of this API to the
 *   shared `pic8_sim_*` naming; that rename is deferred to Phase 2 so
 *   Phase 1 stays a pure scaffold symmetric with the PIC16 tree.
 */

#ifndef PIC18F2455_SIM_H
#define PIC18F2455_SIM_H

#include <stdint.h>
#include "pic18f2455.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic18_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Phase 1: no-op (no peripherals modeled yet). Phase 2 drives
 *        Timer0, GPIO, etc.
 */
void pic18_sim_step(uint32_t ticks);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. The host harness registers the family dispatcher
 *        here. Phase 1: stored but never fired.
 */
typedef void (*pic18_sim_irq_cb_t)(void);
void pic18_sim_set_irq_callback(pic18_sim_irq_cb_t cb);

#endif /* PIC18F2455_SIM_H */