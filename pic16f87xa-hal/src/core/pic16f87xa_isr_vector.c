/**
 * @file    pic16f87xa_isr_vector.c
 * @brief   Real-target interrupt-vector dispatcher (XC8 only).
 *
 * @details
 *   The PIC16F87XA family has a single interrupt vector at 0x0004
 *   (DS39582B §14.11). On a real XC8 target this file installs the
 *   `__interrupt()` handler the CPU jumps to on any enabled IRQ, and
 *   dispatches to every peripheral's IRQHandler.
 *
 *   Each peripheral IRQHandler checks its own flag and returns
 *   immediately when its source is not pending (see e.g.
 *   @ref TIMER0_IRQHandler), so calling them all in turn is correct
 *   and costs only a few cycles per interrupt.
 *
 *   The host simulation backend does NOT use this file: the whole
 *   translation unit is guarded by `#if !defined(PIC16F87XA_USE_SIMULATOR)`.
 *   On the host the application registers a single callback via
 *   @ref pic16f87xa_sim_set_irq_callback, which the sim invokes on
 *   every modelled interrupt event instead.
 *
 *   Note: XC8 has no concept of weak symbols, so the peripheral
 *   handlers are strong definitions. An application overrides behaviour
 *   by setting a driver handle callback (e.g.
 *   @ref TIMER1_HandleTypeDef.OverflowCallback), not by redefining the
 *   handler. Applications that need RBIF/INTF (INTCON) handling can
 *   extend this dispatcher.
 */

#include "core/pic16f87xa_interrupt.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "peripherals/pic16f87xa_timer2.h"
#include "peripherals/pic16f87xa_ccp.h"
#include "peripherals/pic16f87xa_usart.h"
#include "peripherals/pic16f87xa_ssp.h"
#include "peripherals/pic16f87xa_adc.h"
#include "peripherals/pic16f87xa_eeprom.h"
#include "peripherals/pic16f87xa_comp.h"
#if PIC16F87XA_FAMILY_HAS_PSP
#include "peripherals/pic16f87xa_psp.h"
#endif

#if !defined(PIC16F87XA_USE_SIMULATOR)

/**
 * @brief  Single PIC16 interrupt-vector handler.
 *
 *         The CPU vectors here on any enabled interrupt; dispatch to
 *         every peripheral handler. Each handler is a no-op unless its
 *         own flag is set, so the order does not matter and the cost
 *         of checking every source is a handful of instructions.
 */
void __interrupt() PIC16F87XA_IRQ_Handler(void)
{
    TIMER0_IRQHandler();
    TIMER1_IRQHandler();
    TIMER2_IRQHandler();
    CCP1_IRQHandler();
    CCP2_IRQHandler();
    SSP_IRQHandler();
    USART_RX_IRQHandler();
    USART_TX_IRQHandler();
    ADC_IRQHandler();
    EEPROM_IRQHandler();
    COMP_IRQHandler();
#if PIC16F87XA_FAMILY_HAS_PSP
    PSP_IRQHandler();
#endif
}

#endif /* !PIC16F87XA_USE_SIMULATOR */