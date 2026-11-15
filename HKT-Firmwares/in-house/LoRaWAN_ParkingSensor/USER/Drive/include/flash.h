
#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define FLASH_ONE_PAGE_SIZE   (FLASH_PAGE_SIZE) /* FLASH Page Size */
#define FLASH_USER_START_ADDR ((u32)0x08030000) /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR   ((u32)0x08040000) /* End @ of user Flash area */ // 256KB

#define FLASH_DEVEUI_APPEUI_APPKEY_ADDR    (FLASH_USER_START_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR (FLASH_DEVEUI_APPEUI_APPKEY_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_LORAWAN_CFG_ADDR             (FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR + FLASH_ONE_PAGE_SIZE) /* 0xA0 LoRaWAN link cfg (was PORT_CLASS, unused) */
#define FLASH_PARK_MODE_ADDR               (FLASH_LORAWAN_CFG_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_REPORT_INTERVAL_ADDR         (FLASH_PARK_MODE_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_MAG_PARAM_ADDR               (FLASH_REPORT_INTERVAL_ADDR + FLASH_ONE_PAGE_SIZE)
/* 电量计数环形槽（2026-11-14 用户裁决：与上报周期同页共用，均低频写）。页内布局：页首8B=上报
 * 周期；offset 0x400 起 16 槽 x 8B。槽结构：magic(0xB1)+seq(u8环绕)+capacity(u32 LE)+sum(u16)。
 * 空槽可直接编程（L4 双字写），槽满才擦页（重灌周期+槽0）；掉电写坏由 sum 校验+上一有效槽兜底 */
#define FLASH_BATTERY_SLOT_ADDR            (FLASH_REPORT_INTERVAL_ADDR + 0x400)
#define FLASH_BATTERY_SLOT_NUM             16
#define FLASH_RKB_PARAM_ADDR               (FLASH_MAG_PARAM_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_PARK_POWER_ADDR              (FLASH_RKB_PARAM_ADDR + FLASH_ONE_PAGE_SIZE)
#define FLASH_MAG_PARAM_BACK_ADDR          (FLASH_PARK_POWER_ADDR + FLASH_ONE_PAGE_SIZE) // 地磁校准数据备份地址

ErrorStatus FLASH_Write_NDoubleWord(u32 Address, uint64_t *Data, int Len);
void FLASH_Read_NDoubleWord(u32 Address, uint64_t *Data, int Len);

ErrorStatus FLASH_Write_Park_Mode(void);
void FLASH_Read_Park_Mode(void);
ErrorStatus FLASH_Write_LoRaWAN_Cfg(void);
void FLASH_Read_LoRaWAN_Cfg(void);
ErrorStatus FLASH_Write_Report_Interval(void);
void FLASH_Read_Report_Interval(void);
ErrorStatus FLASH_Write_Battery(void);
void FLASH_Read_Battery(void);
ErrorStatus FLASH_Write_Mag_Param(void);
ErrorStatus FLASH_Write_Mag_Param_Back(void);
void FLASH_Read_Mag_Param(void);
ErrorStatus FLASH_Write_Rkb_Param(void);
void FLASH_Read_Rkb_Param(void);
ErrorStatus FLASH_Write_Park_Power(void);
void FLASH_Read_Park_Power(void);
ErrorStatus FLASH_Write_Update_Flag(void);
ErrorStatus FLASH_Clear_Update_Flag(void);
void FLASH_Reset_Param(void);
void FLASH_Read_Param(void);

#ifdef __cplusplus
}
#endif

#endif
