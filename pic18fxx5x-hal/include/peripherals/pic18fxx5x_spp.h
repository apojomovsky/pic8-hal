/**
 * @file    peripherals/pic18fxx5x_spp.h
 * @brief   Streaming Parallel Port (SPP) driver.
 *
 * @details
 *   Source: DS39632E §18.0 (Streaming Parallel Port), Registers 18-1
 *   (SPPCON), 18-2 (SPPCFG), 18-3 (SPPEPS).
 *
 *   ⚠ This driver is **40/44-pin only** (PIC18F4455 / PIC18F4550). The
 *   28-pin PIC18F2455 / PIC18F2550 have no SPP. The compile-time check
 *   `PIC18FXX5X_FAMILY_HAS_SPP` (defined in pic18fxx5x.h) keeps this header
 *   out of 28-pin builds.
 *
 *   The SPP is a USB-era parallel port: an 8-bit data register (SPPDATA)
 *   plus clock/chip-select outputs, accessed through a USB endpoint
 *   address (SPPEPS<ADDR>) or directly by the microcontroller. It is the
 *   PIC18 analog of the PIC16 PSP, but USB-based.
 *
 *   This driver is **register-level only**: it programs SPPCON / SPPCFG /
 *   SPPEPS and provides byte-level read/write primitives to SPPDATA plus
 *   the busy / read-occurred / write-occurred status flags and the SPPIF
 *   interrupt. The USB streaming protocol (endpoint management, CLK1/CLK2
 *   toggling, ownership handoff to the USB peripheral) is left to the
 *   user, the same way the I²C state machine is left to the user for MSSP.
 *
 *   Register map (Access Bank):
 *     - SPPDATA (0xF62): read/write data byte.
 *     - SPPCFG  (0xF63): CLKCFG1:0 (bits 7:6), CSEN (5), CLK1EN (4),
 *                        WS3:WS0 (3:0, wait states 0..30 in steps of 2).
 *     - SPPEPS  (0xF64): ADDR3:0 (bits 3:0, endpoint), SPPBUSY (4),
 *                        WRSPP (6), RDSPP (7) — status read-only.
 *     - SPPCON  (0xF65): SPPOWN (1, USB vs MCU ownership), SPPEN (0).
 *
 *   Reset state (DS39632E Table 5-1): all 0x00.
 */

#ifndef PIC18FXX5X_SPP_H
#define PIC18FXX5X_SPP_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

#if !PIC18FXX5X_FAMILY_HAS_SPP
#error "pic18fxx5x_spp.h is for 40/44-pin PIC18FXX5X parts only (4455/4550)"
#endif

/**
 * @brief SPP ownership (SPPCON<SPPOWN>, Register 18-1).
 */
typedef enum {
    SPP_OWN_MICROCONTROLLER = 0x0U,   /**< SPPOWN=0: MCU directly controls SPP. */
    SPP_OWN_USB             = 0x1U,   /**< SPPOWN=1: USB peripheral controls SPP. */
} SPP_OwnershipTypeDef;

/**
 * @brief SPP clock configuration (SPPCFG<CLKCFG1:CLKCFG0>, Register 18-2).
 *        The driver shifts these into bits 7:6.
 */
typedef enum {
    SPP_CLKCFG_ADDR_WRITE_DATA_RW = 0x0U,   /**< 00: CLK1 on addr write, CLK2 on data R/W. */
    SPP_CLKCFG_WRITE_READ         = 0x1U,   /**< 01: CLK1 on write, CLK2 on read.    */
    SPP_CLKCFG_ODD_EVEN          = 0x2U,   /**< 1x: CLK1 on odd addr, CLK2 on even. */
} SPP_ClockConfigTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    SPP_OwnershipTypeDef       Ownership;
    SPP_ClockConfigTypeDef      ClockConfig;
    bool                        CSEnable;     /**< CSEN: RB4 as SPP CS output. */
    bool                        CLK1Enable;   /**< CLK1EN: RE0 as SPP CLK1 output. */
    uint8_t                     WaitStates;   /**< WS3:WS0, 0..15 (-> 0..30 wait states). */
    uint8_t                     Endpoint;     /**< ADDR3:ADDR0, 0..15. */
    /** @brief Optional transfer callback (fires on SPPIF). */
    void (*TransferCallback)(void);
} SPP_HandleTypeDef;

#define SPP_HANDLE_DEFAULT {                                              \
    .Ownership        = SPP_OWN_MICROCONTROLLER,                          \
    .ClockConfig     = SPP_CLKCFG_ADDR_WRITE_DATA_RW,                      \
    .CSEnable        = false,                                              \
    .CLK1Enable      = false,                                              \
    .WaitStates      = 0,                                                  \
    .Endpoint        = 0,                                                  \
    .TransferCallback = NULL,                                             \
}

/* ───────────────────────── init / deinit ────────────────────────── */

HAL_StatusTypeDef HAL_SPP_Init(const SPP_HandleTypeDef *h);
HAL_StatusTypeDef HAL_SPP_DeInit(void);

/* ───────────────────────── data access ───────────────────────────── */

/**
 * @brief  Write a byte to SPPDATA for endpoint `ep` (sets SPPEPS<ADDR>
 *         first, then loads SPPDATA). The CLK/CS outputs strobe per the
 *         SPPCFG configuration.
 */
void HAL_SPP_WriteByte(uint8_t ep, uint8_t data);

/**
 * @brief  Read a byte from SPPDATA for endpoint `ep`. Returns the byte.
 */
uint8_t HAL_SPP_ReadByte(uint8_t ep);

/* ───────────────────────── status ─────────────────────────────────── */

/** Returns 1 if the SPP is busy (SPPEPS<SPPBUSY>). */
uint8_t HAL_SPP_IsBusy(void);

/** Returns 1 if a write occurred since the flag was last cleared (SPPEPS<WRSPP>). */
uint8_t HAL_SPP_HasWriteOccurred(void);

/** Returns 1 if a read occurred since the flag was last cleared (SPPEPS<RDSPP>). */
uint8_t HAL_SPP_HasReadOccurred(void);

/** Returns 1 if SPPIF is set. */
uint8_t HAL_SPP_IsInterruptFlag(void);

/** Clear the SPPIF flag (must be done in the transfer IRQ). */
void HAL_SPP_ClearITFlag(void);

/* ───────────────────────── interrupts ───────────────────────────── */

void SPP_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18FXX5X_SPP_H */