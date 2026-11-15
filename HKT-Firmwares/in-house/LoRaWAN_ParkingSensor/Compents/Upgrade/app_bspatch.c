/**
 * @file    app_bspatch.c
 * @brief   bsdiff + segmented-LZO patch application - see app_bspatch.h.
 *
 *          Ported from Smart_Fire_Hydrant_BootLoad/boot/{bsdiff/bspatch.c,
 *          boot_lzo.c} (reader half). Flash I/O goes through upgrade_flash;
 *          the hydrant boot_mem region abstraction collapses to two fixed
 *          areas: Patch (input, ext flash) and NewFW (output, ext flash).
 *          The old firmware is read memory-mapped from internal flash.
 *
 *          Original bspatch copyright: Colin Percival 2003-2005,
 *          Matthew Endsley 2012.
 */
#include <limits.h>
#include <string.h>

#include "stm32l4xx.h"

#include "uart.h"

#include "app_bspatch.h"
#include "minilzo/minilzo.h"
#include "upgrade_crc.h"
#include "upgrade_flash.h"

/* Patch body starts right after the 48-byte header. */
#define UPG_PATCH_HDR_SIZE      ((uint32_t)sizeof(upg_patch_header_t))

/* LZO segment geometry (same limits as the hydrant project / PC tool) */
#define UPG_LZO_LEVEL_MIN       256U
#define UPG_LZO_LEVEL_MAX       4096U
#define UPG_LZO_SEG_MAX         (UPG_LZO_LEVEL_MAX + (UPG_LZO_LEVEL_MAX / 16U) + 64U + 3U)

#define UPG_BS_SECTOR           4096U   /* ext-flash erase unit */
#define UPG_BS_CHUNK            512U    /* CRC read-back chunk */

#define UPG_LZO_OK              0
#define UPG_LZO_ERROR           (-1)

/* segment compression types (lzo_format_t.lzo_compress) */
#define LZO_COMPRESS_NONE       0x00U
#define LZO_COMPRESS_MINI_LZO   0x01U

#pragma pack(1)
typedef struct {
    uint32_t len;               /* segment data length */
    uint8_t  lzo_compress;      /* 0 = raw, 1 = miniLZO */
    uint8_t  cs;                /* byte-sum checksum of the uncompressed data */
} lzo_format_t;
#pragma pack()

typedef enum {
    lzo_read_type_ctrl = 0,     /* control data triple */
    lzo_read_type_diff,         /* diff string */
    lzo_read_type_old,          /* add old data to diff string */
    lzo_read_type_extra,        /* extra string */
} lzo_read_type_e;

typedef struct {
    uint32_t            patch_off;      /* current offset into the patch body */
    upg_patch_header_t *p_header;       /* caller's validated header copy */
    uint8_t            *p_work;         /* decompression work buffer */
    uint32_t            work_len;       /* valid bytes in work buffer */
    uint32_t            work_pt;        /* read position in work buffer */
    uint8_t            *read_buff;      /* bspatch read output */
    uint32_t            read_len;
    lzo_read_type_e     read_type;
} lzo_read_t;

/*==============================================================================
 * Staging-area erase window (perf: skip the sector read-back on forward writes)
 *
 * upg_bspatch_apply() erases [base, base+size) before bspatch() runs, and the
 * diff/extra strings only ever move forward. Bytes at or above the write
 * frontier (prog_hi) inside that window still read 0xFF and can be programmed
 * directly - no read-modify-write. Ported verbatim from boot_lzo.c.
 *============================================================================*/
static uint32_t bs_erased_end;
static uint32_t bs_prog_hi;

static void flash_write_set_erased_region(uint32_t base, uint32_t size)
{
    bs_erased_end = base + size;
    bs_prog_hi = base;
}

/*==============================================================================
 * CRC32 helpers over the two ext-flash areas
 *============================================================================*/
static uint32_t upg_ext_crc32(uint32_t addr, uint32_t len)
{
    static uint8_t buf[UPG_BS_CHUNK];
    uint32_t state;
    uint32_t chunk;

    upg_crc32_start(&state);
    while (len != 0U) {
        chunk = UPG_MIN(len, sizeof(buf));
        if (upg_flash_read(addr, buf, chunk) != 0) {
            return 0xFFFFFFFFUL;
        }
        upg_crc32_update(&state, buf, chunk);
        addr += chunk;
        len -= chunk;
        LL_IWDG_ReloadCounter(IWDG);
    }
    return upg_crc32_finish(state);
}

