
#include "LTR_3XXALS_01.h"
#include "iic.h"
#include "control_center.h"
#include "uart.h"
#include "systick.h"
#include "math.h"

#include "sensirion_i2c.h"
#include "sensirion_common.h"
#include "sensirion_config.h"
#include "sensirion_i2c_hal.h"

const static u8 gain_values[] = {1, 2, 4, 8, 0, 0, 48, 96};
const static float int_values[] = {1, 0.5, 2, 4, 1.5, 2.5, 3, 3.5};
static u32 ALS_Lux = 0;
static u16 ALS_CH1_ADC_Data;
static u16 ALS_CH0_ADC_Data;

u8 ltr3xx_serial_number(u8 *serial_number)
{
   // u8 error;
    u8 buffer[6];
    u16 offset = 0;
    buffer[offset++] = (uint8_t)LTR3XX_PART_ID;

    sensirion_i2c_hal_write_read(LTR3XX_I2C_DEV_ID, buffer, offset, buffer, 1);

    //    error = sensirion_i2c_hal_write(LTR3XX_I2C_DEV_ID, &buffer[0], offset);
    //    if (error)
    //    {
    //        return error;
    //    }
    //    sensirion_i2c_hal_sleep_usec(10000);
    //    error = sensirion_i2c_hal_read(LTR3XX_I2C_DEV_ID, &buffer[0], 1);
    //    if (error)
    //    {
    //        return error;
    //    }
    *serial_number = buffer[0];
    return NO_ERROR;
}

