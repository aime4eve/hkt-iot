#include "W25X40CLSNIG.h"

#include "spi.h"
#include <fdb_def.h>
#include <flashdb.h>

#include "SI12T.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "sound.h"
#include "systick.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

/* TSDB object */
struct fdb_tsdb tsdb = {0};
struct fdb_kvdb kvdb_pass = {0};
struct fdb_kvdb kvdb_fingerprint = {0};
struct fdb_kvdb kvdb_card = {0};

struct fdb_blob blob_pass = {0};
struct fdb_blob blob_fingerprint = {0};
struct fdb_blob blob_card = {0};

static uint32_t boot_count = 0;
static time_t boot_time[10] = {0, 1, 2, 3};
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
    {"username", "hkt_smarthard", 0},                /* string KV */
    {"password", "12345678", 0},                     /* string KV */
    {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
    {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};

// static struct fdb_default_kv_node default_kv_table[] = {
//     {"username", "000", 0},               /* string KV */
//     {"password", "12345678", 0},                     /* string KV */
//     {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
// };

u16 export_len;

/* counts for simulated timestamp */
static time_t nor_flash_timestamp;

static bool query_cb(fdb_tsl_t tsl, void *arg);
static bool query_by_time_cb(fdb_tsl_t tsl, void *arg);
static bool set_status_cb(fdb_tsl_t tsl, void *arg);

static void lock(fdb_db_t db)
{
    __disable_irq();
}

static void unlock(fdb_db_t db)
{
    __enable_irq();
}

static fdb_time_t get_time(void)
{
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return Timestamp;
}

#define LOG_NAME_LEN 15
static char log_name[][LOG_NAME_LEN] =
    {
        "",
        "unlock_success",
        "unlock_fail",
        "enter_root",
        "input_action",
        "delete_action",
        "read_action",
        "joined",
        "offline",
        "is_local",
        "tamper_alarm",
        "tamper_recover",
        "factory",
        "type",
        "number",
        "word",
        "timestamp"};

static char *_itoa(int num, char *str)
{
    if (str == NULL) {
        return NULL;
    }
    snprintf(str, sizeof(str), "%d", num);
    return str;
}

static int getNumLen(int num)
{
    char str[LOG_NAME_LEN] = {0};
    int len = strlen(_itoa(num, str));
    return len;
}

void nor_flash_write(fdb_tsdb_t tsdb, struct lock_log *exc_lock_info_t)
{
    if (device_t.sleepState)
        return;
    /* 临界区上锁 */
    tx_mutex_get(&f_mutex_lock, TX_WAIT_FOREVER);

    struct fdb_blob blob;
    struct lock_log *info = exc_lock_info_t;

    { /* APPEND new TSL (time series log) */
        /* append new log to TSDB */
        fdb_tsl_append(tsdb, fdb_blob_make(&blob, info, sizeof(struct lock_log)));
#if NORFLASH_DEBUG_PRINTF
        INFO("\n============================ Write Log Table =============================");
        INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[1]), log_name[1],
             LOG_NAME_LEN, strlen(log_name[2]), log_name[2],
             LOG_NAME_LEN, strlen(log_name[3]), log_name[3],
             LOG_NAME_LEN, strlen(log_name[4]), log_name[4]);
        INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.unlockSuccess), info->u_exec.unlockSuccess,
             LOG_NAME_LEN, getNumLen(info->u_exec.unlockFail), info->u_exec.unlockFail,
             LOG_NAME_LEN, getNumLen(info->u_exec.enterRoot), info->u_exec.enterRoot,
             LOG_NAME_LEN, getNumLen(info->u_exec.inputAction), info->u_exec.inputAction);
        INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[5]), log_name[5],
             LOG_NAME_LEN, strlen(log_name[6]), log_name[6],
             LOG_NAME_LEN, strlen(log_name[7]), log_name[7],
             LOG_NAME_LEN, strlen(log_name[8]), log_name[8]);
        INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.deleteAction), info->u_exec.deleteAction,
             LOG_NAME_LEN, getNumLen(info->u_exec.readAction), info->u_exec.readAction,
             LOG_NAME_LEN, getNumLen(info->u_exec.joined), info->u_exec.joined,
             LOG_NAME_LEN, getNumLen(info->u_exec.offline), info->u_exec.offline);
        INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[9]), log_name[9],
             LOG_NAME_LEN, strlen(log_name[10]), log_name[10],
             LOG_NAME_LEN, strlen(log_name[11]), log_name[11],
             LOG_NAME_LEN, strlen(log_name[12]), log_name[12]);
        INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.isLocal), info->u_exec.isLocal,
             LOG_NAME_LEN, getNumLen(info->u_exec.tamperAlarm), info->u_exec.tamperAlarm,
             LOG_NAME_LEN, getNumLen(info->u_exec.tamperRecover), info->u_exec.tamperRecover,
             LOG_NAME_LEN, getNumLen(info->u_exec.factory), info->u_exec.factory);
        INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[13]), log_name[13],
             LOG_NAME_LEN, strlen(log_name[14]), log_name[14],
             LOG_NAME_LEN, strlen(log_name[15]), log_name[15],
             LOG_NAME_LEN, strlen(log_name[16]), log_name[16]);
        INFO("\n| %-*.*d | %-*.*d | %-*.*s | %-*.*d |", LOG_NAME_LEN, getNumLen(info->type), info->type,
             LOG_NAME_LEN, getNumLen(info->number), info->number,
             LOG_NAME_LEN, strlen(info->word), info->word,
             LOG_NAME_LEN, getNumLen(info->stamp), info->stamp);
        INFO("\n==========================================================================\n");
