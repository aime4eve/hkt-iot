/**
 * @file    upgrade_frame.c
 * @brief   BLE upgrade frame codec - see upgrade_frame.h.
 */
#include <string.h>

#include "upgrade_common.h"
#include "upgrade_frame.h"

uint8_t upg_frame_checksum(const uint8_t *frame, uint16_t up_to)
{
    uint16_t i;
    uint8_t  sum = 0;

    for (i = 0; i < up_to; i++) {
        sum = (uint8_t)(sum + frame[i]);
    }
    return sum;
}

int upg_frame_parse(const uint8_t *buf, uint16_t len, upg_frame_t *out)
{
    if ((buf == NULL) || (out == NULL) || (len < UPG_FRAME_MIN_LEN)) {
        return -1;
    }
    if ((buf[0] != UPG_SYNC_B0) || (buf[1] != UPG_SYNC_B1) ||
        (buf[2] != UPG_SYNC_B2)) {
        return -1;
    }

    out->special = buf[3];
    out->seq     = buf[4];
    out->type    = buf[5];

    /* Upgrade-class frames always carry the checksum (BIT1) - reject the rest */
    if (out->special & UPG_FR_CHECKSUM) {
        if (upg_frame_checksum(buf, (uint16_t)(len - 1)) != buf[len - 1]) {
            return -1;
        }
        out->data     = &buf[UPG_FRAME_HEAD_LEN];
        out->data_len = (uint16_t)(len - 1 - UPG_FRAME_HEAD_LEN);
    } else {
        return -1;
    }
    return 0;
}

int upg_frame_build(uint8_t *out, uint16_t cap, uint8_t special, uint8_t seq,
                    uint8_t type, const uint8_t *data, uint16_t data_len)
{
    uint16_t total = (uint16_t)(UPG_FRAME_HEAD_LEN + data_len);
    uint16_t i;

    if (special & UPG_FR_CHECKSUM) {
        total++;
    }
    if ((out == NULL) || (total > cap) ||
        ((data == NULL) && (data_len != 0))) {
        return -1;
    }

    out[0] = UPG_SYNC_B0;
    out[1] = UPG_SYNC_B1;
    out[2] = UPG_SYNC_B2;
    out[3] = special;
    out[4] = seq;
    out[5] = type;
    for (i = 0; i < data_len; i++) {
        out[UPG_FRAME_HEAD_LEN + i] = data[i];
    }
    if (special & UPG_FR_CHECKSUM) {
        out[total - 1] = upg_frame_checksum(out, (uint16_t)(total - 1));
    }
    return (int)total;
}
