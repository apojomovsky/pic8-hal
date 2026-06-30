/**
 * @file    pic16f87xa_sim.c
 * @brief   Host simulation backend for the PIC16F87XA HAL.
 *
 * @details
 *   Implements the SFR register file (`pic16f87xa_sim_sfr`) that the
 *   pre-processor SFR macros dereference when PIC16F87XA_USE_SIMULATOR
 *   is defined. Also models the peripherals the test rig actually exercises:
 *
 *     - PORTA..PORTE + their TRISx latches, including read-modify-write
 *       semantics (DS39582B §4.x — every port is "read the pin, write the
 *       latch").
 *     - Timer0 8-bit up-counter with prescaler.
 *     - A/D conversion timing (simplified).
 *
 *   Everything not modelled here (USART, MSSP, etc.) is fully observable
 *   via direct SFR reads in tests — their behavior is whatever the code
 *   under test wrote to the registers.
 */

#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include <string.h>

/* ───────────────────────── register file ────────────────────────── */

/** SFR backing store. Indices match the datasheet register map (Bank 0
 *  occupies 0x00..0x1F, Bank 1 0x80..0x9F, etc.).
 *
 *  Defined as `extern` in pic16f87xa.h and provided here. The total size
 *  covers the EEPROM bank (Bank 2 = 0x100..0x11F) plus Bank 3 reserved.
 */
uint8_t pic16f87xa_sim_sfr[0x200];

/** Pin latch overrides driven by the test rig (per pin). */
static uint8_t sim_input_override[5] = {0};   /* ports A..E, bits = override mask. */
static uint8_t sim_input_value   [5] = {0};

/** Optional ISR hook. */
static pic16f87xa_sim_irq_cb_t sim_irq_cb = 0;

/* ───────────────────────── GPIO model ───────────────────────────── */

/* Helper: read the latched value of port `port` (0=A..4=E) honoring
 * read-modify-write: writes only update the latch, reads return the pin
 * state if the pin is configured as input, else the latch. */
static uint8_t port_latch(char port)
{
    switch (port) {
        case 'A': case 'a': return pic16f87xa_sim_sfr[PIC_REG_PORTA];
        case 'B': case 'b': return pic16f87xa_sim_sfr[PIC_REG_PORTB];
        case 'C': case 'c': return pic16f87xa_sim_sfr[PIC_REG_PORTC];
        case 'D': case 'd': return pic16f87xa_sim_sfr[PIC_REG_PORTD];
        case 'E': case 'e': return pic16f87xa_sim_sfr[PIC_REG_PORTE];
        default:             return 0xFFU;
    }
}

static uint8_t tris_reg(char port)
{
    switch (port) {
        case 'A': case 'a': return pic16f87xa_sim_sfr[PIC_REG_TRISA];
        case 'B': case 'b': return pic16f87xa_sim_sfr[PIC_REG_TRISB];
        case 'C': case 'c': return pic16f87xa_sim_sfr[PIC_REG_TRISC];
        case 'D': case 'd': return pic16f87xa_sim_sfr[PIC_REG_TRISD];
        case 'E': case 'e': return pic16f87xa_sim_sfr[PIC_REG_TRISE];
        default:             return 0xFFU;
    }
}

static uint8_t port_index(char port)
{
    switch (port) {
        case 'A': case 'a': return 0;
        case 'B': case 'b': return 1;
        case 'C': case 'c': return 2;
        case 'D': case 'd': return 3;
        case 'E': case 'e': return 4;
        default:             return 0;
    }
}

/* ───────────────────────── public API ───────────────────────────── */

