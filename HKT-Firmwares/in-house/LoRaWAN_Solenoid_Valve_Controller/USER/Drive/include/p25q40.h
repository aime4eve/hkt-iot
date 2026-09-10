
#ifndef __P25Q40_H__
#define __P25Q40_H__

#include "config.h"
#include "fdb_def.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


struct local_log
{
    u8 park;
    u8 mode;
    u8 tamper;
    time_t stamp;
};



extern struct fdb_tsdb tsdb;

void nor_flash_write(fdb_tsdb_t tsdb);
void flash_db_init(void);
void p25q40_rst(void);
void p25q40_release(void);
void p25q40_init(void);
void p25q40_power_down(void);
void p25q40_chip_erase(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
