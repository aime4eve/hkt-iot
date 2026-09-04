
#include "sgp4x.h"

#include "sgp40_i2c.h"
#include "sensirion_common.h"
#include "sensirion_voc_algorithm.h"

#include "control_center.h"
#include "uart.h"
#include "systick.h"

static VocAlgorithmParams voc_algorithm_params;

/**
 * @brief	初始化VOC传感器
 * @param
 * @retval
 */
void SGP4X_Init(void)
{
    int16_t error = 0;
    VocAlgorithm_init(&voc_algorithm_params);

    uint16_t serial_number[3];
    uint8_t serial_number_size = 3;

    error = sgp40_get_serial_number(serial_number, serial_number_size);
    if (error)
    {
        DEBUG_TRACE(ERROR_TAG, "SGP4X Get Serial Number ERROR: %i", error);
    }
    else
    {
        DEBUG_TRACE(LOG_TAG, "SGP4X Serial Number: 0x%04x%04x%04x", serial_number[0], serial_number[1], serial_number[2]);
    }
    readSGP4XConvertResult();
#if 0
    uint16_t default_rh = 0x8000;
    uint16_t default_t = 0x6666;
    int32_t voc_index;
    for (int i = 0; i < 100; i++)  //采样数据
    {
        uint16_t sraw_voc;
        error = sgp40_measure_raw_signal(default_rh, default_t, &sraw_voc);
        if (error)
        {
            DEBUG_TRACE(ERROR_TAG, "Error executing SGP4X Measurement Raw Signal: %i", error);
        }
        else
        {
            // printf("SRAW VOC: %u\n", sraw_voc);
            VocAlgorithm_process(&voc_algorithm_params, sraw_voc, &voc_index);
            device_t.sensor_t.tvoc = voc_index;
            // 根据 AQI 空气质量指数划分,按等级0-5上报：
            // 0 级：0-50
            // 1 级：51-100
            // 2 级：101-150
            // 3 级：151-200
            // 4 级：201-250
            // 5 级：251-500
            if (device_t.sensor_t.tvoc <= 50)
            {
                device_t.sensor_t.tvoc_level = 0;
            }
            else if (device_t.sensor_t.tvoc <= 100)
            {
                device_t.sensor_t.tvoc_level = 1;
            }
            else if (device_t.sensor_t.tvoc <= 150)
            {
                device_t.sensor_t.tvoc_level = 2;
            }
            else if (device_t.sensor_t.tvoc <= 200)
            {
                device_t.sensor_t.tvoc_level = 3;
            }
            else if (device_t.sensor_t.tvoc <= 250)
            {
                device_t.sensor_t.tvoc_level = 4;
            }
            else
            {
                device_t.sensor_t.tvoc_level = 5;
            }
            DEBUG_TRACE(LOG_TAG, "SGP4X Read SRAW VOC: %u, VOC: %d, Levle: %d", sraw_voc, voc_index, device_t.sensor_t.tvoc_level);
        }
        tx_thread_sleep(1000);
    }
#endif
}

/**
 * @brief	获取VOC传感器转换结果
 * @param
 * @retval
 */
void readSGP4XConvertResult(void)
{
    // Parameters for deactivated humidity compensation:
    int16_t error = 0;
    int32_t voc_index;
    //  for (int i = 0; i < 60; i++)
    {
        uint16_t sraw_voc;
        error = sgp40_measure_raw_signal(device_t.sensor_t.humidity, device_t.sensor_t.temperature, &sraw_voc);
        if (error)
        {
            DEBUG_TRACE(ERROR_TAG, "Error executing SGP4X Measurement Raw Signal: %i", error);
        }
        else
        {
            // printf("SRAW VOC: %u\n", sraw_voc);
            VocAlgorithm_process(&voc_algorithm_params, sraw_voc, &voc_index);
            device_t.sensor_t.tvoc = voc_index;
            // 根据 AQI 空气质量指数划分,按等级0-5上报：
            // 0 级：0-50
            // 1 级：51-100
            // 2 级：101-150
            // 3 级：151-200
            // 4 级：201-250
            // 5 级：251-500
            if (device_t.sensor_t.tvoc <= 50)
            {
                device_t.sensor_t.tvoc_level = 0;
            }
            else if (device_t.sensor_t.tvoc <= 100)
            {
                device_t.sensor_t.tvoc_level = 1;
            }
            else if (device_t.sensor_t.tvoc <= 150)
            {
                device_t.sensor_t.tvoc_level = 2;
            }
            else if (device_t.sensor_t.tvoc <= 200)
            {
                device_t.sensor_t.tvoc_level = 3;
            }
            else if (device_t.sensor_t.tvoc <= 250)
            {
                device_t.sensor_t.tvoc_level = 4;
            }
            else
            {
                device_t.sensor_t.tvoc_level = 5;
            }
            DEBUG_TRACE(LOG_TAG, "SGP4X Read SRAW VOC: %u, VOC: %d, Level: %d", sraw_voc, voc_index, device_t.sensor_t.tvoc_level);
        }
    }
}



u8 SGP4X_Check(void)
{
    int16_t error = 0;
    uint16_t serial_number[3];
    uint8_t serial_number_size = 3;
    error = sgp40_get_serial_number(serial_number, serial_number_size);
    if (error)
    {
        return 1;
    }
    return 0;
}
