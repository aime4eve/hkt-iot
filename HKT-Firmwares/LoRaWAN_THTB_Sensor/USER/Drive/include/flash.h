
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define FLASH_ONE_PAGE_SIZE ((u32)0x00000400)   /* FLASH Page Size */
#define FLASH_USER_START_ADDR ((u32)0x0800E000) /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR ((u32)0x08010000)   /* End @ of user Flash area */

#define FLASH_DEVEUI_APPEUI_APPKEY_ADDR (FLASH_USER_START_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR (FLASH_DEVEUI_APPEUI_APPKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_LoRaWAN_PORT_CLASS_ADDR (FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_SYSTEM_CONFIG_ADDR (FLASH_LoRaWAN_PORT_CLASS_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_REPORT_INTERVAL_ADDR (FLASH_SYSTEM_CONFIG_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_GPS_LOCATE_INTERVAL_ADDR (FLASH_REPORT_INTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)

#define FLASH_CACHE_DATA_ADDR ((u32)0x08080000)       //缓存数据 EEPROM起始地址
#define FLASH_EEPROM_END_ADDR ((u32)0x080807FF)       // EEPROM结束地址
#define MAX_CACHE_DATA_LEN 42

    extern struct system_config flash_config;

    ErrorStatus FLASH_Write_NWord(u32 Address, u32 *Data, int Len);
    void FLASH_Read_NWord(u32 Address, u32 *Data, int Len);

    ErrorStatus FLASH_Write_DevEUI_AppEUI_AppKEY(void);
    void FLASH_Read_DevEUI_AppEUI_AppKEY(void);
    ErrorStatus FLASH_Write_DevADDR_AppSKEY_NwkSKEY(void);
    void FLASH_Read_DevADDR_AppSKEY_NwkSKEY(void);

    ErrorStatus FLASH_Write_LoRaWAN_Port_Class(void);
    void FLASH_Read_LoRaWAN_Port_Class(void);

    ErrorStatus FLASH_Write_System_Config(void);
    void FLASH_Read_System_Config(void);
    ErrorStatus FLASH_Write_Report_Interval(void);
    void FLASH_Read_Report_Interval(void);
    ErrorStatus FLASH_Write_Gps_Locate_Interval(void);
    void FLASH_Read_Gps_Locate_Interval(void);
    void FLASH_Read_Cache_Data(void);
    void FLASH_Write_Cache_Data(u8 *buf, u8 data_len);

    void FLASH_Reset_Param(void);
    void FLASH_Read_Param(void);

#ifdef __cplusplus
}
#endif

#endif
