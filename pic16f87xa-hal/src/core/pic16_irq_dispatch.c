/**
 * @file    pic16_irq_dispatch.c
 * @brief   Fan-out from the single PIC16 interrupt vector to every
 *          peripheral IRQHandler. Shared by both builds.
 *
 * @details
 *   The PIC16F87XA family has a single interrupt vector at 0x0004
 *   (DS39582B §14.11). On a real target the XC8 `__interrupt()` handler in
 *   pic16_isr_vector.c calls this; on the host the harness registers
 *   this as the sim IRQ callback. Both reach the same dispatch, so there
 *   is one source of truth and no duplication.
 *
 *   Each peripheral IRQHandler checks its own flag and returns
 *   immediately when its source is not pending (see e.g.
 *   @ref TIMER0_IRQHandler), so calling them all in turn is correct and
 *   costs only a few cycles per interrupt.
 *
 *   The handlers are declared `PIC8_WEAK` in their own headers (to
 *   allow optional user override). That makes a reference through those
 *   headers a *weak* reference, which the linker will NOT use to pull the
 *   handler's object out of the static library, leaving the call target
 *   NULL for any handler not already pulled by a strong reference. To
 *   force the linker to resolve every handler, this file declares them
 *   with strong prototypes and does not include the peripheral headers.
 *   (On the XC8 target there is no weak attribute, so this is a no-op
 *   there; it only matters for the host link.)
 */

#include "core/pic16_irq.h"

extern void TIMER0_IRQHandler(void);
extern void TIMER1_IRQHandler(void);
extern void TIMER2_IRQHandler(void);
extern void CCP1_IRQHandler(void);
extern void CCP2_IRQHandler(void);
extern void SSP_IRQHandler(void);
extern void USART_RX_IRQHandler(void);
extern void USART_TX_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void EEPROM_IRQHandler(void);
extern void COMP_IRQHandler(void);
#if PIC16F87XA_FAMILY_HAS_PSP
extern void PSP_IRQHandler(void);
#endif

void pic8_dispatch_all_irqs(void)
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