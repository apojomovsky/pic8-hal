/**
 * @file    peripherals/pic18fxx5x_gpio.h
 * @brief   General-Purpose I/O port driver for the PIC18F2455 family.
 *
 * @details
 *   Cube-style API: every pin is addressed by (GPIOx, GPIO_PIN_n). The
 *   names and signatures match pic16f87xa_gpio.h exactly so consumer code
 *   (the task manager, the examples) is portable across families; only the
 *   register-level bodies differ.
 *
 *   PIC18 GPIO difference from PIC16 (DS39632E §10.0): the output latch is
 *   exposed as its own mapped register, LATx. **Writes go through LATx, not
 *   PORTx.** This is a real correctness improvement PIC18 provides natively:
 *   on PIC16 a write to PORTx is a read-modify-write of the latch, which can
 *   corrupt an input pin's state on a shared write; PIC18 lets you write the
 *   latch directly. Reads of PORTx return the pin state (input) or the
 *   latched output (output). This driver therefore:
 *     - writes LATx in HAL_GPIO_WritePin / TogglePin / WritePort,
 *     - reads PORTx in HAL_GPIO_ReadPin / ReadPort,
 *     - programs direction in TRISx (TRIS = 1 input, 0 output, §10.0).
 *
 *   PORTA..PORTC are present on every device. PORTD and PORTE exist only on
 *   40/44-pin parts (PIC18F4455 / 4550, DS39632E Table 1-1). PORTA is 6-bit
 *   wide on these parts (RA0..RA5), PORTE is 3-bit; the driver enforces
 *   those widths. PORTB weak pull-ups live in INTCON2<RBPU> (§9.0 / §10.2),
 *   active-low.
 */

#ifndef PIC18FXX5X_GPIO_H
#define PIC18FXX5X_GPIO_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief GPIO port identifier. Matches the Cube convention where
 *        `GPIOx` selects the port (x = A..E).
 *
 * PORTA..PORTC are present on every device. PORTD and PORTE exist only on
 * 40/44-pin parts (PIC18F4455 / 4550, DS39632E §10.0).
 */
typedef enum {
    GPIOA = 0,   /**< PORTA, 6 bits (RA0..RA5), DS39632E §10.0. */
    GPIOB = 1,   /**< PORTB, 8 bits (RB0..RB7), DS39632E §10.0. */
    GPIOC = 2,   /**< PORTC, 8 bits (RC0..RC7), DS39632E §10.0. */
#if PIC18FXX5X_FAMILY_HAS_PORTD
    GPIOD = 3,   /**< PORTD, 8 bits (RD0..RD7), 40/44-pin only. */
#endif
#if PIC18FXX5X_FAMILY_HAS_PORTE
    GPIOE = 4,   /**< PORTE, 3 bits (RE0..RE2), 40/44-pin only. */
#endif
} GPIO_TypeDef;

/**
 * @brief Pin identifiers. Each port has up to 8 pins.
 *        Use @ref GPIO_PIN_All for whole-port operations.
 */
#define GPIO_PIN_0    PIC8_BIT(0)
#define GPIO_PIN_1    PIC8_BIT(1)
#define GPIO_PIN_2    PIC8_BIT(2)
#define GPIO_PIN_3    PIC8_BIT(3)
#define GPIO_PIN_4    PIC8_BIT(4)
#define GPIO_PIN_5    PIC8_BIT(5)
#define GPIO_PIN_6    PIC8_BIT(6)
#define GPIO_PIN_7    PIC8_BIT(7)
#define GPIO_PIN_All  0xFFU

/**
 * @brief Pin logical state.
 */
typedef enum {
    GPIO_PIN_RESET = 0U,   /**< Logic low. */
    GPIO_PIN_SET   = 1U    /**< Logic high. */
} GPIO_PinState;

/**
 * @brief Pin direction / operating mode.
 *
 * On PIC18 the TRIS bit controls direction (DS39632E §10.0):
 *   TRIS = 1  → pin is input (driver high-impedance)
 *   TRIS = 0  → pin is output (drives the LATx value)
 */
typedef enum {
    GPIO_MODE_INPUT  = 0x1U,   /**< TRIS bit = 1, high-impedance. */
    GPIO_MODE_OUTPUT = 0x2U,   /**< TRIS bit = 0, drives LATx.      */
    GPIO_MODE_ANALOG = 0x3U,   /**< Pin released to an analog peripheral. */
} GPIO_ModeTypeDef;

