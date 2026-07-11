/**
 * @file    target/pic16f87xa_platform.h
 * @brief   Real-target platform: how SFRs are accessed and how the weak
 *          attribute is spelled, for the XC8 build.
 *
 * @details
 *   This is the target half of the SFR mapping layer. The companion
 *   host/pic16f87xa_platform.h is used by the CMake host build. Which one
 *   is included is decided by the build's include path (the XC8 Makefile
 *   puts include/target first; CMake puts include/host first), so
 *   pic16f87xa.h includes "pic16f87xa_platform.h" unconditionally and
 *   there is no `#ifdef` around code anywhere in the HAL.
 *
 *   On a real PIC every SFR access is a direct volatile dereference of
 *   the literal address, exactly what the XC8 linker maps to the SFR.
 *   The address is cast through uintptr_t so XC8 does not warn about
 *   converting an integer to a pointer. XC8 has no weak symbols, so
 *   PIC8_WEAK is empty.
 */

#ifndef PIC16F87XA_PLATFORM_H
#define PIC16F87XA_PLATFORM_H

#include <stdint.h>

/* XC8 has no concept of weak symbols. */
#define PIC8_WEAK

/* SFR access resolves to a direct volatile dereference of the address. */
#define PIC8_SFR_PTR(addr)       ((volatile uint8_t *)(uintptr_t)(addr))
#define pic8_sfr_read8(addr)     (*(volatile uint8_t *)(uintptr_t)(addr))
#define pic8_sfr_write8(addr, v) \
    do { *(volatile uint8_t *)(uintptr_t)(addr) = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC8_REG8(addr)          (*(volatile uint8_t *)(uintptr_t)(addr))

#endif /* PIC16F87XA_PLATFORM_H */