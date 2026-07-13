/**
 * @file    pic18_sim.c
 * @brief   PIC18F2455 family host simulation backend.
 *
 * @details
 *   Linked by the CMake host build only; the XC8 Makefile does not compile
 *   this file. Provides `pic18_sim_sfr[]`, the 4096-byte memory-backed
 *   register file the host SFR macros (include/host/pic18_platform.h)
 *   dereference, and the hooks declared in pic18f2455_sim.h.
 *
 *   Phase 2 models Timer0 (8/16-bit, prescaler, overflow -> TMR0IF) and
 *   GPIO drive/read. The flat-array / physical-address approach (per the
 *   plan's Phase 2 task 2 decision) means every SFR the drivers touch is
 *   in the Access Bank (0xF60-0xFFF) and is just an index into this array;
 *   no BSR translation is needed.
 */

#include "pic18f2455_sim.h"
#include "pic18f2455_sfr.h"
#include "pic18_platform.h"

#include <string.h>

/* ───────────────────────── register file ────────────────────────── */

/* 4096-byte memory-backed register file, referenced by
 * include/host/pic18_platform.h. Provisionally the full 12-bit data-memory
 * footprint; all SFRs the drivers use live in 0xF60-0xFFF. */
uint8_t pic18_sim_sfr[0x1000];

/* Per-pin input overrides set by the host application (A..E). */
static uint8_t sim_input_override[5] = {0};
static uint8_t sim_input_value   [5] = {0};

/* Optional ISR hook (the family dispatcher, registered by the harness). */
static pic18_sim_irq_cb_t sim_irq_cb = 0;

static void sim_step_timer0(void);
static void sim_step_timer1(void);
static void sim_step_timer2(void);
static void sim_step_timer3(void);

/* ───────────────────────── helpers ──────────────────────────────── */

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

/** LATx address for a port letter. */
static uint16_t lat_addr(char port)
{
    switch (port) {
        case 'A': case 'a': return PIC_REG_LATA;
        case 'B': case 'b': return PIC_REG_LATB;
        case 'C': case 'c': return PIC_REG_LATC;
#if PIC18F2455_FAMILY_HAS_PORTD
        case 'D': case 'd': return PIC_REG_LATD;
#endif
#if PIC18F2455_FAMILY_HAS_PORTE
        case 'E': case 'e': return PIC_REG_LATE;
#endif
        default:             return PIC_REG_LATA;
    }
}

/** TRISx address for a port letter. */
static uint16_t tris_addr(char port)
{
    switch (port) {
        case 'A': case 'a': return PIC_REG_TRISA;
        case 'B': case 'b': return PIC_REG_TRISB;
        case 'C': case 'c': return PIC_REG_TRISC;
#if PIC18F2455_FAMILY_HAS_PORTD
        case 'D': case 'd': return PIC_REG_TRISD;
#endif
#if PIC18F2455_FAMILY_HAS_PORTE
        case 'E': case 'e': return PIC_REG_TRISE;
#endif
        default:             return PIC_REG_TRISA;
    }
}

/* ───────────────────────── public API ───────────────────────────── */

