
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define MCU_PAGE_SIZE 512
#define FLASH_ONE_PAGE_SIZE ((u32)0x00000800) /* FLASH Page Size */
#define FLASH_APP_START_ADDR ((u32)0x0004000) /* Start @ of app Flash area */

#define FLASH_UPDATE_FLAG_ADDR ((u32)0x0007F000) /* Start @ of app Flash area */
#define FLASH_END_ADDR ((u32)0x00080000)         /* End @ of user Flash area */

    void FLASH_Write_NWord(uint32_t Address, uint32_t *Data, uint32_t Len);
    void FLASH_Read_NWord(u32 Address, u32 *Data, int Len);
    bool FlashPageErase(uint32_t address);
    bool FLASH_WriteCheck_NWord(uint32_t Address, uint32_t *Data, uint32_t Len);

    void FLASH_Write_Update_Flag(void);
    void FLASH_Read_Update_Flag(void);

#ifdef __cplusplus
}
#endif

#endif
