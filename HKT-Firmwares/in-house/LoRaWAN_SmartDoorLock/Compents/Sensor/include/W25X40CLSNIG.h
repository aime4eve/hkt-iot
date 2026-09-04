
#ifndef __W25X40CLSNIG_H__
#define __W25X40CLSNIG_H__

#include "config.h"
#include "control_center.h"
#include "fdb_def.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern struct fdb_tsdb tsdb;

void SpiFlash_Init(void);
void nor_flash_write(fdb_tsdb_t tsdb, struct lock_log *exc_lock_info_t);
void nor_flash_read(fdb_tsdb_t tsdb);
void nor_flash_read_by_time(fdb_tsdb_t tsdb);
void nor_flash_read_export(fdb_tsdb_t tsdb);

#if FUNC_OPERATIONAL_VERSION_ENABLE
void print_coded_key_info(void);
u8 find_coded_key_num(u8 num, u8 arg);
void add_coded_key_num(u8 num, struct secret_key_timeliness *key);
void delete_coded_key_num(u8 num);
void read_coded_key_num(u8 num, u8 *buffer);
u8 find_coded_card_num(u8 num, u8 arg);
void add_coded_card_num(u8 num, struct secret_card_timeliness *card);
void delete_coded_card_num(u8 num);
void read_coded_card_num(u8 num, u8 *buffer);
void set_coded_open_mode(struct coded_lock_open_mode *mode);
void set_coded_lock_volume(void);

#else

void set_coded_lock(u8 userNum, char *password, u8 syncEvent);
void del_coded_lock(u8 userNum, u8 syncEvent);
void del_sync_coded_lock(u8 userNum);
void read_coded_lock(u8 userNum, char *password, u8 *event);
void reset_coded_lock(void);

void set_card_binding(u8 userNum, char *card_id, u8 syncEvent);
void del_card_binding(u8 userNum, u8 syncEvent);
void del_sync_card_binding(u8 userNum);
void read_card_binding(u8 userNum, char *card_id, u8 *event);
void reset_card_binding(void);

void set_fingerprint(u8 userNum, u8 syncEvent);
void del_fingerprint(u8 userNum, u8 syncEvent);
void del_sync_fingerprint(u8 userNum);
void read_fingerprint(u8 userNum, char *status, u8 *event);
void reset_fingerprint(void);
void printf_local_info(void);

void coded_lock_init(void);
u8 cb_root_num(void);
void thread_local_sync(ULONG thread_input);

#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