/**
 * @brief  Internal weak-pull-up control (PORTB only, DS39632E §10.2,
 *         RBPU bit in INTCON2<7>).
 */
typedef enum {
    GPIO_NOPULL   = 0U,   /**< Weak pull-ups disabled (RBPU = 1). */
    GPIO_PULLUP   = 1U    /**< Weak pull-ups enabled  (RBPU = 0). */
} GPIO_PullTypeDef;

/* ───────────────────────── init / deinit ────────────────────────── */

/**
 * @brief  Configure one or more pins of a port to the same direction.
 *
 * @param  port   GPIOA..GPIOE
 * @param  pins   Bitmask of @ref GPIO_PIN_0 .. GPIO_PIN_All
 * @param  mode   One of @ref GPIO_ModeTypeDef
 *
 * @note   Does not configure alternate-function peripherals (e.g. ADC,
 *         USART); call the relevant peripheral driver first.
 */
void HAL_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode);

/** Restore all pins of `port` to input mode and clear the latch. */
void HAL_GPIO_DeInit(GPIO_TypeDef port);

/* ───────────────────────── read / write / toggle ────────────────── */

/**
 * @brief  Drive a pin high or low. Writes the LATx latch directly
 *         (DS39632E §10.0), the PIC18-native way (no read-modify-write
 *         of PORTx).
 */
void HAL_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state);

/** Toggle a set of pins (LATx ^= mask). */
void HAL_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins);

/**
 * @brief  Read the current level seen on `pins` from PORTx. For pins
 *         configured as outputs this returns the latched value; for input
 *         pins it returns whatever the pin is being driven to externally.
 */
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins);

/** Atomically write the entire 8-bit port latch (LATx). */
void HAL_GPIO_WritePort(GPIO_TypeDef port, uint8_t value);

/** Read the entire port (PORTx). */
uint8_t HAL_GPIO_ReadPort(GPIO_TypeDef port);

/* ───────────────────────── PORTB pull-ups ───────────────────────── */

/**
 * @brief  Enable or disable PORTB internal weak pull-ups.
 *         Maps to INTCON2<RBPU> (DS39632E §10.2).
 *
 * @note   INTCON2<7> is inverted: RBPU = 1 disables pull-ups.
 */
void HAL_GPIO_SetPullups(GPIO_PullTypeDef pull);

/* ───────────────────────── PORTB change interrupt ─────────────────── */

/**
 * @brief  Register a single whole-port callback fired from the RB<7:4>
 *         change interrupt (DS39632E §9.0/§10.2, INTCON<RBIF>/<RBIE>).
 *
 * @param  callback  function called once per RB-change interrupt with the
 *                   freshly-read PORTB byte, or NULL to unregister.
 *
 * @details
 *   Same names/signature as pic16f87xa_gpio.h's hook (the fixed contract
 *   across families), PIC18 register-level body. There is only ever one
 *   PORTB, so (unlike Timer2's per-handle callback) there is no handle
 *   struct: exactly one callback slot. NULL is safe (the handler no-ops).
 *   Fanning one received byte out to N consumers is application-level
 *   composition, not a HAL registry.
 *
 *   The handler reads PORTB *before* clearing RBIF (DS39632E §9.0: the
 *   mismatch comparator latches the value at the last read, so reading
 *   PORTB is what re-arms detection; clearing the flag first risks a
 *   spurious re-interrupt or a silently-missed change). See @ref
 *   RB_IRQHandler.
 */
void HAL_GPIO_RegisterChangeCallback(void (*callback)(uint8_t portb_value));

/**
 * @brief  Weak RB<7:4> change-interrupt ISR (DS39632E §9.0/§10.2).
 *
 * @details
 *   Mirrors every other `*_IRQHandler` in this HAL: weak so user code may
 *   override it to add application logic, with a default body that clears
 *   RBIF and forwards the already-read PORTB byte to the callback
 *   registered via @ref HAL_GPIO_RegisterChangeCallback.
 *
 *   The read/clear order is mandatory, not stylistic: PORTB is read into a
 *   local *before* RBIF is cleared, then the callback receives that
 *   already-read value (never a second, later read). This is the datasheet
 *   "read PORTB to end the mismatch condition" sequence, identical to the
 *   PIC16 hook's rationale.
 */
void RB_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18FXX5X_GPIO_H */
