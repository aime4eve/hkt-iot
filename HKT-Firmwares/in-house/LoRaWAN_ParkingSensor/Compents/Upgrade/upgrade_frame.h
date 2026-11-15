/**
 * @file    upgrade_frame.h
 * @brief   Codec for the BLE upgrade frame family (APP project only - the
 *          BootLoader has no BLE).
 *
 * Frame:  hkt(3) special(1) seq(1) type(1) data(n) checksum(1, optional)
 *         - upgrade frames force BIT1 (checksum); parser rejects inbound
 *           frames without it (doc: 升级类指令强制带校验).
 *         - checksum = byte sum from the sync head through the last data byte,
 *           mod 256 (verified against every example in the protocol doc).
 *         - multi-byte payload fields are big-endian.
 */
#ifndef __UPGRADE_FRAME_H__
#define __UPGRADE_FRAME_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UPG_FRAME_HEAD_LEN      6U      /* hkt(3)+special(1)+seq(1)+type(1) */
#define UPG_FRAME_MIN_LEN       7U      /* head + empty data + checksum */

typedef struct {
    uint8_t         special;
    uint8_t         seq;
    uint8_t         type;
    const uint8_t  *data;               /* points into the input buffer */
    uint16_t        data_len;
} upg_frame_t;

/* Sum of frame[0..up_to) mod 256. */
uint8_t upg_frame_checksum(const uint8_t *frame, uint16_t up_to);

/**
 * Validate and parse an inbound frame.
 * @return 0 on success (fields filled), -1 on any mismatch.
 * Requires: sync bytes, special & UPG_FR_CHECKSUM, checksum byte.
 */
int upg_frame_parse(const uint8_t *buf, uint16_t len, upg_frame_t *out);

/**
 * Build an outbound frame (checksum appended when special & BIT1).
 * @return total frame length, or -1 if the buffer is too small.
 */
int upg_frame_build(uint8_t *out, uint16_t cap, uint8_t special, uint8_t seq,
                    uint8_t type, const uint8_t *data, uint16_t data_len);

#ifdef __cplusplus
}
#endif

#endif /* __UPGRADE_FRAME_H__ */