void pic18_sim_reset(void)
{
    memset(pic18_sim_sfr, 0, sizeof pic18_sim_sfr);

    /* Power-on reset values, DS39632E Table 5-1 + Register 4-1. */
    pic18_sim_sfr[PIC_REG_STATUS]   = PIC_STATUS_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_BSR]      = PIC_BSR_POR_VALUE;      /* 0x00 */
    pic18_sim_sfr[PIC_REG_RCON]     = PIC_RCON_POR_VALUE;     /* 0x57 */
    pic18_sim_sfr[PIC_REG_INTCON]   = PIC_INTCON_POR_VALUE;   /* 0x00 */
    pic18_sim_sfr[PIC_REG_INTCON2]  = PIC_INTCON2_POR_VALUE;  /* 0xFB */
    pic18_sim_sfr[PIC_REG_INTCON3]  = PIC_INTCON3_POR_VALUE;  /* 0xC0 */
    pic18_sim_sfr[PIC_REG_PIR1]     = PIC_PIR1_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_PIE1]     = PIC_PIE1_POR_VALUE;     /* 0x00 */
    pic18_sim_sfr[PIC_REG_IPR1]     = PIC_IPR1_POR_VALUE;     /* 0xFF */
    pic18_sim_sfr[PIC_REG_T0CON]    = PIC_T0CON_POR_VALUE;    /* 0xFF */
    pic18_sim_sfr[PIC_REG_T1CON]    = PIC_T1CON_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_T2CON]    = PIC_T2CON_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_T3CON]    = PIC_T3CON_POR_VALUE;    /* 0x00 */
    pic18_sim_sfr[PIC_REG_PR2]     = PIC_PR2_POR_VALUE;       /* 0xFF */
    pic18_sim_sfr[PIC_REG_PIR2]    = PIC_PIR2_POR_VALUE;      /* 0x00 */
    pic18_sim_sfr[PIC_REG_PIE2]    = PIC_PIE2_POR_VALUE;      /* 0x00 */
    pic18_sim_sfr[PIC_REG_IPR2]    = PIC_IPR2_POR_VALUE;      /* 0xFF */

    /* TRIS defaults: 1 = input. PORTA is 6-bit, PORTE 3-bit. */
    pic18_sim_sfr[PIC_REG_TRISA] = 0x3FU;
    pic18_sim_sfr[PIC_REG_TRISB] = PIC_TRIS_POR_VALUE;
    pic18_sim_sfr[PIC_REG_TRISC] = PIC_TRIS_POR_VALUE;
#if PIC18F2455_FAMILY_HAS_PORTD
    pic18_sim_sfr[PIC_REG_TRISD] = PIC_TRIS_POR_VALUE;
#endif
#if PIC18F2455_FAMILY_HAS_PORTE
    pic18_sim_sfr[PIC_REG_TRISE] = 0x07U;
#endif
    /* Latches clear. */
    pic18_sim_sfr[PIC_REG_LATA] = PIC_LAT_POR_VALUE;
    pic18_sim_sfr[PIC_REG_LATB] = PIC_LAT_POR_VALUE;
    pic18_sim_sfr[PIC_REG_LATC] = PIC_LAT_POR_VALUE;
#if PIC18F2455_FAMILY_HAS_PORTD
    pic18_sim_sfr[PIC_REG_LATD] = PIC_LAT_POR_VALUE;
#endif
#if PIC18F2455_FAMILY_HAS_PORTE
    pic18_sim_sfr[PIC_REG_LATE] = PIC_LAT_POR_VALUE;
#endif

    memset(sim_input_override, 0, sizeof sim_input_override);
    memset(sim_input_value,    0, sizeof sim_input_value);
    sim_irq_cb = 0;
}

void pic18_sim_step(uint32_t ticks)
{
    for (uint32_t i = 0; i < ticks; i++) {
        sim_step_timer0();
        sim_step_timer1();
        sim_step_timer2();
        sim_step_timer3();
    }
}

/* ───────────────────────── Timer0 step ──────────────────────────── */

