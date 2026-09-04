
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "acc.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "rtc.h"
#include "systick.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

/**
 * @brief  FLASH 字节写入
 * @param   Address：待写入FLASH首地址
 * @param   Data：待写入源数据
 * @param   Len：待写入数据长度
 * @retval  写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_NDoubleWord(u32 Address, uint64_t *Data, int Len)
{
    HAL_StatusTypeDef status = HAL_OK;
    u32 writeADDR = Address;
    for (int i = 0; i < Len; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, writeADDR, Data[i]);
        if (status != HAL_OK || *(__IO uint64_t *)writeADDR != Data[i])
            return ERROR;
        writeADDR = writeADDR + 8;
    }
    return SUCCESS;
}

/**
 * @brief  FLASH 字节读取
 * @param   Address：待读取FLASH首地址
 * @param   Data：待读取源数据
 * @param   Len：待读取数据长度
 * @retval
 */
void FLASH_Read_NDoubleWord(u32 Address, uint64_t *Data, int Len)
{
    u32 readADDR = Address;
    for (int i = 0; i < Len; i++) {
        Data[i] = *(__IO uint64_t *)readADDR;
        readADDR = readADDR + 8;
    }
}

/**
 * @brief  写入上报周期（分钟）
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Report_Interval(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_REPORT_INTERVAL_ADDR;
    uint64_t buffer = device_t.report_interval;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, &buffer, 1);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取上报周期（分钟）
 * @param
 * @retval
 */
void FLASH_Read_Report_Interval(void)
{
    uint64_t buffer;
    u32 Address = FLASH_REPORT_INTERVAL_ADDR;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    device_t.report_interval = buffer;
    if (device_t.report_interval > 1440 || device_t.report_interval < 1) // 数据出错，超出范围
        device_t.report_interval = DEFAULT_REPORT_INTERVAL;                                  // 默认间隔30分钟
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval : %dmin", device_t.report_interval);
}

/**
 * @brief  写入GPS定位周期
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Gps_Period(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_GPS_PERIOD_ADDR;
    uint64_t buffer = device_t.gps_convert_interval;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, &buffer, 1);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取GPS定位周期
 * @param
 * @retval
 */
void FLASH_Read_Gps_Period(void)
{
    uint64_t buffer;
    u32 Address = FLASH_GPS_PERIOD_ADDR;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    device_t.gps_convert_interval = buffer;
    if ((device_t.gps_convert_interval > 1440 || (device_t.gps_convert_interval < 10) && device_t.gps_convert_interval != 0))
        device_t.gps_convert_interval = 1440;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Gps Period: %dmin", device_t.gps_convert_interval);
}

/**
 * @brief  写入加速计基准参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Acc_Reference(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_ACC_REFERENCE_ADDR;
    uint64_t buffer[10] = {0};

    buffer[0] = acc_reference_t.offset.x * 1000;
    buffer[1] = acc_reference_t.offset.y * 1000;
    buffer[2] = acc_reference_t.offset.z * 1000;

    buffer[3] = acc_reference_t.scale.x * 1000;
    buffer[4] = acc_reference_t.scale.y * 1000;
    buffer[5] = acc_reference_t.scale.z * 1000;

    buffer[6] = acc_reference_t.roll * 1000;
    buffer[7] = acc_reference_t.pitch * 1000;

    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取加速计基准参数
 * @param
 * @retval
 */
void FLASH_Read_Acc_Reference(void)
{
    uint64_t buffer[10] = {0};
    u32 Address = FLASH_ACC_REFERENCE_ADDR;
    FLASH_Read_NDoubleWord(Address, buffer, 10);

    if (buffer[0] / 1000 > 1) {
        memset(&acc_reference_t, 0, sizeof(acc_reference_t));
    } else {
        acc_reference_t.offset.x = (float_t)buffer[0] / 1000;
        acc_reference_t.offset.y = (float_t)buffer[1] / 1000;
        acc_reference_t.offset.z = (float_t)buffer[2] / 1000;

        acc_reference_t.scale.x = (float_t)buffer[3] / 1000;
        acc_reference_t.scale.y = (float_t)buffer[4] / 1000;
        acc_reference_t.scale.z = (float_t)buffer[5] / 1000;

        acc_reference_t.roll = (float_t)buffer[6] / 1000;
        acc_reference_t.pitch = (float_t)buffer[7] / 1000;
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Acc Reference: offset[%.4f, %.4f, %.4f], scale[%.4f, %.4f, %.4f], angle[%.2f, %.2f]", acc_reference_t.offset.x,
                acc_reference_t.offset.y, acc_reference_t.offset.z, acc_reference_t.scale.x, acc_reference_t.scale.y, acc_reference_t.scale.z,
                acc_reference_t.roll, acc_reference_t.pitch);
}

