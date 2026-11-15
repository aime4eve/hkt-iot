/**
 * @file    app_bspatch.h
 * @brief   Diff synthesis: bsdiff + segmented-LZO patch application, running
 *          inside the APP firmware (protocol doc req.3: 合成移出 BOOT).
 *
 *          Reader half ported from Smart_Fire_Hydrant_BootLoad/boot
 *          (bspatch.c + boot_lzo.c); the patch file format (patch_header_t +
 *          lzo_format_t segments) stays binary-compatible with the PC-side
 *          diff generator of that project.
 *
 *          Patch input : ext flash Patch area (received over BLE by
 *                        ble_upgrade.c), header at UPG_EX_PATCH_BASE.
 *          Old input   : running image, memory-mapped at UPG_APP_BASE.
 *          New output  : ext flash NewFW area, erased then written with a
 *                        4KB sector buffer (read/add semantics preserved).
 */
#ifndef __APP_BSPATCH_H__
#define __APP_BSPATCH_H__

#include <stdint.h>

#include "upgrade_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Validate the patch header (magic/pid/cid/sizes/level/versions/patch CRC/
 * old-image CRC), then synthesize the new image into the NewFW area and
 * verify its CRC32.
 *
 * @param running_version    packed (hw<<8|sw) of the firmware now executing
 * @param expect_new_version packed version from the 0xF0 notify (session truth)
 * @param received_len      total bytes received into the Patch area (header
 *                           included) - must equal header+patch_size
 * @param st                filled on success (src=NewFW base, len, crc, ...)
 * @return 0 on success, -1 on any failure (old firmware left fully intact)
 */
int upg_bspatch_apply(uint16_t running_version, uint16_t expect_new_version,
                      uint32_t received_len, upg_staging_t *st);

#ifdef __cplusplus
}
#endif

#endif /* __APP_BSPATCH_H__ */
