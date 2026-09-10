
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define FLASH_ONE_PAGE_SIZE ((u32)0x00000400)   /* FLASH Page Size */
#define FLASH_USER_START_ADDR ((u32)0x0800C000) /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR ((u32)0x08010000)   /* End @ of user Flash area */

#define FLASH_DEVEUI_APPEUI_APPKEY_ADDR (FLASH_USER_START_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR (FLASH_DEVEUI_APPEUI_APPKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_LoRaWAN_PORT_CLASS_ADDR (FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_REPORT_MODE_ADDR (FLASH_LoRaWAN_PORT_CLASS_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_REPORT_INTERVAL_ADDR (FLASH_REPORT_MODE_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_IRREPORT_INTERVAL_ADDR (FLASH_REPORT_INTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_REPORT_TOTAL_COUNT_ADDR (FLASH_IRREPORT_INTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_REPORT_HEARTBEATINTERVAL_ADDR (FLASH_REPORT_TOTAL_COUNT_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_TIME_EVENT_ADDR (FLASH_REPORT_HEARTBEATINTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)



    ErrorStatus FLASH_Write_NWord(u32 Address, u32 *Data, int Len);
    void FLASH_Read_NWord(u32 Address, u32 *Data, int Len);

    ErrorStatus FLASH_Write_DevEUI_AppEUI_AppKEY(void);
    void FLASH_Read_DevEUI_AppEUI_AppKEY(void);
    ErrorStatus FLASH_Write_DevADDR_AppSKEY_NwkSKEY(void);
    void FLASH_Read_DevADDR_AppSKEY_NwkSKEY(void);
    
    ErrorStatus FLASH_Write_LoRaWAN_Port_Class(void);
    void FLASH_Read_LoRaWAN_Port_Class(void);

    ErrorStatus FLASH_Write_Report_Mode(void);
    void FLASH_Read_Report_Mode(void);
    ErrorStatus FLASH_Write_Report_Interval(void);
    void FLASH_Read_Report_Interval(void);
    ErrorStatus FLASH_Write_IRReport_Interval(void);
    void FLASH_Read_IRReport_Interval(void);
    ErrorStatus FLASH_Write_Report_Total_Count(void);
    void FLASH_Read_Report_Total_Count(void);
    ErrorStatus FLASH_Write_Report_HeartbeatInterval(void);
    void FLASH_Read_Report_HeartbeatInterval(void);
    void FLASH_Reset_Param(void);
    void FLASH_Read_Param(void);
    void FLASH_Read_Time_Event(void);
    ErrorStatus FLASH_Write_Time_Event(void);

#ifdef __cplusplus
}
#endif

#endif