/* CRC32 over a memory-mapped range (the running old image) */
static uint32_t upg_mem_crc32(const uint8_t *p, uint32_t len)
{
    uint32_t state;
    uint32_t chunk;

    upg_crc32_start(&state);
    while (len != 0U) {
        chunk = UPG_MIN(len, UPG_BS_CHUNK);
        upg_crc32_update(&state, p, chunk);
        p += chunk;
        len -= chunk;
        LL_IWDG_ReloadCounter(IWDG);
    }
    return upg_crc32_finish(state);
}

/*==============================================================================
 * flash_write - buffered NewFW writer with add semantics (from boot_lzo.c)
 *
 * Modes:
 *   over - copy bytes out (diff/extra strings)
 *   add  - add old-image bytes onto what the diff pass left in the sector
 *          buffer (or in flash, after a flush: read back, add, re-program)
 *============================================================================*/
typedef enum {
    flash_write_type_over = 0,
    flash_write_type_add,
} flash_write_type_e;

/* Sector buffer: uninitialized bytes MUST read 0x00 (additive identity), not
 * 0xFF - see the long comment in boot_lzo.c; 0xFF + old == old - 1 (mod 256)
 * silently corrupts every byte the add pass touches without a matching
 * read-back. */
static uint8_t  bs_sector_buf[UPG_BS_SECTOR] = {0};
static uint8_t  bs_overflow;

/* Program [addr, addr+len) assumed-erased (inside the pre-erased window and
 * above the write frontier). Out-of-window orders are a corrupt/unordered
 * patch stream: reject rather than attempt a sector read-modify-write - the
 * old firmware is untouched either way, which is the safe failure. */
static int bs_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if ((bs_erased_end == 0U) || (addr < bs_prog_hi) ||
        ((addr + len) > bs_erased_end)) {
        DEBUG_TRACE(ERROR_TAG, "bs: write out of erased window @%lu", addr);
        return -1;
    }
    if (upg_flash_write(addr, data, len) != 0) {
        return -1;
    }
    if ((addr + len) > bs_prog_hi) {
        bs_prog_hi = addr + len;
    }
    return 0;
}

static int flash_write(flash_write_type_e write_type, uint32_t addr,
                       const uint8_t *p_write_buff, uint32_t len, lzo_read_t *p_lzo_read)
{
    uint32_t pt = 0;
    uint32_t page_addr;
    uint32_t offset;
    uint32_t write_len;
    uint32_t block_write_len;
    uint32_t file_length;
    uint32_t buff_length;
    uint32_t i;

    if ((write_type > flash_write_type_add) || (p_write_buff == NULL) || (len == 0U)) {
        return -1;
    }
    /* the writer only ever targets the NewFW area (req.8/9 address gate) */
    if ((addr < UPG_EX_NEWFW_BASE) || (addr + len > UPG_EX_NEWFW_BASE + UPG_EX_NEWFW_SIZE)) {
        return -1;
    }

    while (pt < len) {
        page_addr = addr & ~(uint32_t)(UPG_BS_SECTOR - 1U);
        offset = addr - page_addr;
        write_len = UPG_MIN(UPG_BS_SECTOR - offset, (len - pt));

        if (write_type == flash_write_type_over) {
            if (bs_overflow != 0U) {
                if (bs_program(page_addr + offset, p_write_buff + pt, write_len) != 0) {
                    return -1;
                }
                LL_IWDG_ReloadCounter(IWDG);
            } else {
                memcpy(bs_sector_buf + offset, p_write_buff + pt, write_len);
            }
        } else {    /* flash_write_type_add */
            if ((bs_overflow != 0U) && (pt != 0U)) {
                /* add onto bytes already programmed: read them back into the
                 * sector buffer first (aligned: offset is 0 on this path) */
                if (upg_flash_read(page_addr + offset, bs_sector_buf + offset,
                                   write_len) != 0) {
                    return -1;
                }
            }
            for (i = 0; i < write_len; i++) {
                bs_sector_buf[i + offset] += p_write_buff[pt + i];
            }
        }

        /* limit the last (partial) sector's flush to the real file end */
        file_length = p_lzo_read->p_header->new_fw_size + UPG_EX_NEWFW_BASE - page_addr;
        buff_length = write_len + offset;
        block_write_len = UPG_MIN(UPG_BS_SECTOR, file_length);

        if (buff_length == block_write_len) {
            if ((p_lzo_read->read_type == lzo_read_type_old) ||
                (p_lzo_read->read_type == lzo_read_type_extra)) {
                if (bs_program(page_addr, bs_sector_buf, block_write_len) != 0) {
                    return -1;
                }
                /* clear to 0x00: additive identity for the next add pass */
                memset(bs_sector_buf, 0x00, sizeof(bs_sector_buf));
                LL_IWDG_ReloadCounter(IWDG);
            } else {
                /* buffer full on diff data: switch to write-through. The add
                 * pass will fold the old bytes in when it reaches them. */
                bs_overflow = 1U;
            }
        }

        pt += write_len;
        addr += write_len;
    }

    if (p_lzo_read->read_type == lzo_read_type_old) {
        bs_overflow = 0U;
    }
    return 0;
}

