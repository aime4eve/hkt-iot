/**
 * @file    upgrade_common.h
 * @brief   Shared constants and data contracts for the BLE diff/full upgrade
 *          (protocol source: Docs/蓝牙升级协议.docx, 2026-11).
 *
 *          This header is the single source of truth for the partition table
 *          and the on-flash data structures exchanged between the APP firmware
 *          (ble_upgrade / synthesis) and the BootLoader (transfer-only).
 *          It is compiled into BOTH Keil projects.
 *
 * @note    Flash base assumed 0x08000000 (STM32L431RC, 256KB / 64KB SRAM).
 *          External flash: Puya P25Q40, 512KB, 256B page / 4KB sector.
 */
#ifndef __UPGRADE_COMMON_H__
#define __UPGRADE_COMMON_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * Internal flash layout (absolute addresses)
 *==============================================================================
 * 0x08000000 - 0x08003FFF  (16KB)  BOOT (new boot, transfer-only, no BLE)
 * 0x08004000 - 0x0802FFFF  (176KB) APP image  (protocol says 228KB; capped here
 *                                    because user params at 0x08030000+ must
 *                                    survive upgrades - see UPG_APP_MAX_SIZE)
 * 0x08030000 - 0x0803EFFF  (60KB)  user params (DevEUI/keys/park config) -
 *                                    NEVER written by the upgrade path
 * 0x0803F000 - 0x0803F7FF  (2KB)   VAR page: upgrade flag struct
 * 0x0803F800 - 0x0803FFFF  (2KB)   legacy update-flag page (retired, unused)
 *============================================================================*/
#define UPG_BOOT_BASE           0x08000000UL
#define UPG_BOOT_SIZE           0x4000UL                /* 16KB */

#define UPG_APP_BASE            0x08004000UL
#define UPG_APP_MAX_SIZE        0x2C000UL               /* 176KB, ends at 0x08030000 */
#define UPG_APP_END             (UPG_APP_BASE + UPG_APP_MAX_SIZE)

#define UPG_VAR_PAGE            0x0803F000UL            /* 2KB page */
#define UPG_INT_PAGE_SIZE       2048U                   /* STM32L4 page erase size */

/*==============================================================================
 * External flash layout (P25Q40 512KB) - whole chip owned by the upgrade
 *==============================================================================
 * 0x000000 - 0x03EFFF  (252KB) Patch: received file (diff patch or full image)
 * 0x03F000 - 0x03FFFF  (4KB)   Session sector: rx-session descriptor + staging
 *                              info (see upg_session_t / upg_staging_t)
 * 0x040000 - 0x07FFFF  (256KB) NewFW: synthesis output (diff) / none (full)
 *============================================================================*/
#define UPG_EX_SIZE             0x80000UL
#define UPG_EX_PATCH_BASE       0x000000UL
#define UPG_EX_PATCH_DATA_SIZE  0x03F000UL              /* 252KB */
#define UPG_EX_SESSION_BASE     0x03F000UL
#define UPG_EX_NEWFW_BASE       0x040000UL
#define UPG_EX_NEWFW_SIZE       0x040000UL              /* 256KB */

/*==============================================================================
 * Protocol frame constants (蓝牙升级协议.docx)
 *============================================================================*/
#define UPG_SYNC_B0             0x68U                   /* 'h' */
#define UPG_SYNC_B1             0x6BU                   /* 'k' */
#define UPG_SYNC_B2             0x74U                   /* 't' */

/* special-type bits */
#define UPG_FR_NEED_ACK         0x01U                   /* BIT0 */
#define UPG_FR_CHECKSUM         0x02U                   /* BIT1 */
#define UPG_FR_HISTORY          0x04U                   /* BIT2 */
#define UPG_FR_UPGRADE          0x08U                   /* BIT3 (implies BIT1) */
#define UPG_FR_UPGRADE_FRAME    (UPG_FR_NEED_ACK | UPG_FR_CHECKSUM | UPG_FR_UPGRADE)  /* 0x0B */
#define UPG_FR_DEVICE_REPLY     (UPG_FR_CHECKSUM)       /* 0x02, device FE/F3 frames */

/* data types */
#define UPG_MSG_NOTIFY          0xF0U                   /* APP->device, 11B */
#define UPG_MSG_DATA            0xF1U                   /* APP->device, 6+N */
#define UPG_MSG_REQUEST         0xF2U                   /* device->APP, 2B */
#define UPG_MSG_RESULT          0xF3U                   /* device->APP, 2B */
#define UPG_MSG_ACK             0xFEU                   /* generic ack, 1B */

/* 0xF0 payload sizes */
#define UPG_NOTIFY_LEN          11U                     /* ver2+total2+pktlen2+type1+crc32 */
#define UPG_DATA_HEAD_LEN       6U                      /* 0xF1: seq2+crc16_2+len2 */

/* package type (0xF0 byte 7) */
#define UPG_PKG_FULL            0x00U
#define UPG_PKG_DIFF            0x01U

/* 0xF3 result codes */
#define UPG_RESULT_OK           0x00U
#define UPG_RESULT_CRC_FAIL     0x01U
#define UPG_RESULT_WRITE_FAIL   0x02U
#define UPG_RESULT_TIMEOUT      0x03U

/*==============================================================================
 * Upgrade flags (VAR page magic values)
 *============================================================================*/
#define UPG_FLAG_JUMP           0x4A554D50UL            /* "JUMP" - confirmed, run APP */
#define UPG_FLAG_RECOVER        0x5245434FUL            /* "RECO" - BOOT: install staged FW */
#define UPG_FLAG_CONFIRM_PENDING 0x434F4E50UL           /* "CONP" - transferred, awaiting
                                                         * first successful version report */

