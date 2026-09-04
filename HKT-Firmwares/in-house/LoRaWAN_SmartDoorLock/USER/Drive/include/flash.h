
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define MCU_PAGE_SIZE 512
#define FLASH_ONE_PAGE_SIZE ((u32)0x00000800)   /* FLASH Page Size */
#define FLASH_APP_START_ADDR ((u32)0x0004000)   /* Start @ of app Flash area */
#define FLASH_USER_START_ADDR ((u32)0x00070000) /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR ((u32)0x00080000)   /* End @ of user Flash area */
#define FLASH_UPDATE_FLAG_ADDR ((u32)0x0007F000)

#define FLASH_DEVEUI_APPEUI_APPKEY_ADDR (FLASH_USER_START_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR (FLASH_DEVEUI_APPEUI_APPKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_LoRaWAN_PORT_CLASS_ADDR (FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVICE_PARAM_ADDR (FLASH_LoRaWAN_PORT_CLASS_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVICE_FACTORY_ADDR (FLASH_DEVICE_PARAM_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVICE_OPEN_MODE_ADDR (FLASH_DEVICE_FACTORY_ADDR + FLASH_ONE_PAGE_SIZE)

#define FLASH_PASSWORD_ADDR ((u32)0x00060000)

// extern u32 temp_flashbuf[];

bool FlashPageErase(uint32_t address);
void FLASH_Write_NWord(uint32_t Address, uint32_t *Data, uint32_t Len);
void FLASH_Read_NWord(u32 Address, u32 *Data, int Len);

void FLASH_Write_DevEUI_AppEUI_AppKEY(void);
void FLASH_Read_DevEUI_AppEUI_AppKEY(void);
void FLASH_Write_DevADDR_AppSKEY_NwkSKEY(void);
void FLASH_Read_DevADDR_AppSKEY_NwkSKEY(void);

void FLASH_Write_LoRaWAN_Port_Class(void);
void FLASH_Read_LoRaWAN_Port_Class(void);

void FLASH_Write_Device_Param(void);
void FLASH_Read_Device_Param(void);

void FLASH_Write_Update_Flag(void);
void FLASH_Read_Update_Flag(void);

void FLASH_Write_Factory_Flag(void);
void FLASH_Read_Factory_Flag(void);

void FLASH_Write_Door_Lock_Coded_Test(void);
void FLASH_Write_Door_Lock_Coded(void);
void FLASH_Read_Door_Lock_Coded(void);

void FLASH_Write_Open_Mode(void);
void FLASH_Read_Open_Mode(void);

void FLASH_Reset_Param(void);
void FLASH_Read_Param(void);

#ifdef __cplusplus
}
#endif

#endif
