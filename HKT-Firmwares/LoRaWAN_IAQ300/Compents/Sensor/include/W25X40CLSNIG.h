
#ifndef __W25X40CLSNIG_H__
#define __W25X40CLSNIG_H__

#include "config.h"
#include "fdb_def.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern struct fdb_tsdb tsdb;

    void SpiFlash_Init(void);
    void nor_flash_write(fdb_tsdb_t tsdb);
    void nor_flash_read(fdb_tsdb_t tsdb);
    void nor_flash_read_by_time(fdb_tsdb_t tsdb);
    void nor_flash_read_export(fdb_tsdb_t tsdb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
