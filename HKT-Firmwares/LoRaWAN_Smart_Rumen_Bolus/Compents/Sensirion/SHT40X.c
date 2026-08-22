
#include "sht40x.h"

#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sht4x_i2c.h"
#include "sts4x_i2c.h"

#include "communicate.h"
#include "control_center.h"
#include "systick.h"
#include "uart.h"

/**
 *
 * @brief	初始化温湿度传感器
 * @param
 * @retval
 */
void SHT40X_Init(void)
{
    int error = 0;
    sensirion_i2c_hal_init();

#if STS4X_ENABLE

    init_driver(ADDR_STS4X_ALT);

    uint32_t serial_number;
    error = sts4x_serial_number(&serial_number);
    if (error) {
        DEBUG_TRACE(ERROR_TAG, "SHT40X/STS40 Get Serial Number ERROR: %d", error);
    } else {
        DEBUG_TRACE(LOG_TAG, "SHT40X/STS40 Serial Number: %d", serial_number);
    }

#else
    sht4x_soft_reset();

    uint32_t serial_number;
    error = sht4x_serial_number(&serial_number);
    if (error) {
        DEBUG_TRACE(ERROR_TAG, "SHT40X/STS40 Get Serial Number ERROR: %d", error);
    } else {
        DEBUG_TRACE(LOG_TAG, "SHT40X/STS40 Serial Number: %d", serial_number);
    }
    // readSHT40XConvertResult();

#endif
}

/**
 * @brief	获取温湿度传感器转换结果
 * @param
 * @retval
 */
void readSHT40XConvertResult(void)
{
    int32_t temperature;
    int32_t humidity;
    if (dataConvertTimestamp <= Timestamp) {
#if ZN_COM_ENABLE
        // dataConvertTimestamp = Timestamp + 2 * 60; // 2分钟检测一次
        dataConvertTimestamp = Timestamp + (1DEFAULT_REPORT_INTERVAL * 60 / 7); // 15分钟上报一次 采样7个温度数据
#else
        // dataConvertTimestamp = Timestamp + 3 * 60; // 3分钟检测一次
        dataConvertTimestamp = Timestamp + (DEFAULT_REPORT_INTERVAL * 60 / 5); // 上报周期内采样5个温度数据
#endif

#if STS4X_ENABLE
        // Read Measurement
        int error = sts4x_measure_high_precision(&temperature);

        if (error) {
            DEBUG_TRACE(ERROR_TAG, "Error executing SHT40X/STS40 Measurement High Precision: %d\n", error);
        } else {
            if (temperature >= 0) {
                device_t.sensor_t.temperature = temperature;
            } else {
                device_t.sensor_t.temperature = (-temperature) | 0x800000;
            }
            DEBUG_TRACE(LOG_TAG, "STS40 Read Temperature: %0.2fC", temperature / 1000.0f);
        }
        sts4x_soft_reset(); // 关闭转换
#else
        // Read Measurement
        int error = sht4x_measure_high_precision(&temperature, &humidity);
        // int error = sht4x_measure_lowest_precision(&temperature, &humidity);
        if (error) {
            DEBUG_TRACE(ERROR_TAG, "Error executing SHT40X/STS40 Measurement High Precision: %d\n", error);
        } else {
            if (temperature >= 0) {
                device_t.sensor_t.temperature = temperature;
            } else {
                device_t.sensor_t.temperature = (-temperature) | 0x800000;
            }
            device_t.sensor_t.humidity = humidity;
            DEBUG_TRACE(LOG_TAG, "SHT40X Read Temperature: %0.2fC, Humidity: %0.2f%RH", temperature / 1000.0f, humidity / 1000.0f);
        }
        sht4x_soft_reset(); // 关闭转换
#endif

        uint32_t temp_t = device_t.sensor_t.temperature;
#if TEMP_POWER_TEST
        temp_t = 30000;
#endif
        if (temp_t >= 24000 && temp_t < 0x800000 && device_t.reportInterval == 240) {
            device_t.reportInterval = DEFAULT_REPORT_INTERVAL;
            dataReportTimestamp = Timestamp;
        } else if ((temp_t < 24000 || temp_t >= 0x800000)) {
            if (device_t.reportInterval == DEFAULT_REPORT_INTERVAL) {
								if(DEFAULT_REPORT_INTERVAL != 1){  // 测试使用
									device_t.reportInterval = 240;
								}
            }
            // device_t.temperature_group = 0;
        }

        if (temperature >= 0) {
            device_t.temperature[device_t.temperature_group++] = temperature / 10;
        } else {
            device_t.temperature[device_t.temperature_group++] = (-temperature / 10) | 0x8000;
        }

#if ZN_COM_ENABLE
        if (device_t.temperature_group >= 7) {
            if (device_t.reportInterval == 240) {
                device_t.temperature_group = 0;
            } else {
                device_t.temperature_group = 7;
            }
        }
#else
        if (device_t.temperature_group >= 5) {
            if (device_t.reportInterval == 240) {
                device_t.temperature_group = 0;
            } else {
                device_t.temperature_group = 5;
            }
        }
#endif
    }
}
