/**
 * @file    pic16f87xa_sim.h
 * @brief   Public API for the PIC16F87XA host simulation backend.
 *
 * @details
 *   On the host build every SFR access indexes a host-side register file
 *   (the platform header include/host/pic16f87xa_platform.h, see
 *   @ref pic16f87xa_sfr.h). The hooks declared here let the host
 *   application:
 *     - drive external input pins (e.g. simulate a button press on RB0),
 *     - read output-pin levels as an external load would see them,
 *     - advance the simulated timers / A/D / USART by N instruction
 *       cycles via pic16f87xa_sim_step(),
 *     - inject peripheral events (ADC done, USART byte received, etc.).
 *
 *   The simulator models the most-used peripherals (Timer0/1/2, GPIO,
 *   ADC, USART, MSSP, EEPROM); see the implementation in
 *   src/sim/pic16f87xa_sim.c for the full list.
 */

#ifndef PIC16F87XA_SIM_H
#define PIC16F87XA_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include "pic16f87xa.h"

/**
 * @brief Initialize the simulation backend and reset every SFR to its
 *        power-on-reset value. Must be called before any HAL call.
 */
void pic16f87xa_sim_reset(void);

/**
 * @brief Advance the simulated peripherals by `ticks` instruction cycles.
 *        Drives Timer0, A/D, etc.
 */
void pic16f87xa_sim_step(uint32_t ticks);

/**
 * @brief Drive a digital input pin from the test rig (when the pin is
 *        configured as input, TRIS bit = 1).
 *
 * @param port   One of 'A'..'E'.
 * @param pin    Pin number 0..7.
 * @param level  0 = low, 1 = high.
 */
void pic16f87xa_sim_drive_input(char port, uint8_t pin, uint8_t level);

/**
 * @brief Read the level currently driven onto an output pin (what an
 *        external load would see). For pins configured as outputs, this
 *        returns the latched value in PORTx. For inputs, this returns
 *        the last value driven via @ref pic16f87xa_sim_drive_input.
 */
uint8_t pic16f87xa_sim_read_output(char port, uint8_t pin);

/**
 * @brief Hook a user callback fired whenever the simulated CPU would take
 *        an interrupt. Lets tests assert "an interrupt should fire here".
 */
typedef void (*pic16f87xa_sim_irq_cb_t)(void);
void pic16f87xa_sim_set_irq_callback(pic16f87xa_sim_irq_cb_t cb);

/**
 * @brief Inject a byte into the USART receiver as if it had just been
 *        received off the wire. Sets PIR1<RCIF> and stores the byte in
 *        RCREG. The next call to HAL_USART_Receive() will return it.
 */
void pic16f87xa_sim_drive_usart_rx(uint8_t data);

/**
 * @brief Inject a byte into the SSP receiver (SPI slave or I²C target).
 *        Sets SSPSTAT<BF> and PIR1<SSPIF>. The next call to
 *        HAL_SSP_ReadByte() will return the byte.
 */
void pic16f87xa_sim_drive_ssp_rx(uint8_t data);

/**
 * @brief Drive an A/D conversion to completion with a given 10-bit
 *        result. Stores `result` in ADRESH:ADRESL (right-justified),
 *        clears GO/DONE, and sets PIR1<ADIF>. The next call to
 *        HAL_ADC_Read() will return the result.
 */
void pic16f87xa_sim_drive_adc_done(uint16_t result);

/**
 * @brief Place a byte in the simulated EEPROM. Subsequent calls to
 *        HAL_EEPROM_ReadByte(addr) return it.
 */
void pic16f87xa_sim_drive_eeprom_byte(uint8_t addr, uint8_t data);

/**
 * @brief Simulate a completed EEPROM write at `addr` with `data` and
 *        set PIR2<EEIF>.
 */
void pic16f87xa_sim_drive_eeprom_done(uint8_t addr, uint8_t data);

#endif /* PIC16F87XA_SIM_H */
