
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define FLASH_ONE_PAGE_SIZE ((u32)0x00000800)                                /* FLASH Page Size */
#define FLASH_USER_START_ADDR ((u32)0x08030000)                              /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR ((u32)0x08040000) /* End @ of user Flash area */ // 256KB

#define FLASH_REPORT_INTERVAL_ADDR (FLASH_USER_START_ADDR)
#define FLASH_GPS_PERIOD_ADDR (FLASH_REPORT_INTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_ACC_REFERENCE_ADDR (FLASH_GPS_PERIOD_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_UD_REFERENCE_ADDR (FLASH_ACC_REFERENCE_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVICE_PARAM_ADDR (FLASH_UD_REFERENCE_ADDR + FLASH_ONE_PAGE_SIZE)

ErrorStatus FLASH_Write_NDoubleWord(u32 Address, uint64_t *Data, int Len);
void FLASH_Read_NDoubleWord(u32 Address, uint64_t *Data, int Len);

ErrorStatus FLASH_Write_Report_Interval(void);
void FLASH_Read_Report_Interval(void);

void FLASH_Read_Gps_Period(void);
ErrorStatus FLASH_Write_Gps_Period(void);

void FLASH_Read_Acc_Reference(void);
ErrorStatus FLASH_Write_Acc_Reference(void);

void FLASH_Read_Ud_Reference(void);
ErrorStatus FLASH_Write_Ud_Reference(void);

void FLASH_Read_Device_Param(void);
ErrorStatus FLASH_Write_Device_Param(void);

ErrorStatus FLASH_Write_Update_Flag(void);
ErrorStatus FLASH_Clear_Update_Flag(void);
void FLASH_Reset_Param(void);
void FLASH_Read_Param(void);

#ifdef __cplusplus
}
#endif

#endif
