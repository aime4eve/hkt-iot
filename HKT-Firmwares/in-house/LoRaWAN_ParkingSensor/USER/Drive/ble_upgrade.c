/**
 * @file    ble_upgrade.c
 * @brief   BLE upgrade session state machine (APP side) - see ble_upgrade.h.
 *          Protocol source: Docs/蓝牙升级协议.docx (F0/F1/F2/F3/FE frames,
 *          device-driven stop-and-wait pull transfer).
 */
#include <string.h>

#include "stm32l4xx.h"

#include "ble_upgrade.h"
#include "flash.h"
#include "gpio.h"
#include "spi.h"
#include "systick.h"
#include "uart.h"

#include "app_bspatch.h"
#include "upgrade_common.h"
#include "upgrade_crc.h"
#include "upgrade_flash.h"
#include "upgrade_frame.h"

/* Millisecond tick for upgrade_flash timeouts (APP flavor). */
uint32_t upg_tick_ms(void)
{
    return get_syspant_ms();
}

/*==============================================================================
 * Tunables
 *============================================================================*/
/* Pull pacing: an unanswered F2 for the same packet index is re-sent at most
 * once per second (keeps a live-engine ping-pong with the phone from becoming
 * a sub-second F2 storm); 1s also stays well inside the engine's 6s watchdog,
 * which every F2 resets. Advancing pulls (next packet after a stored one) are
 * NOT throttled - they are stop-and-wait bounded by the transfer itself. */
#define UPG_PULL_GAP_MS        1000U  /* unanswered-pull re-send interval */
#define UPG_PULL_RETRY_MAX     10U     /* doc req: 连续 10 次重发无应答退出 */
#define UPG_PKT_MAX            512U   /* fits the 576B LPUART1 RX buffer */
#define UPG_SECTOR_SIZE        4096U  /* ext-flash erase unit (lazy erase) */
#define UPG_CRC_CHUNK          512U

typedef enum {
    UPG_ST_IDLE = 0,
    UPG_ST_PULLING,                   /* F2/F1 loop running */
    UPG_ST_DONE,                      /* F3 sent, finishing (never leaves) */
} upg_state_t;

static upg_state_t   g_state;
static upg_session_t g_sess;          /* RAM copy of the persisted descriptor */
static uint16_t      g_next;          /* next packet index to request */
static uint16_t      g_tx_seq = 1;    /* device frame sequence counter */
static uint32_t      g_file_len;      /* absolute: last stored seq*plen + plen */
static uint32_t      g_last_rx_ms;    /* last valid inbound frame / F2 sent */
static uint32_t      g_last_pull_ms;  /* last F2 actually sent */
static uint8_t       g_pull_pending;  /* same-index re-pull waiting for the gap */
static uint16_t      g_pull_seq = 0xFFFFU; /* packet index of the last F2 sent */
static uint8_t       g_retry;         /* re-sends spent on the current index */
static uint8_t       g_txbuf[UPG_FRAME_HEAD_LEN + 8];

/*==============================================================================
 * Small helpers
 *============================================================================*/
static void upg_send_frame(uint8_t special, uint8_t type, const uint8_t *data, uint8_t data_len)
{
    int n = upg_frame_build(g_txbuf, sizeof(g_txbuf), special, (uint8_t)(g_tx_seq++),
                            type, data, data_len);
    if (n > 0) {
        BLE_SendData(g_txbuf, (u16)n);
        DebugHexInfo("UPGSend", g_txbuf, n);
    }
}

static void upg_send_result(uint8_t code)
{
    uint8_t payload[2] = {code, 0x00};

    upg_send_frame(UPG_FR_DEVICE_REPLY, UPG_MSG_RESULT, payload, 2);
}

/* Actually emit F2(g_next). Only reached with gap clearance for same-index
 * re-sends; a fresh advancing index may send immediately. */
static void upg_do_pull(void)
{
    uint8_t payload[2];

    payload[0] = (uint8_t)(g_next >> 8);
    payload[1] = (uint8_t)g_next;
    g_last_pull_ms = upg_tick_ms();
    g_last_rx_ms = g_last_pull_ms;    /* unanswered-timeout base */
    g_pull_seq = g_next;
    g_pull_pending = 0;
    upg_send_frame(UPG_FR_UPGRADE_FRAME, UPG_MSG_REQUEST, payload, 2);
}

/* Request a pull of g_next. Same-index re-sends (packet CRC fail, duplicate
 * F1, timeout) go through the 1s pacer in upg_ble_poll(); an advancing index
 * sends right away. */
