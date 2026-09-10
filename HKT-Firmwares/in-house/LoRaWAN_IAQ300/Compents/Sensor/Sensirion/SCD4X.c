
#include "scd4x.h"

#include "scd4x_i2c.h"
#include "sensirion_common.h"

#include "control_center.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

/*
1、350～450ppm：同一般室外环境。 　　
2、350～1000ppm：空气清新，呼吸顺畅。 　　
3、1000～2000ppm：感觉空气浑浊，并开始觉得昏昏欲睡。 　　
4、2000～5000ppm：感觉头痛、嗜睡、呆滞、注意力无法集中、心跳加速、轻度恶心。
5、大于5000ppm：可能导致严重缺氧，造成永久性脑损伤、昏迷、甚至死亡。
*/

void SCD4X_DeInit(void)
{
    scd4x_stop_periodic_measurement();
    scd4x_reinit();
    scd4x_power_down();
}

void SCD4X_ReStart(void)
{
    CO2_PWR_CTRL_H;
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();
#if FUNC_PM2_5
    // int error = scd4x_start_periodic_measurement();
    // if (error) {
    //     DEBUG_TRACE(ERROR_TAG, "SCD4X Start Periodic Measurement ERROR: %i", error);
    // }
#endif
}

void SCD4X_Start_Periodic(void)
{
    CO2_PWR_CTRL_H;
    mDelay(1000);
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();
    int error = scd4x_start_periodic_measurement();
    if (error) {
        DEBUG_TRACE(ERROR_TAG, "SCD4X Start Periodic Measurement ERROR: %i", error);
    }
}

void SCD4X_Read_Periodic_Data(u8 save)
{
    bool     data_ready_flag = 0;
    int      i               = 0;
    int32_t  temperature = 0, humidity = 0;
    int16_t  error;
    uint16_t co2;
    error = scd4x_get_data_ready_flag(&data_ready_flag);
    if (error) {
        DEBUG_TRACE(ERROR_TAG, "SCD4X Get Data Ready Flag ERROR: %i", error);
        return;
    }

    error = scd4x_read_measurement(&co2, &temperature, &humidity);
    if (error) {
        DEBUG_TRACE(ERROR_TAG, "Error executing SCD4X Read Measurement: %i", error);
    } else {
        DEBUG_TRACE(LOG_TAG, "SCD4X Read CO2: %u", co2);
    }

    if (!save)
        return;

    if (co2 == 0) {
        co2 = device_t.sensor_t.co2; // 读取失败使用上一次的值
        DEBUG_TRACE(ERROR_TAG, "CO2 Invalid sample detected, skipping.");
        goto __co_histery;
    } else {
    __co_histery:
        device_t.sensor_t.co2 = co2;
        for (i = 0; i < 36; i++) {
            if (device_t.co2_history_buf[i] == 0)
                break;
        }
        if (i < 36) {
            device_t.co2_history_buf[i] = co2;
        } else {
            for (i = 0; i < 35; i++) {
                device_t.co2_history_buf[i] = device_t.co2_history_buf[i + 1];
            }
            device_t.co2_history_buf[35] = co2;
        }
        DEBUG_TRACE(LOG_TAG, "SCD4X Read CO2: %u, Temperature: %.2fC, Humidity: %.2fRH", co2, temperature / 1000.0f, humidity / 1000.0f);
    }
}

bool SCD4X_ForcedPPM(u16 forced)
{
    u16 ppm = 0;
    // SCD4X_ReStart();
    scd4x_perform_forced_recalibration(forced, &ppm);
    DEBUG_TRACE(LOG_TAG, "SCD4X ForcedPPM Write: %u, Read: %d", forced, ppm - 0x8000);
    if (ppm != 0xFFFF)
        return true;
    else
        return false;
}

/**
 * @brief	初始化二氧化碳传感器
 * @param
 * @retval
 */