#endif
    }
    {   /* SET the TSL status */
        /* Change the TSL status by iterator or time iterator
         * set_status_cb: the change operation will in this callback
         *
         * NOTE: The actions to modify the state must be in order.
         *       like: FDB_TSL_WRITE -> FDB_TSL_USER_STATUS1 -> FDB_TSL_DELETED -> FDB_TSL_USER_STATUS2
         *       The intermediate states can also be ignored.
         *       such as: FDB_TSL_WRITE -> FDB_TSL_DELETED
         */
#if NORFLASH_ITER_PRINTF
        fdb_tsl_iter(tsdb, set_status_cb, tsdb);
#endif
    }

    /* 临界区释放锁 */
    tx_mutex_put(&f_mutex_lock);
}

void nor_flash_read(fdb_tsdb_t tsdb)
{
    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(tsdb, query_cb, tsdb);
    }
}

void nor_flash_read_by_time(fdb_tsdb_t tsdb)
{
    { /* QUERY the TSDB by time */
        /* prepare query time (from 1970-01-01 00:00:00 to 2020-05-05 00:00:00) */
        struct tm tm_from = {.tm_year = 1970 - 1900, .tm_mon = 0, .tm_mday = 1, .tm_hour = 0, .tm_min = 0, .tm_sec = 0};
        struct tm tm_to = {.tm_year = systime.tm_year - 1900, .tm_mon = systime.tm_mon, .tm_mday = systime.tm_mday, .tm_hour = systime.tm_hour, .tm_min = systime.tm_min, .tm_sec = systime.tm_sec};
        time_t from_time = mktime(&tm_from), to_time = mktime(&tm_to);
        size_t count;
        /* query all TSL in TSDB by time */
        fdb_tsl_iter_by_time(tsdb, from_time, to_time, query_by_time_cb, tsdb);
        /* query all FDB_TSL_WRITE status TSL's count in TSDB by time */
        count = fdb_tsl_query_count(tsdb, from_time, to_time, FDB_TSL_WRITE);
        INFO("\nquery count is: %zu", count);
    }
}

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct lock_log lock_log_t;
    struct lock_log *info = &lock_log_t;
    fdb_tsdb_t db = arg;
    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &lock_log_t, sizeof(lock_log_t))));

#if NORFLASH_DEBUG_PRINTF
    INFO("\n============================ Read Log Table ==============================");
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[1]), log_name[1],
         LOG_NAME_LEN, strlen(log_name[2]), log_name[2],
         LOG_NAME_LEN, strlen(log_name[3]), log_name[3],
         LOG_NAME_LEN, strlen(log_name[4]), log_name[4]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.unlockSuccess), info->u_exec.unlockSuccess,
         LOG_NAME_LEN, getNumLen(info->u_exec.unlockFail), info->u_exec.unlockFail,
         LOG_NAME_LEN, getNumLen(info->u_exec.enterRoot), info->u_exec.enterRoot,
         LOG_NAME_LEN, getNumLen(info->u_exec.inputAction), info->u_exec.inputAction);
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[5]), log_name[5],
         LOG_NAME_LEN, strlen(log_name[6]), log_name[6],
         LOG_NAME_LEN, strlen(log_name[7]), log_name[7],
         LOG_NAME_LEN, strlen(log_name[8]), log_name[8]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.deleteAction), info->u_exec.deleteAction,
         LOG_NAME_LEN, getNumLen(info->u_exec.readAction), info->u_exec.readAction,
         LOG_NAME_LEN, getNumLen(info->u_exec.joined), info->u_exec.joined,
         LOG_NAME_LEN, getNumLen(info->u_exec.offline), info->u_exec.offline);
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[9]), log_name[9],
         LOG_NAME_LEN, strlen(log_name[10]), log_name[10],
         LOG_NAME_LEN, strlen(log_name[11]), log_name[11],
         LOG_NAME_LEN, strlen(log_name[12]), log_name[12]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.isLocal), info->u_exec.isLocal,
         LOG_NAME_LEN, getNumLen(info->u_exec.tamperAlarm), info->u_exec.tamperAlarm,
         LOG_NAME_LEN, getNumLen(info->u_exec.tamperRecover), info->u_exec.tamperRecover,
         LOG_NAME_LEN, getNumLen(info->u_exec.factory), info->u_exec.factory);
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[13]), log_name[13],
         LOG_NAME_LEN, strlen(log_name[14]), log_name[14],
         LOG_NAME_LEN, strlen(log_name[15]), log_name[15],
         LOG_NAME_LEN, strlen(log_name[16]), log_name[16]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*s | %-*.*d |", LOG_NAME_LEN, getNumLen(info->type), info->type,
         LOG_NAME_LEN, getNumLen(info->number), info->number,
         LOG_NAME_LEN, strlen(info->word), info->word,
         LOG_NAME_LEN, getNumLen(info->stamp), info->stamp);
    INFO("\n==========================================================================\n");
#endif
    return false;
}

static bool query_by_time_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct lock_log lock_log_t;
    struct lock_log *info = &lock_log_t;
    fdb_tsdb_t db = arg;
    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &lock_log_t, sizeof(lock_log_t))));