/*==============================================================================
 * LZO segment reader (from boot_lzo.c, offset model)
 *============================================================================*/
static uint8_t cal_check_sum(const uint8_t *input, uint32_t len)
{
    uint8_t cs = 0;

    while (len-- != 0U) {
        cs = (uint8_t)(cs + *input);
        input++;
    }
    return cs;
}

static uint8_t seg_buf[UPG_LZO_SEG_MAX];    /* compressed segment staging */

static int lzo_unpack(uint32_t body_off, uint32_t *p_in_len,
                      uint8_t *p_out, uint32_t *p_out_len, uint32_t level)
{
    lzo_format_t fmt;
    uint32_t addr = UPG_EX_PATCH_BASE + UPG_PATCH_HDR_SIZE + body_off;

    if ((level < UPG_LZO_LEVEL_MIN) || (level > UPG_LZO_LEVEL_MAX)) {
        return UPG_LZO_ERROR;
    }
    *p_in_len = 0;

    if (upg_flash_read(addr, (uint8_t *)&fmt, sizeof(fmt)) != 0) {
        return UPG_LZO_ERROR;
    }
    /* fmt.len comes straight from the patch data: bound it against the
     * staging buffer before anything reads into it */
    if ((fmt.len > sizeof(seg_buf)) || (fmt.len == 0U)) {
        DEBUG_TRACE(ERROR_TAG, "bs: segment len bad %lu", fmt.len);
        return UPG_LZO_ERROR;
    }
    if (upg_flash_read(addr + sizeof(fmt), seg_buf, fmt.len) != 0) {
        return UPG_LZO_ERROR;
    }

    if (fmt.lzo_compress == LZO_COMPRESS_NONE) {
        if (fmt.len > level) {   /* work buffer is only level bytes */
            return UPG_LZO_ERROR;
        }
        memcpy(p_out, seg_buf, fmt.len);
        *p_out_len = fmt.len;
    } else if (fmt.lzo_compress == LZO_COMPRESS_MINI_LZO) {
        lzo_uint out_len = level;

        if (lzo1x_decompress_safe(seg_buf, fmt.len, p_out, &out_len, NULL) != LZO_E_OK) {
            DEBUG_TRACE(ERROR_TAG, "bs: decompress fail");
            return UPG_LZO_ERROR;
        }
        *p_out_len = (uint32_t)out_len;
    } else {
        return UPG_LZO_ERROR;
    }

    LL_IWDG_ReloadCounter(IWDG);

    if (fmt.cs != cal_check_sum(p_out, *p_out_len)) {
        DEBUG_TRACE(ERROR_TAG, "bs: segment cs fail");
        return UPG_LZO_ERROR;
    }

    *p_in_len = sizeof(fmt) + fmt.len;
    return UPG_LZO_OK;
}

