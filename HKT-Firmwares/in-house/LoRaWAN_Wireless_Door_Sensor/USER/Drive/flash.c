
/* Includes ------------------------------------------------------------------*/
#include "flash.h"
#include "uart.h"
#include "systick.h"
#include "adc.h"
#include "control_center.h"
#include "communicate.h"
#include <LoRaWAN_ATCMD.h>
#include <LoRaWAN_APPLY.h>
#include "gpio.h"

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
        dfu += 4; //偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppEUI;
    for (t = 0; t < sizeof(LoRaWAN.AppEUI); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; //偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppKEY); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; //偏移2个字节
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
        dfu += 4; //偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppSKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppSKEY); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; //偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.NwkSKEY;
    for (t = 0; t < sizeof(LoRaWAN.NwkSKEY); t += 4)
    {
        temp = (u32)dfu[0] << 24;
        temp += (u32)dfu[1] << 16;
        temp += (u32)dfu[2] << 8;
        temp += (u32)dfu[3];
        dfu += 4; //偏移2个字节
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
    if (LoRaWAN.port > 254 || LoRaWAN.port < 1) //数据出错，超出范围
        LoRaWAN.port = 10;
    if (LoRaWAN.class != 0 && LoRaWAN.class != 2) //数据出错，超出范围
        LoRaWAN.class = 0;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device LoRaWAN Port: %d, Class: %s", LoRaWAN.port, LoRaWAN.class ? "Class C" : "Class A");
}

/**
 * @brief  写入心跳周期
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Heart_Interval(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_HEART_INTERVAL_ADDR;
    u32 buffer = device_t.heater_intervel;
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
 * @brief  读取心跳周期
 * @param
 * @retval
 */
void FLASH_Read_Heart_Interval(void)
{
    u32 buffer;
    u32 Address = FLASH_HEART_INTERVAL_ADDR;

    FLASH_Read_NWord(Address, &buffer, 1);
    device_t.heater_intervel = buffer;
    if (device_t.heater_intervel > 2) //数据出错，超出范围
        device_t.heater_intervel = 0;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Heart Interval : %d(0-24h,1-12h,2-8h)", device_t.heater_intervel);
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
    u32 buffer = device_t.counter_intervel;
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
    device_t.counter_intervel = buffer;
    if (device_t.counter_intervel > 480 || device_t.counter_intervel < 1) //数据出错，超出范围
        device_t.counter_intervel = 30;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval : %dmin", device_t.counter_intervel);
}

/**
 * @brief  写入计数时间段
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Count_Time(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_COUNT_TIME_ADDR;
    u32 PAGEError;
    u32 buffer[4];

    buffer[0] = device_t.c_time.start_hour;
    buffer[1] = device_t.c_time.start_min;
    buffer[2] = device_t.c_time.end_hour;
    buffer[3] = device_t.c_time.end_min;

    FLASH_EraseInitTypeDef EraseInitStruct;
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = Address;
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NWord(Address, buffer, 4);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取计数时间段
 * @param
 * @retval
 */
void FLASH_Read_Count_Time(void)
{
    u32 buffer[4];
    u32 Address = FLASH_COUNT_TIME_ADDR;
    struct dcounter_time dct;
    FLASH_Read_NWord(Address, buffer, 4);

    dct.start_hour = buffer[0];
    dct.start_min = buffer[1];
    dct.end_hour = buffer[2];
    dct.end_min = buffer[3];

    if (dct.start_hour < 24 && dct.start_min < 60 && dct.end_hour < 24 && dct.end_min < 60 &&
        (dct.start_hour * 60 + dct.start_min) < (dct.end_hour * 60 + dct.end_min))
    {
        memcpy(&device_t.c_time, &dct, sizeof(dct));
    }
    else //默认监控时间段为 8:00 --- 16:00
    {
        device_t.c_time.start_hour = 8;
        device_t.c_time.start_min = 0;
        device_t.c_time.end_hour = 16;
        device_t.c_time.end_min = 0;
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Count Time : %.2d:%.2d-%.2d:%.2d",
                device_t.c_time.start_hour, device_t.c_time.start_min, device_t.c_time.end_hour, device_t.c_time.end_min);
}

/**
 * @brief  写入运行模式
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Run_Mode(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_RUN_MODE_ADDR;
    u32 buffer = device_t.run_mode;
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
 * @brief  读取运行模式
 * @param
 * @retval
 */
void FLASH_Read_Run_Mode(void)
{
    u32 buffer;
    u32 Address = FLASH_RUN_MODE_ADDR;

    FLASH_Read_NWord(Address, &buffer, 1);
    device_t.run_mode = buffer;
    if (device_t.run_mode > 2 || device_t.run_mode < 1) //数据出错，超出范围
        device_t.run_mode = 2;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Run Mode : %d", device_t.run_mode);
}

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.counter_intervel = 30;
    device_t.heater_intervel = 0;
    device_t.run_mode = 2;

    device_t.c_time.start_hour = 8;
    device_t.c_time.start_min = 0;
    device_t.c_time.end_hour = 16;
    device_t.c_time.end_min = 0;

    /* 参数写入FLASH */
    FLASH_Write_Heart_Interval();
    FLASH_Write_Report_Interval();
    FLASH_Write_Count_Time();
    FLASH_Write_Run_Mode();
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
    INFO("****************** HKT : Door Sense Sensor *****************\n");
    INFO("******** HARDWARE_VER: 0x%02x ,SOFTWARE_VER: 0x%02x ********\n", HARDWARE_VER, SOFTWARE_VER);

    FLASH_Read_Heart_Interval();
    FLASH_Read_Report_Interval();
    FLASH_Read_Count_Time();
    FLASH_Read_Run_Mode();

    FLASH_Read_DevEUI_AppEUI_AppKEY();    //读取出厂写入密钥
    FLASH_Read_DevADDR_AppSKEY_NwkSKEY(); //读取出厂写入密钥
    FLASH_Read_LoRaWAN_Port_Class();

    batteryLevel = 100; //电池电量初始值100%

    if (getDoorSensorState())
        device_t.doorState = 0;
    else
        device_t.doorState = 1;

    if (getTamperState())
        device_t.tamperState = 0;
    else
        device_t.tamperState = 1;

    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; //初始化LoRa
}