/* how many unconfirmed boots before BOOT parks waiting for a reflash */
#define UPG_CONFIRM_MAX_ATTEMPTS 3U

/* version packing: (hw << 8) | sw, matching 0xF0 bytes 1..2 */
#define UPG_MAKE_VERSION(hw, sw) ((((uint16_t)(hw)) << 8) | ((uint16_t)(sw)))

/*==============================================================================
 * VAR-page flag struct (APP writes / BOOT reads-writes, layout must match)
 *============================================================================*/
typedef struct {
    uint32_t upgrade_flag;      /* UPG_FLAG_xxx */
    uint16_t old_version;       /* running fw version before upgrade */
    uint16_t new_version;       /* version being installed / just installed */
    uint32_t new_fw_len;        /* staged new firmware length in bytes */
    uint16_t confirm_attempts;  /* boots under CONFIRM_PENDING without confirm */
    uint16_t crc16;             /* CRC16-KERMIT over the bytes above */
} upg_flag_t;

/*==============================================================================
 * Staging info (written by APP into the session sector after the new firmware
 * is complete and CRC-verified in external flash; read by BOOT on RECO)
 *============================================================================*/
#define UPG_STAGING_MAGIC       0x3157464EUL            /* "NFW1" */

typedef struct {
    uint32_t magic;             /* UPG_STAGING_MAGIC */
    uint8_t  pkg_type;          /* UPG_PKG_FULL / UPG_PKG_DIFF */
    uint8_t  reserved[3];
    uint32_t src_addr;          /* ext-flash base of the verified new image:
                                 * full -> UPG_EX_PATCH_BASE, diff -> UPG_EX_NEWFW_BASE */
    uint32_t new_fw_len;        /* image length (<= UPG_APP_MAX_SIZE) */
    uint32_t new_fw_crc;        /* CRC32-IEEE of the image */
    uint16_t new_version;
    uint16_t old_version;
    uint16_t reserved2;
    uint16_t crc16;             /* CRC16-KERMIT over the bytes above. MUST stay
                                 * the LAST field: every crc site uses the
                                 * "sizeof(struct) - 2" span idiom, which only
                                 * excludes the crc when nothing follows it. */
} upg_staging_t;

/*==============================================================================
 * Receive-session descriptor (persisted for BLE reconnect resume, doc req.6)
 *============================================================================*/
#define UPG_SESSION_MAGIC       0x31534553UL            /* "SES1" */
#define UPG_MAX_PACKETS         1024U                   /* 252KB / 256B min packet 1... */
#define UPG_BITMAP_WORDS        ((UPG_MAX_PACKETS + 31) / 32)   /* 32 words */

typedef struct {
    uint32_t magic;             /* UPG_SESSION_MAGIC */
    uint32_t file_crc;          /* 0xF0 file CRC32 (session identity) */
    uint16_t total_packets;     /* 0xF0 total */
    uint16_t packet_len;        /* 0xF0 per-packet length */
    uint8_t  pkg_type;          /* 0xF0 package type */
    uint8_t  reserved;
    uint16_t new_version;       /* 0xF0 hw/sw pair */
    uint32_t recv_bitmap[UPG_BITMAP_WORDS]; /* bit i = 1 -> packet i stored */
    uint16_t recv_crc16;        /* CRC16-KERMIT over the bytes above - last
                                 * field, see the upg_staging_t.crc16 note */
} upg_session_t;

/* session sector placement (one 4KB sector, staging info after the descriptor) */
#define UPG_SESSION_DESC_ADDR   (UPG_EX_SESSION_BASE)
#define UPG_STAGING_ADDR        (UPG_EX_SESSION_BASE + 0x1000UL - 128UL) /* last 128B of sector */

/*==============================================================================
 * Diff patch header - MUST stay binary-compatible with the PC-side diff
 * generator (same format as Smart_Fire_Hydrant, bsdiff+segmented-LZO).
 * The header itself is not covered by patch_crc (CRC starts right after it).
 *============================================================================*/
#define UPG_PATCH_MAGIC         "DIFF"
#define UPG_PATCH_MAGIC_LEN     4U

#define UPG_PID                 0x0007U                 /* shared PC diff tool stamps 0x07 (same value as the hydrant boot_common.h PID) */
#define UPG_CID                 0xFFFFU                 /* company id (PC tool must match) */

#pragma pack(1)
typedef struct {
    char     magic[UPG_PATCH_MAGIC_LEN];    /* "DIFF" */
    uint32_t patch_size;                    /* patch body size, excl. header */
    uint32_t patch_crc;                     /* CRC32 of patch body */
    uint32_t level;                         /* LZO block size (256..4096) */
    uint16_t pid;
    uint16_t cid;
    uint32_t new_fw_version;                /* packed (hw<<8|sw) in low 16 bits */
    uint32_t new_fw_size;
    uint32_t new_fw_crc;
    uint32_t old_fw_version;
    uint32_t old_fw_size;
    uint32_t old_fw_crc;                    /* base check: CRC32 of running image */
    uint8_t  reserved[4];
} upg_patch_header_t;
#pragma pack()

/*==============================================================================
 * Misc helpers
 *============================================================================*/
#ifndef UPG_MIN
#define UPG_MIN(a, b)           ((a) < (b) ? (a) : (b))
#endif
#ifndef UPG_MAX
#define UPG_MAX(a, b)           ((a) > (b) ? (a) : (b))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UPGRADE_COMMON_H__ */
