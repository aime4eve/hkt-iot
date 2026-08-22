
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "systick.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

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
    for (int i = 0; i < Len; i++) {
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
    for (int i = 0; i < Len; i++) {
        Data[i] = *(__IO u32 *)readADDR;
        readADDR = readADDR + 4;
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
    if ((device_t.reportInterval > 1440) || (device_t.reportInterval < 10)) // 数据出错，超出范围
       device_t.reportInterval = DEFAULT_REPORT_INTERVAL;
		// device_t.reportInterval = 1;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval: %dmin", device_t.reportInterval);
}


/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.reportInterval = DEFAULT_REPORT_INTERVAL;

    /* 参数写入FLASH */
    FLASH_Write_Report_Interval();
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
    INFO("*************** HKT : Smart Rumen Bolus ***************\n");
    INFO("******** HARDWARE_VER: 0x%02x ,SOFTWARE_VER: 0x%02x ********\n", HARDWARE_VER, SOFTWARE_VER);

    FLASH_Read_Report_Interval();
    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
}
