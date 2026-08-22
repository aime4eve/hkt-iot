
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

u32 flash_buffer[12] = {0};
struct system_config flash_config = {0};

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
    // if ((LoRaWAN.AppEUI[0] == 0 || LoRaWAN.AppEUI[0] == 0xFF) && (LoRaWAN.AppKEY[0] == 0 || LoRaWAN.AppKEY[0] == 0xFF))
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
    if (LoRaWAN.class == 0 || LoRaWAN.class == 2) // 数据出错，超出范围
        LoRaWAN.class = 0;
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
    if (device_t.reportInterval > 1440 || device_t.reportInterval < 1) // 数据出错，超出范围
       device_t.reportInterval = DEFAULT_REPORT_INTERVAL;                                  // 默认间隔60分钟
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval : %dmin", device_t.reportInterval);
}

/**
 * @brief  写入GPS定位周期（分钟）
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Gps_Locate_Interval(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_GPS_LOCATE_INTERVAL_ADDR;
    u32 buffer = device_t.gpsLocateInterval;
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
 * @brief  读取GPS定位周期（分钟）
 * @param
 * @retval
 */
void FLASH_Read_Gps_Locate_Interval(void)
{
    u32 buffer;
    u32 Address = FLASH_GPS_LOCATE_INTERVAL_ADDR;

    FLASH_Read_NWord(Address, &buffer, 1);
    device_t.gpsLocateInterval = buffer;
    if (device_t.gpsLocateInterval > 1440 || device_t.gpsLocateInterval < 1) // 数据出错，超出范围
        device_t.gpsLocateInterval = 1440;                                   // 默认24小时
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Gps Locate Interval : %dmin", device_t.gpsLocateInterval);
}

/**
 * @brief  写入设备运行参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_System_Config(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_SYSTEM_CONFIG_ADDR;
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
    status = FLASH_Write_NWord(Address, (u32 *)&device_t.system_config_t, sizeof(device_t.system_config_t) / sizeof(u32));
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取设备运行参数
 * @param
 * @retval
 */
void FLASH_Read_System_Config(void)
{
    u32 Address = FLASH_SYSTEM_CONFIG_ADDR;

    FLASH_Read_NWord(Address, (u32 *)&flash_config, sizeof(flash_config) / sizeof(u32));
    if (flash_config.angle_alarm_threshold == 0 || flash_config.angle_alarm_threshold > 90) // 未被写入过
    {
        flash_config.system_isable = 1;  // 系统运行使能
        flash_config.battery_isable = 1; // 电量检测使能
        flash_config.move_isable = 1;    // 运动检测使能

        flash_config.angle_alarm_threshold = DEFAULT_ANGLE_THRESHOLD;                       // 角度变化报警阈值
        flash_config.temperature_alarm_low_threshold = DEFAULT_TEMPERATURE_LOW_THRESHOLD;   // 温度过低报警阈值
        flash_config.temperature_alarm_high_threshold = DEFAULT_TEMPERATURE_HIGH_THRESHOLD; // 温度过高报警阈值

        flash_config.humidity_alarm_low_threshold = DEFAULT_HUMIDITY_LOW_THRESHOLD;   // 湿度过低报警阈值
        flash_config.humidity_alarm_high_threshold = DEFAULT_HUMIDITY_HIGH_THRESHOLD; // 湿度过高报警阈值
    }
    memcpy(&device_t.system_config_t, &flash_config, sizeof(flash_config));
    DEBUG_TRACE(FLASH_TAG, "Read Flash System Config");
    INFO("\r\n\tsystem run enable: %d", flash_config.system_isable);
    INFO("\r\n\tbattery decetion enable: %d", flash_config.battery_isable);
    INFO("\r\n\tmove decetion enable: %d", flash_config.move_isable);
    INFO("\r\n\tangle alarm threshold: %d", flash_config.angle_alarm_threshold);
    INFO("\r\n\ttemp alarm threshold, low: %d, high: %d", flash_config.temperature_alarm_low_threshold, flash_config.temperature_alarm_high_threshold);
    INFO("\r\n\thumi alarm threshold, low: %d, high: %d", flash_config.humidity_alarm_low_threshold, flash_config.humidity_alarm_high_threshold);
}

/*------------------------------------------------------------
Func: EEPROM  Read
Note:
-------------------------------------------------------------*/
void EEPROM_Read(u32 Addr, u32 *Buffer, u16 Length)
{
    HAL_FLASHEx_DATAEEPROM_Unlock();
    while (Length--)
    {
        *Buffer++ = *(u32 *)Addr;
        Addr += 4;
        HAL_FLASHEx_DATAEEPROM_Lock();
    }
}

/*------------------------------------------------------------
Func: EEPROM Write
Note:
-------------------------------------------------------------*/
void EEPROM_Write(u32 WriteAddr, u32 *pBuffer, u16 NumToWrite)
{
    u16 t;
    HAL_FLASHEx_DATAEEPROM_Unlock();
    for (t = 0; t < NumToWrite; t++)
    {
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, WriteAddr + t * 4, pBuffer[t]);
    }
    HAL_FLASHEx_DATAEEPROM_Unlock();
}

