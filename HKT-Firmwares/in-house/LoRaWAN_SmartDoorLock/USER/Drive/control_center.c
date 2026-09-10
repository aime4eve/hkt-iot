
#include "control_center.h"
#include "LoRaWAN_APPLY.h"
#include "adc.h"
#include "communicate.h"
#include "flash.h"
#include "gpio.h"
#include "iic.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

struct device device_t;
#if FUNC_OPERATIONAL_VERSION_ENABLE
struct coded_lock_timeliness coded_lock_t;
struct coded_lock_open_mode coded_open_mode_t;
#else
struct coded_lock coded_lock_t;
#endif

TX_EVENT_FLAGS_GROUP main_event_group;

/**
 * @brief  主机功能初始化
 * @param
 * @retval
 */
void FuncInit(void)
{
    INFO("\n************************************************************\n");
    INFO("************************************************************\n");
    INFO("**************** HKT : Smart Door Lock ***************\n");
    INFO("******** HARDWARE_VER: 0x%04x ,SOFTWARE_VER: 0x%04x ********\n", HARDWARE_VER, SOFTWARE_VER);

    // INFO("\n------------- RMU->RSR: 0x%04X ------------------\n", RMU->RSR);
    // INFO("------------- IWDT->CFGR: 0x%04X ------------------\n", IWDT->CFGR);
    // INFO("------------- DBG->HDFR: 0x%04X ------------------\n", DBG->HDFR);

    // RMU->RSR = 0xFFFFFFFF;
    // DBG->HDFR = 0xFFFFFFFF;

    FLASH_Read_Param();
    systemTime_Init();
    if (*(u32 *)FLASH_UPDATE_FLAG_ADDR == 0x12345678) {
        FlashPageErase(FLASH_UPDATE_FLAG_ADDR); // 清除更新标志位
    }

    batteryLevel = 100; // 电池电量初始值100%
    // device_t.sleepDelay = MAX_WAIT_SLEEP_TIMEOUT;
    device_t.sleepDelay = 10;
    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    ADC_BatterySglConvert();

    UINT status = tx_event_flags_create(&main_event_group, "main_event_group");
    if (status) {
        DEBUG_TRACE(ERROR_TAG, "main_event_group Creat Fail !!! %d", status);
    }
}