static void upg_request_pull(void)
{
    if (g_next == g_pull_seq) {
        g_pull_pending = 1;           /* paced re-send, poll() fires it */
    } else {
        upg_do_pull();
    }
}

/* end-of-session cleanup: release ext flash, restore the shared SPI3 driver
 * state the sleep path expects, back to IDLE */
static void upg_abort(uint8_t result_code)
{
    if (result_code != 0xFFU) {
        upg_send_result(result_code);
    }
    upg_flash_powerdown();
    Spi3Init();
    g_state = UPG_ST_IDLE;
    g_pull_pending = 0;
}

/*==============================================================================
 * VAR-page flag access (same layout as the BootLoader writer)
 *============================================================================*/
static void upg_read_flag(upg_flag_t *flag)
{
    memcpy(flag, (const void *)UPG_VAR_PAGE, sizeof(*flag));
}

static ErrorStatus upg_write_flag(const upg_flag_t *flag)
{
    upg_flag_t tmp;
    FLASH_EraseInitTypeDef erase = {0};
    u32 page_err = 0;
    ErrorStatus status;

    memcpy(&tmp, flag, sizeof(tmp));
    tmp.crc16 = upg_crc16((const uint8_t *)&tmp, sizeof(tmp) - 2U);

    HAL_FLASH_Unlock();
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = (UPG_VAR_PAGE - 0x08000000UL) / UPG_INT_PAGE_SIZE;
    erase.NbPages = 1;
    if (HAL_FLASHEx_Erase(&erase, &page_err) == HAL_OK) {
        /* 16-byte struct = 2 double-words */
        status = FLASH_Write_NDoubleWord(UPG_VAR_PAGE, (uint64_t *)&tmp, (int)(sizeof(tmp) / 8U));
    } else {
        status = ERROR;
    }
    HAL_FLASH_Lock();
    return status;
}

/*==============================================================================
 * Session persistence (ext-flash session sector, doc req.6 resume)
 *============================================================================*/
/* Persist descriptor (+ optional staging info right after it) with a
 * sector-erase + rewrite. A power cut inside this window only costs the
 * resume point - the next F0 then starts a fresh session. */
static void upg_session_persist(const upg_staging_t *staging)
{
    g_sess.recv_crc16 = upg_crc16((const uint8_t *)&g_sess, sizeof(g_sess) - 2U);

    (void)upg_flash_erase_4k(UPG_SESSION_DESC_ADDR);
    (void)upg_flash_write(UPG_SESSION_DESC_ADDR, (const uint8_t *)&g_sess, sizeof(g_sess));
    if (staging != NULL) {
        (void)upg_flash_write(UPG_STAGING_ADDR, (const uint8_t *)staging, sizeof(*staging));
    }
}

/* Next missing packet = end of the leading run of stored packets (requests
 * are strictly sequential, so the bitmap is always a prefix). */
static uint16_t upg_bitmap_prefix(const upg_session_t *s)
{
    uint16_t i;

    for (i = 0; i < s->total_packets; i++) {
        if (!(s->recv_bitmap[i / 32U] & (1UL << (i % 32U)))) {
            return i;
        }
    }
    return s->total_packets;
}

/*==============================================================================
 * Finalize: whole-file CRC, then stage + reset (synthesis for diff packages)
 *============================================================================*/
static uint32_t upg_patch_crc32(uint32_t len)
{
    static uint8_t buf[UPG_CRC_CHUNK];
    uint32_t state;
    uint32_t off = 0;
    uint32_t chunk;

    upg_crc32_start(&state);
    while (off < len) {
        chunk = UPG_MIN(len - off, sizeof(buf));
        if (upg_flash_read(UPG_EX_PATCH_BASE + off, buf, chunk) != 0) {
            return 0xFFFFFFFFUL;
        }
        upg_crc32_update(&state, buf, chunk);
        off += chunk;
        LL_IWDG_ReloadCounter(IWDG);
    }
    return upg_crc32_finish(state);
}