#if NORFLASH_DEBUG_PRINTF
    INFO("\n============================ Time Log Table ==============================");
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[1]), log_name[1],
         LOG_NAME_LEN, strlen(log_name[2]), log_name[2],
         LOG_NAME_LEN, strlen(log_name[3]), log_name[3],
         LOG_NAME_LEN, strlen(log_name[4]), log_name[4]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.unlockSuccess), info->u_exec.unlockSuccess,
         LOG_NAME_LEN, getNumLen(info->u_exec.unlockFail), info->u_exec.unlockFail,
         LOG_NAME_LEN, getNumLen(info->u_exec.enterRoot), info->u_exec.enterRoot,
         LOG_NAME_LEN, getNumLen(info->u_exec.inputAction), info->u_exec.inputAction);
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[5]), log_name[5],
         LOG_NAME_LEN, strlen(log_name[6]), log_name[6],
         LOG_NAME_LEN, strlen(log_name[7]), log_name[7],
         LOG_NAME_LEN, strlen(log_name[8]), log_name[8]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.deleteAction), info->u_exec.deleteAction,
         LOG_NAME_LEN, getNumLen(info->u_exec.readAction), info->u_exec.readAction,
         LOG_NAME_LEN, getNumLen(info->u_exec.joined), info->u_exec.joined,
         LOG_NAME_LEN, getNumLen(info->u_exec.offline), info->u_exec.offline);
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[9]), log_name[9],
         LOG_NAME_LEN, strlen(log_name[10]), log_name[10],
         LOG_NAME_LEN, strlen(log_name[11]), log_name[11],
         LOG_NAME_LEN, strlen(log_name[12]), log_name[12]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->u_exec.isLocal), info->u_exec.isLocal,
         LOG_NAME_LEN, getNumLen(info->u_exec.tamperAlarm), info->u_exec.tamperAlarm,
         LOG_NAME_LEN, getNumLen(info->u_exec.tamperRecover), info->u_exec.tamperRecover,
         LOG_NAME_LEN, getNumLen(info->u_exec.factory), info->u_exec.factory);
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[13]), log_name[13],
         LOG_NAME_LEN, strlen(log_name[14]), log_name[14],
         LOG_NAME_LEN, strlen(log_name[15]), log_name[15],
         LOG_NAME_LEN, strlen(log_name[16]), log_name[16]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*s | %-*.*d |", LOG_NAME_LEN, getNumLen(info->type), info->type,
         LOG_NAME_LEN, getNumLen(info->number), info->number,
         LOG_NAME_LEN, strlen(info->word), info->word,
         LOG_NAME_LEN, getNumLen(info->stamp), info->stamp);
    INFO("\n==========================================================================\n");
#endif
    return false;
}

static bool set_status_cb(fdb_tsl_t tsl, void *arg)
{
    fdb_tsdb_t db = arg;
#if NORFLASH_DEBUG_PRINTF
    INFO("set the TSL (time %ld) status from %d to %d\n", tsl->time, tsl->status, FDB_TSL_USER_STATUS1);
#endif
    fdb_tsl_set_status(db, tsl, FDB_TSL_USER_STATUS1);

    return false;
}

static bool export_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct lock_log info;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &info, sizeof(info))));
    INFO("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%ld,",
         info.u_exec.unlockSuccess,
         info.u_exec.unlockFail,
         info.u_exec.enterRoot,
         info.u_exec.inputAction,
         info.u_exec.deleteAction,
         info.u_exec.readAction,
         info.u_exec.joined,
         info.u_exec.offline,
         info.u_exec.isLocal,
         info.u_exec.tamperAlarm,
         info.u_exec.tamperRecover,
         info.u_exec.factory,
         info.type,
         info.number,
         info.word,
         info.stamp); // 操作时间戳
    return false;
}

void nor_flash_read_export(fdb_tsdb_t tsdb)
{
    INFO("\nexport sensor data __back,");
    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(tsdb, export_cb, tsdb);
    }
    INFO("export sensor data end\n");
}

#if FUNC_OPERATIONAL_VERSION_ENABLE

// 片外FLASH初始化
void SpiFlash_Init(void)
{
    extern int spi_flash_init(void);
    spi_flash_init();
    // W25X40_ReleasePowerDown();

    fdb_err_t result = fdb_tsdb_init(&tsdb, "offline_data", "fdb_tsdb", get_time, 128, NULL);
    /* read last saved time for simulated timestamp */
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &nor_flash_timestamp);

    while (result != FDB_NO_ERR) {
        mDelay(1000);
        INFO("\n%s Fail\n", __func__);
    }
    // fdb_kv_set_default(&kvdb_pass);
    // fdb_kv_set_default(&kvdb_card);
    // fdb_tsl_clean(&tsdb);
}

void print_coded_key_info(void)
{
    INFO("\r\n----------------------------------------\r\n");
    // INFO("coded lock list, total num pass[%d] card[%d] bt[%d]\r\n",
    //      coded_lock_t.total_num_pass, coded_lock_t.total_num_card, coded_lock_t.total_num_bt);
    INFO("coded lock list, total num pass[%d] card[%d]\r\n",
         coded_lock_t.total_num_pass, coded_lock_t.total_num_card);
    INFO("-----------------pass-------------------\r\n");
    if (coded_lock_t.total_num_pass > 0) {
        for (int i = 0; i < MAX_SECRET_TIMELINESS_NUM + 1; i++) {
            if (coded_lock_t.pass[i].user_num || coded_lock_t.pass[i].isable)
                INFO("usernum[%d], isable[%d], word[%s], valid[%d-%d], cnt[%d]\r\n",
                     coded_lock_t.pass[i].user_num, coded_lock_t.pass[i].isable, coded_lock_t.pass[i].word,
                     coded_lock_t.pass[i].start_timestamp, coded_lock_t.pass[i].end_timestamp, coded_lock_t.pass[i].valid_cnt);
        }
    }
    // INFO("------------------bt--------------------\r\n");
    // if (coded_lock_t.total_num_bt > 0) {
    //     for (int i = 0; i < MAX_SECRET_TIMELINESS_NUM; i++) {
    //         if (coded_lock_t.bt_pass[i].user_num)
    //             INFO("usernum[%d], isable[%d], word[%s], valid[%d-%d], cnt[%d]\r\n",
    //                  coded_lock_t.bt_pass[i].user_num, coded_lock_t.bt_pass[i].isable, coded_lock_t.bt_pass[i].word,
    //                  coded_lock_t.bt_pass[i].start_timestamp, coded_lock_t.bt_pass[i].end_timestamp, coded_lock_t.bt_pass[i].valid_cnt);
    //     }
    // }
    INFO("-----------------card-------------------\r\n");
    if (coded_lock_t.total_num_card > 0) {
        for (int i = 0; i < MAX_SECRET_TIMELINESS_NUM; i++) {
            if (coded_lock_t.card[i].user_num)
                INFO("usernum[%d], isable[%d], id[%02X%02X%02X%02X], key[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X], valid[%d-%d], cnt[%d]\r\n",
                     coded_lock_t.card[i].user_num, coded_lock_t.card[i].isable,
                     coded_lock_t.card[i].id[0], coded_lock_t.card[i].id[1], coded_lock_t.card[i].id[2], coded_lock_t.card[i].id[3],
                     coded_lock_t.card[i].id_key[0], coded_lock_t.card[i].id_key[1], coded_lock_t.card[i].id_key[2], coded_lock_t.card[i].id_key[3], coded_lock_t.card[i].id_key[4], coded_lock_t.card[i].id_key[5], coded_lock_t.card[i].id_key[6], coded_lock_t.card[i].id_key[7],
                     coded_lock_t.card[i].id_key[8], coded_lock_t.card[i].id_key[9], coded_lock_t.card[i].id_key[10], coded_lock_t.card[i].id_key[11], coded_lock_t.card[i].id_key[12], coded_lock_t.card[i].id_key[13], coded_lock_t.card[i].id_key[14], coded_lock_t.card[i].id_key[15],
                     coded_lock_t.card[i].start_timestamp, coded_lock_t.card[i].end_timestamp, coded_lock_t.card[i].valid_cnt);
        }
    }
    INFO("------------------temp--------------------\r\n");
    INFO("isable[%d], word[%s], valid[%d]min\r\n", coded_lock_t.temp_pass.isable, coded_lock_t.temp_pass.word, coded_lock_t.temp_pass.time);
}

