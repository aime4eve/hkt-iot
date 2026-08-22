
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "flash.h"
#include "uart.h"
#include "systick.h"

bool FlashPageErase(uint32_t address)
{
#define FLS_PAGE_ERASE_KEY0 0x96969696
#define FLS_PAGE_ERASE_KEY1 0xEAEAEAEA
#define FLS_PAGE_ERASE_DATA 0x1234ABCD

    uint16_t i;
    CMU_PERCLK_SetableEx(FLASHCLK, ENABLE);
    CMU_OPCCR2_NVMCKE_Setable(ENABLE);

    FLS_EPCR_ERTYPE_Set(FLS_EPCR_ERTYPE_PAGE);

    FLS_EPCR_EREQ_Set(FLS_EPCR_EREQ_Msk);
    FLS_KEY_Write(FLS_PAGE_ERASE_KEY0);
    FLS_KEY_Write(FLS_PAGE_ERASE_KEY1);
    *(uint32_t *)address = FLS_PAGE_ERASE_DATA;

    while (SET != FLS_ISR_ERD_Chk())
    {
        __NOP();
    }
    FLS_ISR_ERD_Clr();

    FLS_KEY_Write(0x00000000);

    CMU_OPCCR2_NVMCKE_Setable(DISABLE);
    CMU_PERCLK_SetableEx(FLASHCLK, DISABLE);
    for (i = 0; i < 128; i++)
    {
        if (*(uint32_t *)(address + i * 4) != 0xFFFFFFFF)
        {
            return false;
        }
    }
    return true;

#undef FLS_PAGE_ERASE_KEY0
#undef FLS_PAGE_ERASE_KEY1
#undef FLS_PAGE_ERASE_DATA
}

void FLASH_Write_Word(uint32_t address, uint32_t data)
{
#define FLS_PROG_KEY0 0xA5A5A5A5
#define FLS_PROG_KEY1 0xF1F1F1F1

    CMU_OPCCR2_NVMCKE_Setable(ENABLE);

    FLS_EPCR_PREQ_Set(FLS_EPCR_PREQ_Msk);
    FLS_KEY_Write(FLS_PROG_KEY0); // 写入flash编程key0
    FLS_KEY_Write(FLS_PROG_KEY1); // 写入flash编程key1
    *(uint32_t *)address = data;

    while (FLS_ISR_PRD_Chk() == RESET)
        ;              // 等待写操作完成
    FLS_ISR_PRD_Clr(); // 清除写完成中断标志
    FLS_KEY_Write(0x00000000);

    CMU_OPCCR2_NVMCKE_Setable(DISABLE);

#undef FLS_PROG_KEY0
#undef FLS_PROG_KEY1
}

/**
 * @brief  FLASH 字节写入
 * @param   Address：待写入FLASH首地址
 * @param   Data：待写入源数据
 * @param   Len：待写入数据长度
 * @retval  写入状态 1：成功 0：失败
 */
void FLASH_Write_NWord(uint32_t Address, uint32_t *Data, uint32_t Len)
{
    CMU_PERCLK_SetableEx(FLASHCLK, ENABLE);
    while (Len--)
    {
        FLASH_Write_Word(Address, Data[0]);
        Data += 1;
        Address += 4;
    }
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

/**
 * @brief  FLASH 字节写入
 * @param   Address：待写入FLASH首地址
 * @param   Data：待写入源数据
 * @param   Len：待写入数据长度
 * @retval  写入状态 1：成功 0：失败
 */
bool FLASH_WriteCheck_NWord(uint32_t Address, uint32_t *Data, uint32_t Len)
{
    //     u32 Flash_Addr = Address;
    //     u32 Data_Len = Len;
    //     u32 *Data_Src = Data;
    // #define FLS_PROG_KEY0 0xA5A5A5A5
    // #define FLS_PROG_KEY1 0xF1F1F1F1

    //     CMU_PERCLK_SetableEx(FLASHCLK, ENABLE);
    //     CMU_OPCCR2_NVMCKE_Setable(ENABLE);

    //     FLS_ISR_KEYERR_Clr();
    //     FLS_EPCR_PREQ_Set(FLS_EPCR_PREQ_Msk);
    //     FLS_KEY_Write(FLS_PROG_KEY0); // 写入flash编程key0
    //     FLS_KEY_Write(FLS_PROG_KEY1); // 写入flash编程key1

    //     while (Data_Len--)
    //     {
    //         FLS_EPCR_PREQ_Set(FLS_EPCR_PREQ_Msk);
    //         *(uint32_t *)Flash_Addr = *Data_Src;
    //         Data_Src += 1;
    //         Flash_Addr += 4;
    //         while (FLS_ISR_PRD_Chk() == RESET)
    //             ;              // 等待写操作完成
    //         FLS_ISR_PRD_Clr(); // 清除写完成中断标志
    //     }

    //     FLS_KEY_Write(0x00000000);

    //     CMU_OPCCR2_NVMCKE_Setable(DISABLE);
    //     // CMU_PERCLK_SetableEx(FLASHCLK, DISABLE);

    // #undef FLS_PROG_KEY0
    // #undef FLS_PROG_KEY1

    //    // 校验写入数据
    //    Flash_Addr = Address;
    //    Data_Src = Data;
    //    while (Data_Len--)
    //    {
    //        if (*(uint32_t *)Flash_Addr != *Data_Src)
    //            return 1;
    //        Data_Src += 1;
    //        Flash_Addr += 4;
    //    }

    FLASH_Write_NWord(Address, Data, Len);
    return 0;
}

/**
 * @brief  写入更新状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Update_Flag(void)
{
    u32 Address = FLASH_UPDATE_FLAG_ADDR;
    u32 buffer = 0x12345678;
    FlashPageErase(Address);
    FLASH_Write_NWord(Address, &buffer, 1);
}

/**
 * @brief  读取更新状态
 * @param
 * @retval
 */
void FLASH_Read_Update_Flag(void)
{
    u32 buffer;
    u32 Address = FLASH_UPDATE_FLAG_ADDR;

    FLASH_Read_NWord(Address, &buffer, 1);

    if (buffer != 0x12345678)
    {
        NVIC_DisableIRQ(UART0_IRQn);
        CloseIO(GPIOF, GPIO_Pin_3);                // PF3 UART0 RX
        CloseIO(GPIOF, GPIO_Pin_4);                // PF4 UART0 TX
        UARTx_IER_TXSE_IE_Setable(UART0, DISABLE); // 关闭发送中断
        UARTx_IER_RXBF_IE_Setable(UART0, DISABLE); // 关闭接收中断
        UARTx_CSR_RXEN_Setable(UART0, DISABLE);    // 关闭接收使能
        UARTx_CSR_TXEN_Setable(UART0, DISABLE);    // 关闭发送使能
        __disable_irq();                           // 关闭全局中断使能

        /* 跳转到应用程序位置 */
        AppProgramRun();
    }
    // DEBUG_TRACE(LOG_TAG, "Run BootLoad");
}
