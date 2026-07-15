/*
 * M-Stack mmc.h config for the host test build.
 *
 * Binds MMC_SPI_TRANSFER/SET_CS/SET_SPEED to pic8_sdcard_mock_spi.c
 * instead of real hardware -- see that file's header comment and
 * pic8-sdcard/docs/pic8-sdcard-plan.md, "Host build story".
 *
 * MMC_USE_TIMER is deliberately NOT defined here: a host mock has no real
 * SPI latency to time out against, so mmc.c's bounded-retry-count
 * fallback (see mmc.c's own #ifndef MMC_USE_TIMER block) is both simpler
 * and sufficient for host tests. The real target's src/target/mmc_config.h
 * DOES define it, bound to pic8-tick -- that's the one that matters for
 * real hardware.
 */

#ifndef PIC8_SDCARD_MOCK_MMC_CONFIG_H__
#define PIC8_SDCARD_MOCK_MMC_CONFIG_H__

#define MMC_SPI_TRANSFER  pic8_sdcard_mock_spi_transfer
#define MMC_SPI_SET_CS    pic8_sdcard_mock_spi_set_cs
#define MMC_SPI_SET_SPEED pic8_sdcard_mock_spi_set_speed

#endif /* PIC8_SDCARD_MOCK_MMC_CONFIG_H__ */
