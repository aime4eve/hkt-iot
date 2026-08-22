
#include "sht40x.h"

#include "sht4x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

#include "control_center.h"
#include "uart.h"
#include "systick.h"

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
    sht4x_soft_reset();

    uint32_t serial_number;
    error = sht4x_serial_number(&serial_number);
    if (error)
    {
        DEBUG_TRACE(ERROR_TAG, "SHT40X Get Serial Number ERROR: %d", error);
    }
    else
    {
        DEBUG_TRACE(LOG_TAG, "SHT40X Serial Number: %d", serial_number);
    }
    readSHT40XConvertResult();
}

/**
 * @brief	获取温湿度传感器转换结果
 * @param
 * @retval
 */
void readSHT40XConvertResult(void)
{
    static time_t stamp;
    int32_t temperature;
    int32_t humidity;
    if (stamp <= Timestamp)
    {
        stamp = Timestamp + 180; // 180秒检测一次
        // Read Measurement
        int error = sht4x_measure_high_precision(&temperature, &humidity);
        // int error = sht4x_measure_lowest_precision(&temperature, &humidity);
        if (error)
        {
            DEBUG_TRACE(ERROR_TAG, "Error executing SHT40X Measurement High Precision: %d\n", error);
        }
        else
        {
            if (temperature >= 0)
                device_t.sensor_t.temperature = temperature;
            else
                device_t.sensor_t.temperature = (-temperature) | 0x800000;
            device_t.sensor_t.humidity = humidity;
            DEBUG_TRACE(LOG_TAG, "SHT40X Read Temperature: %0.2fC, Humidity: %0.2f%RH", temperature / 1000.0f, humidity / 1000.0f);
        }
        // error = sht4x_measure_high_precision_ticks(&temperature, &humidity);
        // if (error)
        // {
        //     DEBUG_TRACE(ERROR_TAG, "Error executing SHT40X Measurement High Precision Ticks: %i\n", error);
        // }
        // else
        // {
        //     INFO("\n");
        //     DEBUG_TRACE(LOG_TAG, "SHT40X Read Temperature: %0.2f °C, Humidity: %0.2f %%RH\n", temperature, humidity);
        // }

        sht4x_soft_reset(); // 关闭转换
    }
}
