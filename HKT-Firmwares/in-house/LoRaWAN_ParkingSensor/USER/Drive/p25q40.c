#include "p25q40.h"
#include "communicate.h"
#include "control_center.h"
#include "spi.h"
#include "systick.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>
#include <fdb_def.h>
#include <flashdb.h>

/* TSDB object */
struct fdb_tsdb tsdb = {0};

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

#define LOG_NAME_LEN 15
static char log_name[][LOG_NAME_LEN] =
    {
        "",
        "park",
        "mode",
        "tamper",
        "timestamp"};

static void lock(fdb_db_t db)
{
    __disable_irq();
}

static void unlock(fdb_db_t db)
{
    __enable_irq();
}

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
		snprintf(str, sizeof(str), "%d", num);
    int len = strlen(str);
    return len;
}

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct local_log local_log_t;
    local_log_t.park = device_t.park_state;
    local_log_t.mode = device_t.park_mode;
    local_log_t.tamper = device_t.tamper_alarm;
    local_log_t.stamp = Timestamp;

    struct local_log *info = &local_log_t;
    fdb_tsdb_t db = arg;
    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &local_log_t, sizeof(local_log_t))));

#if NORFLASH_DEBUG_PRINTF
    INFO("\n============================ Read Log Table ==============================");
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[1]), log_name[1],
         LOG_NAME_LEN, strlen(log_name[2]), log_name[2],
         LOG_NAME_LEN, strlen(log_name[3]), log_name[3],
         LOG_NAME_LEN, strlen(log_name[4]), log_name[4]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->park), info->park,
         LOG_NAME_LEN, getNumLen(info->mode), info->mode,
         LOG_NAME_LEN, getNumLen(info->tamper), info->tamper,
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

static bool query_by_time_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct local_log local_log_t;
    local_log_t.park = device_t.park_state;
    local_log_t.mode = device_t.park_mode;
    local_log_t.tamper = device_t.tamper_alarm;
    local_log_t.stamp = Timestamp;

    struct local_log *info = &local_log_t;
    fdb_tsdb_t db = arg;
    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &local_log_t, sizeof(local_log_t))));

#if NORFLASH_DEBUG_PRINTF
    INFO("\n============================ Time Log Table ==============================");
    INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[1]), log_name[1],
         LOG_NAME_LEN, strlen(log_name[2]), log_name[2],
         LOG_NAME_LEN, strlen(log_name[3]), log_name[3],
         LOG_NAME_LEN, strlen(log_name[4]), log_name[4]);
    INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->park), info->park,
         LOG_NAME_LEN, getNumLen(info->mode), info->mode,
         LOG_NAME_LEN, getNumLen(info->tamper), info->tamper,
         LOG_NAME_LEN, getNumLen(info->stamp), info->stamp);
    INFO("\n==========================================================================\n");
#endif
    return false;
}

static fdb_time_t get_time(void)
{
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return Timestamp;
}

void nor_flash_read(fdb_tsdb_t tsdb)
{
    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(tsdb, query_cb, tsdb);
    }
}

void nor_flash_write(fdb_tsdb_t tsdb)
{
    if (device_t.sleep_state || !device_t.power_on)
        return;

    struct fdb_blob blob;

    { /* APPEND new TSL (time series log) */
        /* append new log to TSDB */
        struct local_log local_log_t;
        local_log_t.park = device_t.park_state;
        local_log_t.mode = device_t.park_mode;
        local_log_t.tamper = device_t.tamper_alarm;
        local_log_t.stamp = Timestamp;

        struct local_log *info = &local_log_t;
        fdb_tsl_append(tsdb, fdb_blob_make(&blob, &local_log_t, sizeof(local_log_t)));
#if NORFLASH_DEBUG_PRINTF
        INFO("\n============================ Write Log Table =============================");
        INFO("\n| %-*.*s | %-*.*s | %-*.*s | %-*.*s |", LOG_NAME_LEN, strlen(log_name[1]), log_name[1],
             LOG_NAME_LEN, strlen(log_name[2]), log_name[2],
             LOG_NAME_LEN, strlen(log_name[3]), log_name[3],
             LOG_NAME_LEN, strlen(log_name[4]), log_name[4]);
        INFO("\n| %-*.*d | %-*.*d | %-*.*d | %-*.*d |", LOG_NAME_LEN, getNumLen(info->park), info->park,
             LOG_NAME_LEN, getNumLen(info->mode), info->mode,
             LOG_NAME_LEN, getNumLen(info->tamper), info->tamper,
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
}

static bool export_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct local_log local_log_t;
    fdb_tsdb_t db = arg;

    if (export_len == 0)
        INFO("\nexport sensor data start,");
    // else if(export_len == 1000)
    //{
    //	export_len = 0;
    //	INFO("export sensor data end\n");
    //	mDelay(5000);
    // }
    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &local_log_t, sizeof(local_log_t))));
    INFO("%d,%d,%d,%ld,",
         local_log_t.park, local_log_t.mode, local_log_t.tamper, local_log_t.stamp);
    export_len++;
    return false;
}

void nor_flash_read_export(fdb_tsdb_t tsdb)
{
    export_len = 0;
    { /* QUERY the TSDB */
        /* query all TSL in TSDB by iterator */
        fdb_tsl_iter(tsdb, export_cb, tsdb);
    }
    // if(export_len > 0 && export_len != 1000)
    {
        export_len = 0;
        INFO("export sensor data end\n");
    }
}

void flash_db_init(void)
{
    extern int spi_flash_init(void);
    p25q40_init();
    spi_flash_init();
    fdb_err_t result = fdb_tsdb_init(&tsdb, "offline_data", "fdb_tsdb", get_time, 128, NULL);
    /* read last saved time for simulated timestamp */
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &nor_flash_timestamp);

    while (result != FDB_NO_ERR) {
        mDelay(1000);
        INFO("\n%s Fail\n", __func__);
    }

    // fdb_tsl_clean(&tsdb);
}

void p25q40_rst(void)
{
    u8 txbuf = 0x66;
    Spi3WriteReadNByte(&txbuf, 1, NULL, 0);
    txbuf = 0x99;
    Spi3WriteReadNByte(&txbuf, 1, NULL, 0);
}

void p25q40_release(void)
{
    u8 txbuf[4] = {0};
    u8 rxbuf[2] = {0};
    txbuf[0] = RLPWD; // release power down
    Spi3WriteReadNByte(txbuf, 4, rxbuf, 1);
}

void p25q40_init(void)
{
    u8 txbuf[4] = {0};
    u8 rxbuf[2] = {0};
    txbuf[0] = RLPWD; // release power down
    Spi3WriteReadNByte(txbuf, 4, rxbuf, 1);

    txbuf[0] = RDID; // manufact device ID
    Spi3WriteReadNByte(txbuf, 4, rxbuf, 2);
    DEBUG_TRACE(LOG_TAG, "Read P25Q40 Manufact 0x%02x, ID 0x%02x", rxbuf[0], rxbuf[1]);

    // p25q40_chip_erase();
}

void p25q40_power_down(void)
{
    u8 txbuf = PWD;
    Spi3WriteReadNByte(&txbuf, 1, NULL, 0);
}

void p25q40_chip_erase(void)
{
    u8 txbuf = WREN;
    Spi3WriteReadNByte(&txbuf, 1, NULL, 0);
    txbuf = CER;
    Spi3WriteReadNByte(&txbuf, 1, NULL, 0);
}
