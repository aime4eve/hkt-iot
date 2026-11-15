
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"

#include "flash.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#include "upgrade_crc.h"

/**
 * @brief  FLASH double-word program
 * @param   Address: flash address (double-word aligned)
 * @param   Data:   source buffer
 * @param   Len:    number of double-words
 * @retval  SUCCESS / ERROR
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
 * @brief  FLASH double-word read (memory-mapped)
 */
void FLASH_Read_NDoubleWord(u32 Address, uint64_t *Data, int Len)
{
    u32 readADDR = Address;
    for (int i = 0; i < Len; i++) {
        Data[i] = *(__IO uint64_t *)readADDR;
        readADDR = readADDR + 8;
    }
}

/*==============================================================================
 * Page erase helper
 *============================================================================*/
static ErrorStatus flash_erase_page(u32 page_addr)
{
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (page_addr - 0x08000000) / UPG_INT_PAGE_SIZE;
    EraseInitStruct.NbPages = 1;
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
        return ERROR;
    }
    return SUCCESS;
}

/**
 * @brief  Erase the APP-region page and program dw_len double-words into it.
 *         Address gate: refuses anything outside [UPG_APP_BASE, UPG_APP_END).
 */
ErrorStatus BL_FLASH_WriteAppPage(u32 page_addr, uint64_t *data, int dw_len)
{
    ErrorStatus status;

    if ((page_addr < UPG_APP_BASE) || (page_addr >= UPG_APP_END) ||
        (page_addr % UPG_INT_PAGE_SIZE) != 0) {
        DEBUG_TRACE(ERROR_TAG, "BL write gated: 0x%08lX", page_addr);
        return ERROR;
    }

    HAL_FLASH_Unlock();
    status = flash_erase_page(page_addr);
    if (status == SUCCESS) {
        status = FLASH_Write_NDoubleWord(page_addr, data, dw_len);
    }
    HAL_FLASH_Lock();
    return status;
}

/*==============================================================================
 * Upgrade flag (VAR page)
 *============================================================================*/
void BL_FLASH_ReadFlag(upg_flag_t *flag)
{
    memcpy(flag, (const void *)UPG_VAR_PAGE, sizeof(*flag));
}

ErrorStatus BL_FLASH_WriteFlag(const upg_flag_t *flag)
{
    upg_flag_t tmp;
    ErrorStatus status;

    memcpy(&tmp, flag, sizeof(tmp));
    tmp.crc16 = upg_crc16((const uint8_t *)&tmp, sizeof(tmp) - 2U);

    HAL_FLASH_Unlock();
    status = flash_erase_page(UPG_VAR_PAGE);
    if (status == SUCCESS) {
        /* VAR struct is 16 bytes = 2 double-words */
        status = FLASH_Write_NDoubleWord(UPG_VAR_PAGE, (uint64_t *)&tmp,
                                         (int)(sizeof(tmp) / 8U));
    }
    HAL_FLASH_Lock();
    return status;
}

/*==============================================================================
 * Jump to application
 *============================================================================*/
int BL_JumpToApp(void)
{
    u32 sp = *(__IO u32 *)UPG_APP_BASE;
    u32 pc = *(__IO u32 *)(UPG_APP_BASE + 4U);
    void (*appEntry)(void);

    if ((sp & 0x2FFE0000UL) != 0x20000000UL) {
        DEBUG_TRACE(ERROR_TAG, "invalid APP SP 0x%08lX", sp);
        return -1;
    }

    /* De-init everything the BootLoader touched so the APP starts clean */
    App_Uart3_Deinit();
    LL_TIM_DeInit(TIM16);
    NVIC_DisableIRQ(TIM1_UP_TIM16_IRQn);

    __disable_irq();
    __set_MSP(sp);
    appEntry = (void (*)(void))pc;
    appEntry();                 /* does not return */

    return -1;
}