/* Write the RECO flag and reset into the BootLoader installer. */
static void upg_enter_recover(const upg_staging_t *st, upg_flag_t *flag)
{
    upg_session_persist(st);

    flag->upgrade_flag = UPG_FLAG_RECOVER;
    flag->old_version = st->old_version;
    flag->new_version = st->new_version;
    flag->new_fw_len = st->new_fw_len;
    flag->confirm_attempts = 0;
    if (upg_write_flag(flag) != SUCCESS) {
        DEBUG_TRACE(ERROR_TAG, "UPG: RECO flag write fail");
        upg_abort(UPG_RESULT_WRITE_FAIL);   /* report F3=2, stay on old firmware */
        return;
    }
    upg_send_result(UPG_RESULT_OK);   /* success only once staging + flag are durable */
    INFO("\n[UPG] transfer done, resetting into BOOT\n");
    mDelay(100);                /* let the F3 frame drain through BLE */
    NVIC_SystemReset();
}

static void upg_finalize(void)
{
    upg_staging_t st;
    upg_flag_t flag;
    uint32_t crc;

    crc = upg_patch_crc32(g_file_len);
    if (crc != g_sess.file_crc) {
        DEBUG_TRACE(ERROR_TAG, "UPG: file CRC fail 0x%08lX != 0x%08lX", crc, g_sess.file_crc);
        upg_abort(UPG_RESULT_CRC_FAIL);
        return;
    }

    g_state = UPG_ST_DONE;
    INFO("\n[UPG] file OK len=%lu type=%s ver=0x%04X\n",
         g_file_len, (g_sess.pkg_type == UPG_PKG_DIFF) ? "diff" : "full", g_sess.new_version);

    upg_read_flag(&flag);       /* keeps old_version sane on mismatch */

    /* F3 timing: the result frame is sent only after synthesis + staging +
     * flag all succeed, right before the reset (see upg_enter_recover).
     * Sending OK before synthesis stranded the app in the success flow when
     * synthesis later failed (e.g. wrong base package); every failure below
     * now reports F3=1/2 so the app can surface it. The app allows 30s for
     * this (pullTimeoutMs), synthesis takes ~2-5s. */
    if (g_sess.pkg_type == UPG_PKG_DIFF) {
        /* Synthesis happens HERE, in the running APP (doc req.3), never in
         * BOOT. On failure report F3=1 and go back to IDLE on the old fw. */
        if (upg_bspatch_apply(UPG_MAKE_VERSION(HARDWARE_VER, SOFTWARE_VER),
                              g_sess.new_version, g_file_len, &st) != 0) {
            DEBUG_TRACE(ERROR_TAG, "UPG: bspatch apply fail");
            upg_abort(UPG_RESULT_CRC_FAIL);
            return;
        }
    } else {
        st.magic = UPG_STAGING_MAGIC;
        st.pkg_type = UPG_PKG_FULL;
        st.reserved[0] = st.reserved[1] = st.reserved[2] = 0;
        st.src_addr = UPG_EX_PATCH_BASE;    /* full image already verified in place */
        st.new_fw_len = g_file_len;
        st.new_fw_crc = g_sess.file_crc;
        st.new_version = g_sess.new_version;
        st.old_version = flag.old_version;
        st.crc16 = 0;
        st.reserved2 = 0;
        st.crc16 = upg_crc16((const uint8_t *)&st, sizeof(st) - 2U);
    }
    if (st.new_fw_len > UPG_APP_MAX_SIZE) {
        DEBUG_TRACE(ERROR_TAG, "UPG: image too big %lu", st.new_fw_len);
        upg_abort(UPG_RESULT_CRC_FAIL);
        return;
    }
    upg_enter_recover(&st, &flag);
}

/*==============================================================================
 * Frame handlers
 *============================================================================*/
