/**
 * @file    peripherals/pic16f87xa_vref.h
 * @brief   Comparator Voltage Reference driver.
 *
 * @details
 *   Source: DS39582B §13.0 (Voltage Reference Module), Register 13-1
 *   (CVRCON), Figure 13-1 (resistor ladder).
 *
 *   The CVREF output is a 16-tap resistor ladder.  When CVRR=0, the
 *   range is 0..(0.75 × VDD); when CVRR=1, the range is
 *   0.25..(0.75 × VDD).  Each step is CVRSRC/24 (CVRR=0) or
 *   CVRSRC/32 (CVRR=1).
 *
 *   When CVROE=1, the output is routed to the RA2/AN2/VREF- pin
 *   (shared with the comparator and ADC reference inputs).
 *
 *   Reset state: CVRCON = 0x00 — reference disabled, output = 0 V.
 */

#ifndef PIC16F87XA_VREF_H
#define PIC16F87XA_VREF_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

/**
 * @brief Voltage-reference range select (CVRCON<CVRR>, Register 13-1).
 */
typedef enum {
    VREF_RANGE_LOW  = 0x0U,   /* 0..0.75 VDD  (steps of VDD/24) */
    VREF_RANGE_HIGH = 0x1U,   /* 0.25..0.75 VDD (steps of VDD/32) */
} VREF_RangeTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    VREF_RangeTypeDef      Range;     /* Low or high range. */
    uint8_t                Value;     /* 0..15 — ladder tap. */
    bool                   OutputEnable;  /* Route to RA2. */
    bool                   Enabled;        /* CVREN. */
} VREF_HandleTypeDef;

#define VREF_HANDLE_DEFAULT {                                              \
    .Range         = VREF_RANGE_LOW,                                       \
    .Value         = 0,                                                    \
    .OutputEnable  = false,                                                \
    .Enabled       = false,                                                \
}

PIC16F87XA_StatusTypeDef HAL_VREF_Init(const VREF_HandleTypeDef *h);
PIC16F87XA_StatusTypeDef HAL_VREF_DeInit(void);

/**
 * @brief  Compute the nominal output voltage (mV) for a given range +
 *         tap value. Assumes CVRSRC = Vdd_mv.
 */
uint32_t HAL_VREF_MilliVolts(uint32_t vdd_mv,
                             VREF_RangeTypeDef range,
                             uint8_t value);

#endif /* PIC16F87XA_VREF_H */
