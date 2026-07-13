/**
 * @file    pic18_irq_dispatch.c
 * @brief   Fan-out from the PIC18 interrupt vectors to every peripheral
 *          IRQHandler. Shared by both builds.
 *
 * @details
 *   The PIC18F2455 family has two interrupt vectors: 0008h high-priority
 *   and 0018h low-priority (DS39632E §9.0). On a real target the XC8
 *   `__interrupt(high_priority)` / `__interrupt(low_priority)` handlers in
 *   pic18_isr_vector.c both call this; on the host the harness registers
 *   this as the sim IRQ callback. Both reach the same dispatch, so there is
 *   one source of truth and no duplication.
 *
 *   Each peripheral IRQHandler checks its own flag and returns immediately
 *   when its source is not pending (see @ref TIMER0_IRQHandler), so calling
 *   them all in turn is correct and costs only a few cycles per interrupt.
 *
 *   Phase 2 MVP: only Timer0 has a handler (the task manager's tick source
 *   and the blink example). Phase 4 adds the per-source handlers for the
 *   remaining peripherals (Timer1/2/3, CCP, MSSP, ADC, USART, ...) the same
 *   way PIC16's pic16_irq_dispatch.c does.
 *
 *   The handler prototypes are declared here as strong externs (not via the
 *   peripheral headers, whose `PIC8_WEAK` declaration would make the
 *   reference weak and let the linker drop the handler's object from the
 *   static library). On the XC8 target there is no weak attribute, so this
 *   is a no-op there; it only matters for the host link.
 */

extern void TIMER0_IRQHandler(void);
extern void TIMER1_IRQHandler(void);
extern void TIMER2_IRQHandler(void);
extern void TIMER3_IRQHandler(void);
extern void CCP1_IRQHandler(void);
extern void CCP2_IRQHandler(void);
extern void SSP_IRQHandler(void);
extern void USART_TX_IRQHandler(void);
extern void USART_RX_IRQHandler(void);
extern void COMP_IRQHandler(void);
extern void EEPROM_IRQHandler(void);
extern void ADC_IRQHandler(void);

void pic8_dispatch_all_irqs(void)
{
    TIMER0_IRQHandler();
    TIMER1_IRQHandler();
    TIMER2_IRQHandler();
    TIMER3_IRQHandler();
    CCP1_IRQHandler();
    CCP2_IRQHandler();
    SSP_IRQHandler();
    USART_TX_IRQHandler();
    USART_RX_IRQHandler();
    COMP_IRQHandler();
    EEPROM_IRQHandler();
    ADC_IRQHandler();
}