
/* Includes ------------------------------------------------------------------*/
#include "flash.h"
#include "LoRaWAN_APPLY.h"
#include "adc.h"
#include "communicate.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

bool FLASH_Erase(u32 erase_addr)
{
    u32 Addr = erase_addr;
    FLASH_State FLASHState = FLASH_ErasePage(Addr);
    if (FLASHState != FLASH_COMPLETE) {
        return FALSE;
    }
    return SUCCESS;
}

bool FLASH_Program_NWord(u32 program_addr, u8 byte[], u16 len)
{
    u32 Addr = program_addr;
    FLASH_State FLASHState;

    for (int i = 0; i < len; len++) {
        FLASHState = FLASH_ProgramWordData(Addr, byte[i]);
        if (FLASHState != FLASH_COMPLETE)
            return FALSE;
        if (rw(Addr) != byte[i]) // 校验数据
        {
            return FALSE;
        }
        Addr += 4;
    }
    return SUCCESS;
}

void FLASH_Read_NWord(u32 read_addr, u8 byte[], u16 len)
{
    u32 Addr = read_addr;
    for (int i = 0; i < len; len++) {
        byte[i] = (u8)rw(Addr);
    }
}

/**
 * @brief  写设备周期上报间隔时间到FLash
 * @retval
 */
void Flash_Write_Report_Interval(void)
{
    u32 value = device_t.report_interval;
    bool state = FLASH_Erase(SLAVE_REPORT_INTERVAL_ADDR);
    state &= FLASH_ProgramWordData(SLAVE_REPORT_INTERVAL_ADDR, value);
    DEBUG_TRACE(FLASH_TAG, "Write Report Interval %dmin %s", device_t.report_interval, state ? "Success" : "Fail");
}

/**
 * @brief  读设备周期上报间隔时间
 * @retval
 */
void Flash_Read_Report_Interval(void)
{
    u32 value = rw(SLAVE_REPORT_INTERVAL_ADDR);
    if ((value >= 10 && value <= 24 * 60) || value == 0) {
        device_t.report_interval = value;
    } else {
        device_t.report_interval = DEFAULT_REPORT_INTERVAL;
    }
    DEBUG_TRACE(FLASH_TAG, "Read Report Interval %dmin", device_t.report_interval);
}

/**
 * @brief  复位配置参数
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.report_interval = DEFAULT_REPORT_INTERVAL;
    Flash_Write_Report_Interval();
}

/**
 * @brief  系统参数初始化
 * @retval
 */
void Slave_Param_Init(void)
{
    Flash_Read_Report_Interval();
}

void App_Init(void)
{
    INFO("\n************************************************************\n");
    INFO("************************************************************\n");
    INFO("*************** HKT : Smoke Alarm *************************\n");
    INFO("******** HARDWARE_VER: 0x%02x ,SOFTWARE_VER: 0x%02x ********\n", HARDWARE_VER, SOFTWARE_VER);

    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    device_t.sleep_delay = 300;

    Slave_Param_Init();

#if FUNC_NTC_ENABLE
    ADC_NtcSglConvert();
#endif

#if FUNC_TAMPER_ENABLE
    if (READ_TAMPER_STATE) {
        device_t.tamper_state = 0;
        DEBUG_TRACE(WARN_TAG, "Smoke DisTamper!!!!");
    } else {
        device_t.tamper_state = 1;
        DEBUG_TRACE(WARN_TAG, "Smoke Tamper!!!!");
    }
#endif

    if (READ_DAT2_STATE) {
        device_t.battery_state = 0;
        DEBUG_TRACE(LOG_TAG, "Smoke Battery Low!!!!");
    } else {
        device_t.battery_state = 1;
        DEBUG_TRACE(LOG_TAG, "Smoke Battery Normal!!!!");
    }
}
