#include "W25X40CLSNIG.h"

#include "LTR_3XXALS_01.h"
#include "EPD.h"
#include "PMSA003.h"
#include "SEN0231.h"
#include "hp203b.h"
#include "im8601pa.h"
#include "scd4x.h"
#include "sgp4x.h"
#include "sht40x.h"
#include "spi.h"
#include <fdb_def.h>
#include <flashdb.h>

#include "control_center.h"
#include "systick.h"
#include "uart.h"

/* TSDB object */
struct fdb_tsdb tsdb = {0};

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

// 片外FLASH初始化
void SpiFlash_Init(void)
{
    extern int spi_flash_init(void);
    spi_flash_init();

    fdb_err_t result;
    /* set the lock and unlock function if you want */
    // fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, (void *)lock);
    // fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, (void *)unlock);
    /* Time series database initialization
     *
     *       &tsdb: database object
     *       "log": database name
     * "fdb_tsdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
     *              Please change to YOUR partition name.
     *    get_time: The get current timestamp function.
     *         128: maximum length of each log
     *        NULL: The user data if you need, now is empty.
     */
    result = fdb_tsdb_init(&tsdb, "offline_data", "fdb_tsdb", get_time, 128, NULL);
    /* read last saved time for simulated timestamp */
    fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &nor_flash_timestamp);

    if (result != FDB_NO_ERR) {
        while (1) {
            tx_thread_sleep(1000);
            INFO("\n%s Fail\n", __func__);
        }
    }
    // fdb_tsl_clean(&tsdb);
    // nor_flash_write(&tsdb);
}

void nor_flash_write(fdb_tsdb_t tsdb)
{
    struct fdb_blob blob;

    { /* APPEND new TSL (time series log) */
        struct sensor status = {0};

        memcpy(&status, &device_t.sensor_t, sizeof(status));
        /* append new log to TSDB */
        fdb_tsl_append(tsdb, fdb_blob_make(&blob, &status, sizeof(status)));
#if NORFLASH_DEBUG_PRINTF
        INFO("\nappend the new,   \
                            \n\tstatus.pressure (%d),\
                            \n\tstatus.tvoc (%d),status.tvoc level(%d),\
                            \n\tstatus.co2(%d),\
                            \n\tstatus.light_data(%d),status.light_level(%d),\
                            \n\tstatus.temperature(%d),\
                            \n\tstatus.humidity(%d),\
                            \n\tstatus.pir_signal(%d),\
                            \n\tstatus.atmos_pm_1_0(%d),status.atmos_pm_2_5(%d),status.atmos_pm_10_0(%d),\
                            \n\tstatus.o3_level(%d),\
                            \n\tstatus.hcho_ppm(%d),\
                            \n\ttimestamp(%d)\n",
             status.pressure, status.tvoc, device_t.sensor_t.tvoc_level, status.co2, status.light_data, status.light_level, status.temperature, status.humidity, status.pir_signal,
             status.atmos_pm_1_0, status.atmos_pm_2_5, status.atmos_pm_10_0, status.o3_level, status.hcho_ppm, status.stamp);
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
#if NORFLASH_DEBUG_PRINTF
        fdb_tsl_iter(tsdb, set_status_cb, tsdb);
#endif
        // nor_flash_read(tsdb);
    }
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
        INFO("query count is: %zu\n", count);
    }
}

static bool query_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct sensor status;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
#if NORFLASH_DEBUG_PRINTF
    INFO("[query_cb] queried a TSL: time: %ld,   \
                            \n\tstatus.pressure (%d),\
                            \n\tstatus.tvoc (%d),status.tvoc level(%d),\
                            \n\tstatus.co2(%d),\
                            \n\tstatus.light_data(%d),status.light_level(%d),\
                            \n\tstatus.temperature(%d),\
                            \n\tstatus.humidity(%d),\
                            \n\tstatus.pir_signal(%d),\
                            \n\tstatus.atmos_pm_1_0(%d),status.atmos_pm_2_5(%d),status.atmos_pm_10_0(%d),\
                            \n\tstatus.o3_level(%d),\
                            \n\tstatus.hcho_ppm(%d),\
                            \n\ttimestamp(%d)\n",
         tsl->time, status.pressure, status.tvoc, device_t.sensor_t.tvoc_level, status.co2, status.light_data, status.light_level, status.temperature, status.humidity, status.pir_signal,
         status.atmos_pm_1_0, status.atmos_pm_2_5, status.atmos_pm_10_0, status.o3_level, status.hcho_ppm, status.stamp);
#endif
    return false;
}

static bool query_by_time_cb(fdb_tsl_t tsl, void *arg)
{
    struct fdb_blob blob;
    struct sensor status;
    fdb_tsdb_t db = arg;

    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
#if NORFLASH_DEBUG_PRINTF
    INFO("[query_by_time_cb] queried a TSL: time: %ld,   \
                            \n\tstatus.pressure (%d),\
                            \n\tstatus.tvoc (%d),status.tvoc level(%d),\
                            \n\tstatus.co2(%d),\
                            \n\tstatus.light_data(%d),status.light_level(%d),\
                            \n\tstatus.temperature(%d),\
                            \n\tstatus.humidity(%d),\
                            \n\tstatus.pir_signal(%d),\
                            \n\tstatus.atmos_pm_1_0(%d),status.atmos_pm_2_5(%d),status.atmos_pm_10_0(%d),\
                            \n\tstatus.o3_level(%d),\
                            \n\tstatus.hcho_ppm(%d),\
                            \n\ttimestamp(%d)\n",
         tsl->time, status.pressure, status.tvoc, device_t.sensor_t.tvoc_level, status.co2, status.light_data, status.light_level, status.temperature, status.humidity, status.pir_signal,
         status.atmos_pm_1_0, status.atmos_pm_2_5, status.atmos_pm_10_0, status.o3_level, status.hcho_ppm, status.stamp);
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
    struct sensor status;
    fdb_tsdb_t db = arg;
    if (export_len == 0)
        INFO("\nexport sensor data start,");
    // else if(export_len == 1000)
    //{
    //	export_len = 0;
    //	INFO("export sensor data end\n");
    //	mDelay(5000);
    // }
    fdb_blob_read((fdb_db_t)db, fdb_tsl_to_blob(tsl, fdb_blob_make(&blob, &status, sizeof(status))));
    INFO("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld,",
         status.pressure, status.tvoc, device_t.sensor_t.tvoc_level, status.co2, status.light_data, status.light_level, status.temperature, status.humidity, status.pir_signal,
         status.atmos_pm_1_0, status.atmos_pm_2_5, status.atmos_pm_10_0, status.o3_level, status.hcho_ppm, status.stamp);
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