void pic16f87xa_sim_reset(void)
{
    memset(pic16f87xa_sim_sfr, 0, sizeof pic16f87xa_sim_sfr);

    /* Power-on reset values from DS39582B Table 14-6. */
    pic16f87xa_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_PCON]     = PIC_PCON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_PIR1]     = PIC_PIR1_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_PIR2]     = PIC_PIR2_POR_VALUE;
    /* PIE1 / PIE2 reset to 0 — same addresses as PIR1/PIR2 in bank 1. */
    pic16f87xa_sim_sfr[0x8CU]            = PIC_PIE1_POR_VALUE;
    pic16f87xa_sim_sfr[0x8DU]            = PIC_PIE2_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_T1CON]    = PIC_T1CON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_T2CON]    = PIC_T2CON_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_ADCON0]   = PIC_ADCON0_POR_VALUE;
    pic16f87xa_sim_sfr[PIC_REG_ADCON1]   = PIC_ADCON1_POR_VALUE;

    /* PORTA on POR reads as analog input '0' (DS39582B §4.1). */
    pic16f87xa_sim_sfr[PIC_REG_PORTA]    = 0x00U;
    pic16f87xa_sim_sfr[PIC_REG_PORTE]    = 0x00U;

    /* TRIS defaults: 1 = input, so PORTA:6 and others = 1. */
    pic16f87xa_sim_sfr[PIC_REG_TRISA]    = 0x3FU;     /* 6-bit wide. */
    pic16f87xa_sim_sfr[PIC_REG_TRISB]    = 0xFFU;
    pic16f87xa_sim_sfr[PIC_REG_TRISC]    = 0xFFU;
    pic16f87xa_sim_sfr[PIC_REG_TRISD]    = 0xFFU;
    pic16f87xa_sim_sfr[PIC_REG_TRISE]    = 0x07U;

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
}

void pic16f87xa_sim_step(uint32_t ticks)
{
    /* Read the active Timer0 prescaler.
     * T0PS<2:0> live in OPTION_REG, bits 0..2. */
    uint8_t option = pic16f87xa_sim_sfr[PIC_REG_OPTION];
    uint8_t ps     = option & 0x07U;                  /* PS2:PS1:PS0 */
    uint8_t psa    = (option >> 3) & 0x01U;           /* PSA */

    static uint16_t t0_prescaler = 0U;
    /* Prescaler assignment to WDT (psa=1) means Timer0 runs at Fosc/4. */
    (void)psa;

    /* OPTION_REG<PS2:PS0> prescaler mapping (DS39582B §5.0, Table 5-1).
     *   000 → 1:2
     *   001 → 1:4
     *   010 → 1:8
     *   011 → 1:16
     *   100 → 1:32
     *   101 → 1:64
     *   110 → 1:128
     *   111 → 1:256
     * Value = number of input ticks per TMR0 increment. */
    static const uint8_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 255};

    uint32_t rate = ps_idx[ps];

    for (uint32_t i = 0; i < ticks; i++) {
        t0_prescaler++;
        if (t0_prescaler >= rate) {
            t0_prescaler = 0U;
            uint8_t t0 = pic16f87xa_sim_sfr[PIC_REG_TMR0];
            t0++;
            /* DS39582B §14.11.2: TMR0IF is set on the 0xFF → 0x00
             * overflow. We detect the wrap when the post-increment
             * value has rolled back to 0x00. */
            if (t0 == 0x00U) {
                pic16f87xa_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
                if (sim_irq_cb) sim_irq_cb();
            }
            pic16f87xa_sim_sfr[PIC_REG_TMR0] = t0;
        }
    }
}

void pic16f87xa_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    if (pin > 7U) return;
    uint8_t idx = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    sim_input_override[idx] |= mask;
    if (level) sim_input_value[idx] |= mask;
    else       sim_input_value[idx] &= (uint8_t)~mask;
}

uint8_t pic16f87xa_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    uint8_t tris = tris_reg(port);

    if (tris & mask) {
        /* Pin configured as input: return the externally driven level. */
        return (sim_input_override[idx] & mask) ?
               ((sim_input_value[idx] & mask) ? 1U : 0U) :
               /* No override — floating, simulate as 0. */
               0U;
    }
    /* Pin configured as output: return the latch bit. */
    return (port_latch(port) & mask) ? 1U : 0U;
}

void pic16f87xa_sim_set_irq_callback(pic16f87xa_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}