static int app_lzo_read_patch(lzo_read_t *p_lzo_read)
{
    uint32_t pt = 0;
    uint32_t in_offset;

    while (pt < p_lzo_read->read_len) {
        if ((p_lzo_read->work_pt > p_lzo_read->work_len) ||
            (p_lzo_read->work_len > p_lzo_read->p_header->level)) {
            return UPG_LZO_ERROR;
        }
        if (p_lzo_read->work_pt == p_lzo_read->work_len) {
            if (lzo_unpack(p_lzo_read->patch_off, &in_offset,
                           p_lzo_read->p_work, &p_lzo_read->work_len,
                           p_lzo_read->p_header->level) != UPG_LZO_OK) {
                return UPG_LZO_ERROR;
            }
            p_lzo_read->work_pt = 0;
            p_lzo_read->patch_off += in_offset;
        }

        {
            uint32_t write_len = UPG_MIN(p_lzo_read->read_len - pt,
                                         p_lzo_read->work_len - p_lzo_read->work_pt);

            if (p_lzo_read->read_type != lzo_read_type_ctrl) {
                if (flash_write(flash_write_type_over,
                                (uint32_t)(size_t)(p_lzo_read->read_buff + pt),
                                p_lzo_read->p_work + p_lzo_read->work_pt,
                                write_len, p_lzo_read) != 0) {
                    DEBUG_TRACE(ERROR_TAG, "bs: flash_write fail");
                    return UPG_LZO_ERROR;
                }
                LL_IWDG_ReloadCounter(IWDG);
            } else {
                memcpy(p_lzo_read->read_buff + pt,
                       p_lzo_read->p_work + p_lzo_read->work_pt, write_len);
            }
            p_lzo_read->work_pt += write_len;
            pt += write_len;
        }
    }
    return UPG_LZO_OK;
}

/*==============================================================================
 * bspatch core (from bsdiff/bspatch.c)
 *============================================================================*/
struct bspatch_stream {
    void *opaque;
    int (*read)(const struct bspatch_stream *stream, void *buffer, int length);
};

static int64_t offtin(uint8_t *buf)
{
    int64_t y;

    y = buf[7] & 0x7F;
    y = y * 256; y += buf[6];
    y = y * 256; y += buf[5];
    y = y * 256; y += buf[4];
    y = y * 256; y += buf[3];
    y = y * 256; y += buf[2];
    y = y * 256; y += buf[1];
    y = y * 256; y += buf[0];

    if (buf[7] & 0x80) {
        y = -y;
    }
    return y;
}

static int stream_read(const struct bspatch_stream *stream, void *buffer, int length)
{
    lzo_read_t *p = (lzo_read_t *)stream->opaque;

    p->read_buff = (uint8_t *)buffer;
    p->read_len = (uint32_t)length;
    return (app_lzo_read_patch(p) != UPG_LZO_OK) ? -1 : 0;
}

static int bspatch(const uint8_t *old, int64_t oldsize,
                   uint8_t *new_data, int64_t newsize,
                   struct bspatch_stream *stream)
{
    lzo_read_t *p_lzo_read = (lzo_read_t *)stream->opaque;
    uint8_t buf[8];
    int64_t oldpos, newpos;
    int64_t ctrl[3];
    int64_t i;

