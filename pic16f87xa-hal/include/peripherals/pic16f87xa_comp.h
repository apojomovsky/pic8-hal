/**
 * @file    peripherals/pic16f87xa_comp.h
 * @brief   Comparator driver — two on-chip comparators.
 *
 * @details
 *   Source: DS39582B §12.0 (Comparator Module), Register 12-1 (CMCON),
 *   Figure 12-1 (8 operating modes), §12.6 (interrupts).
 *
 *   Wiring on the part:
 *     - Two comparators C1 and C2.
 *     - 8 operating modes selected by CMCON<CM2:CM0> (Register 12-1,
 *       Figure 12-1):
 *         000 — Comparators reset (POR default)
 *         111 — Comparators off (read 0)
 *         010 — Two independent
 *         011 — Two independent with outputs
 *         100 — Two common reference
 *         101 — Two common reference with outputs
 *         001 — One independent with output
 *         110 — Four inputs multiplexed to two comparators
 *     - Inputs are multiplexed onto PORTA pins (RA0..RA3, RA5) and
 *       the VREF output if enabled.
 *     - Outputs on RA4 (C1OUT) and RA5 (C2OUT) when enabled.
 *     - Interrupts on change of either output (CMIF + CMIE).
 *
 *   Reset state: CMCON = 0x07 (CM2:CM0 = 111, comparators off, PIC16F87X
 *   compatibility).
 */

#ifndef PIC16F87XA_COMP_H
#define PIC16F87XA_COMP_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Comparator operating mode (CMCON<CM2:CM0>, Figure 12-1).
 */
typedef enum {
    COMP_MODE_RESET          = 0x0U,   /* 000 — comparators in reset. */
    COMP_MODE_ONE_WITH_OUT   = 0x1U,   /* 001 — C1 only, output on RA4. */
    COMP_MODE_TWO_INDEP      = 0x2U,   /* 010 — C1 and C2, no outputs. */
    COMP_MODE_TWO_WITH_OUT   = 0x3U,   /* 011 — C1 and C2, outputs on RA4/RA5. */
    COMP_MODE_TWO_COMMON     = 0x4U,   /* 100 — common ref, no outputs. */
    COMP_MODE_TWO_COMMON_OUT = 0x5U,   /* 101 — common ref, with outputs. */
    COMP_MODE_FOUR_MUXED     = 0x6U,   /* 110 — 4 inputs muxed to 2 comparators. */
    COMP_MODE_OFF            = 0x7U,   /* 111 — comparators off. */
} COMP_ModeTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    COMP_ModeTypeDef      Mode;
    bool                  C1Inverted;     /* C1INV bit. */
    bool                  C2Inverted;     /* C2INV bit. */
    bool                  CIS;            /* Comparator input switch. */
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

PIC16F87XA_StatusTypeDef HAL_COMP_Init(const COMP_HandleTypeDef *h);
PIC16F87XA_StatusTypeDef HAL_COMP_DeInit(void);

/** Returns 1 if C1 output is high (CMCON<C1OUT>). */
uint8_t HAL_COMP_C1Out(void);

/** Returns 1 if C2 output is high (CMCON<C2OUT>). */
uint8_t HAL_COMP_C2Out(void);

/** Returns 1 if CMIF is set. */
uint8_t HAL_COMP_IsChangeFlag(void);

/** Clear the CMIF flag (must be done in the change IRQ). */
void HAL_COMP_ClearChangeFlag(void);

void COMP_IRQHandler(void) PIC16F87XA_WEAK;

#ifdef __cplusplus
}
#endif

#endif /* PIC16F87XA_COMP_H */