u8 find_coded_key_num(u8 num, u8 arg)
{
    struct secret_key_timeliness *key_t = coded_lock_t.pass;
    u8 i;
    for (i = 0; i < MAX_SECRET_TIMELINESS_NUM + 1; i++) {
        if (arg == 0) { // 修改  新增
            if ((key_t[i].user_num == num && key_t[i].word[0] > 0) || (key_t[i].user_num == 0 && key_t[i].word[0] == 0)) {
                break;
            }
        } else if (arg == 1 || arg == 2) { // 删除 读取
            if (key_t[i].user_num == num && key_t[i].word[0] > 0) {
                break;
            }
        }
    }
    return i;
}

void add_coded_key_num(u8 num, struct secret_key_timeliness *key)
{
    if (key == NULL || num >= MAX_SECRET_TIMELINESS_NUM + 1) {
        return; // 无效输入
    }

    struct secret_key_timeliness *existing_key = &coded_lock_t.pass[num];

    // 无密钥信息 执行新增操作
    if (existing_key->word[0] == 0) {
        coded_lock_t.total_num_pass++;
    }

    // 复制新密钥信息
    memcpy(existing_key, key, sizeof(struct secret_key_timeliness));
    FLASH_Write_Door_Lock_Coded();
}

void delete_coded_key_num(u8 num)
{
    if (num >= MAX_SECRET_TIMELINESS_NUM + 1) {
        return; // 无效输入
    }

    struct secret_key_timeliness *existing_key = &coded_lock_t.pass[num];
    if (existing_key->word[0] > 0) {
        coded_lock_t.total_num_pass--;

        // 如果不是最后一个密钥，则将其后的密钥向前移动
        if (num < coded_lock_t.total_num_pass) {
            memmove(&coded_lock_t.pass[num], &coded_lock_t.pass[num + 1], (coded_lock_t.total_num_pass - num) * sizeof(struct secret_key_timeliness));
        }
        memset(&coded_lock_t.pass[coded_lock_t.total_num_pass], 0, sizeof(struct secret_key_timeliness));
        FLASH_Write_Door_Lock_Coded();
    }
}

void read_coded_key_num(u8 num, u8 *buffer)
{
    struct secret_key_timeliness *key_t = &coded_lock_t.pass[num];
    buffer[0] = key_t->user_num;
    buffer[1] = key_t->isable;
    buffer[2] = key_t->len;
    memcpy(buffer + 3 + (8 - key_t->len), key_t->word, key_t->len);
    buffer[11] = (key_t->start_timestamp >> 24) & 0xFF;
    buffer[12] = (key_t->start_timestamp >> 16) & 0xFF;
    buffer[13] = (key_t->start_timestamp >> 8) & 0xFF;
    buffer[14] = key_t->start_timestamp & 0xFF;
    buffer[15] = (key_t->end_timestamp >> 24) & 0xFF;
    buffer[16] = (key_t->end_timestamp >> 16) & 0xFF;
    buffer[17] = (key_t->end_timestamp >> 8) & 0xFF;
    buffer[18] = key_t->end_timestamp & 0xFF;
    buffer[19] = (key_t->valid_cnt >> 8) & 0xFF;
    buffer[20] = key_t->valid_cnt & 0xFF;
}

u8 find_coded_card_num(u8 num, u8 arg)
{
    struct secret_card_timeliness *key_t = coded_lock_t.card;
    u8 i;
    for (i = 0; i < MAX_SECRET_TIMELINESS_NUM; i++) {
        if (arg == 0) { // 新增修改
            if (key_t[i].user_num == num || key_t[i].user_num == 0) {
                break;
            }
        } else if (arg == 1 || arg == 2) { // 删除 读取
            if (key_t[i].user_num == num && key_t[i].user_num != 0) {
                break;
            }
        }
    }
    return i;
}

void add_coded_card_num(u8 num, struct secret_card_timeliness *card)
{
    if (card == NULL || num >= MAX_SECRET_TIMELINESS_NUM) {
        return; // 无效输入
    }

    struct secret_key_timeliness *existing_key = &coded_lock_t.pass[num];

    // 无密钥信息 执行新增操作
    if (num >= coded_lock_t.total_num_card)
        coded_lock_t.total_num_card++;

    // 复制新密钥信息
    memcpy(&coded_lock_t.card[num], card, sizeof(struct secret_card_timeliness));
    FLASH_Write_Door_Lock_Coded();
}

