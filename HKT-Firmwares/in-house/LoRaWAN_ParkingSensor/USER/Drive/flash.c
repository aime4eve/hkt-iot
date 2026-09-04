
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "qmc5883l.h"
#include "rkb1145d.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

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
        if (status != HAL_OK) {
            DEBUG_TRACE(FLASH_TAG, "Write failed at address: 0x%08X, data: 0x%016llX", writeADDR, Data[i]);
            return ERROR;
        }
        // 增加延时以确保数据稳定
        HAL_Delay(1);
        if (*(__IO uint64_t *)writeADDR != Data[i]) {
            DEBUG_TRACE(FLASH_TAG, "Verification failed at address: 0x%08X, expected: 0x%016llX, actual: 0x%016llX", writeADDR, Data[i],
                        *(__IO uint64_t *)writeADDR);
            return ERROR;
        }
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
 * @brief  写入工作模式
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Park_Mode(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_PARK_MODE_ADDR;
    uint64_t buffer = device_t.park_mode;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
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
 * @brief  读取工作模式
 * @param
 * @retval
 */
void FLASH_Read_Park_Mode(void)
{
    uint64_t buffer;
    u32 Address = FLASH_PARK_MODE_ADDR;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    device_t.park_mode = buffer;
    if (device_t.park_mode > 2) // 数据出错，超出范围(0/1/2)
        device_t.park_mode = 0;
    DEBUG_TRACE(FLASH_TAG, "Read Park Mode : %d", device_t.park_mode);
}

/**
 * @brief  写入开机状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Park_Power(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_PARK_POWER_ADDR;
    uint64_t buffer = device_t.power_on;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
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
 * @brief  读取开机状态
 * @param
 * @retval
 */
void FLASH_Read_Park_Power(void)
{
    uint64_t buffer;
    u32 Address = FLASH_PARK_POWER_ADDR;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    if (buffer > 1) // 数据出错，超出范围
        device_t.power_on = 0;
    else
        device_t.power_on = buffer;
    DEBUG_TRACE(FLASH_TAG, "Read Park Power: %d", device_t.power_on);
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
    if ((buffer >= 1 && buffer <= 1440) || buffer == 0) {
        device_t.report_interval = buffer;
    } else { // 数据出错，超出范围
        device_t.report_interval = 1440; // 默认间隔1440分钟
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval : %dmin", device_t.report_interval);
}

/**
 * @brief  写入地磁参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Mag_Param(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_MAG_PARAM_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    HAL_FLASH_Unlock();
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    uint64_t buffer[10] = {0};
    buffer[0] = qmc5883l_t.offset.x;
    buffer[1] = qmc5883l_t.offset.y;
    buffer[2] = qmc5883l_t.offset.z;
    buffer[3] = qmc5883l_t.threshold.x;
    buffer[4] = qmc5883l_t.threshold.y;
    buffer[5] = qmc5883l_t.threshold.z;
    buffer[6] = qmc5883l_t.is_calibration;
    buffer[7] = qmc5883l_t.state;
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

ErrorStatus FLASH_Write_Mag_Param_Back(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_MAG_PARAM_BACK_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    HAL_FLASH_Unlock();
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    uint64_t buffer[10] = {0};
    buffer[0] = qmc5883l_t.offset.x;
    buffer[1] = qmc5883l_t.offset.y;
    buffer[2] = qmc5883l_t.offset.z;
    buffer[3] = qmc5883l_t.threshold.x;
    buffer[4] = qmc5883l_t.threshold.y;
    buffer[5] = qmc5883l_t.threshold.z;
    buffer[6] = qmc5883l_t.is_calibration;
    buffer[7] = qmc5883l_t.state;
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取地磁参数
 * @param
 * @retval
 */
void FLASH_Read_Mag_Param(void)
{
    u32 Address = FLASH_MAG_PARAM_ADDR;
    uint64_t buffer[10] = {0};
    FLASH_Read_NDoubleWord(Address, buffer, 10);
    qmc5883l_t.offset.x = buffer[0];
    qmc5883l_t.offset.y = buffer[1];
    qmc5883l_t.offset.z = buffer[2];
    qmc5883l_t.threshold.x = buffer[3];
    qmc5883l_t.threshold.y = buffer[4];
    qmc5883l_t.threshold.z = buffer[5];
    qmc5883l_t.is_calibration = buffer[6];
    qmc5883l_t.state = buffer[7];
    // 数据出错，超出范围
    if (qmc5883l_t.state > MAGNETOMETER_IN_WORK || qmc5883l_t.threshold.x < MAGNETOMETER_THRESHOLD_X / 2 ||
        qmc5883l_t.threshold.y < MAGNETOMETER_THRESHOLD_Y / 2 || qmc5883l_t.threshold.z < MAGNETOMETER_THRESHOLD_Z / 2) {

        // 读取异常时读备份区域数据
        Address = FLASH_MAG_PARAM_BACK_ADDR;
        FLASH_Read_NDoubleWord(Address, buffer, 10);
        qmc5883l_t.offset.x = buffer[0];
        qmc5883l_t.offset.y = buffer[1];
        qmc5883l_t.offset.z = buffer[2];
        qmc5883l_t.threshold.x = buffer[3];
        qmc5883l_t.threshold.y = buffer[4];
        qmc5883l_t.threshold.z = buffer[5];
        qmc5883l_t.is_calibration = buffer[6];
        qmc5883l_t.state = buffer[7];
        if (qmc5883l_t.state > MAGNETOMETER_IN_WORK || qmc5883l_t.threshold.x < MAGNETOMETER_THRESHOLD_X / 2 ||
            qmc5883l_t.threshold.y < MAGNETOMETER_THRESHOLD_Y / 2 || qmc5883l_t.threshold.z < MAGNETOMETER_THRESHOLD_Z / 2) {

            memset(&qmc5883l_t, 0, sizeof(qmc5883l_t));
            qmc5883l_t.threshold.x = MAGNETOMETER_THRESHOLD_X;
            qmc5883l_t.threshold.y = MAGNETOMETER_THRESHOLD_Y;
            qmc5883l_t.threshold.z = MAGNETOMETER_THRESHOLD_Z;
            qmc5883l_t.is_calibration = 0;
            qmc5883l_t.state = MAGNETOMETER_IN_IDLE;
        } else {
            // 读取备份区域数据成功，写入正常区域
            FLASH_Write_Mag_Param();
        }
    }
    qmc5883l_t.is_changed = 0;
    DEBUG_TRACE(FLASH_TAG, "Read Mag Param ");
    INFO("[%d, %d, %d] Offset XYZ, [%d, %d, %d] Threshold, Calibration[%d]", qmc5883l_t.offset.x, qmc5883l_t.offset.y, qmc5883l_t.offset.z,
         qmc5883l_t.threshold.x, qmc5883l_t.threshold.y, qmc5883l_t.threshold.z, qmc5883l_t.is_calibration);
}

/**
 * @brief  写入雷达参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Rkb_Param(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_RKB_PARAM_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    HAL_FLASH_Unlock();
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    uint64_t buffer[10] = {0};
    for (int i = 0; i < 10; i++) {
        buffer[i] = rkb1145d_t.base_power[i];
    }
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取雷达参数
 * @param
 * @retval
 */
void FLASH_Read_Rkb_Param(void)
{
    u32 Address = FLASH_RKB_PARAM_ADDR;
    uint64_t buffer[10] = {0};

    FLASH_Read_NDoubleWord(Address, buffer, 10);
    for (int i = 0; i < 10; i++) {
        rkb1145d_t.base_power[i] = buffer[i];
    }
    if (rkb1145d_t.base_power[0] >= 0xFFFF || rkb1145d_t.base_power[1] >= 0xFFFF) // 数据出错，超出范围
        memset(&rkb1145d_t, 0, sizeof(rkb1145d_t));

    u8 max_num;
    u16 max_power = 0;
    DEBUG_TRACE(FLASH_TAG, "Read RD Base Power[");
    for (u8 i = 0; i < 10; i++) {
        INFO(" %d", rkb1145d_t.base_power[i]);
        if (max_power < rkb1145d_t.base_power[i]) {
            max_power = rkb1145d_t.base_power[i];
            max_num = i;
        }
    }
    u16 max_power_distance = (max_num + 1) * 10;
    INFO("], Max Power[%d] Distance[%d]", max_power, max_power_distance);
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
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.report_interval = 1440;
    memset(&rkb1145d_t, 0, sizeof(rkb1145d_t));
    memset(&qmc5883l_t, 0, sizeof(qmc5883l_t));

    /* 参数写入FLASH */
    FLASH_Write_Report_Interval();
    FLASH_Write_Mag_Param();
    FLASH_Write_Mag_Param_Back();
    FLASH_Write_Rkb_Param();

    qmc5883l_t.threshold.x = MAGNETOMETER_THRESHOLD_X;
    qmc5883l_t.threshold.y = MAGNETOMETER_THRESHOLD_Y;
    qmc5883l_t.threshold.z = MAGNETOMETER_THRESHOLD_Z;
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
    INFO("*************** HKT : Parking Sensor ***************\n");
    INFO("******** HARDWARE_VER: 0x%02X ,SOFTWARE_VER: 0x%02X ********\n", HARDWARE_VER, SOFTWARE_VER);

    FLASH_Read_Report_Interval();
    FLASH_Read_Park_Mode();
    FLASH_Read_Park_Power();
    FLASH_Read_Mag_Param();
    FLASH_Read_Rkb_Param();
    FLASH_Clear_Update_Flag();

    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));

    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));

    batteryLevel = 100; // 电池电量初始值100%
    if (device_t.power_on) {
        LPTIM1_Init();
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    }
    factoryMode = 0;
    device_t.ble_connected = BLE_CONNECT_STA;

    device_t.ble_wake_event = 0;
    DEBUG_TRACE(LOG_TAG, "BLE Connect State %d", device_t.ble_connected);
}