static void upg_handle_notify(const uint8_t *data, uint16_t data_len)
{
    uint16_t hw, sw, total, pkt_len;
    uint8_t pkg_type;
    uint32_t file_crc;
    upg_session_t saved;

    if (data_len != UPG_NOTIFY_LEN) {
        return;
    }
    hw = data[0];
    sw = data[1];
    total = (uint16_t)((data[2] << 8) | data[3]);
    pkt_len = (uint16_t)((data[4] << 8) | data[5]);
    pkg_type = data[6];
    file_crc = ((uint32_t)data[7] << 24) | ((uint32_t)data[8] << 16) |
               ((uint32_t)data[9] << 8) | (uint32_t)data[10];

    if (hw != HARDWARE_VER) {
        DEBUG_TRACE(ERROR_TAG, "UPG: hw ver mismatch 0x%02X", hw);
        return;                 /* no FE -> phone engine fails on its own timeout */
    }
    if ((total == 0U) || (total > UPG_MAX_PACKETS) ||
        (pkt_len == 0U) || (pkt_len > UPG_PKT_MAX) ||
        (pkg_type != UPG_PKG_FULL && pkg_type != UPG_PKG_DIFF) ||
        ((uint32_t)total * pkt_len > UPG_EX_PATCH_DATA_SIZE)) {
        DEBUG_TRACE(ERROR_TAG, "UPG: bad notify total=%d plen=%d", total, pkt_len);
        return;
    }
    if (g_state == UPG_ST_PULLING) {
        /* re-notify of the same file after a BLE drop: resume where we left */
        if (file_crc != g_sess.file_crc) {
            upg_abort(0xFFU);   /* different file: restart cleanly below */
        }
    }
    if (g_state == UPG_ST_IDLE) {
        if (upg_flash_init() != 0) {
            DEBUG_TRACE(ERROR_TAG, "UPG: no ext flash");
            return;
        }
        memset(&g_sess, 0, sizeof(g_sess));
        g_sess.magic = UPG_SESSION_MAGIC;
        g_sess.file_crc = file_crc;
        g_sess.total_packets = total;
        g_sess.packet_len = pkt_len;
        g_sess.pkg_type = pkg_type;
        g_sess.new_version = UPG_MAKE_VERSION(hw, sw);

        /* resume an interrupted session of this exact file, if any */
        memset(&saved, 0, sizeof(saved));
        (void)upg_flash_read(UPG_SESSION_DESC_ADDR, (uint8_t *)&saved, sizeof(saved));
        if ((saved.magic == UPG_SESSION_MAGIC) &&
            (saved.file_crc == file_crc) &&
            (saved.total_packets == total) &&
            (saved.packet_len == pkt_len) &&
            (upg_crc16((const uint8_t *)&saved, sizeof(saved) - 2U) == saved.recv_crc16)) {
            memcpy(g_sess.recv_bitmap, saved.recv_bitmap, sizeof(g_sess.recv_bitmap));
            g_next = upg_bitmap_prefix(&saved);
            g_file_len = (uint32_t)g_next * pkt_len;
            DEBUG_TRACE(LOG_TAG, "UPG: resume at packet %d/%d", g_next, total);
        } else {
            g_next = 0;
            g_file_len = 0;
            upg_session_persist(NULL);  /* fresh descriptor + drop stale staging */
        }
    }

    uint8_t ack_type = UPG_MSG_NOTIFY;

    /* FE(F0) acknowledges the notify; the first F2 moves the phone engine on.
     * Resume edge case: everything was stored before the power cut but the
     * finalize never ran - go straight to the whole-file CRC. */
    if (g_next >= g_sess.total_packets) {
        /* recovered length is nominal (next*plen); a short trailing packet
         * costs a CRC mismatch -> clean retry, power cut already happened */
        g_file_len = (uint32_t)g_next * g_sess.packet_len;
        upg_finalize();
        return;
    }
    upg_send_frame(UPG_FR_DEVICE_REPLY, UPG_MSG_ACK, &ack_type, 1);
    g_state = UPG_ST_PULLING;
    g_retry = 0;
    g_pull_seq = 0xFFFFU;            /* fresh session: first pull is immediate */
    g_pull_pending = 0;
    upg_request_pull();
}

