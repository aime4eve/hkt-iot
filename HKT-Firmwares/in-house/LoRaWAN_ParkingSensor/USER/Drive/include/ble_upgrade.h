
#ifndef __BLE_UPGRADE_H__
#define __BLE_UPGRADE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

/*
 * BLE upgrade session (protocol source: Docs/蓝牙升级协议.docx).
 * State machine: IDLE --F0(notify)--> PULLING --last packet + CRC ok--> done
 *   PULLING: device-driven stop-and-wait (F2 request -> F1 data), resumable
 *            across a BLE reconnect / power cycle via the ext-flash session
 *            descriptor, retried per-packet up to UPG_PULL_RETRY_MAX.
 *   done:    F3(0x00) sent, then full package -> stage + RECO + reset,
 *            diff package -> synthesize in APP (NewFW area) -> stage + reset.
 */

/* non-zero while an upgrade session owns the radio/flash (gates STOP2 sleep) */
u8 upg_ble_active(void);

/* Early intercept for fromBleDataHandle: consumes "hkt"+BIT3 upgrade frames.
 * Returns 1 when the frame was handled (caller must not run the legacy
 * string-command parser on it), 0 otherwise. */
u8 upg_ble_rx(u8 *data, u16 len);

/* Main-loop tick: F2 retransmit on packet timeout, session abort on retries
 * exhausted. Call from the superloop. */
void upg_ble_poll(void);

/* Confirm hook: call right after a successful version report (BLE query /
 * periodic report). Flips CONFIRM_PENDING -> JUMP on the VAR page. */
void upg_ble_confirm(void);

#ifdef __cplusplus
}
#endif

#endif