void SCD4X_Init(void)
{
    bool     data_ready_flag = 0;
    int      i               = 0;
    int32_t  temperature = 0, humidity = 0;
    int16_t  error;
    uint16_t co2;

    CO2_PWR_CTRL_H;
    tx_thread_sleep(20);

    // Clean up potential SCD40 states
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();

    u16 serial_0;
    u16 serial_1;
    u16 serial_2;
    error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
    if (error) {
        DEBUG_TRACE(ERROR_TAG, "SCD4X Get Serial Number ERROR: %i", error);
    } else {
        DEBUG_TRACE(LOG_TAG, "SCD4X Serial: 0x%04x%04x%04x", serial_0, serial_1, serial_2);
    }

    // #if FUNC_PM2_5
    //     error = scd4x_start_periodic_measurement();
    //     if (error) {
    //         DEBUG_TRACE(ERROR_TAG, "SCD4X Start Periodic Measurement ERROR: %i", error);
    //     }
    // #endif
    // 丢掉首次开机读取的2个数据
    for (i = 0; i < 1; i++) {
        if (device_t.sleepDelay < 20)
            device_t.sleepDelay = 20;
        // tx_thread_sleep(5000);
        error = scd4x_measure_single_shot();
        if (error) {
            DEBUG_TRACE(ERROR_TAG, "SCD4X Start Single Measurement ERROR: %i", error);
        } else {
            error = scd4x_get_data_ready_flag(&data_ready_flag);
            if (error) {
                DEBUG_TRACE(ERROR_TAG, "SCD4X Get Data Ready Flag ERROR: %i", error);
            } else {
                if (data_ready_flag != true) {
                    DEBUG_TRACE(WARN_TAG, "SCD4X Get Data Not Ready: %d", data_ready_flag);
                    tx_thread_sleep(1000);
                    scd4x_get_data_ready_flag(&data_ready_flag);
                    DEBUG_TRACE(WARN_TAG, "SCD4X Get Data Not Ready: %d", data_ready_flag);
                    if (data_ready_flag == true) {
                        error = scd4x_read_measurement(&co2, &temperature, &humidity);
                        if (error) {
                            DEBUG_TRACE(ERROR_TAG, "Error executing SCD4X Read Measurement: %i", error);
                        } else {
                            DEBUG_TRACE(LOG_TAG, "SCD4X Read CO2: %u, Temperature: %.2fC, Humidity: %.2fRH", co2, temperature / 1000.0f, humidity / 1000.0f);
                        }
                    }
                } else {
                    error = scd4x_read_measurement(&co2, &temperature, &humidity);
                    if (error) {
                        DEBUG_TRACE(ERROR_TAG, "Error executing SCD4X Read Measurement: %i", error);
                    } else {
                        DEBUG_TRACE(LOG_TAG, "SCD4X Read CO2: %u, Temperature: %.2fC, Humidity: %.2fRH", co2, temperature / 1000.0f, humidity / 1000.0f);
                    }
                }
            }
        }
    }
}

/**
 * @brief	获取二氧化碳传感器转换结果
 * @param
 * @retval
 */
void readSCD4XConvertResult(u8 save)
{
    bool     data_ready_flag = 0;
    int      i               = 0;
    int32_t  temperature = 0, humidity = 0;
    int16_t  error;
    uint16_t co2;

    SCD4X_ReStart();
    // Start Measurement
    for (i = 0; i < 1; i++) {
        error = scd4x_measure_single_shot();
        if (error) {
            DEBUG_TRACE(ERROR_TAG, "SCD4X Start Single Measurement ERROR: %i", error);
        } else {
            error = scd4x_get_data_ready_flag(&data_ready_flag);
            if (error) {
                DEBUG_TRACE(ERROR_TAG, "SCD4X Get Data Ready Flag ERROR: %i", error);
            }
            if (data_ready_flag != true) {
                co2 = 0;
                DEBUG_TRACE(WARN_TAG, "SCD4X Get Data Not Ready: %d", data_ready_flag);
                tx_thread_sleep(1000);
                scd4x_get_data_ready_flag(&data_ready_flag);
                DEBUG_TRACE(WARN_TAG, "SCD4X Get Data Not Ready: %d", data_ready_flag);
                if (data_ready_flag == true) {
                    error = scd4x_read_measurement(&co2, &temperature, &humidity);
                    if (error) {
                        co2 = 0;
                        DEBUG_TRACE(ERROR_TAG, "Error executing SCD4X Read Measurement: %i", error);
                    } else {
                        DEBUG_TRACE(LOG_TAG, "SCD4X Read CO2: %u, Temperature: %.2fC, Humidity: %.2fRH", co2, temperature / 1000.0f, humidity / 1000.0f);
                    }
                }
            }
            error = scd4x_read_measurement(&co2, &temperature, &humidity);
            if (error) {
                co2 = 0;
                DEBUG_TRACE(ERROR_TAG, "Error executing SCD4X Read Measurement: %i", error);
            }
        }
        // tx_thread_sleep(5000);
    }

    SCD4X_DeInit();

    if(!save)
        return;

    if (co2 == 0) {
        co2 = device_t.sensor_t.co2; // 读取失败使用上一次的值
        DEBUG_TRACE(ERROR_TAG, "CO2 Invalid sample detected, skipping.");
        goto __co_histery;
    } else {
    __co_histery:
        device_t.sensor_t.co2 = co2;
        for (i = 0; i < 36; i++) {
            if (device_t.co2_history_buf[i] == 0)
                break;
        }
        if (i < 36) {
            device_t.co2_history_buf[i] = co2;
        } else {
            for (i = 0; i < 35; i++) {
                device_t.co2_history_buf[i] = device_t.co2_history_buf[i + 1];
            }
            device_t.co2_history_buf[35] = co2;
        }
        DEBUG_TRACE(LOG_TAG, "SCD4X Read CO2: %u, Temperature: %.2fC, Humidity: %.2fRH", co2, temperature / 1000.0f, humidity / 1000.0f);
    }
}

u8 SCD4X_Check(void)
{
    int error = 0;
    CO2_PWR_CTRL_H;
    tx_thread_sleep(20);
    // Clean up potential SCD40 states
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();

    u16 serial_0;
    u16 serial_1;
    u16 serial_2;
    error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
    if (error) {
        return 1;
    } else {
        return 0;
    }
}