void delete_coded_card_num(u8 num)
{
    if (coded_lock_t.card[num].user_num) {
        coded_lock_t.total_num_card--;

        // 如果不是最后一个密钥，则将其后的密钥向前移动
        if (num < coded_lock_t.total_num_card) {
            memmove(&coded_lock_t.card[num], &coded_lock_t.card[num + 1], (coded_lock_t.total_num_card - num) * sizeof(struct secret_card_timeliness));
        }
        memset(&coded_lock_t.card[coded_lock_t.total_num_card], 0, sizeof(struct secret_card_timeliness));
        FLASH_Write_Door_Lock_Coded();
    }
}

void read_coded_card_num(u8 num, u8 *buffer)
{
    struct secret_card_timeliness *card_t = &coded_lock_t.card[num];
    buffer[0] = card_t->user_num;
    buffer[1] = card_t->isable;
    memcpy(buffer + 2, card_t->id, 4);
    memcpy(buffer + 6, card_t->id_key, 16);
    buffer[22] = (card_t->start_timestamp >> 24) & 0xFF;
    buffer[23] = (card_t->start_timestamp >> 16) & 0xFF;
    buffer[24] = (card_t->start_timestamp >> 8) & 0xFF;
    buffer[25] = card_t->start_timestamp & 0xFF;
    buffer[26] = (card_t->end_timestamp >> 24) & 0xFF;
    buffer[27] = (card_t->end_timestamp >> 16) & 0xFF;
    buffer[28] = (card_t->end_timestamp >> 8) & 0xFF;
    buffer[29] = card_t->end_timestamp & 0xFF;
    buffer[30] = (card_t->valid_cnt >> 8) & 0xFF;
    buffer[31] = card_t->valid_cnt & 0xFF;
}

void set_coded_open_mode(struct coded_lock_open_mode *mode)
{
    memcpy(&coded_open_mode_t, mode, sizeof(struct coded_lock_open_mode));
    if (coded_open_mode_t.mode == 0) {
        /* 语音 */
        // 关锁提示音
        // 已关锁
        Sound_Insert_Number(0, VOICE_TONE_OFF_LOCK);
        Sound_Insert_Number(1, VOICE_OFF_LOCK);
        /* 反转 */
        MOTOR_REVERSAL();
        tx_thread_sleep(150);
        /* 刹车 */
        MOTOR_BRAKE();
        /* 待机 */
        MOTOR_STANDBY();
    } else if (coded_open_mode_t.mode == 1) {
        tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
    }
    FLASH_Write_Open_Mode();
}

void set_coded_lock_volume(void)
{
    u16 volume = device_t.volume * 8 / 100;
    if (device_t.volume >= 90) {
        sound_t.volume_number = VOICE_VOLUME8;
    } else {
        sound_t.volume_number = voice_volume_enum[volume];
    }
    INFO("\r\n Set Volume Number %d\r\n", sound_t.volume_number);
    Sound_Play(sound_t.volume_number);
    FLASH_Write_Device_Param();
}

#else

// 片外FLASH初始化
void SpiFlash_Init(void)
{
    extern int spi_flash_init(void);
    spi_flash_init();
    // W25X40_ReleasePowerDown();

    fdb_err_t result = fdb_tsdb_init(&tsdb, "offline_data", "fdb_tsdb", get_time, 128, NULL);
    /* read last saved time for simulated timestamp */
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &nor_flash_timestamp);

    //    struct fdb_default_kv default_kv;
    //    default_kv.kvs = default_kv_table;
    //    default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

    /* set the lock and unlock function if you want */
    // fdb_kvdb_control(&kvdb_pass, FDB_KVDB_CTRL_SET_LOCK, lock);
    fdb_kvdb_control(&kvdb_pass, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
    // result |= fdb_kvdb_init(&kvdb_pass, "password", "fdb_kvdb_password", &default_kv, NULL);
    result |= fdb_kvdb_init(&kvdb_pass, "password", "fdb_kvdb_password", NULL, NULL);

    // fdb_kvdb_control(&kvdb_fingerprint, FDB_KVDB_CTRL_SET_LOCK, lock);
    fdb_kvdb_control(&kvdb_fingerprint, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
    // result |= fdb_kvdb_init(&kvdb_fingerprint, "fingerprint", "fdb_kvdb_fingerprint", &default_kv, NULL);
    result |= fdb_kvdb_init(&kvdb_fingerprint, "fingerprint", "fdb_kvdb_fingerprint", NULL, NULL);

    // fdb_kvdb_control(&kvdb_card, FDB_KVDB_CTRL_SET_LOCK, lock);
    fdb_kvdb_control(&kvdb_card, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
    // result |= fdb_kvdb_init(&kvdb_card, "card", "fdb_kvdb_card", &default_kv, NULL);
    result |= fdb_kvdb_init(&kvdb_card, "card", "fdb_kvdb_card", NULL, NULL);

    while (result != FDB_NO_ERR) {
        mDelay(1000);
        INFO("\n%s Fail\n", __func__);
    }

    // fdb_kv_set_default(&kvdb_pass);
    // fdb_kv_set_default(&kvdb_fingerprint);
    // fdb_kv_set_default(&kvdb_card);
    // fdb_tsl_clean(&tsdb);
}

/**
 * @brief  设置开锁密码
 * @param  userNum: 用户编号
 * @param  password：用户密码
 * @param  syncEvent：操作同步事件
 * @retval
 */
void set_coded_lock(u8 userNum, char *password, u8 syncEvent)
{
    char key[3];
    sprintf(key, "%03d", userNum);
    fdb_kv_set(&kvdb_pass, key, password);

    int data = syncEvent; // 需要同步新增命令到平台
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_pass, key_blob, fdb_blob_make(&blob_pass, &data, sizeof(data)));
    coded_lock_t.pass[userNum].event = syncEvent;
}

/**
 * @brief  删除开锁密码
 * @param  userNum: 用户编号
 * @param  syncEvent：操作同步事件
 * @retval
 */
void del_coded_lock(u8 userNum, u8 syncEvent)
{
    char key[3];
    sprintf(key, "%03d", userNum);
    fdb_kv_del(&kvdb_pass, key);

    int data = syncEvent; // 需要同步删除命令到平台
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_pass, key_blob, fdb_blob_make(&blob_pass, &data, sizeof(data)));
    coded_lock_t.pass[userNum].event = syncEvent;
}

