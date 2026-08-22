
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "flash.h"
#include "uart.h"
#include "systick.h"
#include "adc.h"
#include "control_center.h"
#include "communicate.h"
#include <LoRaWAN_ATCMD.h>
#include <LoRaWAN_APPLY.h>
#include "gps.h"

//** 注意： 因未使用HAL系统时钟库，直接调用HAL FLASH操作函数时会报错，所以修改了HAL库内FLASH的FLASH操作源函数内容，请勿进行函数库更新修改  **//

/**
 * @brief  FLASH 字节写入
 * @param   Address：待写入FLASH首地址
 * @param   Data：待写入源数据
 * @param   Len：待写入数据长度
 * @retval  写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_NWord(u32 Address, u32 *Data, int Len)
{
    HAL_StatusTypeDef status = HAL_OK;
    u32 writeADDR = Address;
    for (int i = 0; i < Len; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, writeADDR, Data[i]);
        if (status != HAL_OK || *(__IO u32 *)writeADDR != Data[i])
            return ERROR;
        writeADDR = writeADDR + 4;
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
void FLASH_Read_NWord(u32 Address, u32 *Data, int Len)
{
    u32 readADDR = Address;
    for (int i = 0; i < Len; i++)
    {
        Data[i] = *(__IO u32 *)readADDR;
        readADDR = readADDR + 4;
    }
}

void u32tou8(u8 *dfu, u32 src)
{
    dfu[0] = (src >> 24) & 0xFF;
    dfu[1] = (src >> 16) & 0xFF;
    dfu[2] = (src >> 8) & 0xFF;
    dfu[3] = src & 0xFF;
}

/**
 * @brief  写入 DevEUI AppEUI AppKEY参数到FLASH
 * @param
 * @retval  写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_DevEUI_AppEUI_AppKEY(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_DEVEUI_APPEUI_APPKEY_ADDR;

    int t, i = 0;
    u32 temp;
    u32 buffer[8] = {0};

    u8 *dfu = LoRaWAN.DevEUI;
    for (t = 0; t < sizeof(LoRaWAN.DevEUI); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppEUI;
    for (t = 0; t < sizeof(LoRaWAN.AppEUI); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppKEY); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = Address;
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取 DevEUI AppEUI AppKEY参数
 * @param
 * @retval
 */
void FLASH_Read_DevEUI_AppEUI_AppKEY(void)
{
    u32 buffer[8] = {0};
    u32 Address = FLASH_DEVEUI_APPEUI_APPKEY_ADDR;

    FLASH_Read_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));

    u32 *src = buffer;
    int i = 0;

    u8 *dfu = LoRaWAN.DevEUI;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);

    dfu = LoRaWAN.AppEUI;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);

    dfu = LoRaWAN.AppKEY;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i]);

    /* 参数未写入 */
    if ((LoRaWAN.AppEUI[0] == 0 || LoRaWAN.AppEUI[0] == 0xFF) && (LoRaWAN.AppKEY[0] == 0 || LoRaWAN.AppKEY[0] == 0xFF))
    {
        memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
        memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));
    }
    // DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN DevEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
    //             LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
    // DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN APPEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
    //             LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
    // DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //             LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
    //             LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
}

/**
 * @brief  写入 DevADDR AppSKEY NwkSKEY参数到FLASH
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_DevADDR_AppSKEY_NwkSKEY(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR;

    int t, i = 0;
    u32 temp;
    u32 buffer[9] = {0};

    u8 *dfu = LoRaWAN.DevADDR;
    for (t = 0; t < sizeof(LoRaWAN.DevADDR); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppSKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppSKEY); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.NwkSKEY;
    for (t = 0; t < sizeof(LoRaWAN.NwkSKEY); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = Address;
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取 DevADDR AppSKEY NwkSKEY参数
 * @param
 * @retval
 */
