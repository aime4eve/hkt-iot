
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "flash.h"
#include "gpio.h"
#include "systick.h"
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

ErrorStatus FLASH_Write_OTA_Data(u32 Address, uint64_t *Data, int Len)
{
    ErrorStatus status = SUCCESS;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, Data, Len);
    HAL_FLASH_Lock();
    return status;
}

__IO uint32_t AppSpInitVal;
__IO uint32_t AppJumpAddr;
void (*pAppFun)(void); // 定义一个函数指针.用于指向APP程序入口.

/* 跳转到应用程序位置 */
void AppProgramRun(void)
{
    AppSpInitVal = *(__IO uint32_t *)(FLASH_APP_START_ADDR);    // 取APP的SP初值.
    AppJumpAddr = *(__IO uint32_t *)(FLASH_APP_START_ADDR + 4); // 取程序入口.
    if ((AppSpInitVal & 0x2FFE0000) == 0x20000000)              // 检查栈顶地址是否合法.
    {
				App_Uart3_Deinit();
        App_LpUart1DeInit();
        LL_TIM_DeInit(TIM16);
        // FLASH_Clear_Update_Flag();
        __set_MSP(AppSpInitVal);               // 设置SP.
        pAppFun = (void (*)(void))AppJumpAddr; // 生成跳转函数.
        (*pAppFun)();                          // 跳转.不再返回.
    }
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
 * @brief  读取更新状态
 * @param
 * @retval
 */
void FLASH_Read_Update_Flag(void)
{
    uint64_t buffer;
    u32 Address = FLASH_USER_END_ADDR - FLASH_ONE_PAGE_SIZE;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    if (buffer != 0x12345678) {
        __disable_irq(); // 关闭全局中断使能
        /* 跳转到应用程序位置 */
        AppProgramRun();
    }
    INFO("\n************************  Run BootLoader ************************\n");
    __enable_irq(); // 关闭全局中断使能
}