/**
 * @brief  删除开锁密码云同步事件
 * @param  userNum: 用户编号
 * @retval
 */
void del_sync_coded_lock(u8 userNum)
{
    char key[3];
    sprintf(key, "%03d", userNum);

    int data = 0; // 删除同步事件
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_pass, key_blob, fdb_blob_make(&blob_pass, &data, sizeof(data)));
    coded_lock_t.pass[userNum].event = 0;
}

/**
 * @brief  读取开锁密码
 * @param  userNum: 用户编号
 * @param  password：待读取用户密码缓冲区
 * @param  event：事件 1同步新增密码到云 2同步删除密码到云 0无操作
 * @retval
 */
void read_coded_lock(u8 userNum, char *password, u8 *event)
{
    char key[3];
    sprintf(key, "%03d", userNum);

    char *value = fdb_kv_get(&kvdb_pass, key);
    /* the return value is NULL when get the value failed */
    if (value != NULL) {
        strncpy(password, value, strlen(value));
        INFO("\n\"%s\" get the '%s' value is: %s", __func__, key, value);
    }

    int data = 0;
    *event = 0;
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_get_blob(&kvdb_pass, key_blob, fdb_blob_make(&blob_pass, &data, sizeof(data)));
    /* the blob.saved.len is more than 0 when get the value successful */
    if (blob_pass.saved.len > 0 && value != NULL) {
        INFO("\n\"%s\" get the '%s' event is: %d", __func__, key, data);
        *event = data;
    }
}

/**
 * @brief  开锁密码复位
 * @param
 * @retval
 */
void reset_coded_lock(void)
{
    fdb_kv_set_default(&kvdb_pass);
}

/**
 * @brief  绑定卡编号
 * @param  userNum: 用户编号
 * @param  card_id：卡ID号
 * @param  syncEvent：操作同步事件
 * @retval
 */
void set_card_binding(u8 userNum, char *card_id, u8 syncEvent)
{
    char key[3];
    sprintf(key, "%03d", userNum);
    fdb_kv_set(&kvdb_card, key, card_id);

    int data = syncEvent; // 需要同步新增命令到平台
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_card, key_blob, fdb_blob_make(&blob_card, &data, sizeof(data)));
    coded_lock_t.card[userNum].event = syncEvent;
}

/**
 * @brief  删除已绑定的卡
 * @param  userNum: 用户编号
 * @param  syncEvent：操作同步事件
 * @retval
 */
void del_card_binding(u8 userNum, u8 syncEvent)
{
    char key[3];
    sprintf(key, "%03d", userNum);
    fdb_kv_del(&kvdb_card, key);

    int data = syncEvent; // 需要同步删除命令到平台
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_card, key_blob, fdb_blob_make(&blob_card, &data, sizeof(data)));
    coded_lock_t.card[userNum].event = syncEvent;
}

/**
 * @brief  删除开锁卡云同步事件
 * @param  userNum: 用户编号
 * @retval
 */
void del_sync_card_binding(u8 userNum)
{
    char key[3];
    sprintf(key, "%03d", userNum);

    int data = 0; // 删除同步事件
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_card, key_blob, fdb_blob_make(&blob_card, &data, sizeof(data)));
    coded_lock_t.card[userNum].event = 0;
}

/**
 * @brief  读取卡绑定状态
 * @param  userNum: 用户编号
 * @param  card_id：卡ID号
 * @param  event：事件 1同步新增密码到云 2同步删除密码到云 0无操作
 * @retval
 */
void read_card_binding(u8 userNum, char *card_id, u8 *event)
{
    char key[3];
    sprintf(key, "%03d", userNum);

    char *value = fdb_kv_get(&kvdb_card, key);
    /* the return value is NULL when get the value failed */
    if (value != NULL) {
        strncpy(card_id, value, strlen(value));
        INFO("\n\"%s\" get the '%s' value is: %s", __func__, key, value);
    }

    int data = 0;
    *event = 0;
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_get_blob(&kvdb_card, key_blob, fdb_blob_make(&blob_card, &data, sizeof(data)));
    /* the blob.saved.len is more than 0 when get the value successful */
    if (blob_card.saved.len > 0 && value != NULL) {
        INFO("\n\"%s\" get the '%s' event is: %d", __func__, key, data);
        *event = data;
    }
}

/**
 * @brief  卡绑定复位
 * @param
 * @retval
 */
void reset_card_binding(void)
{
    fdb_kv_set_default(&kvdb_card);
}

/**
 * @brief  设置开锁指纹(仅存储编号,指纹信息存储在指纹模块内)
 * @param  userNum: 用户编号
 * @param  syncEvent：操作同步事件
 * @retval
 */
void set_fingerprint(u8 userNum, u8 syncEvent)
{
    char key[3];
    sprintf(key, "%03d", userNum);
    fdb_kv_set(&kvdb_fingerprint, key, "1");

    int data = syncEvent; // 需要同步新增命令到平台
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_fingerprint, key_blob, fdb_blob_make(&blob_fingerprint, &data, sizeof(data)));
    coded_lock_t.finger[userNum].event = syncEvent;
}

