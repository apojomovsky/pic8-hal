/**
 * @file    pic16f87xa_gpio.c
 * @brief   GPIO driver — implementation matching datasheet §4.1..§4.5.
 */

#include "peripherals/pic16f87xa_gpio.h"

/**
 * @brief  Map a GPIO_TypeDef to the address of its TRIS register.
 *         TRISx is always at PORTx | 0x80 in the banked register file
 *         (DS39582B Figure 2-3 / 2-4).
 */
static uint8_t tris_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_TRISA;
        case GPIOB: return PIC_REG_TRISB;
        case GPIOC: return PIC_REG_TRISC;
#if PIC16F87XA_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_TRISD;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_TRISE;
#endif
        default:    return PIC_REG_TRISA;
    }
}

/** Map a GPIO_TypeDef to the PORTx register address. */
static uint8_t port_addr(GPIO_TypeDef port)
{
    switch (port) {
        case GPIOA: return PIC_REG_PORTA;
        case GPIOB: return PIC_REG_PORTB;
        case GPIOC: return PIC_REG_PORTC;
#if PIC16F87XA_FAMILY_HAS_PORTD
        case GPIOD: return PIC_REG_PORTD;
#endif
#if PIC16F87XA_FAMILY_HAS_PORTE
        case GPIOE: return PIC_REG_PORTE;
#endif
        default:    return PIC_REG_PORTA;
    }
}

/**
 * @brief  Upper pin bound for a port. PORTA = 6, PORTE = 3, others = 8.
 *         Anything above this is unimplemented (DS39582B Table 4-1..4-9,
 *         "—" marks).
 */
static uint8_t port_width(GPIO_TypeDef port)
{
#if PIC16F87XA_FAMILY_HAS_PORTE
    if (port == GPIOE) return 3U;
#endif
    if (port == GPIOA) return 6U;
    return 8U;
}

/* ───────────────────────── init / deinit ────────────────────────── */

void HAL_GPIO_Init(GPIO_TypeDef port, uint16_t pins, GPIO_ModeTypeDef mode)
{
    const uint8_t pa = port_addr(port);
    const uint8_t ta = tris_addr(port);
    uint8_t mask   = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);

    uint8_t tris = PIC16F87XA_REG8(ta);

    switch (mode) {
        case GPIO_MODE_INPUT:
        case GPIO_MODE_ANALOG:
            /* Both modes set TRIS=1 (input). Analog mode additionally
             * requires ADCON1 configuration, which the user does separately
             * via HAL_ADC_ConfigChannels(). */
            tris |= mask;
            break;
        case GPIO_MODE_OUTPUT:
            tris &= (uint8_t)~mask;
            break;
        default:
            return;
    }
    PIC16F87XA_REG8(ta) = tris;
}

void HAL_GPIO_DeInit(GPIO_TypeDef port)
{
    const uint8_t ta = tris_addr(port);
    /* Reset all implemented bits of TRISx to 1 = input. */
    PIC16F87XA_REG8(ta) = (uint8_t)((1U << port_width(port)) - 1U);
}

/* ───────────────────────── read / write / toggle ────────────────── */

void HAL_GPIO_WritePin(GPIO_TypeDef port, uint16_t pins, GPIO_PinState state)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t pa = port_addr(port);
    uint8_t cur = PIC16F87XA_REG8(pa);
    if (state == GPIO_PIN_SET) cur |= mask;
    else                       cur &= (uint8_t)~mask;
    PIC16F87XA_REG8(pa) = cur;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef port, uint16_t pins)
{
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t pa = port_addr(port);
    PIC16F87XA_REG8(pa) = PIC16F87XA_REG8(pa) ^ mask;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef port, uint16_t pins)
{
    /* Reading the PIN level semantics:
     *   - If TRIS bit is 1 (input) → returns pin state.
     *   - If TRIS bit is 0 (output) → returns the latch.
     * The sim backend implements the same behavior; on a real XC8
     * target the compiler lowers this to a single MOVF. */
    uint8_t mask = (uint8_t)pins & (uint8_t)((1U << port_width(port)) - 1U);
    uint8_t pa = port_addr(port);
    return (PIC16F87XA_REG8(pa) & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_WritePort(GPIO_TypeDef port, uint8_t value)
{
    uint8_t mask = (uint8_t)((1U << port_width(port)) - 1U);
    PIC16F87XA_REG8(port_addr(port)) = (uint8_t)(value & mask);
}

uint8_t HAL_GPIO_ReadPort(GPIO_TypeDef port)
{
    return PIC16F87XA_REG8(port_addr(port));
}

/* ───────────────────────── PORTB pull-ups ───────────────────────── */

void HAL_GPIO_SetPullups(GPIO_PullTypeDef pull)
{
    uint8_t opt = PIC16F87XA_REG8(PIC_REG_OPTION);
    if (pull == GPIO_PULLUP) {
        PIC16F87XA_BIT_CLR(opt, (uint8_t)0x80);    /* RBPU = 0 → enabled */
    } else {
        PIC16F87XA_BIT_SET(opt, (uint8_t)0x80);    /* RBPU = 1 → disabled */
    }
    PIC16F87XA_REG8(PIC_REG_OPTION) = opt;
}