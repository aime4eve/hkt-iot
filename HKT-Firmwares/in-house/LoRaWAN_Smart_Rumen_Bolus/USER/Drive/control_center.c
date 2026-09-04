
#include "stdio.h"
#include "string.h"

#include "LoRaWAN_APPLY.h"
#include "acc.h"
#include "accelerometer.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "iic.h"
#include "sht40x.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

struct device device_t;
u16 ledBlinkTime;

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    if (ledBlinkTime) {
        ledBlinkTime--;
        GPIO_LED_L;
        if (!ledBlinkTime)
            GPIO_LED_H;
    }
}

/**
 * @brief  系统执行低功耗模式配置
 * @param
 * @retval
 */
void systemLowPowerMode_Config(void)
{
    if (!uartLoRa.sendDelay && !device_t.sleepDelay && !device_t.da_int &&
        ((LoRaWAN.joinState && !lwrb_write_count && !device_t.syncDeviceState) ||
         (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE))) {
    __sleep:
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        GPIO_LED_H;

        lpmSleepTimeInit();
        // LL_TIM_DeInit(TIM6);
        // App_Uart2_Deinit();
        LL_EXTI_EnableIT_0_31(LL_SYSCFG_EXTI_LINE7);
        device_t.sleepState = 1;
        EnterStopMode();

        // __disable_irq();
        // Systick_Init();
        // SystemClock_Config();
        // TIM6_Init(1000, 4);
        // App_Uart1Cfg();
        // App_Uart2Cfg();
        I2C1_Init();

        // RTC睡眠唤醒事件,重新使能串口和时钟
        if (device_t.rtcWakeEvent || device_t.extiWakeEvent) {
            if (device_t.rtcWakeEvent) {
                device_t.rtcWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
            }
            if (device_t.extiWakeEvent) {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI");
                device_t.extiWakeEvent = 0;
            }
        }
        sysTimeSwitchToRtcTime();
        DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                    systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
        if (device_t.da_int) {
            device_t.da_int = 0;
        }
#if FUNC_THTB_ENABLE
        readSHT40XConvertResult();
#endif

#if ZN_COM_ENABLE
        if (Timestamp >= dataReportTimestamp || device_t.temperature_group >= 7) {
#else
        if (Timestamp >= dataReportTimestamp || device_t.temperature_group >= 5) {
#endif

// #if ZN_COM_ENABLE
//         if (device_t.temperature_group >= 7) {
// #else
//         if (device_t.temperature_group >= 5) {
// #endif

#if TRACK_ACC_ENABLE

#if DA270_ACC_ENABLE
            convertAccData();
#else 
            bma456_convertData();
#endif

#endif
            ADC_BatterySglConvert();
        } else
            goto __sleep;
    }
}