void Clear_EEPROM(void)
{
    HAL_FLASHEx_DATAEEPROM_Unlock();
    for (int t = 0; t < 512; t++)
    {
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, FLASH_CACHE_DATA_ADDR + t * 4, 0);
    }
    HAL_FLASHEx_DATAEEPROM_Unlock();
}

void U32ToU8Array(u8 *buf, u32 u32Value)
{
    buf[0] = ((u32Value >> 24) & 0xFF);
    buf[1] = ((u32Value >> 16) & 0xFF);
    buf[2] = ((u32Value >> 8) & 0xFF);
    buf[3] = (u32Value & 0xFF);
}

u32 U8ArrayToU32(u8 *buf)
{
    u32 value = (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
    return value;
}

/**
 * @brief  写入离线缓存数据
 * @param
 * @retval
 */
void FLASH_Write_Cache_Data(u8 *buf, u8 data_len)
{
    u32 Address = FLASH_CACHE_DATA_ADDR;
    u32 dataStartAddr;
    u32 len = 0;
    u32 buffer[12] = {0};

    EEPROM_Read(Address, &len, 1);               // 读取缓存数据条数
    EEPROM_Read(Address + 4, &dataStartAddr, 1); // 读取缓存数据起始地址
    if ((dataStartAddr < FLASH_CACHE_DATA_ADDR + 8) || (dataStartAddr > FLASH_EEPROM_END_ADDR - 12 * 4))
    {
        dataStartAddr = len * 12 * 4 + FLASH_CACHE_DATA_ADDR + 8;
    }

    if (len >= MAX_CACHE_DATA_LEN) // 计算存储数据写入地址
    {
        dataStartAddr = (len % MAX_CACHE_DATA_LEN) * 12 * 4 + 8 + FLASH_CACHE_DATA_ADDR; // 覆盖旧缓存数据
    }
    else
    {
        dataStartAddr = 8 + (len * 12 * 4) + FLASH_CACHE_DATA_ADDR; // 指向下一条缓存数据
    }
    len++;

    u16 i = 0, cnt = 0;
    cnt = data_len / 4;
    if (data_len % 4 != 0)
        cnt++;
    for (i = 0; i < cnt; i++)
    {
        buffer[i] = U8ArrayToU32(&buf[i * 4]);
    }

    EEPROM_Write(Address, &len, 1);
    EEPROM_Write(Address + 4, &dataStartAddr, 1);
    EEPROM_Write(dataStartAddr, buffer, 12);
    device_t.flash_data = 1;
    DEBUG_TRACE(FLASH_TAG, "Eeprom Write Cache Data Addr[%d]: 0x%08x", len, dataStartAddr);
}

/**
 * @brief  读取GPS定位周期（分钟）
 * @param
 * @retval
 */
void FLASH_Read_Cache_Data(void)
{
    u32 len, send_len;
    u16 i, j;

    char cache_buf[60] = {0};
    char dev_buf[18] = {0};
    char temp_buf[4] = {0};
    char send_buf[100] = {0};

    u32 Address = FLASH_CACHE_DATA_ADDR;
    u32 dataStartAddr;
    EEPROM_Read(Address, &len, 1);               // 读取缓存数据条数
    EEPROM_Read(Address + 4, &dataStartAddr, 1); // 读取缓存数据起始地址
    DEBUG_TRACE(FLASH_TAG, "Read Flash Cache Data Len: %d, Cache Data Addr: 0x%08x", len, dataStartAddr);
    if (len > 0)
    {
        send_len = len;
        if (send_len > MAX_CACHE_DATA_LEN)
        {
            send_len = MAX_CACHE_DATA_LEN;
        }
        else
        {
            dataStartAddr = FLASH_CACHE_DATA_ADDR + 8;
        }
        cmd_get_hex2hexstr((u8 *)dev_buf, 16, LoRaWAN.DevEUI, 8);
        for (i = 0; i < send_len; i++)
        {
            memset(&send_buf, 0, 100);
            memset(&cache_buf, 0, 60);

            strcat(send_buf, "T,S1,");
            strncat(send_buf, dev_buf, 16);
            strcat(send_buf, ",");

            // Software Version
            sprintf(temp_buf, "%d,", SOFTWARE_VER);
            strcat(send_buf, temp_buf);

            // 包序号
            sprintf(temp_buf, "%d,", packSyncNumber++);
            strcat(send_buf, temp_buf);

            sprintf(temp_buf, "%d,", DATATYPE_DEVICE_CACHED_TEMP);
            strcat(send_buf, temp_buf);
            DEBUG_TRACE(FLASH_TAG, "Eeprom Read Cache Data Addr[%d]: 0x%08x", i, dataStartAddr);
            EEPROM_Read(dataStartAddr, flash_buffer, 12);
            dataStartAddr += 12 * 4; // 下一次读取缓存数据的地址
            if (dataStartAddr > FLASH_EEPROM_END_ADDR - 12 * 4)
            {
                dataStartAddr = FLASH_CACHE_DATA_ADDR + 8;
            }
            for (j = 0; j < 12; j++)
            {
                U32ToU8Array((u8 *)&cache_buf[j * 4], flash_buffer[j]);
            }
            // 缓存数据
            strcat(send_buf, cache_buf);
            DEBUG_TRACE(LOG_TAG, "LoRa SendCacheData: %s", send_buf);

            if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
                setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
            if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
                setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
            /* 等待模组就绪 */
            while (!getLoRaWANBusyIOLevel())
                ;
            HAL_Delay(20);

            LoRa_SendCMD(send_buf); // 发送数据
            if (getLoRaWANBusyStat())
            {
                DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
            }
            if (getLoRaWANSendStat())
            {
                DEBUG_TRACE(WARN_TAG, "LoRa SendCacheData Fail, Exit\r\n");
                device_t.flash_data = 1;
                device_t.send_failcnt++;
                if (device_t.send_failcnt >= 5)
                {
                    device_t.send_failcnt = 0;
                    LoRaWAN.joinState = 0;
                    LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                    reconnect_stamp = Timestamp + 30 * 60;
                }
                return;
            }
            device_t.send_failcnt = 0;
            DEBUG_TRACE(LOG_TAG, "LoRa SendCacheData Success");
            fromUartDataHandle();
            len--;
            EEPROM_Write(Address, &len, 1);
            EEPROM_Write(Address + 4, &dataStartAddr, 1);
        }
        uartLoRa.sendDelay = 10 * SEC_DELAY;

        Clear_EEPROM(); // 清空本地存储
        len = 0;
        dataStartAddr = Address + 4;
        EEPROM_Write(Address, &len, 1);
        EEPROM_Write(Address + 4, &dataStartAddr, 1);
        DEBUG_TRACE(FLASH_TAG, "Clear Flash Cache Data ");
    }
}

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.reportInterval = 60;
    device_t.gpsLocateInterval = 1440;

    device_t.system_config_t.system_isable = 1;  // 系统运行使能
    device_t.system_config_t.battery_isable = 1; // 电量检测使能
    device_t.system_config_t.move_isable = 1;    // 运动检测使能

    device_t.system_config_t.angle_alarm_threshold = DEFAULT_ANGLE_THRESHOLD;                       // 角度变化报警阈值
    device_t.system_config_t.temperature_alarm_low_threshold = DEFAULT_TEMPERATURE_LOW_THRESHOLD;   // 温度过低报警阈值
    device_t.system_config_t.temperature_alarm_high_threshold = DEFAULT_TEMPERATURE_HIGH_THRESHOLD; // 温度过高报警阈值

    device_t.system_config_t.humidity_alarm_low_threshold = DEFAULT_HUMIDITY_LOW_THRESHOLD;   // 湿度过低报警阈值
    device_t.system_config_t.humidity_alarm_high_threshold = DEFAULT_HUMIDITY_HIGH_THRESHOLD; // 湿度过高报警阈值

    /* 参数写入FLASH */
    FLASH_Write_Report_Interval();
    FLASH_Write_Gps_Locate_Interval();
    FLASH_Write_System_Config();
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
    INFO("*************** HKT : THTB Sensor ***************\n");
    INFO("******** HARDWARE_VER: 0x%02x ,SOFTWARE_VER: 0x%02x ********\n", HARDWARE_VER, SOFTWARE_VER);

    FLASH_Read_Report_Interval();
    FLASH_Read_Gps_Locate_Interval();
    // FLASH_Read_DevEUI_AppEUI_AppKEY();    // 读取出厂写入密钥
    // FLASH_Read_DevADDR_AppSKEY_NwkSKEY(); // 读取出厂写入密钥
    // FLASH_Read_LoRaWAN_Port_Class();
    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));

    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));
    FLASH_Read_System_Config();

    u32 len = 0;
    u32 Address = FLASH_CACHE_DATA_ADDR;
    u32 dataStartAddr;
    EEPROM_Read(Address, &len, 1); // 读取缓存数据条数
    if (len > 1000)                // 首次上电或数据有误
    {
        Clear_EEPROM(); // 清空本地存储
        len = 0;
        dataStartAddr = Address + 8;
        EEPROM_Write(Address, &len, 1);
        EEPROM_Write(Address + 4, &dataStartAddr, 1);
        DEBUG_TRACE(FLASH_TAG, "Clear Flash Cache Data ");
    }
    else if (len > 0)
    {
        device_t.flash_data = 1;
    }

    batteryLevel = 100; // 电池电量初始值100%
    device_t.powerIn = 1;
    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    factoryMode = 0;
}
