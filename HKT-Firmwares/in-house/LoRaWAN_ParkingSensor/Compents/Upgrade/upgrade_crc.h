/**
 * @file    upgrade_crc.h
 * @brief   CRC helpers for the BLE upgrade (shared by APP and BootLoader)
 *
 *          - CRC32: reflected 0xEDB88320 (IEEE 802.3), init 0xFFFFFFFF,
 *            final xor 0xFFFFFFFF - matches Android CRC32.ieee and the PC diff
 *            generator (boot_crc32.c lineage).
 *          - CRC16: KERMIT (reflected 0x8408 = reverse of 0x1021, init 0x0000,
 *            no final xor) - same as Compents/BootLoader/crclib.c crc16_ccitt()
 *            and Android CRC16.ccitt. Used for per-packet checks and struct
 *            integrity.
 */
#ifndef __UPGRADE_CRC_H__
#define __UPGRADE_CRC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*--- CRC32 (streaming + one-shot) -----------------------------------------*/
void     upg_crc32_start(uint32_t *state);
void     upg_crc32_update(uint32_t *state, const uint8_t *data, uint32_t len);
uint32_t upg_crc32_finish(uint32_t state);
uint32_t upg_crc32(const uint8_t *data, uint32_t len);

/*--- CRC16/KERMIT ----------------------------------------------------------*/
uint16_t upg_crc16(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __UPGRADE_CRC_H__ */