void FLASH_Read_DevADDR_AppSKEY_NwkSKEY(void)
{
    u32 buffer[9] = {0};
    u32 Address = FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR;

    FLASH_Read_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));

    u32 *src = buffer;
    int i = 0;

    u8 *dfu = LoRaWAN.DevADDR;
    u32tou8(dfu, src[i++]);
    ;

    dfu = LoRaWAN.AppSKEY;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);

    dfu = LoRaWAN.NwkSKEY;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i++]);
    dfu += 4;
    u32tou8(dfu, src[i]);

    /* 参数未写入 */
    if ((LoRaWAN.DevADDR[0] == 0 || LoRaWAN.DevADDR[0] == 0xFF) && (LoRaWAN.AppSKEY[0] == 0 || LoRaWAN.AppSKEY[0] == 0xFF))
    {
        memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
        memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
        memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));
    }

    // DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN DevADDR: %02x %02x %02x %02x",
    //             LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
    // DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN AppSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //             LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7],
    //             LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
    // DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN NwkSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //             LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7],
    //             LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
}

/**
 * @brief  写入LoRaWAN端口与入网类型
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_LoRaWAN_Port_Class(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_LoRaWAN_PORT_CLASS_ADDR;
    u32 buffer[2];
    buffer[0] = LoRaWAN.port;
    buffer[1] = LoRaWAN.class;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = Address;
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NWord(Address, buffer, 2);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取LoRaWAN端口与入网类型
 * @param
 * @retval
 */
void FLASH_Read_LoRaWAN_Port_Class(void)
{
    u32 buffer[2];
    u32 Address = FLASH_LoRaWAN_PORT_CLASS_ADDR;

    FLASH_Read_NWord(Address, buffer, 2);
    LoRaWAN.port = buffer[0];
    LoRaWAN.class = buffer[1];
    if (LoRaWAN.port > 254 || LoRaWAN.port < 1) // 数据出错，超出范围
        LoRaWAN.port = LoRaWAN_DEFAULT_PORT;
    if (LoRaWAN.class != 0 && LoRaWAN.class != 2) // 数据出错，超出范围
        LoRaWAN.class = LoRaWAN_DEFAULT_CLASS;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device LoRaWAN Port: %d, Class: %s", LoRaWAN.port, LoRaWAN.class ? "Class C" : "Class A");
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
    u32 buffer = device_t.reportInterval;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = Address;
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NWord(Address, &buffer, 1);
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
    u32 buffer;
    u32 Address = FLASH_REPORT_INTERVAL_ADDR;

    FLASH_Read_NWord(Address, &buffer, 1);
    device_t.reportInterval = buffer;
    if ((device_t.reportInterval > 1440) || (device_t.reportInterval < 10 && device_t.reportInterval != 0)) // 数据出错，超出范围
        device_t.reportInterval = 480;                                                                      // 默认间隔8小时
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval: %dmin", device_t.reportInterval);
}

// /**
//  * @brief  写入GPS定位周期（分钟）
//  * @param
//  * @retval 写入状态 1：成功 0：失败
//  */
// ErrorStatus FLASH_Write_Gps_Locate_Interval(void)
// {
//     ErrorStatus status = SUCCESS;
//     u32 Address = FLASH_GPS_LOCATE_INTERVAL_ADDR;
//     u32 buffer = device_t.gpsLocateInterval;
//     u32 PAGEError;
//     FLASH_EraseInitTypeDef EraseInitStruct;
//     /* Unlock the Flash to enable the flash control register access *************/
//     HAL_FLASH_Unlock();
//     /* Fill EraseInit structure*/
//     EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
//     EraseInitStruct.PageAddress = Address;
//     EraseInitStruct.NbPages = 1;
//     while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
//         ;
//     status = FLASH_Write_NWord(Address, &buffer, 1);
//     HAL_FLASH_Lock();
//     return status;
// }

// /**
//  * @brief  读取GPS定位周期（分钟）
//  * @param
//  * @retval
//  */
// void FLASH_Read_Gps_Locate_Interval(void)
// {
//     u32 buffer;
//     u32 Address = FLASH_GPS_LOCATE_INTERVAL_ADDR;

//     FLASH_Read_NWord(Address, &buffer, 1);
//     device_t.gpsLocateInterval = buffer;
//     if (device_t.gpsLocateInterval > 1440 || device_t.gpsLocateInterval < 10) // 数据出错，超出范围
//         device_t.gpsLocateInterval = 30;                                      // 默认30分钟
//     DEBUG_TRACE(FLASH_TAG, "Read Flash Device Gps Locate Interval : %dmin", device_t.gpsLocateInterval);
// }