    oldpos = 0;
    newpos = 0;
    while (newpos < newsize) {
        p_lzo_read->read_type = lzo_read_type_ctrl;
        for (i = 0; i <= 2; i++) {
            if (stream->read(stream, buf, 8) != 0) {
                DEBUG_TRACE(ERROR_TAG, "bs: ctrl read fail @%ld", (long)newpos);
                return -1;
            }
            ctrl[i] = offtin(buf);
        }

        if (ctrl[0] < 0 || ctrl[0] > INT_MAX ||
            ctrl[1] < 0 || ctrl[1] > INT_MAX ||
            newpos + ctrl[0] > newsize) {
            DEBUG_TRACE(ERROR_TAG, "bs: bad ctrl @%ld", (long)newpos);
            return -1;
        }

        /* A triple may advance newpos by zero bytes (bsdiff emits (0,0,c2) to
         * reposition oldpos); it cannot spin: every iteration consumes 24
         * bytes of patch stream, so a stuck stream fails on the next ctrl
         * read instead of looping. */

        /* Common case: the diff run lies inside the old file. Read+add one
         * sector at a time so each sector holds diff+old bytes before its
         * single program pass (avoids a read-back/erase/re-program per
         * sector - see boot_lzo.c for the full story). */
        if ((ctrl[0] > 0) && (oldpos >= 0) && ((oldpos + ctrl[0]) <= oldsize)) {
            int64_t off = 0;
            int64_t slice;

            while (off < ctrl[0]) {
                slice = (int64_t)UPG_BS_SECTOR -
                        ((newpos + off) % (int64_t)UPG_BS_SECTOR);
                if (slice > ctrl[0] - off) {
                    slice = ctrl[0] - off;
                }

                p_lzo_read->read_type = lzo_read_type_diff;
                if (stream->read(stream, new_data + newpos + off, (int)slice) != 0) {
                    return -1;
                }

                p_lzo_read->read_type = lzo_read_type_old;
                if (flash_write(flash_write_type_add,
                                (uint32_t)(size_t)(new_data + newpos + off),
                                old + oldpos + off, (uint32_t)slice,
                                p_lzo_read) != 0) {
                    DEBUG_TRACE(ERROR_TAG, "bs: flash_write add fail");
                    return -1;
                }
                off += slice;
            }
        } else {
            /* diff string */
            p_lzo_read->read_type = lzo_read_type_diff;
            if (stream->read(stream, new_data + newpos, (int)ctrl[0]) != 0) {
                return -1;
            }

            /* add old data where the run stays inside the old file */
            p_lzo_read->read_type = lzo_read_type_old;
            for (i = 0; i < ctrl[0]; i++) {
                uint32_t write_len;

                if (oldpos + i < 0) {
                    continue;
                }
                if (oldpos + i >= oldsize) {
                    break;
                }
                write_len = (uint32_t)UPG_MIN(ctrl[0] - i, oldsize - (oldpos + i));
                if (flash_write(flash_write_type_add,
                                (uint32_t)(size_t)(new_data + newpos + i),
                                old + oldpos + i, write_len,
                                p_lzo_read) != 0) {
                    DEBUG_TRACE(ERROR_TAG, "bs: flash_write add fail");
                    return -1;
                }
                break;
            }
        }

        newpos += ctrl[0];
        oldpos += ctrl[0];

        if (newpos + ctrl[1] > newsize) {
            DEBUG_TRACE(ERROR_TAG, "bs: extra overrun @%ld", (long)newpos);
            return -1;
        }

        /* extra string */
        p_lzo_read->read_type = lzo_read_type_extra;
        if (stream->read(stream, new_data + newpos, (int)ctrl[1]) != 0) {
            return -1;
        }

        newpos += ctrl[1];
        oldpos += ctrl[2];

        LL_IWDG_ReloadCounter(IWDG);
    }
    return 0;
}

/*==============================================================================
 * Header validation + orchestrator
 *============================================================================*/
static int patch_header_check(upg_patch_header_t *h, uint32_t patch_body_size,
                              uint16_t running_version, uint16_t expect_new_version)
{
    uint32_t crc;

    if (memcmp(h->magic, UPG_PATCH_MAGIC, UPG_PATCH_MAGIC_LEN) != 0) {
        DEBUG_TRACE(ERROR_TAG, "bs: magic error");
        return -1;
    }
    /* size bounds: each field is used as a LENGTH (erase/write/read ranges) */
    if ((h->new_fw_size == 0U) || (h->new_fw_size > UPG_APP_MAX_SIZE) ||
        (h->new_fw_size > UPG_EX_NEWFW_SIZE) ||
        (h->patch_size == 0U) ||
        (h->patch_size > (UPG_EX_PATCH_DATA_SIZE - UPG_PATCH_HDR_SIZE)) ||
        (h->old_fw_size == 0U) || (h->old_fw_size > UPG_APP_MAX_SIZE)) {
        DEBUG_TRACE(ERROR_TAG, "bs: size range new=%lu patch=%lu old=%lu",
                    h->new_fw_size, h->patch_size, h->old_fw_size);
        return -1;
    }
    if (h->patch_size != patch_body_size) {
        DEBUG_TRACE(ERROR_TAG, "bs: patch_size != received len");
        return -1;
    }
    if ((h->level < UPG_LZO_LEVEL_MIN) || (h->level > UPG_LZO_LEVEL_MAX)) {
        DEBUG_TRACE(ERROR_TAG, "bs: level %lu", h->level);
        return -1;
    }
    if (h->pid != UPG_PID) {
        DEBUG_TRACE(ERROR_TAG, "bs: pid 0x%04X", h->pid);
        return -1;
    }
    if (h->cid != UPG_CID) {
        DEBUG_TRACE(ERROR_TAG, "bs: cid 0x%04X", h->cid);
        return -1;
    }
    if ((uint16_t)(h->old_fw_version & 0xFFFFU) != running_version) {
        DEBUG_TRACE(ERROR_TAG, "bs: old ver 0x%04X != run 0x%04X",
                    (uint16_t)h->old_fw_version, running_version);
        return -1;
    }
    if ((uint16_t)(h->new_fw_version & 0xFFFFU) != expect_new_version) {
        DEBUG_TRACE(ERROR_TAG, "bs: new ver 0x%04X != f0 0x%04X",
                    (uint16_t)h->new_fw_version, expect_new_version);
        return -1;
    }

    /* patch body CRC (header itself is not covered) */
    crc = upg_ext_crc32(UPG_EX_PATCH_BASE + UPG_PATCH_HDR_SIZE, h->patch_size);
    if (crc != h->patch_crc) {
        DEBUG_TRACE(ERROR_TAG, "bs: patch crc %08lX != %08lX",
                    crc, h->patch_crc);
        return -1;
    }

    /* base-image full CRC (req.2: 版本对但底包内容残缺也要拦住) */
    crc = upg_mem_crc32((const uint8_t *)UPG_APP_BASE, h->old_fw_size);
    if (crc != h->old_fw_crc) {
        DEBUG_TRACE(ERROR_TAG, "bs: old fw crc %08lX != %08lX", crc, h->old_fw_crc);
        return -1;
    }
    return 0;
}