static void upg_handle_data(const uint8_t *data, uint16_t data_len)
{
    uint16_t seq, pcrc, plen;
    uint32_t offset;

    if (g_state != UPG_ST_PULLING) {
        return;
    }
    if (data_len < UPG_DATA_HEAD_LEN + 1U) {
        return;
    }
    seq = (uint16_t)((data[0] << 8) | data[1]);
    pcrc = (uint16_t)((data[2] << 8) | data[3]);
    plen = (uint16_t)((data[4] << 8) | data[5]);
    if ((plen == 0U) || (plen > UPG_PKT_MAX) || (plen != data_len - UPG_DATA_HEAD_LEN)) {
        return;
    }

    if (seq == (uint16_t)(g_next - 1U)) {
        /* our previous F2 was lost - the phone resent the last packet;
         * re-pull goes through the pacer so this cannot ping-pong fast */
        upg_request_pull();
        return;
    }
    if (seq != g_next) {
        return;                 /* out of order: ignore, timeout will re-pull */
    }
    /* only the last packet may be shorter than the nominal packet_len */
    if ((seq != g_sess.total_packets - 1U) && (plen != g_sess.packet_len)) {
        return;
    }

    g_last_rx_ms = upg_tick_ms();
    if (upg_crc16(data + UPG_DATA_HEAD_LEN, plen) != pcrc) {
        DEBUG_TRACE(ERROR_TAG, "UPG: pkt %d crc fail", seq);
        /* paced re-request; the retry budget is spent in upg_ble_poll() when
         * the re-send actually goes out */
        upg_request_pull();
        return;
    }

    /* lazy sector erase: sequential writes start a fresh sector exactly on
     * its boundary; a resumed mid-sector packet lands in an already-erased
     * sector (its earlier packets are present in the bitmap). */
    offset = (uint32_t)seq * g_sess.packet_len;
    if ((offset % UPG_SECTOR_SIZE) == 0U) {
        if (upg_flash_erase_4k(UPG_EX_PATCH_BASE + offset) != 0) {
            DEBUG_TRACE(ERROR_TAG, "UPG: erase fail @%lu", offset);
            upg_abort(UPG_RESULT_WRITE_FAIL);
            return;
        }
    }
    if (upg_flash_write(UPG_EX_PATCH_BASE + offset, data + UPG_DATA_HEAD_LEN, plen) != 0) {
        DEBUG_TRACE(ERROR_TAG, "UPG: write fail @%lu", offset);
        upg_abort(UPG_RESULT_WRITE_FAIL);
        return;
    }

    g_sess.recv_bitmap[seq / 32U] |= 1UL << (seq % 32U);
    g_file_len = offset + plen;
    g_retry = 0;

    if (seq == g_sess.total_packets - 1U) {
        upg_finalize();
        return;
    }
    /* checkpoint the bitmap every sector's worth of packets (wear-bounded) */
    if ((seq % 16U) == 15U) {
        upg_session_persist(NULL);
    }
    g_next++;
    upg_request_pull();
}

/*==============================================================================
 * Public API
 *============================================================================*/
u8 upg_ble_rx(u8 *data, u16 len)
{
    upg_frame_t frame;

    if ((len < UPG_FRAME_HEAD_LEN + 2U) ||
        (data[0] != UPG_SYNC_B0) || (data[1] != UPG_SYNC_B1) || (data[2] != UPG_SYNC_B2) ||
        ((data[3] & UPG_FR_UPGRADE) == 0U)) {
        return 0;               /* not an upgrade frame - legacy parser takes it */
    }
    if (upg_frame_parse(data, len, &frame) != 0) {
        return 1;               /* upgrade frame with bad checksum: swallow it */
    }
    DebugHexInfo("UPGRecv", data, len);

    if (frame.type == UPG_MSG_NOTIFY) {
        upg_handle_notify(frame.data, frame.data_len);
    } else if (frame.type == UPG_MSG_DATA) {
        upg_handle_data(frame.data, frame.data_len);
    }
    /* device->device frame types (F2/F3/FE) are never accepted inbound */
    return 1;
}

void upg_ble_poll(void)
{
    uint32_t now;

    if (g_state != UPG_ST_PULLING) {
        return;
    }
    now = upg_tick_ms();

    if (g_pull_pending) {
        /* paced same-index re-send: at most one F2 per UPG_PULL_GAP_MS */
        if ((now - g_last_pull_ms) >= UPG_PULL_GAP_MS) {
            if (g_retry >= UPG_PULL_RETRY_MAX) {
                DEBUG_TRACE(ERROR_TAG, "UPG: pull no answer x%d, abort", g_retry);
                upg_abort(UPG_RESULT_TIMEOUT);
                return;
            }
            g_retry++;
            upg_do_pull();
        }
        return;
    }

    /* unanswered watchdog: no valid F1 since our last F2 */
    if ((now - g_last_rx_ms) >= UPG_PULL_GAP_MS) {
        g_pull_pending = 1;         /* poll paces the actual re-send */
    }
}

u8 upg_ble_active(void)
{
    return (g_state != UPG_ST_IDLE) ? 1U : 0U;
}

void upg_ble_confirm(void)
{
    upg_flag_t flag;

    upg_read_flag(&flag);
    if ((flag.upgrade_flag != UPG_FLAG_CONFIRM_PENDING) ||
        (upg_crc16((const uint8_t *)&flag, sizeof(flag) - 2U) != flag.crc16)) {
        return;
    }
    /* the new firmware just reported its version - mark it confirmed */
    flag.upgrade_flag = UPG_FLAG_JUMP;
    flag.confirm_attempts = 0;
    if (upg_write_flag(&flag) == SUCCESS) {
        DEBUG_TRACE(LOG_TAG, "UPG: new fw 0x%04X confirmed", flag.new_version);
    }
}