/**
 * @brief  写入GPS定位周期（分钟）
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Gps_Locate_Interval(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_GPS_LOCATE_INTERVAL_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = Address;
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NWord(Address, &system_schedule_t, sizeof(system_schedule_t) / sizeof(int));
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取GPS定位周期（分钟）
 * @param
 * @retval
 */
void FLASH_Read_Gps_Locate_Interval(void)
{
    u32 buffer;
    u32 Address = FLASH_GPS_LOCATE_INTERVAL_ADDR;

    FLASH_Read_NWord(Address, &system_schedule_t, sizeof(system_schedule_t) / sizeof(int));

    if (system_schedule_t.runMode == 0)
    {
        if (system_schedule_t.idleReportInterval > 1440 || system_schedule_t.idleReportInterval < 1)
        {
            goto __config_default;
        }
    }
    else if (system_schedule_t.runMode == 1)
    {
        // 时间段1
        if (system_schedule_t.reportInterval[0] > 1440 || system_schedule_t.reportInterval[0] < 1 ||
            system_schedule_t.start_hour[0] >= 24 || system_schedule_t.start_min[0] >= 60 ||
            system_schedule_t.end_hour[0] >= 24 || system_schedule_t.end_min[0] >= 60)
        {
            goto __config_default;
        }

        // 时间段2
        if (system_schedule_t.reportInterval[1] > 1440 || system_schedule_t.reportInterval[1] < 1 ||
            system_schedule_t.start_hour[1] >= 24 || system_schedule_t.start_min[1] >= 60 ||
            system_schedule_t.end_hour[1] >= 24 || system_schedule_t.end_min[1] >= 60)
        {
            goto __config_default;
        }

        if (system_schedule_t.idleReportInterval > 1440 || system_schedule_t.idleReportInterval < 1)
        {
            goto __config_default;
        }
    }
    else
    {
    __config_default:
        memset(&system_schedule_t, 0, sizeof(system_schedule_t));
        system_schedule_t.runMode = 0;
        system_schedule_t.idleReportInterval = 30;
    }
    device_t.gpsLocateInterval = system_schedule_t.idleReportInterval;

    INFO("\r\nRead Flash Schedule Mode[%d]:\n\tSchedule[1] %dmin [%d:%d-%d:%d]\n\tSchedule[2] %dmin [%d:%d-%d:%d]\n\tIdle Report Interval %dmin",
         system_schedule_t.runMode, system_schedule_t.reportInterval[0], system_schedule_t.start_hour[0], system_schedule_t.start_min[0], system_schedule_t.end_hour[0], system_schedule_t.end_min[0],
         system_schedule_t.reportInterval[1], system_schedule_t.start_hour[1], system_schedule_t.start_min[1], system_schedule_t.end_hour[1], system_schedule_t.end_min[1], system_schedule_t.idleReportInterval);
}

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    // device_t.reportInterval = 480;
    // device_t.gpsLocateInterval = 30;

    memset(&system_schedule_t, 0, sizeof(system_schedule_t));
    system_schedule_t.runMode = 0;
    system_schedule_t.idleReportInterval = 30;
    device_t.gpsLocateInterval = 30;
    gps_stamp = Timestamp;
    

    /* 参数写入FLASH */
    // FLASH_Write_Report_Interval();
    FLASH_Write_Gps_Locate_Interval();
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
    INFO("*************** HKT : Cattle And Sheep Track ***************\n");
    INFO("******** HARDWARE_VER: 0x%02x ,SOFTWARE_VER: 0x%02x ********\n", HARDWARE_VER, SOFTWARE_VER);

    // FLASH_Read_Report_Interval();
    FLASH_Read_Gps_Locate_Interval();
    // FLASH_Read_DevEUI_AppEUI_AppKEY();    // 读取出厂写入密钥
    // FLASH_Read_DevADDR_AppSKEY_NwkSKEY(); // 读取出厂写入密钥
    FLASH_Read_LoRaWAN_Port_Class();

    batteryLevel = 100; // 电池电量初始值100%

    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    if (!getTamperState())
    {
        device_t.sensor_t.tamper_status = 1; // 防拆报警
    }
}
