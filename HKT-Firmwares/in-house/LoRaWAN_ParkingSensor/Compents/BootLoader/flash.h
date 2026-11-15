
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#include "upgrade_common.h"

/* Raw double-word program / read (no erase, no address gate - internal use) */
ErrorStatus FLASH_Write_NDoubleWord(u32 Address, uint64_t *Data, int Len);
void FLASH_Read_NDoubleWord(u32 Address, uint64_t *Data, int Len);

/*
 * Erase the 2KB page containing page_addr and program dw_len double-words.
 * GATED (protocol doc req.8/9): target must be inside the APP image region
 * [UPG_APP_BASE, UPG_APP_END). The VAR page has its own dedicated writer.
 */
ErrorStatus BL_FLASH_WriteAppPage(u32 page_addr, uint64_t *data, int dw_len);

/* Upgrade flag struct on the VAR page (upg_crc16 integrity field kept valid). */
void BL_FLASH_ReadFlag(upg_flag_t *flag);
ErrorStatus BL_FLASH_WriteFlag(const upg_flag_t *flag);

/*
 * Validate the APP stack pointer, de-init BootLoader peripherals and jump to
 * UPG_APP_BASE. Only returns (-1) when the APP image looks invalid.
 */
int BL_JumpToApp(void);

#ifdef __cplusplus
}
#endif

#endif
