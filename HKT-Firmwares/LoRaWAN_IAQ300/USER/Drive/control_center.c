
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

#include "EPD.h"
#include "LTR_3XXALS_01.h"
#include "PMSA003.h"
#include "hp203b.h"
#include "im8601pa.h"
#include "scd4x.h"
#include "sgp40_i2c.h"
#include "sgp4x.h"
#include "sht40x.h"
// #include "SEN0231.h"
#include "W25X40CLSNIG.h"
#include "cb_hcho_v4c.h"
#include "sht4x_i2c.h"
#include "zmod441x.h"
#include "zmod451x.h"

struct device device_t;

/**
 * @brief  工厂测试模式
 * @param
 * @retval
 */
void factoryTestMode(void)
{
    DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:1"); // 开始测试
    mDelay(1000);
    DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:2"); // 传感器检测
    mDelay(1000);

    if (SCD4X_Check()) // CO2传感器
    {
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:3");
    } else
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:4");
    mDelay(1000);

    if (SHT40X_Check()) // 温湿度传感器
    {
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:5");
    } else
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:6");
    mDelay(1000);

#if FUNC_TVOC
    if (ZMOD4410_Check()) // TVOC传感器
    {
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:7");
    } else
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:8");
    mDelay(1000);
#else
    if (SGP4X_Check()) // VOC传感器
    {
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:7");
    } else
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:8");
    mDelay(1000);
#endif
    if (HP203B_Check()) // 大气压传感器信息
    {
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:9");
    } else
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:10");
    mDelay(1000);

    // PIR_Init();                // PIR配置初始化
    DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:11");
    mDelay(1000);

    if (LightIntensity_Check()) // 环境光传感器
    {
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:12");
    } else
        DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:13");
    mDelay(1000);

    // #if FUNC_PM2_5
    //     PMSA003_Init(); // PM2.5传感器
    // #if FUNC_HCHO
    //     // SEN0231_Init(); // HCHO 传感器
    //     HCHO_Init();
    // #endif
    // #if FUNC_O3
    //     ZMOD4510_Init(); // 臭氧传感器
    // #endif
    // #endif
    // SpiFlash_Init(); // nor flash 初始化
    // EPD_ShowUpdate();
    DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:14"); // 等待设备入网
    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;                        // 重新初始化设备
}

/**
 * @brief  主机功能初始化
 * @param
 * @retval
 */
void FuncInit(void)
{
    INFO("\n************************************************************\n");
    INFO("************************************************************\n");
    INFO("**************** HKT : Air Monitoring Sensor ***************\n");
    INFO("******** HARDWARE_VER: 0x%04x ,SOFTWARE_VER: 0x%04x ********\n", HARDWARE_VER, SOFTWARE_VER);

    systemTime_Init();
    readDCPowerInputStatus();
    FLASH_Read_Param();
    if (*(u32 *)FLASH_UPDATE_FLAG_ADDR == 0x12345678) {
        FlashPageErase(FLASH_UPDATE_FLAG_ADDR); // 清除更新标志位
    }

    CO2_PWR_CTRL_H;
    EPD_PWR_CTRL_L;
    BEEP_CTRL_L;

    device_t.epdShowModeWriteDelay = -1;
    device_t.ledShowModeWriteDelay = -1;

    batteryLevel = 100;                         // 电池电量初始值100%
    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    device_t.sleepDelay = MAX_WAIT_SLEEP_TIMEOUT;
}
