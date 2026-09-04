
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define FLASH_ONE_PAGE_SIZE    ((u32)0x00000800) /* FLASH Page Size */
#define FLASH_APP_START_ADDR   ((u32)0x0004000) /* Start @ of app Flash area */
#define FLASH_USER_START_ADDR  ((u32)0x00070000) /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR    ((u32)0x00080000) /* End @ of user Flash area */
#define FLASH_UPDATE_FLAG_ADDR ((u32)0x0007F000)

#define FLASH_DEVEUI_APPEUI_APPKEY_ADDR    (FLASH_USER_START_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR (FLASH_DEVEUI_APPEUI_APPKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_LoRaWAN_PORT_CLASS_ADDR      (FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_SLEEP_INTERVAL_ADDR          (FLASH_LoRaWAN_PORT_CLASS_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_REPORT_INTERVAL_ADDR         (FLASH_SLEEP_INTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_EPD_SHOW_MODE_ADDR           (FLASH_REPORT_INTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVICE_WORK_MODE_ADDR        (FLASH_EPD_SHOW_MODE_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVICE_TEMP_OFFSET_ADDR      (FLASH_DEVICE_WORK_MODE_ADDR + FLASH_ONE_PAGE_SIZE)

bool FlashPageErase(uint32_t address);
void FLASH_Write_NWord(uint32_t Address, uint32_t *Data, uint32_t Len);
void FLASH_Read_NWord(u32 Address, u32 *Data, int Len);

void FLASH_Write_DevEUI_AppEUI_AppKEY(void);
void FLASH_Read_DevEUI_AppEUI_AppKEY(void);
void FLASH_Write_DevADDR_AppSKEY_NwkSKEY(void);
void FLASH_Read_DevADDR_AppSKEY_NwkSKEY(void);

void FLASH_Write_LoRaWAN_Port_Class(void);
void FLASH_Read_LoRaWAN_Port_Class(void);

void FLASH_Write_Report_Interval(void);
void FLASH_Read_Report_Interval(void);

void FLASH_Write_EPD_Show_Mode(void);
void FLASH_Read_EPD_Show_Mode(void);

void FLASH_Write_Device_Work_Mode(void);
void FLASH_Read_Device_Work_Mode(void);

void FLASH_Write_Temperature_Offset(void);
void FLASH_Read_Temperature_Offset(void);

void FLASH_Write_Update_Flag(void);
void FLASH_Read_Update_Flag(void);

void FLASH_Reset_Param(void);
void FLASH_Read_Param(void);

#ifdef __cplusplus
}
#endif

#endif
