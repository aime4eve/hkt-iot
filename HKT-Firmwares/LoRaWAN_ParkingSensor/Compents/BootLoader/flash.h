
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define FLASH_ONE_PAGE_SIZE ((u32)0x00000800)   /* FLASH Page Size */
#define FLASH_APP_START_ADDR ((u32)0x08004000) /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR ((u32)0x08040000)   /* End @ of user Flash area */  //256KB

ErrorStatus FLASH_Write_NDoubleWord(u32 Address, uint64_t *Data, int Len);
void FLASH_Read_NDoubleWord(u32 Address, uint64_t *Data, int Len);
ErrorStatus FLASH_Write_OTA_Data(u32 Address, uint64_t *Data, int Len);
void AppProgramRun(void);
ErrorStatus FLASH_Write_Update_Flag(void);
void FLASH_Read_Update_Flag(void);
ErrorStatus FLASH_Clear_Update_Flag(void);

#ifdef __cplusplus
}
#endif

#endif
