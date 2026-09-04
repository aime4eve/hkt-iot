
#include "stdio.h"
#include "string.h"

#include "systick.h"
#include "gpio.h"
#include "adc.h"
#include "tim.h"
#include "iic.h"
#include "uart.h"
#include "gps.h"
#include "acc.h"
#include "lis3dh.h"
#include "lsm6ds3_reg.h"
#include "control_center.h"
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "sht40x.h"
#include "LTR_3XXALS_01.h"

struct device device_t;

u16 ledRedBlinkTime;
u16 ledGreenBlinkTime;

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    static u16 tick;
    if (device_t.waitDafaultMode)
    {
        tick++;
        if (tick == 500)
        {
            GPIO_GREEN_LED_H;
            GPIO_RED_LED_L;
        }
        else if (tick == 1000)
        {
            tick = 0;
            GPIO_GREEN_LED_L;
            GPIO_RED_LED_H;
        }
    }
    else
    {
        if (ledGreenBlinkTime)
        {
            ledGreenBlinkTime--;
            GPIO_GREEN_LED_L;
            if (!ledGreenBlinkTime)
                GPIO_GREEN_LED_H;
        }
        if (ledRedBlinkTime)
        {
            ledRedBlinkTime--;
            GPIO_RED_LED_L;
            if (!ledRedBlinkTime)
                GPIO_RED_LED_H;
        }
    }
}

void HardFault_Handler(void)
{

    while (1)
    {
        GPIO_RED_LED_L;
        GPIO_GREEN_LED_L;
    }
}

/**
 * @brief  系统执行低功耗模式配置
 * @param
 * @retval
 */
void systemLowPowerMode_Config(void)
{
    static u32 time;
    if (time > get_syspant_ms())
        return;
    time = get_syspant_ms() + 500;
    if (time >= 86400000)
        time = 0;

    if (getLoRaWANStatus() == LoRaWAN_WAKE_STATUS)
    {
        if (!uartLoRa.sendDelay && (LoRaWAN.joinState == 1 || (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE)))
        {
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        }
    }

    if ((!device_t.powerState && !ledGreenBlinkTime && !ledRedBlinkTime && getButtonPressState() && !device_t.pressLongEvent) ||
        (!device_t.rtcWakeEvent && !device_t.extiWakeEvent && !uartLoRa.sendDelay && !device_t.sleepDelay && (!LoRaWAN.joinState && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE)))
    {
        LL_EXTI_DisableIT_0_31(LL_SYSCFG_EXTI_LINE0);
        LL_EXTI_DisableIT_0_31(LL_SYSCFG_EXTI_LINE1);
        LL_EXTI_DisableIT_0_31(LL_SYSCFG_EXTI_LINE7);
        if (device_t.powerState)
        {
            lpmSleepTimeInit();
        }
        else
        {
#if TRACK_ACC_ENABLE
            lis3dh_set_mode(acc_sensor, lis3dh_power_down, lis3dh_low_power, true, true, true);
#endif
            DEBUG_TRACE(LOG_TAG, "Device Enter Stop Mode");
        }
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        // 清除入网状态
        LoRaWAN.joinState = 0;
        // LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;

        GPIO_GPS_PWR_CTRL_L;
        GPIO_GPS_BPWR_CTRL_L; // 关闭冷启动电源
        GPIO_RED_LED_H;
        GPIO_GREEN_LED_H;

        LL_EXTI_EnableIT_0_31(LL_SYSCFG_EXTI_LINE0);
        LL_EXTI_EnableIT_0_31(LL_SYSCFG_EXTI_LINE1);
        LL_EXTI_EnableIT_0_31(LL_SYSCFG_EXTI_LINE7);
        device_t.sleepState = 1;
        // if (device_t.powerState)
        // {
        //     EnterStopMode();
        // }
        // else
        // {
        //     EnterStandbyMode();
        // }

        EnterStopMode();

        __disable_irq();
        Systick_Init();
        SystemClock_Config();
        TIM6_Init(1000, 1);
        App_PortCfg();
        App_Uart1Cfg();
        App_LpUart1Cfg();
        App_Uart2Cfg();
        __enable_irq();
        I2C1_Init();

        // RTC睡眠唤醒事件,重新使能串口和时钟
        if (device_t.rtcWakeEvent || device_t.extiWakeEvent)
        {
            if (device_t.rtcWakeEvent)
            {
                device_t.rtcWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
            }
            if (device_t.extiWakeEvent)
            {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI");
                device_t.extiWakeEvent = 0;
                if (!LoRaWAN.joinState && device_t.powerState)
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 离线状态下主动按按键进行重新入网
            }
        }
        if (!device_t.powerState)
        {
            gps_stamp = Timestamp + 100; // 按键时不定位
            ledRedBlinkTime = SEC_DELAY / 2;
        }
        else
        {
            sysTimeSwitchToRtcTime();
            DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                        systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
        }
    }
}

/**
 * @brief  所有传感器读取与转换
 * @param
 * @retval
 */
void sensorReadAndConvert(void)
{
    // static time_t stamp;
    // RTC自动唤醒/ACC触发/按键触发
    if (device_t.rtcWakeEvent || device_t.extiWakeEvent) // 10分钟间隔
    {
        if (device_t.rtcWakeEvent)
            device_t.rtcWakeEvent = 0;
        if (device_t.extiWakeEvent)
        {
            device_t.extiWakeEvent = 0;
        }
        // 读取所有传感器数值
        readSHT40XConvertResult();
    }
}

/**
 * @brief  工厂测试模式
 * @param
 * @retval
 */
void factoryTestMode(void)
{
    //     mDelay(1000);
    //     DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:1"); // 开始测试
    //     mDelay(1000);
    //     DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:2"); // 等待门磁触发关闭
    //     int state = getDoorSensorState();
    //     if (state) // 已是闭合状态
    //     {
    //         while (getDoorSensorState() > 0)
    //             ; // 等待开启重新触发
    //         state = 0;
    //     }
    //     while (getDoorSensorState() == 0)
    //         ; // 等待闭合
    //     mDelay(500);

    //     DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:3"); // 等待门磁触发开启
    //     state = getDoorSensorState();
    //     if (state == 0) // 已是开启状态
    //     {
    //         while (getDoorSensorState() == 0)
    //             ; // 等待闭合重新触发
    //     }
    //     while (getDoorSensorState() > 0)
    //         ; // 等待开启
    //     mDelay(500);

    //     DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:4"); // 等待门磁触发开启
    //     mDelay(2000);
    // #if TRACK_ACC_ENABLE
    //     //    if (acc_sensor)
    //     //    {
    //     //        DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:5");
    //     //    }
    //     //    else
    //     {
    //         DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:6");
    //     }
    //     mDelay(1000);
    // #endif
    //     if (!LightIntensity_Check()) // 环境光传感器
    //     {
    //         DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:7");
    //     }
    //     else
    //         DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:8");
    //     mDelay(1000);

    //     DEBUG_TRACE(ACK_TAG, "factoryMode Cattle Sheep Track Step:9"); // 等待设备入网
    //     mDelay(1000);
    //     LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 重新初始化设备
}