u8 ltr3xx_start_periodic_measurement()
{
    u8 error;
    u8 buffer[2];
    u16 offset = 0;
    offset = sensirion_i2c_add_command_to_buffer(&buffer[0], offset, LTR3XX_ALS_CONTR << 8 | 0x01);
    error = sensirion_i2c_write_data(LTR3XX_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(1000);
    return NO_ERROR;
}

u8 ltr3xx_stop_periodic_measurement()
{
    u8 error;
    u8 buffer[2];
    u16 offset = 0;
    offset = sensirion_i2c_add_command_to_buffer(&buffer[0], offset, LTR3XX_ALS_CONTR << 8 | 0x00);
    error = sensirion_i2c_write_data(LTR3XX_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(1000);
    return NO_ERROR;
}

u8 ltr3xx_get_data_ready_flag(u8 *status)
{
    u8 error;
    u8 buffer[6];
    u16 offset = 0;
    buffer[offset++] = (uint8_t)LTR3XX_ALS_STATUS;

    error = sensirion_i2c_hal_write(LTR3XX_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000);
    error = sensirion_i2c_hal_read(LTR3XX_I2C_DEV_ID, &buffer[0], 1);
    if (error)
    {
        return error;
    }
    *status = buffer[0];
    return NO_ERROR;
}

u8 ltr3xx_get_measurement_rate(u8 *measurement)
{
    u8 error;
    u8 buffer[6];
    u16 offset = 0;
    buffer[offset++] = (uint8_t)LTR303_MEAS_RATE;

    error = sensirion_i2c_hal_write(LTR3XX_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000);
    error = sensirion_i2c_hal_read(LTR3XX_I2C_DEV_ID, &buffer[0], 1);
    if (error)
    {
        return error;
    }
    *measurement = buffer[0];
    return NO_ERROR;
}

u8 ltr3xx_get_data(u8 *data, u8 reg)
{
    u8 error;
    u8 buffer[6];
    u16 offset = 0;
    buffer[offset++] = (uint8_t)reg;

    error = sensirion_i2c_hal_write(LTR3XX_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000);
    error = sensirion_i2c_hal_read(LTR3XX_I2C_DEV_ID, &buffer[0], 1);
    if (error)
    {
        return error;
    }
    *data = buffer[0];
    return NO_ERROR;
}

void lightConvertLux(u8 integrationTime, u8 gain)
{
    if ((ALS_CH0_ADC_Data == 0xFFFF) || (ALS_CH1_ADC_Data == 0xFFFF))
    {
        ALS_Lux = 0;
        return;
    }
    u8 gain_x = gain_values[gain];
    float int_ms = int_values[integrationTime];
    // float ratio = (double)ALS_CH1_ADC_Data / (ALS_CH0_ADC_Data + ALS_CH1_ADC_Data);

    // if (ratio < 0.45)
    //     ALS_Lux = (1.7743 * ALS_CH0_ADC_Data + 1.1059 * ALS_CH1_ADC_Data) / gain_x / int_ms;
    // else if (ratio < 0.64)
    //     ALS_Lux = (4.2785 * ALS_CH0_ADC_Data - 1.9548 * ALS_CH1_ADC_Data) / gain_x / int_ms;
    // else if (ratio < 0.85)
    //     ALS_Lux = (0.5926 * ALS_CH0_ADC_Data + 0.1185 * ALS_CH1_ADC_Data) / gain_x / int_ms;
    // else
    //     ALS_Lux = 0;
    u32 ratio = ALS_CH1_ADC_Data * 100 / (ALS_CH0_ADC_Data + ALS_CH1_ADC_Data);

    if (ratio < 45)
        ALS_Lux = (17743 * ALS_CH0_ADC_Data + 11059 * ALS_CH1_ADC_Data) / gain_x / int_ms / 10000;
    else if (ratio < 64)
        ALS_Lux = (42785 * ALS_CH0_ADC_Data - 19548 * ALS_CH1_ADC_Data) / gain_x / int_ms / 10000;
    else if (ratio < 85)
        ALS_Lux = (5926 * ALS_CH0_ADC_Data + 1185 * ALS_CH1_ADC_Data) / gain_x / int_ms / 10000;
    else
        ALS_Lux = 0;
}

/**
 * @brief	获取光照强度
 * @param
 * @retval
 */
void readLightIntensity(void)
{
    static time_t stamp;
    u8 status, measurement;
    u8 buffer[4];

    if (stamp <= Timestamp)
    {
        stamp = Timestamp + 60;                       // 30秒检测一次防拆状态
        u8 ret = ltr3xx_start_periodic_measurement(); // Active mode
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "ltr3xx_start_periodic_measurement Fail");
            return;
        }
        mDelay(100);
        ret = ltr3xx_get_data_ready_flag(&status);
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "ltr3xx_get_data_ready_flag Fail");
            return;
        }
        ret = ltr3xx_get_measurement_rate(&measurement);
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "ltr3xx_get_measurement_rate Fail");
            return;
        }

        // Extract gain
        u8 gain = (status & 0x70) >> 4; // If gain = 0, device is set to 1X gain (default)
                                        // If gain = 1, device is set to 2X gain
                                        // If gain = 2, device is set to 4X gain
                                        // If gain = 3, device is set to 8X gain
                                        // If gain = 4, invalid
                                        // If gain = 5, invalid
                                        // If gain = 6, device is set to 48X gain
                                        // If gain = 7, device is set to 96X gain
        // // Extract interrupt status
        // intrStatus = (status & 0x08) ? true : false;

        // // Extract data status
        // dataStatus = (status & 0x04) ? true : false;

        // Extract integration Time
        u8 integrationTime = (measurement & 0x38) >> 3;

        // Extract measurement Rate
