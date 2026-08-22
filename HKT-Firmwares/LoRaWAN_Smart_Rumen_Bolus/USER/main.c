
/* Includes ------------------------------------------------------------------*/
#include "acc.h"
#include "accelerometer.h"
#include "adc.h"
#include "communicate.h"
#include "config.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "iic.h"
#include "lwrb.h"
#include "sht40x.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>

#include "bma456_common.h"
#include "bma456w.h"

uint32_t last_step;

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    Systick_Init();
    SystemClock_Config();
    TIM6_Init(1000, 4);
    App_PortCfg();
    App_Uart1Cfg();
    App_Uart2Cfg();
    I2C1_Init();
    FLASH_Read_Param();
    systemTime_Init();
    lwrbDataInit();
#if FUNC_THTB_ENABLE
    SHT40X_Init();
#endif

#if FUNC_THTB_ENABLE
#if ZN_COM_ENABLE
    for (uint8_t i = 0; i < 7; i++) {
        readSHT40XConvertResult();
    }
#else
    readSHT40XConvertResult();
#endif

#endif

#if TRACK_ACC_ENABLE

#if DA270_ACC_ENABLE
    Acc_Init();
    convertAccData();
#else
    bma456_init();
    bma456_convertData();
#endif

#endif

    ADC_Init();
    ADC_BatterySglConvert();
    LoRaWAN_Init();

    do {
        LoRaWAN_JoinInit();
    } while (!LoRaWAN.joinState);
    /* Infinite loop */
    while (1) {
        systemLowPowerMode_Config();
#if !LORAWAN_CNF_ENABLE
        LoRaWAN_JoinInit();
#endif
        DeviceStateProcess();
        fromUartDataHandle();
#if DA270_ACC_ENABLE
        Acc_Handler();
#endif
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

// /**\
//  * Copyright (c) 2022 Bosch Sensortec GmbH. All rights reserved.
//  *
//  * SPDX-License-Identifier: BSD-3-Clause
//  **/

// /******************************************************************************/
// /*!                Macro definition                                           */

// /*! Earth's gravity in m/s^2 */
// #define GRAVITY_EARTH       (9.80665f)

// /*! Macro that holds the total number of accel x,y and z axes sample counts to be printed */
// #define ACCEL_SAMPLE_COUNT  UINT8_C(100)

// /******************************************************************************/
// /*!           Static Function Declaration                                     */

// /*!
//  *  @brief This internal API converts raw sensor values(LSB) to meters per seconds square.
//  *
//  *  @param[in] val       : Raw sensor value.
//  *  @param[in] g_range   : Accel Range selected (2G, 4G, 8G, 16G).
//  *  @param[in] bit_width : Resolution of the sensor.
//  *
//  *  @return Accel values in meters per second square.
//  *
//  */
// static float lsb_to_ms2(int16_t val, float g_range, uint8_t bit_width);

// /******************************************************************************/
// /*!            Functions                                                      */

// /* This function starts the execution of program. */
// int main(void)
// {
//     /* Variable to store the status of API */
//     int8_t rslt;

//     /* Sensor initialization configuration */
//     struct bma4_dev bma = { 0 };

//     /* Variable to store accel data ready interrupt status */
//     uint16_t int_status = 0;

//     /* Variable that holds the accelerometer sample count */
//     uint8_t n_data = 1;

//     struct bma4_accel sens_data = { 0 };
//     float x = 0, y = 0, z = 0;
//     struct bma4_accel_config accel_conf = { 0 };

// 		I2C1_Init();

//     /* Interface reference is given as a parameter
//      *         For I2C : BMA4_I2C_INTF
//      *         For SPI : BMA4_SPI_INTF
//      * Variant information given as parameter - BMA45X_VARIANT
//      */
//     rslt = bma4_interface_init(&bma, BMA4_I2C_INTF, BMA45X_VARIANT);
//     bma4_error_codes_print_result("bma4_interface_init", rslt);

//     /* Sensor initialization */
//     rslt = bma456w_init(&bma);
//     bma4_error_codes_print_result("bma456w_init status", rslt);

//     // /* Upload the configuration file to enable the features of the sensor. */
//     // rslt = bma456w_write_config_file(&bma);
//     // bma4_error_codes_print_result("bma456w_write_config status", rslt);

//     /* Accelerometer configuration settings */
//     /* Output data Rate */
//     accel_conf.odr = BMA4_OUTPUT_DATA_RATE_50HZ;

//     /* Gravity range of the sensor (+/- 2G, 4G, 8G, 16G) */
//     accel_conf.range = BMA4_ACCEL_RANGE_2G;

//     /* The bandwidth parameter is used to configure the number of sensor samples that are averaged
//      * if it is set to 2, then 2^(bandwidth parameter) samples
//      * are averaged, resulting in 4 averaged samples
//      * Note1 : For more information, refer the datasheet.
//      * Note2 : A higher number of averaged samples will result in a less noisier signal, but
//      * this has an adverse effect on the power consumed.
//      */
//     accel_conf.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

//     /* Enable the filter performance mode where averaging of samples
//      * will be done based on above set bandwidth and ODR.
//      * There are two modes
//      *  0 -> Averaging samples (Default)
//      *  1 -> No averaging
//      * For more info on No Averaging mode refer datasheet.
//      */
//     accel_conf.perf_mode = BMA4_CIC_AVG_MODE;

//     /* Set the accel configurations */
//     rslt = bma4_set_accel_config(&accel_conf, &bma);
//     bma4_error_codes_print_result("bma4_set_accel_config status", rslt);

//     /* NOTE : Enable accel after set of configurations */
//     rslt = bma4_set_accel_enable(1, &bma);
//     bma4_error_codes_print_result("bma4_set_accel_enable status", rslt);

//     /* Mapping data ready interrupt with interrupt pin 1 to get interrupt status once getting new accel data */
//     rslt = bma456w_map_interrupt(BMA4_INTR1_MAP, BMA4_DATA_RDY_INT, BMA4_ENABLE, &bma);
//     bma4_error_codes_print_result("bma456w_map_interrupt status", rslt);

//     INFO("Data, Acc_Raw_X, Acc_Raw_Y, Acc_Raw_Z, Acc_ms2_X, Acc_ms2_Y, Acc_ms2_Z\n");

//     for (;;)
//     {
//         /* Read interrupt status */
//         rslt = bma456w_read_int_status(&int_status, &bma);
//         bma4_error_codes_print_result("bma456w_read_int_status", rslt);

//         /* Filtering only the accel data ready interrupt */
//         if ((rslt == BMA4_OK) && (int_status & BMA4_ACCEL_DATA_RDY_INT))
//         {
//             /* Read the accel x, y, z data */
//             rslt = bma4_read_accel_xyz(&sens_data, &bma);
//             bma4_error_codes_print_result("bma4_read_accel_xyz status", rslt);

//             if (rslt == BMA4_OK)
//             {

//                 /* Converting lsb to meter per second squared for 16 bit resolution at 2G range */
//                 x = lsb_to_ms2(sens_data.x, (float)2, bma.resolution);
//                 y = lsb_to_ms2(sens_data.y, (float)2, bma.resolution);
//                 z = lsb_to_ms2(sens_data.z, (float)2, bma.resolution);

//                 /* Print the data in m/s2 */
//                 INFO("%d, %d, %d, %d, %4.2f, %4.2f, %4.2f\n", n_data, sens_data.x, sens_data.y, sens_data.z, x, y, z);
//             }

//             /* Increment the count that determines the number of samples to be printed */
//             n_data++;

//             /* When the count reaches more than ACCEL_SAMPLE_COUNT, break and exit the loop */
//             if (n_data > ACCEL_SAMPLE_COUNT)
//             {
//                 break;
//             }
//         }
//     }
//     return rslt;
// }

// /*!
//  * @brief This internal API converts raw sensor values(LSB) to meters per seconds square.
//  */
// static float lsb_to_ms2(int16_t val, float g_range, uint8_t bit_width)
// {
//     double power = 2;

//     float half_scale = (float)((pow((double)power, (double)bit_width) / 2.0f));

//     return (GRAVITY_EARTH * val * g_range) / half_scale;
// }