/**
 * @brief  删除开锁指纹
 * @param  userNum: 用户编号
 * @param  syncEvent：操作同步事件
 * @retval
 */
void del_fingerprint(u8 userNum, u8 syncEvent)
{
    char key[3];
    sprintf(key, "%03d", userNum);
    fdb_kv_del(&kvdb_fingerprint, key);

    int data = syncEvent; // 需要同步删除命令到平台
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_fingerprint, key_blob, fdb_blob_make(&blob_fingerprint, &data, sizeof(data)));
    coded_lock_t.finger[userNum].event = syncEvent;
}

/**
 * @brief  删除开锁指纹云同步事件
 * @param  userNum: 用户编号
 * @retval
 */
void del_sync_fingerprint(u8 userNum)
{
    char key[3];
    sprintf(key, "%03d", userNum);

    int data = 0; // 删除同步事件
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_set_blob(&kvdb_fingerprint, key_blob, fdb_blob_make(&blob_fingerprint, &data, sizeof(data)));
    coded_lock_t.finger[userNum].event = 0;
}

/**
 * @brief  读取开锁指纹是否有效
 * @param  userNum: 用户编号
 * @param  status：待读取用户指纹状态缓冲区
 * @param  event：事件 1同步新增密码到云 2同步删除密码到云 0无操作
 * @retval
 */
void read_fingerprint(u8 userNum, char *status, u8 *event)
{
    char key[3];
    sprintf(key, "%03d", userNum);
    char *value = fdb_kv_get(&kvdb_fingerprint, key);
    /* the return value is NULL when get the value failed */
    if (value != NULL) {
        INFO("\n\"%s\" get the '%s' value is: %s", __func__, key, value);
        if (value[0] == 0x31) {
            status[0] = 0x31;
        }
    }

    int data = 0;
    *event = 0;
    char key_blob[] = "SYNC_100";
    sprintf(key_blob, "SYNC_%03d", userNum);
    fdb_kv_get_blob(&kvdb_fingerprint, key_blob, fdb_blob_make(&blob_fingerprint, &data, sizeof(data)));
    /* the blob.saved.len is more than 0 when get the value successful */
    if (blob_fingerprint.saved.len > 0 && value != NULL) {
        INFO("\n\"%s\" get the '%s' event is: %d", __func__, key, data);
        *event = data;
    }
}

/**
 * @brief  开锁指纹复位
 * @param
 * @retval
 */
void reset_fingerprint(void)
{
#if FUNC_FINGERPRINT_ENABLE
    cancel_cmd();
    clear_fingerprint_maps();
    fdb_kv_set_default(&kvdb_fingerprint);
#endif
}

/**
 * @brief  初始化开锁密钥
 * @param
 * @retval
 */
void coded_lock_init(void)
{
    struct fdb_kv_iterator iterator;
    u8 number;
    char s_number[3];

    device_t.sleepDelay = 200; // 等待初始化成功
    memset(&coded_lock_t, 0, sizeof(coded_lock_t));
    fdb_kv_iterator_init(&iterator);
    while (fdb_kv_iterate(&kvdb_pass, &iterator)) {
        if (!strstr(iterator.curr_kv.name, "SYNC")) {
            memcpy(s_number, iterator.curr_kv.name, 3);
            number = atoi(s_number);
            if (number < 100) {
                read_coded_lock(number, coded_lock_t.pass[number].word, &coded_lock_t.pass[number].event);
            }
        }
    }

    device_t.sleepDelay = 200; // 等待初始化成功
    fdb_kv_iterator_init(&iterator);
    while (fdb_kv_iterate(&kvdb_card, &iterator)) {
        if (!strstr(iterator.curr_kv.name, "SYNC")) {
            memcpy(s_number, iterator.curr_kv.name, 3);
            number = atoi(s_number);
            if (number < 100) {
                read_card_binding(number, coded_lock_t.card[number].word, &coded_lock_t.card[number].event);
            }
        }
    }

    device_t.sleepDelay = 200; // 等待初始化成功
    fdb_kv_iterator_init(&iterator);
    while (fdb_kv_iterate(&kvdb_fingerprint, &iterator)) {
        if (!strstr(iterator.curr_kv.name, "SYNC")) {
            memcpy(s_number, iterator.curr_kv.name, 3);
            number = atoi(s_number);
            if (number < 100) {
                read_fingerprint(number, coded_lock_t.finger[number].word, &coded_lock_t.finger[number].event);
            }
        }
    }

    for (int i = 0; i < 100; i++) {
        if (strlen(coded_lock_t.pass[i].word) >= 6) {
            coded_lock_t.pass[i].isable = 1;
            coded_lock_t.total_num_pass++;
        }
        if (strlen(coded_lock_t.card[i].word) >= 6) {
            coded_lock_t.card[i].isable = 1;
            coded_lock_t.total_num_card++;
        }
        if (coded_lock_t.finger[i].word[0] == 0x31) {
            coded_lock_t.finger[i].isable = 1;
            coded_lock_t.total_num_finger++;
        }
    }
    // if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) // 未设定密码时初始密码为123456
    // {
    //     strncpy(coded_lock_t.pass[0].word, "123456", 6);
    //     set_coded_lock(0, "123456", 0);
    //     coded_lock_t.total_num_pass++;
    //     coded_lock_t.pass[0].isable = 1;
    // }
    INFO("\r\n----------------------------------------\r\n");
    INFO("coded lock list, total num pass[%d] card[%d] finger[%d]\r\n",
         coded_lock_t.total_num_pass, coded_lock_t.total_num_card, coded_lock_t.total_num_finger);
    INFO("-----------------pass-------------------\r\n");
    if (coded_lock_t.total_num_pass > 0) {
        for (int i = 0; i < 100; i++) {
            if (coded_lock_t.pass[i].isable)
                INFO("usernum[%d]: %s, event[%d]\r\n", i, coded_lock_t.pass[i].word, coded_lock_t.pass[i].event);
        }
    }
    INFO("-----------------card-------------------\r\n");
    if (coded_lock_t.total_num_card > 0) {
        for (int i = 0; i < 100; i++) {
            if (coded_lock_t.card[i].isable)
                INFO("usernum[%d]: %s, event[%d]\r\n", i, coded_lock_t.card[i].word, coded_lock_t.card[i].event);
        }
    }
    INFO("-----------------finger-----------------\r\n");
    if (coded_lock_t.total_num_finger > 0) {
        for (int i = 0; i < 100; i++) {
            if (coded_lock_t.finger[i].isable)
                INFO("usernum[%d]: %s, event[%d]\r\n", i, coded_lock_t.finger[i].word, coded_lock_t.finger[i].event);
        }
    }
    INFO("----------------------------------------\r\n");

#if NORFLASH_ITER_PRINTF
    fdb_kv_print(&kvdb_pass);
    fdb_kv_print(&kvdb_card);
    fdb_kv_print(&kvdb_fingerprint);
#endif
}