//        u8 measurementRate = measurement & 0x07; // Default value is 0x03
                                                 // If integrationTime = 0, integrationTime will be 100ms (default)
                                                 // If integrationTime = 1, integrationTime will be 50ms
                                                 // If integrationTime = 2, integrationTime will be 200ms
                                                 // If integrationTime = 3, integrationTime will be 400ms
                                                 // If integrationTime = 4, integrationTime will be 150ms
                                                 // If integrationTime = 5, integrationTime will be 250ms
                                                 // If integrationTime = 6, integrationTime will be 300ms
                                                 // If integrationTime = 7, integrationTime will be 350ms
                                                 //------------------------------------------------------
                                                 // If measurementRate = 0, measurementRate will be 50ms
                                                 // If measurementRate = 1, measurementRate will be 100ms
                                                 // If measurementRate = 2, measurementRate will be 200ms
                                                 // If measurementRate = 3, measurementRate will be 500ms (default)
                                                 // If measurementRate = 4, measurementRate will be 1000ms
                                                 // If measurementRate = 5, measurementRate will be 2000ms
                                                 // If measurementRate = 6, measurementRate will be 2000ms
                                                 // If measurementRate = 7, measurementRate will be 2000ms

        if ((status & 0x80) == 0) //已获得新数据且数据有效
        {
            ltr3xx_get_data(&buffer[0], LTR3XX_ALS_DATA_CH1_0);
            ltr3xx_get_data(&buffer[1], LTR3XX_ALS_DATA_CH1_1);

            ltr3xx_get_data(&buffer[2], LTR3XX_ALS_DATA_CH0_0);
            ltr3xx_get_data(&buffer[3], LTR3XX_ALS_DATA_CH0_1);

            ALS_CH1_ADC_Data = buffer[0] << 8 | buffer[1];
            ALS_CH0_ADC_Data = buffer[2] << 8 | buffer[3];

            lightConvertLux(integrationTime, gain);
#if LIGHT_DEBUG_PRINTF
            DEBUG_TRACE(LOG_TAG, "%s Success, ALS_CH1_ADC_Data: %d, ALS_CH0_ADC_Data: %d, Convert ALS LUX: %d ", __func__, ALS_CH1_ADC_Data, ALS_CH0_ADC_Data, ALS_Lux);
#endif
            device_t.sensor_t.light_data = ALS_Lux / 100;
            if (device_t.sensor_t.light_data < 5)
            {
                device_t.sensor_t.light_level = 0;
            }
            else if (device_t.sensor_t.light_data < 50)
            {
                device_t.sensor_t.light_level = 1;
            }
            else if (device_t.sensor_t.light_data < 100)
            {
                device_t.sensor_t.light_level = 2;
            }
            else if (device_t.sensor_t.light_data < 500)
            {
                device_t.sensor_t.light_level = 3;
            }
            else if (device_t.sensor_t.light_data < 2000)
            {
                device_t.sensor_t.light_level = 4;
            }
            else
            {
                device_t.sensor_t.light_level = 5;
            }
            DEBUG_TRACE(LOG_TAG, "Light Level: %d, Light Value: %d lux", device_t.sensor_t.light_level, device_t.sensor_t.light_data);
        }
        else
        {
            DEBUG_TRACE(ERROR_TAG, "%s Fail", __func__);
        }
        ret = ltr3xx_stop_periodic_measurement(); //  Standby mode
        if (ret)
        {
            DEBUG_TRACE(ERROR_TAG, "ltr3xx_stop_periodic_measurement Fail");
            return;
        }
        if (device_t.sensor_t.light_level > 1 && !device_t.sensor_t.tamper_status)
        {
            device_t.tamperAlarm = 1;
            device_t.sensor_t.tamper_status = 1;
        }
        else if (device_t.sensor_t.light_level <= 1 && device_t.sensor_t.tamper_status)
        {
            device_t.tamperAlarm = 1;
            device_t.sensor_t.tamper_status = 0;
        }
    }
}

/**
 * @brief	初始化环境光传感器
 * @param
 * @retval
 */
void LightIntensity_Init(void)
{
    mDelay(100);
    u8 serial_number = 0;
    u8 erro = ltr3xx_serial_number(&serial_number);
    if (erro)
    {
        DEBUG_TRACE(ERROR_TAG, "LTR3XX Get Serial Number ERROR: %d", serial_number);
    }
    else
    {
        DEBUG_TRACE(LOG_TAG, "LTR3XX Serial Number: %d", serial_number);
    }
    // readLightIntensity();
}

u8 LightIntensity_Check(void)
{
    mDelay(100);
    u8 serial_number = 0;
    u8 erro = ltr3xx_serial_number(&serial_number);
    if (erro)
    {
         return 1;
    }
    return 0;
}