static void sim_step_timer0(void)
{
    /* T0CON layout (DS39632E Register 11-1):
     *   bit 7  TMR0ON
     *   bit 6  T08BIT (1 = 8-bit)
     *   bit 5  T0CS
     *   bit 4  T0SE
     *   bit 3  PSA   (1 = prescaler not assigned -> raw clock)
     *   bit 2..0 T0PS2:T0PS0
     */
    uint8_t t0con = pic18_sim_sfr[PIC_REG_T0CON];
    if (!(t0con & PIC_T0CON_TMR0ON)) return;     /* stopped */

    uint8_t ps  = (uint8_t)(t0con & PIC_T0CON_T0PS_MASK);
    uint8_t psa = (t0con & PIC_T0CON_PSA) ? 1U : 0U;

    /* Prescaler ratio, DS39632E Table 11-1. PSA = 1 -> raw (1:1). uint16_t
     * so the 1:256 entry (256) is not truncated. */
    static const uint16_t ps_idx[8] = {2, 4, 8, 16, 32, 64, 128, 256};
    uint32_t rate = psa ? 1U : ps_idx[ps];

    static uint16_t t0_prescaler = 0U;
    t0_prescaler++;
    if (t0_prescaler < rate) return;
    t0_prescaler = 0U;

    if (t0con & PIC_T0CON_T08BIT) {
        /* 8-bit mode: increment TMR0L. */
        uint8_t t0 = (uint8_t)(pic18_sim_sfr[PIC_REG_TMR0L] + 1U);
        pic18_sim_sfr[PIC_REG_TMR0L] = t0;
        if (t0 == 0x00U) {
            pic18_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    } else {
        /* 16-bit mode: increment TMR0H:TMR0L. */
        uint16_t full = (uint16_t)(((uint16_t)pic18_sim_sfr[PIC_REG_TMR0H] << 8) |
                                   pic18_sim_sfr[PIC_REG_TMR0L]);
        full++;
        pic18_sim_sfr[PIC_REG_TMR0L] = (uint8_t)(full & 0xFFU);
        pic18_sim_sfr[PIC_REG_TMR0H] = (uint8_t)(full >> 8);
        if (full == 0U) {
            pic18_sim_sfr[PIC_REG_INTCON] |= PIC_INTCON_TMR0IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
}

/* ───────────────────────── Timer1 step ──────────────────────────── */

static void sim_step_timer1(void)
{
    /* T1CON layout (DS39632E Register 12-1):
     *   bit 7  RD16
     *   bit 6  T1RUN (status, RO)
     *   bit 5..4 T1CKPS1:T1CKPS0
     *   bit 3  T1OSCEN
     *   bit 2  T1SYNC
     *   bit 1  TMR1CS
     *   bit 0  TMR1ON
     */
    uint8_t t1con = pic18_sim_sfr[PIC_REG_T1CON];
    if (!(t1con & PIC_T1CON_TMR1ON)) return;     /* stopped */

    /* Prescaler 1:1/1:2/1:4/1:8 (T1CKPS1:T1CKPS0). TMR1CS = 1 (external/T1OSC):
     * the sim does not model a real external signal, so it advances at the
     * configured prescaler rate per instruction cycle (lets T1OSC-based
     * firmware run on the host with the same Timer1 config a real target
     * uses; the 32 kHz crystal's actual rate is not reproduced). */
    static const uint8_t ps_idx[4] = {1, 2, 4, 8};
    uint32_t rate = ps_idx[(t1con >> 4) & 0x3U];

    static uint8_t t1_prescaler = 0U;
    t1_prescaler++;
    if (t1_prescaler < rate) return;
    t1_prescaler = 0U;

    /* 16-bit increment (the sim ignores RD16 latching; it reads both bytes
     * atomically). */
    uint16_t full = (uint16_t)(((uint16_t)pic18_sim_sfr[PIC_REG_TMR1H] << 8) |
                               pic18_sim_sfr[PIC_REG_TMR1L]);
    full++;
    pic18_sim_sfr[PIC_REG_TMR1L] = (uint8_t)(full & 0xFFU);
    pic18_sim_sfr[PIC_REG_TMR1H] = (uint8_t)(full >> 8);
    if (full == 0U) {
        pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR1IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}

/* ───────────────────────── Timer2 step ──────────────────────────── */

static void sim_step_timer2(void)
{
    /* T2CON layout (DS39632E Register 12-2):
     *   bit 6..3 T2OUTPS3:T2OUTPS0
     *   bit 2  TMR2ON
     *   bit 1..0 T2CKPS1:T2CKPS0
     */
    uint8_t t2con = pic18_sim_sfr[PIC_REG_T2CON];
    if (!(t2con & PIC_T2CON_TMR2ON)) return;     /* stopped */

    /* T2CKPS1:T2CKPS0 -> 1:1, 1:4, 1:16, 1:16. */
    static const uint8_t pre_idx[4] = {1, 4, 16, 16};
    uint32_t pre = pre_idx[t2con & PIC_T2CON_T2CKPS_MASK];
    /* TOUTPS3:TOUTPS0 -> 1:(N+1). */
    uint8_t  post = (uint8_t)(((t2con & PIC_T2CON_TOUTPS_MASK) >> 3) + 1U);

    uint8_t pr2 = pic18_sim_sfr[PIC_REG_PR2];

    static uint16_t t2_prescaler = 0U;
    static uint8_t  t2_post      = 0U;

    t2_prescaler++;
    if (t2_prescaler < pre) return;
    t2_prescaler = 0U;

    /* TMR2 increments until it matches PR2, then resets (DS39632E §12.0);
     * TMR2IF fires after the postscaler, once per (PR2+1) prescaled cycles. */
    uint8_t t2 = (uint8_t)(pic18_sim_sfr[PIC_REG_TMR2] + 1U);
    if (t2 > pr2) {
        t2 = 0U;
        t2_post++;
        if (t2_post >= post) {
            t2_post = 0U;
            pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_TMR2IF;
            if (sim_irq_cb) sim_irq_cb();
        }
    }
    pic18_sim_sfr[PIC_REG_TMR2] = t2;
}

/* ───────────────────────── Timer3 step ──────────────────────────── */

static void sim_step_timer3(void)
{
    /* T3CON layout (DS39632E Register 14-1):
     *   bit 7  RD16
     *   bit 6  T3CCP2
     *   bit 5..4 T3CKPS1:T3CKPS0
     *   bit 3  T3CCP1
     *   bit 2  T3SYNC
     *   bit 1  TMR3CS
     *   bit 0  TMR3ON
     */
    uint8_t t3con = pic18_sim_sfr[PIC_REG_T3CON];
    if (!(t3con & PIC_T3CON_TMR3ON)) return;     /* stopped */

    static const uint8_t ps_idx[4] = {1, 2, 4, 8};
    uint32_t rate = ps_idx[(t3con >> 4) & 0x3U];

    static uint8_t t3_prescaler = 0U;
    t3_prescaler++;
    if (t3_prescaler < rate) return;
    t3_prescaler = 0U;

    uint16_t full = (uint16_t)(((uint16_t)pic18_sim_sfr[PIC_REG_TMR3H] << 8) |
                               pic18_sim_sfr[PIC_REG_TMR3L]);
    full++;
    pic18_sim_sfr[PIC_REG_TMR3L] = (uint8_t)(full & 0xFFU);
    pic18_sim_sfr[PIC_REG_TMR3H] = (uint8_t)(full >> 8);
    if (full == 0U) {
        pic18_sim_sfr[PIC_REG_PIR2] |= PIC_PIR2_TMR3IF;
        if (sim_irq_cb) sim_irq_cb();
    }
}

/* ───────────────────────── GPIO drive / read ────────────────────── */

void pic18_sim_drive_input(char port, uint8_t pin, uint8_t level)
{
    if (pin > 7U) return;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    sim_input_override[idx] |= mask;
    if (level) sim_input_value[idx] |= mask;
    else       sim_input_value[idx] &= (uint8_t)~mask;
}

uint8_t pic18_sim_read_output(char port, uint8_t pin)
{
    if (pin > 7U) return 0U;
    uint8_t idx  = port_index(port);
    uint8_t mask = (uint8_t)(1U << pin);
    uint8_t tris = pic18_sim_sfr[tris_addr(port)];

    if (tris & mask) {
        /* Input: return the externally driven level (0 if not driven). */
        return (sim_input_override[idx] & mask) ?
               ((sim_input_value[idx] & mask) ? 1U : 0U) : 0U;
    }
    /* Output: return the LATx bit (DS39632E §10.0). */
    return (pic18_sim_sfr[lat_addr(port)] & mask) ? 1U : 0U;
}

void pic18_sim_set_irq_callback(pic18_sim_irq_cb_t cb)
{
    sim_irq_cb = cb;
}

void pic18_sim_drive_ssp_rx(uint8_t data)
{
    /* Place the byte in SSPBUF, set SSPSTAT<BF> + PIR1<SSPIF>. */
    pic18_sim_sfr[PIC_REG_SSPBUF] = data;
    uint8_t stat = (uint8_t)(pic18_sim_sfr[PIC_REG_SSPSTAT] | PIC_SSPSTAT_BF);
    pic18_sim_sfr[PIC_REG_SSPSTAT] = stat;
    pic18_sim_sfr[PIC_REG_PIR1] |= PIC_PIR1_SSPIF;
    if (sim_irq_cb) sim_irq_cb();
}