int upg_bspatch_apply(uint16_t running_version, uint16_t expect_new_version,
                      uint32_t received_len, upg_staging_t *st)
{
    static upg_patch_header_t header;
    static uint8_t work[UPG_LZO_LEVEL_MAX + 1];
    static lzo_read_t lzo_read;
    struct bspatch_stream stream;
    uint32_t off;
    uint32_t crc;

    if (lzo_init() != LZO_E_OK) {
        DEBUG_TRACE(ERROR_TAG, "bs: lzo_init fail");
        return -1;
    }
    if (upg_flash_read(UPG_EX_PATCH_BASE, (uint8_t *)&header, sizeof(header)) != 0) {
        return -1;
    }
    if ((received_len <= UPG_PATCH_HDR_SIZE) ||
        (patch_header_check(&header, received_len - UPG_PATCH_HDR_SIZE,
                            running_version, expect_new_version) != 0)) {
        return -1;
    }

    INFO("\n[UPG] synth: new=%luB patch=%luB level=%lu\n",
         header.new_fw_size, header.patch_size, header.level);

    /* erase the NewFW range the synthesis will write */
    for (off = 0; off < header.new_fw_size; off += UPG_BS_SECTOR) {
        if (upg_flash_erase_4k(UPG_EX_NEWFW_BASE + off) != 0) {
            DEBUG_TRACE(ERROR_TAG, "bs: erase fail @%lu", off);
            return -1;
        }
        LL_IWDG_ReloadCounter(IWDG);
    }
    flash_write_set_erased_region(UPG_EX_NEWFW_BASE,
                                  ((header.new_fw_size + UPG_BS_SECTOR - 1U) /
                                   UPG_BS_SECTOR) * UPG_BS_SECTOR);

    memset(&lzo_read, 0, sizeof(lzo_read));
    memset(work, 0, header.level + 1U);
    lzo_read.patch_off = 0;
    lzo_read.p_header = &header;
    lzo_read.p_work = work;

    stream.read = stream_read;
    stream.opaque = &lzo_read;
    bs_overflow = 0;
    memset(bs_sector_buf, 0, sizeof(bs_sector_buf));

    if (bspatch((const uint8_t *)UPG_APP_BASE, header.old_fw_size,
                (uint8_t *)UPG_EX_NEWFW_BASE, header.new_fw_size,
                &stream) != 0) {
        return -1;
    }

    /* verify the synthesized image */
    crc = upg_ext_crc32(UPG_EX_NEWFW_BASE, header.new_fw_size);
    if (crc != header.new_fw_crc) {
        DEBUG_TRACE(ERROR_TAG, "bs: new fw crc %08lX != %08lX", crc, header.new_fw_crc);
        return -1;
    }

    st->magic = UPG_STAGING_MAGIC;
    st->pkg_type = UPG_PKG_DIFF;
    st->reserved[0] = st->reserved[1] = st->reserved[2] = 0;
    st->src_addr = UPG_EX_NEWFW_BASE;
    st->new_fw_len = header.new_fw_size;
    st->new_fw_crc = header.new_fw_crc;
    st->new_version = expect_new_version;
    st->old_version = running_version;
    st->crc16 = 0;
    st->reserved2 = 0;
    st->crc16 = upg_crc16((const uint8_t *)st, sizeof(*st) - 2U);

    INFO("\n[UPG] synth: OK crc=%08lX\n", crc);
    return 0;
}
