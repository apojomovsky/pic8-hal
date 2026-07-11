/**
 * @file    pic16f87xa_eeprom.c
 * @brief   Data EEPROM driver, implementation (DS39582B §3.0).
 */

#include "peripherals/pic16f87xa_eeprom.h"
#include "core/pic16f87xa_interrupt.h"

static void (*g_eeprom_cb)(void) = NULL;

/* Bank helpers, EEPROM registers are in Banks 2 and 3. */
static void b3_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(3);
    PIC16F87XA_REG8(addr) = v;
    pic_select_bank(prev);
}

static uint8_t b3_read(uint16_t addr)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(3);
    uint8_t v = PIC16F87XA_REG8(addr);
    pic_select_bank(prev);
    return v;
}

static void b2_write(uint16_t addr, uint8_t v)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(2);
    PIC16F87XA_REG8(addr) = v;
    pic_select_bank(prev);
}

static uint8_t b2_read(uint16_t addr)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(2);
    uint8_t v = PIC16F87XA_REG8(addr);
    pic_select_bank(prev);
    return v;
}

PIC16F87XA_StatusTypeDef HAL_EEPROM_Init(void (*callback)(void))
{
    g_eeprom_cb = callback;
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_EEPROM);
    if (callback) PIC16F87XA_IRQ_Enable(PIC16F87XA_IRQ_EEPROM);
    else          PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_EEPROM);
    return PIC16F87XA_OK;
}

PIC16F87XA_StatusTypeDef HAL_EEPROM_DeInit(void)
{
    PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_EEPROM);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_EEPROM);
    g_eeprom_cb = NULL;
    return PIC16F87XA_OK;
}

uint8_t HAL_EEPROM_ReadByte(uint8_t addr)
{
    b2_write(0x0DU, addr);                  /* EEADR. */
    b3_write(0x18CU, 0x00U);                /* EECON1 = 0, set RD. */
    b3_write(0x18CU, 0x01U);                /* EECON1<RD>=1. */
    /* Sim backend: pull the byte from the simulated EEPROM array. */
    extern uint8_t pic16f87xa_sim_eeprom_read(uint8_t addr);
    b2_write(0x0CU, pic16f87xa_sim_eeprom_read(addr));
    return b2_read(0x0CU);
}

PIC16F87XA_StatusTypeDef HAL_EEPROM_WriteByte(uint8_t addr, uint8_t data)
{
    /* §3.4: check WRERR before starting. */
    if (b3_read(0x18CU) & PIC_EECON1_WRERR) return PIC16F87XA_ERROR;

    b2_write(0x0CU, data);                  /* EEDATA. */
    b2_write(0x0DU, addr);                  /* EEADR. */
    b3_write(0x18CU, 0x00U);                /* clear WREN/WR. */
    b3_write(0x18CU, 0x04U);                /* WREN=1. */
    /* Unlock sequence, §3.4 / Example 3-1. */
    b3_write(0x18DU, 0x55U);                /* EECON2 = 0x55. */
    b3_write(0x18DU, 0xAAU);                /* EECON2 = 0xAA. */
    b3_write(0x18CU, PIC_EECON1_WREN | PIC_EECON1_WR);  /* start write. */
    /* WR is held for the write cycle (DS39582B §3.4). On real
     * hardware the CPU sees it clear when the cycle completes; the
     * sim backend mirrors that in sim_step(). The caller polls EEIF
     * (PIR2<4>) to detect completion. */
    return PIC16F87XA_OK;
}

void HAL_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = HAL_EEPROM_ReadByte((uint8_t)(start + i));
    }
}

PIC16F87XA_StatusTypeDef HAL_EEPROM_WriteBuffer(uint8_t start,
                                                const uint8_t *buf,
                                                uint8_t len)
{
    PIC16F87XA_StatusTypeDef st;
    for (uint8_t i = 0; i < len; i++) {
        st = HAL_EEPROM_WriteByte((uint8_t)(start + i), buf[i]);
        if (st != PIC16F87XA_OK) return st;
    }
    return PIC16F87XA_OK;
}

uint8_t HAL_EEPROM_IsWriteComplete(void)
{
    /* EEIF lives in PIR2<4>. */
    return (PIC16F87XA_REG8(0x0DU) & 0x10U) ? 1U : 0U;
}

void HAL_EEPROM_ClearITFlag(void)
{
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_EEPROM);
}

void EEPROM_IRQHandler(void)
{
    if (!PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_EEPROM)) return;
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_EEPROM);
    if (g_eeprom_cb) g_eeprom_cb();
}