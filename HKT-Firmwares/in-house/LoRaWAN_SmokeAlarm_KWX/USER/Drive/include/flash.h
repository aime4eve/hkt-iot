
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define SLAVE_REPORT_INTERVAL_ADDR (0xC000)
#define SMOKE_DEC_INTERVAL_ADDR (SLAVE_REPORT_INTERVAL_ADDR + FLASH_PAGE_SIZE)
#define SMOKE_VOLTAGE_THRESHOLD_ADDR (SMOKE_DEC_INTERVAL_ADDR + FLASH_PAGE_SIZE)
#define SMOKE_ALARM_THRESHOLD_ADDR (SMOKE_VOLTAGE_THRESHOLD_ADDR + FLASH_PAGE_SIZE)
#define SMOKE_DISALARM_THRESHOLD_ADDR (SMOKE_ALARM_THRESHOLD_ADDR + FLASH_PAGE_SIZE)
#define SMOKE_VOLTAGE_DATUM_ADDR (SMOKE_DISALARM_THRESHOLD_ADDR + FLASH_PAGE_SIZE)


void Flash_Write_Report_Interval(void);
void Flash_Read_Report_Interval(void);
void Flash_Write_Smoke_Detection_Interval(void);
void Flash_Read_Smoke_Detection_Interval(void);
void Flash_Write_Smoke_Voltage_Threshold(void);
void Flash_Read_Smoke_Voltage_Threshold(void);
void Flash_Write_Smoke_Alarm_Threshold(void);
void Flash_Read_Smoke_Alarm_Threshold(void);
void Flash_Write_Smoke_DisAlarm_Threshold(void);
void Flash_Read_Smoke_DisAlarm_Threshold(void);
void Flash_Write_Smoke_Datum(void);
void Flash_Read_Smoke_Datum(void);
void FLASH_Reset_Param(void);
void Slave_Param_Init(void);
void App_Init(void);


#ifdef __cplusplus
}
#endif

#endif
