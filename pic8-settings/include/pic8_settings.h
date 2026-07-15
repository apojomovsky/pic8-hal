/**
 * @file    pic8_settings.h
 * @brief   EEPROM-backed settings blobs with CRC-16 validation.
 *
 * @details
 *   Saves an opaque caller-owned blob to EEPROM, appending a trailing
 *   CRC-16-CCITT so a later load can detect corruption or an unwritten
 *   region. There is no instance object and no hidden state: a settings
 *   block is just `(eeprom_addr, size, data)` passed explicitly on each
 *   call, so one firmware can keep several independent settings blobs in
 *   different EEPROM regions.
 *
 *   The module intentionally does not try to distinguish "never written"
 *   from "corrupt" on load failure. Both cases mean the stored blob is not
 *   usable, and the normal recovery is the same: apply defaults.
 */

#ifndef PIC8_SETTINGS_H
#define PIC8_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Save @p size bytes from @p data to EEPROM at @p eeprom_addr,
 *         followed by a trailing CRC-16-CCITT.
 *
 * @param  eeprom_addr  First EEPROM byte to write.
 * @param  data         Caller-owned blob to persist.
 * @param  size         Number of payload bytes in @p data.
 *
 * @return true if every payload byte and both CRC bytes wrote successfully;
 *         false if the underlying EEPROM driver rejected any byte write.
 *
 * @note   The caller must ensure `eeprom_addr + size + 2` stays within the
 *         target device's EEPROM size. This module is intentionally family-
 *         agnostic and does not carry a device-capacity table.
 */
bool pic8_settings_save(uint8_t eeprom_addr, const void *data, uint8_t size);

/**
 * @brief  Load @p size bytes from EEPROM at @p eeprom_addr into @p data,
 *         validating the trailing CRC-16-CCITT first.
 *
 * @param  eeprom_addr  First EEPROM byte of the stored blob.
 * @param  data         Destination buffer to fill on success.
 * @param  size         Number of payload bytes expected.
 *
 * @return true if the stored blob was valid and copied into @p data; false
 *         if the region was blank/corrupt, in which case @p data is left
 *         untouched.
 */
bool pic8_settings_load(uint8_t eeprom_addr, void *data, uint8_t size);

/**
 * @brief  Convenience wrapper: load if valid, otherwise copy
 *         @p default_data into @p data and persist it to EEPROM.
 *
 * @param  eeprom_addr   First EEPROM byte of the stored blob.
 * @param  data          Destination buffer for either the stored blob or the
 *                       applied defaults.
 * @param  size          Number of payload bytes.
 * @param  default_data  Caller-owned default blob of @p size bytes.
 *
 * @return true if a valid stored blob already existed; false if defaults had
 *         to be applied.
 *
 * @note   On the first invalid load this performs a real EEPROM write so the
 *         next boot sees the default blob as already valid.
 */
bool pic8_settings_load_or_default(uint8_t eeprom_addr, void *data, uint8_t size,
                                   const void *default_data);

#endif /* PIC8_SETTINGS_H */
