/*
 * M-Stack mmc.h config for the real PIC18Fxx5x target.
 *
 * Private to this module -- included only by the vendored mmc.c
 * (third_party/m-stack-storage/src/mmc.c) when compiled for real
 * silicon. The host test build uses a completely different
 * tests/mock/mmc_config.h instead (see pic8-sdcard/docs/pic8-sdcard-plan.md,
 * "Host build story" -- mmc.c itself is portable, so unlike pic8-usb this
 * module compiles the SAME vendored source against two different configs
 * rather than needing a separate host-stub reimplementation).
 *
 * Binds MMC_SPI_TRANSFER/SET_CS/SET_SPEED to pic8_sdcard.c's HAL_SSP/
 * HAL_GPIO-backed functions, and the timer macros to pic8-tick -- real
 * wall-clock timeouts instead of mmc.c's bounded-retry-count fallback
 * (see the plan doc's "Confirmed API surface" for why this matters: the
 * spec-derived MMC_READ_TIMEOUT/MMC_WRITE_TIMEOUT constants in mmc.c are
 * otherwise dead weight).
 */

#ifndef PIC8_SDCARD_MMC_CONFIG_H__
#define PIC8_SDCARD_MMC_CONFIG_H__

#define MMC_SPI_TRANSFER  pic8_sdcard_spi_transfer
#define MMC_SPI_SET_CS    pic8_sdcard_spi_set_cs
#define MMC_SPI_SET_SPEED pic8_sdcard_spi_set_speed

#define MMC_USE_TIMER
#define MMC_TIMER_START   pic8_sdcard_timer_start
#define MMC_TIMER_EXPIRED pic8_sdcard_timer_expired
#define MMC_TIMER_STOP    pic8_sdcard_timer_stop

#endif /* PIC8_SDCARD_MMC_CONFIG_H__ */
