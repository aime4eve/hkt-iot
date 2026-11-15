/**
 * @file    upgrade_crc.c
 * @brief   CRC helpers for the BLE upgrade - see upgrade_crc.h for the exact
 *          parameter sets. Table is generated on first use (no 1KB const table
 *          to mistype, negligible init cost).
 */
#include "upgrade_crc.h"

/*==============================================================================
 * CRC32 - reflected 0xEDB88320
 *============================================================================*/
static uint32_t s_crc32_table[256];
static uint8_t  s_crc32_ready = 0;

static void crc32_table_init(void)
{
    uint32_t i, j, v;

    for (i = 0; i < 256; i++) {
        v = i;
        for (j = 0; j < 8; j++) {
            if (v & 1UL) {
                v = (v >> 1) ^ 0xEDB88320UL;
            } else {
                v >>= 1;
            }
        }
        s_crc32_table[i] = v;
    }
    s_crc32_ready = 1;
}

void upg_crc32_start(uint32_t *state)
{
    if (!s_crc32_ready) {
        crc32_table_init();
    }
    *state = 0xFFFFFFFFUL;
}

void upg_crc32_update(uint32_t *state, const uint8_t *data, uint32_t len)
{
    uint32_t crc = *state;
    uint32_t i;

    for (i = 0; i < len; i++) {
        crc = (crc >> 8) ^ s_crc32_table[(crc ^ data[i]) & 0xFFUL];
    }
    *state = crc;
}

uint32_t upg_crc32_finish(uint32_t state)
{
    return state ^ 0xFFFFFFFFUL;
}

uint32_t upg_crc32(const uint8_t *data, uint32_t len)
{
    uint32_t state;

    upg_crc32_start(&state);
    upg_crc32_update(&state, data, len);
    return upg_crc32_finish(state);
}

/*==============================================================================
 * CRC16/KERMIT - reflected 0x8408, init 0x0000, xorout 0x0000
 * (line-for-line equivalent of crclib.c crc16_ccitt())
 *============================================================================*/
uint16_t upg_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0;
    uint32_t n;
    uint8_t  i;

    for (n = 0; n < len; n++) {
        crc ^= data[n];
        for (i = 0; i < 8; i++) {
            if (crc & 1U) {
                crc = (crc >> 1) ^ 0x8408U;     /* 0x8408 = reverse of 0x1021 */
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
