
#include "flash.h"
#include "an001.h"
#include "uart.h"


#if !FUNC_BOARD_VERSION_CL
/*!
    \brief      erase fmc pages from FMC_WRITE_START_ADDR to FMC_WRITE_END_ADDR
    \param[in]  none
    \param[out] none
    \retval     none
*/
void fmc_erase_pages(u32 addr)
{
    /* unlock the flash program/erase controller */
    fmc_unlock();

    /* clear all pending flags */
    fmc_flag_clear(FMC_FLAG_BANK0_END);
    fmc_flag_clear(FMC_FLAG_BANK0_WPERR);
    fmc_flag_clear(FMC_FLAG_BANK0_PGERR);

    fmc_page_erase(addr);

    /* lock the main FMC after the erase operation */
    fmc_lock();
}

/*!
    \brief      写实时上报时间
    \param[in]  none
    \param[out] none
    \retval     none
*/
void fmc_program_report_interval(void)
{

    u32 address = FMC_REPORT_INTERVAL_ADDR;
    fmc_erase_pages(address);

    /* unlock the flash program/erase controller */
    fmc_unlock();
    /* program flash */
    fmc_word_program(address, as_an001_t.period_upload_real_time);
    /* lock the main FMC after the program operation */
    fmc_lock();
}


#else

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

	
	/*!
    \brief      写实时上报时间
    \param[in]  none
    \param[out] none
    \retval     none
*/
void fmc_program_report_interval(void)
{
	  ErrorStatus status = SUCCESS;
    u32 Address = FMC_REPORT_INTERVAL_ADDR;
    u32 buffer = as_an001_t.period_upload_real_time;
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
}

#endif

/*!
    \brief      读实时上报时间
    \param[in]  none
    \param[out] none
    \retval     none
*/
void fmc_read_report_interval(void)
{
    u32 address = FMC_REPORT_INTERVAL_ADDR;
    as_an001_t.period_upload_real_time = *(u32 *)address;
    // if (as_an001_t.period_upload_real_time != 0)
    {
        // if (as_an001_t.period_upload_real_time < 300 || as_an001_t.period_upload_real_time > 36000) //最大时间限制为10小时
        if (as_an001_t.period_upload_real_time < 300 || as_an001_t.period_upload_real_time > 86400) // 最大时间限制为24小时    
        as_an001_t.period_upload_real_time = DEFAULT_AS_REPORT_TIME;
    }
    DEBUG_TRACE(FLASH_TAG, "Read Period Upload Real Time: %d", as_an001_t.period_upload_real_time);
}
