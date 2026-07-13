/**
 * @file    peripherals/pic18fxx5x_comp.h
 * @brief   Comparator driver, two on-chip comparators.
 *
 * @details
 *   Source: DS39632E §22.0 (Comparator Module), Register 22-1 (CMCON),
 *   Figure 22-1 (the eight operating modes).
 *
 *   The API mirrors pic16f87xa_comp.h — the same `COMP_HandleTypeDef`,
 *   `COMP_ModeTypeDef`, `HAL_COMP_*` functions and weak
 *   `COMP_IRQHandler` — so consumer code is portable. The PIC18 CMCON has
 *   the same bit layout as the PIC16 CMCON; the only difference is the
 *   register sits in the Access Bank (0xFB4, no bank switching). The
 *   comparator voltage reference (CVRCON) is ported separately.
 *
 *   Wiring on the part:
 *     - Two comparators C1 and C2.
 *     - 8 operating modes selected by CMCON<CM2:CM0> (Figure 22-1):
 *         000, Comparators reset
 *         111, Comparators off (POR default, read 0)
 *         010, Two independent
 *         011, Two independent with outputs
 *         100, Two common reference
 *         101, Two common reference with outputs
 *         001, One independent with output
 *         110, Four inputs multiplexed to two comparators
 *     - Inputs are multiplexed onto PORTA pins (RA0..RA3, RA5) and the
 *       VREF output if enabled.
 *     - Outputs on RA4 (C1OUT) and RA5 (C2OUT) when enabled.
 *     - Interrupts on change of either output (CMIF + CMIE, PIR2<6>).
 *
 *   Reset state (DS39632E Figure 22-1): CMCON = 0x07 (CM2:CM0 = 111,
 *   comparators off).
 */

#ifndef PIC18FXX5X_COMP_H
#define PIC18FXX5X_COMP_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief Comparator operating mode (CMCON<CM2:CM0>, DS39632E Figure 22-1).
 *        Same eight modes as the PIC16 comparator.
 */
typedef enum {
    COMP_MODE_RESET          = 0x0U,   /**< 000, comparators in reset. */
    COMP_MODE_ONE_WITH_OUT   = 0x1U,   /**< 001, C1 only, output on RA4. */
    COMP_MODE_TWO_INDEP      = 0x2U,   /**< 010, C1 and C2, no outputs. */
    COMP_MODE_TWO_WITH_OUT   = 0x3U,   /**< 011, C1 and C2, outputs on RA4/RA5. */
    COMP_MODE_TWO_COMMON     = 0x4U,   /**< 100, common ref, no outputs. */
    COMP_MODE_TWO_COMMON_OUT = 0x5U,   /**< 101, common ref, with outputs. */
    COMP_MODE_FOUR_MUXED     = 0x6U,   /**< 110, 4 inputs muxed to 2 comparators. */
    COMP_MODE_OFF            = 0x7U,   /**< 111, comparators off (POR default). */
} COMP_ModeTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    COMP_ModeTypeDef      Mode;
    bool                   C1Inverted;     /**< C1INV bit. */
    bool                   C2Inverted;     /**< C2INV bit. */
    bool                   CIS;            /**< Comparator input switch. */
    /** @brief Optional change callback (fires on CMIF). */
    void (*ChangeCallback)(void);
} COMP_HandleTypeDef;

#define COMP_HANDLE_DEFAULT {                                              \
    .Mode            = COMP_MODE_TWO_INDEP,                                \
    .C1Inverted      = false,                                              \
    .C2Inverted      = false,                                              \
    .CIS             = false,                                              \
    .ChangeCallback  = NULL,                                               \
}

/* ───────────────────────── init / deinit ────────────────────────── */

HAL_StatusTypeDef HAL_COMP_Init(const COMP_HandleTypeDef *h);
HAL_StatusTypeDef HAL_COMP_DeInit(void);

/* ───────────────────────── outputs / status ────────────────────────── */

/** Returns 1 if C1 output is high (CMCON<C1OUT>). */
uint8_t HAL_COMP_C1Out(void);

/** Returns 1 if C2 output is high (CMCON<C2OUT>). */
uint8_t HAL_COMP_C2Out(void);

/** Returns 1 if CMIF is set. */
uint8_t HAL_COMP_IsChangeFlag(void);

/** Clear the CMIF flag (must be done in the change IRQ). */
void HAL_COMP_ClearChangeFlag(void);

/* ───────────────────────── interrupts ───────────────────────────── */

void COMP_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18FXX5X_COMP_H */