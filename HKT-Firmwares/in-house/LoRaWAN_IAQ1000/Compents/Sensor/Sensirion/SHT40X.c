
#include "sht40x.h"

#include "sensirion_common.h"
#include "sht4x_i2c.h"

#include "control_center.h"
#include "uart.h"

/**
 * @brief	初始化温湿度传感器
 * @param
 * @retval
 */
void SHT40X_Init(void)
{
    int error = 0;
    sht4x_soft_reset();

    uint32_t serial_number;
    error = sht4x_serial_number(&serial_number);
    if (error) {
        DEBUG_TRACE(ERROR_TAG, "SHT40X Get Serial Number ERROR: %d", error);
    } else {
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
    int32_t temperature;
    int32_t humidity;

    int error = sht4x_measure_high_precision(&temperature, &humidity);
    if (error) { 
        DEBUG_TRACE(ERROR_TAG, "Error executing SHT40X Measurement High Precision: %d\n", error);
    } else {
        if (humidity > 20000 && humidity < 80000)
            humidity += 1000; // 修正湿度

        // 补偿温度
        if(temperature < 12000 && temperature >= 0){
            temperature -= 500;
        }

        device_t.sensor_t.temperature = temperature + (device_t.temperature_offset * 100);
        device_t.sensor_t.humidity = humidity;
        DEBUG_TRACE(LOG_TAG, "SHT40X Read Temperature: %0.2fC %0.2fC, Humidity: %0.2f%RH\n", temperature / 1000.0f,
                    device_t.sensor_t.temperature / 1000.0f, humidity / 1000.0f);
    }
    sht4x_soft_reset();
}

u8 SHT40X_Check(void)
{
    int error = 0;
    sht4x_soft_reset();

    uint32_t serial_number;
    error = sht4x_serial_number(&serial_number);
    if (error) {
        return 1;
    } else {
        return 0;
    }
}
