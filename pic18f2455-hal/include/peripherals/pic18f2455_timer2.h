/**
 * @file    peripherals/pic18f2455_timer2.h
 * @brief   Timer2 driver, 8-bit timer with PR2 period register and postscaler.
 *
 * @details
 *   Source: DS39632E §12.0 (Timer2 Module), Register 12-2 (T2CON),
 *   Register 12-3 (PR2).
 *
 *   The API matches pic16f87xa_timer2.h (same `TIMER2_HandleTypeDef`,
 *   `HAL_TIMER2_*` names/signatures, weak `TIMER2_IRQHandler`); the body is
 *   simpler than PIC16's because PIC18 puts PR2 in the Access Bank
 *   (0xFCB), so there is no bank switching to read/write it (PIC16's PR2
 *   lives in Bank 1). T2CON's layout (postscaler bits 6:3, TMR2ON bit 2,
 *   prescaler bits 1:0) and reset value (0x00) are identical to PIC16.
 *
 *   Wiring: 8-bit counter TMR2 (always Fosc/4); period register PR2 (TMR2
 *   resets to 0x00 on TMR2 == PR2); prescaler 1:1/1:4/1:16 (T2CON<T2CKPS>);
 *   4-bit postscaler 1:1..1:16 (T2CON<T2OUTPS>); TMR2IF (PIR1<1>) fires every
 *   prescaler x postscaler x (PR2+1) instruction cycles. TMR2IF drives the
 *   CCP/ECCP PWM time base.
 *
 *   Reset (DS39632E Table 5-1): T2CON = 0x00 (off, 1:1/1:1), PR2 = 0xFF,
 *   TMR2 = 0x00.
 */

#ifndef PIC18F2455_TIMER2_H
#define PIC18F2455_TIMER2_H

#include "pic18f2455.h"
#include "pic18f2455_sfr.h"

/**
 * @brief Prescaler ratio (T2CON<T2CKPS1:T2CKPS0>, DS39632E Register 12-2).
 */
typedef enum {
    TIMER2_PRESCALER_1_1  = 0x0U,    /**< 00. */
    TIMER2_PRESCALER_1_4  = 0x1U,    /**< 01. */
    TIMER2_PRESCALER_1_16 = 0x2U,    /**< 1x. */
} TIMER2_PrescalerTypeDef;

/**
 * @brief Postscaler ratio (T2CON<T2OUTPS3:T2OUTPS0>, DS39632E Register 12-2).
 *        Postscaler value N -> 1:(N+1) divider.
 */
typedef enum {
    TIMER2_POSTSCALER_1_1  = 0x0U,
    TIMER2_POSTSCALER_1_2  = 0x1U,
    TIMER2_POSTSCALER_1_3  = 0x2U,
    TIMER2_POSTSCALER_1_4  = 0x3U,
    TIMER2_POSTSCALER_1_5  = 0x4U,
    TIMER2_POSTSCALER_1_6  = 0x5U,
    TIMER2_POSTSCALER_1_7  = 0x6U,
    TIMER2_POSTSCALER_1_8  = 0x7U,
    TIMER2_POSTSCALER_1_9  = 0x8U,
    TIMER2_POSTSCALER_1_10 = 0x9U,
    TIMER2_POSTSCALER_1_11 = 0xAU,
    TIMER2_POSTSCALER_1_12 = 0xBU,
    TIMER2_POSTSCALER_1_13 = 0xCU,
    TIMER2_POSTSCALER_1_14 = 0xDU,
    TIMER2_POSTSCALER_1_15 = 0xEU,
    TIMER2_POSTSCALER_1_16 = 0xFU,
} TIMER2_PostscalerTypeDef;

/** Driver handle (Cube-style). */
typedef struct {
    TIMER2_PrescalerTypeDef    Prescaler;
    TIMER2_PostscalerTypeDef   Postscaler;
    uint8_t                    Period;       /**< PR2 value, 0..255. */
    /** @brief Optional overflow callback (fires on TMR2IF, i.e. every
     *         prescaler x (PR2+1) x postscaler cycles). */
    void (*OverflowCallback)(void);
} TIMER2_HandleTypeDef;

#define TIMER2_HANDLE_DEFAULT {                                         \
    .Prescaler        = TIMER2_PRESCALER_1_1,                            \
    .Postscaler       = TIMER2_POSTSCALER_1_1,                           \
    .Period           = 0xFFU,                                           \
    .OverflowCallback = NULL,                                            \
}

HAL_StatusTypeDef HAL_TIMER2_Init(const TIMER2_HandleTypeDef *h);
HAL_StatusTypeDef HAL_TIMER2_DeInit(void);
HAL_StatusTypeDef HAL_TIMER2_Start(const TIMER2_HandleTypeDef *h);
HAL_StatusTypeDef HAL_TIMER2_Stop(void);

uint8_t  HAL_TIMER2_ReadCounter(void);
void     HAL_TIMER2_WriteCounter(uint8_t value);

uint8_t  HAL_TIMER2_ReadPeriod(void);
void     HAL_TIMER2_WritePeriod(uint8_t period);

uint16_t HAL_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p);
uint16_t HAL_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p);

/** Weak Timer2 ISR, override in user code to add application logic. */
void TIMER2_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18F2455_TIMER2_H */