void printf_local_info(void)
{
    INFO("\r\n----------------------------------------\r\n");
    INFO("coded lock list, total num pass[%d] card[%d] finger[%d]\r\n",
         coded_lock_t.total_num_pass, coded_lock_t.total_num_card, coded_lock_t.total_num_finger);
    INFO("-----------------pass-------------------\r\n");
    if (coded_lock_t.total_num_pass > 0) {
        for (int i = 0; i < 100; i++) {
            if (coded_lock_t.pass[i].isable)
                INFO("usernum[%d]: %s, event[%d]\r\n", i, coded_lock_t.pass[i].word, coded_lock_t.pass[i].event);
        }
    }
    INFO("-----------------card-------------------\r\n");
    if (coded_lock_t.total_num_card > 0) {
        for (int i = 0; i < 100; i++) {
            if (coded_lock_t.card[i].isable)
                INFO("usernum[%d]: %s, event[%d]\r\n", i, coded_lock_t.card[i].word, coded_lock_t.card[i].event);
        }
    }
    INFO("-----------------finger-----------------\r\n");
    if (coded_lock_t.total_num_finger > 0) {
        for (int i = 0; i < 100; i++) {
            if (coded_lock_t.finger[i].isable)
                INFO("usernum[%d]: %s, event[%d]\r\n", i, coded_lock_t.finger[i].word, coded_lock_t.finger[i].event);
        }
    }
    INFO("----------------------------------------\r\n");
}

/**
 * @brief  返回管理员数量
 * @param
 * @retval
 */
u8 cb_root_num(void)
{
    u8 total_num = 0;
    if (coded_lock_t.total_num_pass > 0) {
        for (int i = 0; i < 10; i++) {
            if (coded_lock_t.pass[i].isable)
                total_num++;
        }
    }
    if (coded_lock_t.total_num_card > 0) {
        for (int i = 0; i < 10; i++) {
            if (coded_lock_t.card[i].isable)
                total_num++;
        }
    }
    if (coded_lock_t.total_num_finger > 0) {
        for (int i = 0; i < 10; i++) {
            if (coded_lock_t.finger[i].isable)
                total_num++;
        }
    }
    return total_num;
}

/**
 * @brief  云同步事件
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_local_sync(ULONG thread_input)
{
    (void)thread_input;

    u8 buffer[50];
    u8 txlen = 0;

    int i;

    while (1) {
    __back:
        tx_thread_sleep(1000);
        if (LoRaWAN.joinState && !uartLoRa.sendDelay) {
            memset(buffer, 0, sizeof(buffer));
            for (i = 0; i < 100; i++) {
                /* 需要同步本地密码变更到服务器 */
                if (coded_lock_t.pass[i].event >= 1 && coded_lock_t.pass[i].event <= 2) {
                    buffer[0] = 0x2E;                              // 管理开锁密码
                    buffer[1] = coded_lock_t.pass[i].event - 1;    // 操作指令
                    buffer[2] = i;                                 // 用户编号
                    buffer[3] = strlen(coded_lock_t.pass[i].word); // 密码长度
                    sprintf((char *)buffer + 4 + (8 - buffer[3]), "%s", coded_lock_t.pass[i].word);
                    txlen = 12;

                    if (sendLoRaWANData(buffer, txlen) == true) {
                        del_sync_coded_lock(i);
                        coded_lock_t.pass[i].event = 0;
                    }
                    goto __back;
                }

                /* 需要同步本地卡状态变更到服务器 */
                if (coded_lock_t.card[i].event >= 1 && coded_lock_t.card[i].event <= 2) {
                    buffer[0] = 0x31;                           // 管理卡和指纹
                    buffer[1] = coded_lock_t.card[i].event - 1; // 操作指令
                    buffer[2] = i;                              // 用户编号
                    buffer[3] = 0;                              // 类型 卡
                    txlen = 4;

                    if (sendLoRaWANData(buffer, txlen) == true) {
                        del_sync_card_binding(i);
                        coded_lock_t.card[i].event = 0;
                    }
                    goto __back;
                }

                /* 需要同步本地指纹状态变更到服务器 */
                if (coded_lock_t.finger[i].event >= 1 && coded_lock_t.finger[i].event <= 2) {
                    buffer[0] = 0x31;                             // 管理卡和指纹
                    buffer[1] = coded_lock_t.finger[i].event - 1; // 操作指令
                    buffer[2] = i;                                // 用户编号
                    buffer[3] = 1;                                // 类型 指纹
                    txlen = 4;

                    if (sendLoRaWANData(buffer, txlen) == true) {
                        del_sync_fingerprint(i);
                        coded_lock_t.finger[i].event = 0;
                    }
                    goto __back;
                }
            }
        }
    }
}
#endif
