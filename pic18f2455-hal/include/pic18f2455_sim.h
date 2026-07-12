/**
 * @file    pic18f2455_sim.h
 * @brief   Public API for the PIC18F2455 family host simulation backend.
 *
 * @details
 *   On the host build every SFR access indexes a host-side register file
 *   (the platform header include/host/pic18_platform.h). The hooks declared
 *   here let the host application:
 *     - drive external input pins (e.g. simulate a button press on RB0),
 *     - read output-pin levels as an external load would see them (from
 *       LATx, the PIC18 output latch, DS39632E §10.0),
 *     - advance the simulated peripherals by N instruction cycles via
 *       pic18_sim_step(),
 *     - register an interrupt callback fired when a simulated peripheral
 *       raises an IRQ.
 *
 *   Phase 2 models Timer0 (8/16-bit, prescaler, overflow -> TMR0IF) and
 *   GPIO drive/read. The function names mirror pic16f87xa_sim_*; Phase 2
 *   task 8 of the plan notes the family-blind part of this API may move to
 *   shared `pic8_sim_*` naming later, but Phase 2 keeps the family-prefixed
 *   names to stay symmetric with the PIC16 tree.
 */

#ifndef PIC18F2455_SIM_H
#define PIC18F2455_SIM_H

#include <stdint.h>
#include "pic18f2455.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value (DS39632E Table 5-1). Must be called before
 *        any HAL call.
 */
void pic18_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Drives Timer0 (and raises TMR0IF + the IRQ callback on overflow).
 */
void pic18_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        configured as input, TRIS bit = 1).
 *
 * @param port   One of 'A'..'E'.
 * @param pin    Pin number 0..7.
 * @param level  0 = low, 1 = high.
 */
void pic18_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        external load would see). For pins configured as outputs, this
 *        returns the latched value in LATx (DS39632E §10.0). For inputs,
 *        this returns the last value driven via @ref pic18_sim_drive_input.
 */
uint8_t pic18_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. The host harness registers the family dispatcher
 *        here.
 */
typedef void (*pic18_sim_irq_cb_t)(void);
void pic18_sim_set_irq_callback(pic18_sim_irq_cb_t cb);

#endif /* PIC18F2455_SIM_H */