/**
 * @brief  写入超声波基准参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Ud_Reference(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_UD_REFERENCE_ADDR;
    uint64_t buffer[10] = {0};

    buffer[0] = device_t.low_threshold_distance;
    buffer[1] = device_t.high_threshold_distance;

    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取超声波基准参数
 * @param
 * @retval
 */
void FLASH_Read_Ud_Reference(void)
{
    uint64_t buffer[10] = {0};
    u32 Address = FLASH_UD_REFERENCE_ADDR;
    FLASH_Read_NDoubleWord(Address, buffer, 10);

    if (buffer[0] > 4500) {
        device_t.low_threshold_distance = 100;
        device_t.high_threshold_distance = 0;
    } else {
        device_t.low_threshold_distance = buffer[0];
        device_t.high_threshold_distance = buffer[1];
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Ud Reference: low threshold[%dmm], threshold[%dmm]", device_t.low_threshold_distance,
                device_t.high_threshold_distance);
}

/**
 * @brief  读取设备配置参数
 * @param
 * @retval
 */
void FLASH_Read_Device_Param(void)
{
    uint64_t buffer[10];
    u32 Address = FLASH_DEVICE_PARAM_ADDR;
    FLASH_Read_NDoubleWord(Address, buffer, 10);

    if (buffer[0] == 0x1A) {
        device_t.power_on = buffer[5];
    } else {
        device_t.power_on = 0;
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Param Power ON: %d", device_t.power_on);
}

/**
 * @brief  写入设备配置参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Device_Param(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_DEVICE_PARAM_ADDR;
    uint64_t buffer[10] = {0};
    buffer[0] = 0x1A;
    // buffer[1] = device_t.vol_control_sw;
    // buffer[2] = device_t.port_mode;
    // buffer[3] = device_t.stable_time;
    // buffer[4] = device_t.power_mode;
    buffer[5] = device_t.power_on;

    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  写入更新状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Update_Flag(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_USER_END_ADDR - FLASH_ONE_PAGE_SIZE;
    uint64_t buffer = 0x12345678;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, &buffer, 1);
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief  清除更新状态
 * @param
 * @retval 清除状态 1：成功 0：失败
 */
ErrorStatus FLASH_Clear_Update_Flag(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_USER_END_ADDR - FLASH_ONE_PAGE_SIZE;
    u32 PAGEError;

    uint64_t buffer = 0x12345678;
    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    if (buffer != 0xFFFFFFFF) {
        FLASH_EraseInitTypeDef EraseInitStruct = {0};
        EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.Banks = FLASH_BANK_1;
        EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
        EraseInitStruct.NbPages = 1;
        HAL_FLASH_Unlock();
        while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
            ;
        HAL_FLASH_Lock();
    }
    return status;
}

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.report_interval = 240;
    device_t.gps_convert_interval = 1440;
    device_t.low_threshold_distance = 100;
    device_t.high_threshold_distance = 0;
    device_t.threshold_state = 0;
    device_t.sync_state = 1;
    memset(&lis3dh_t, 0, sizeof(lis3dh_t));

    /* 参数写入FLASH */
    FLASH_Write_Report_Interval();
    FLASH_Write_Gps_Period();
    FLASH_Write_Acc_Reference();
    FLASH_Write_Ud_Reference();
}

/**
 * @brief  读取系统默认参数
 * @param
 * @retval
 */
void FLASH_Read_Param(void)
{
    INFO("\n************************************************************\n");
    INFO("************************************************************\n");
    INFO("************* HKT: LoRaWAN Ultrasonic Distance *************\n");
    INFO("******** HARDWARE_VER: 0x%02X ,SOFTWARE_VER: 0x%02X ********\n", HARDWARE_VER, SOFTWARE_VER);

    systemTime_Init();

    FLASH_Read_Device_Param();
    FLASH_Read_Report_Interval(); // 读默认上报周期
    FLASH_Read_Gps_Period();      // 读GPS上报周期
    FLASH_Read_Acc_Reference();
    FLASH_Read_Ud_Reference();
    FLASH_Clear_Update_Flag();

    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));

    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));

    if (device_t.power_on) {
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    }

    factoryMode = 0;
    device_t.ble_connected = BLE_CONNECT_STA;
    device_t.ble_wake_event = 0;
    DEBUG_TRACE(LOG_TAG, "BLE Connect State %d", device_t.ble_connected);
}
