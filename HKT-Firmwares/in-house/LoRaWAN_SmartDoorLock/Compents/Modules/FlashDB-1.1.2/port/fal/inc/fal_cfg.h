/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <config.h>

#define NOR_FLASH_DEV_NAME "norflash0"
#define FAL_PART_HAS_TABLE_CFG 1
#define FAL_DEBUG 1
#define FAL_USING_SFUD_PORT 1
#define FDB_WRITE_GRAN 1

/* ===================== Flash device Configuration ========================= */
extern struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE \
    {                       \
        &nor_flash0,        \
    }
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */ // 4096Kb 512KB
#define FAL_PART_TABLE                                                                                   \
    {                                                                                                    \
        {FAL_PART_MAGIC_WORD, "fdb_tsdb", NOR_FLASH_DEV_NAME, 0, 416 * 1024, 0},                         \
            {FAL_PART_MAGIC_WORD, "fdb_kvdb_password", NOR_FLASH_DEV_NAME, 416 * 1024, 32 * 1024, 0},    \
            {FAL_PART_MAGIC_WORD, "fdb_kvdb_fingerprint", NOR_FLASH_DEV_NAME, 448 * 1024, 32 * 1024, 0}, \
            {FAL_PART_MAGIC_WORD, "fdb_kvdb_card", NOR_FLASH_DEV_NAME, 480 * 1024, 32 * 1024, 0},        \
    }

#endif /* FAL_PART_HAS_TABLE_CFG */

/*
上面这个分区表详细描述信息如下：

| 分区名      | Flash 设备名   | 偏移地址  | 大小  | 说明               |
| ----------- | -------------- | --------- | ----- | ------------------ |
| "fdb_tsdb" | "norflash0"    | 0         | 3MB   | 历史数据缓冲区 |
| "download"  | "norflash0"    | 1024*1024 | 1MB   | OTA 下载区         |
*/

#endif /* _FAL_CFG_H_ */
