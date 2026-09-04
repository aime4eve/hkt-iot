
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "real_time.h"
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
        device_t.report_interval = 60;                                   // 默认间隔60分钟
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval : %dmin\r\n", device_t.report_interval);
}

/**
 * @brief  写入本地定时任务
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Local_Schedule(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_LOCAL_SCHEDULE_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, (uint64_t *)&local_schedule_t.task_t, sizeof(local_schedule_t.task_t) / sizeof(uint64_t));
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取本地定时任务
 * @param
 * @retval
 */
void FLASH_Read_Local_Schedule(void)
{
    u32 Address = FLASH_LOCAL_SCHEDULE_ADDR;

    FLASH_Read_NDoubleWord(Address, (uint64_t *)&local_schedule_t.task_t, sizeof(local_schedule_t.task_t) / sizeof(uint64_t));
    if (local_schedule_t.task_t[0].id == 0xFF) {
        memset(&local_schedule_t.task_t, 0, sizeof(local_schedule_t.task_t));
    }
    local_schedule_init();
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
        device_t.vol_control_sw = buffer[1];
        device_t.port_mode = buffer[2];
        device_t.stable_time = buffer[3];
        device_t.power_mode = buffer[4];
        device_t.power_on = buffer[5];
    } else {
        device_t.vol_control_sw = 0; // 默认12V输出
        device_t.port_mode = 0;      // 默认脉冲模式
        device_t.stable_time = 40;   // 默认40S稳定时长
        device_t.power_mode = 0;     // 默认手动开关机模式
        device_t.power_on = 0;
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Param Vol Control SW: %d, Port Mode: %d, Stable Time: %s, Power Mode: %d, Power ON: %d",
                device_t.vol_control_sw, device_t.port_mode, device_t.stable_time, device_t.power_on);
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
    buffer[1] = device_t.vol_control_sw;
    buffer[2] = device_t.port_mode;
    buffer[3] = device_t.stable_time;
    buffer[4] = device_t.power_mode;
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
 * @brief  读取时区
 * @param
 * @retval
 */
void FLASH_Read_Timezone(void)
{
    uint64_t buffer;
    u32 Address = FLASH_TIMEZONE_ADDR;
    FLASH_Read_NDoubleWord(Address, &buffer, 1);

    int timezone = buffer;
    if (timezone > 26) {       // 为负数
        device_t.timezone = 8; // 默认东8区
    } else if (timezone == 25) {
        device_t.timezone = 3.5;
    } else if (timezone == 26) {
        device_t.timezone = 5.5;
    } else if (timezone < 13) {
        device_t.timezone = timezone;
    } else {
        device_t.timezone = -(timezone - 12);
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Time Zone: %.1f", device_t.timezone);
}

/**
 * @brief  写入时区
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Timezone(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_TIMEZONE_ADDR;
    u8 timezone = 0;
    if (device_t.timezone == 3.5) {
        timezone = 25;
    } else if (device_t.timezone == 5.5) {
        timezone = 26;
    } else if (device_t.timezone < 13) {
        timezone = device_t.timezone;
    } else if (device_t.timezone < 24) {
        timezone = -(device_t.timezone - 12);
    }
    uint64_t buffer = timezone;
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
    device_t.report_interval = 60;
    device_t.vol_control_sw = 0; // 默认12V输出
    device_t.port_mode = 0;      // 默认脉冲模式
    device_t.stable_time = 40;   // 默认40S稳定时长
    device_t.power_mode = 0;     // 默认手动开关机模式

    // 清空本地定时任务
    memset(&local_schedule_t, 0, sizeof(local_schedule_t));
    local_schedule_t.current = NULL;

    /* 参数写入FLASH */
    FLASH_Write_Report_Interval();
    FLASH_Write_Local_Schedule();
    FLASH_Write_Device_Param();
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
    INFO("************** HKT: Solenoid Valve Controller **************\n");
    INFO("********* HARDWARE_VER: 0x%02X ,SOFTWARE_VER: 0x%02X *********\n", HARDWARE_VER, SOFTWARE_VER);
#if 0
    u8 local_test_data[] = {
        1,1,0,2,0,3,0,127,1,10,
        2,2,0,3,1,4,0,127,1,11,
        3,3,0,4,1,5,0,1,1,12,
        4,1,0,5,1,6,0,2,1,13,
        5,1,0,9,0,10,0,4,1,14,
        6,2,0,8,0,8,5,8,1,15,
        7,3,0,11,0,12,0,16,1,16,
        8,1,0,13,0,14,0,32,1,17,
        9,2,0,15,0,16,0,64,1,18,
        10,3,0,17,0,17,2,127,1,0,
        11,3,1,17,4,17,6,127,1,0,
        12,3,1,17,8,17,9,127,1,0,
        13,3,1,17,10,17,11,8,1,0,
        14,3,0,17,12,17,13,8,1,0,
        15,3,0,17,14,17,15,1,1,0,
        16,1,0,17,16,20,18,8,1,5,
    };

    u16 j = 0;
    for (u16 i = 0; i < 16; i++) {
        local_schedule_t.task_t[i].id = local_test_data[j++];
        local_schedule_t.task_t[i].valve = local_test_data[j++];
        local_schedule_t.task_t[i].state = local_test_data[j++];
        local_schedule_t.task_t[i].start_hour = local_test_data[j++];
        local_schedule_t.task_t[i].start_min = local_test_data[j++];;
        local_schedule_t.task_t[i].end_hour = local_test_data[j++];
        local_schedule_t.task_t[i].end_min = local_test_data[j++];
        local_schedule_t.task_t[i].repeat_duty = local_test_data[j++];
        local_schedule_t.task_t[i].is_vaild = local_test_data[j++];
        local_schedule_t.task_t[i].pulse_count = local_test_data[j++];
    }
    FLASH_Write_Local_Schedule();
#endif
    FLASH_Read_Timezone();
    real_time_init();
    read_real_time();
    systemTime_Init();

    FLASH_Read_Report_Interval(); // 读默认上报周期
    FLASH_Read_Local_Schedule();  // 读取本地定时任务
    FLASH_Read_Device_Param();    // 读设备参数
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
    device_t.ble_wake_event = 1;
    device_t.insert1_wake_event = 1;
    device_t.insert2_wake_event = 1;
}
