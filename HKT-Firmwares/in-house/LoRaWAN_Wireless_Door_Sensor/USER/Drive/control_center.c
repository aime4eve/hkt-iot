
#include "systick.h"
#include "gpio.h"
#include "adc.h"
#include "tim.h"
#include "uart.h"
#include "control_center.h"
#include "communicate.h"
#include "LoRaWAN_APPLY.h"

struct device_sensor device_t;

u16 ledBlueBlinkTime;

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    static u32 tick;

    if (ledBlueBlinkTime)
    {
        ledBlueBlinkTime--;
        LL_GPIO_WriteOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN, 1);
        if (!ledBlueBlinkTime)
            LL_GPIO_WriteOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN, 0);
    }
    else if (!LoRaWAN.joinState)
    {
        tick++;
        if (tick % 500 == 0)
            LL_GPIO_TogglePin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN);
    }
}

/**
 * @brief  系统执行低功耗模式配置
 * @param
 * @retval
 */
void systemLowPowerMode_Config(void)
{
    lpmSleepTimeInit();
    LL_TIM_DeInit(TIM6);
    App_Uart2_Deinit();

    LL_GPIO_WriteOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN, 0);
    device_t.sleepState = 1;
    EnterStopMode();

    if (device_t.rtcWakeEvent || device_t.extiWakeEvent) // RTC睡眠唤醒事件,重新使能串口和时钟
    {
        TIM6_Init(1000, 4);
        App_Uart2Cfg();
        if (device_t.rtcWakeEvent)
        {
            device_t.rtcWakeEvent = 0;
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
        }
        if (device_t.extiWakeEvent)
        {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI %s", device_t.doorEvent ? "DoorSensor" : "Tamper");
            device_t.extiWakeEvent = 0;
        }
        sysTimeSwitchToRtcTime();
    }
}

/**
 * @brief  工厂测试模式
 * @param
 * @retval
 */
void factoryTestMode(void)
{
    mDelay(300);
    DEBUG_TRACE(ACK_TAG, "factoryMode Door Sense Sensor Step:1"); // 开始测试
    ledBlueBlinkTime = 5 * SEC_DELAY;                             // LED常亮5秒
    mDelay(300);
    DEBUG_TRACE(ACK_TAG, "factoryMode Door Sense Sensor Step:2"); // 等待门磁触发关闭
    int state = getDoorSensorState();
    if (state) // 已是闭合状态
    {
        while (getDoorSensorState() > 0)
            ; // 等待开启重新触发
        state = 0;
    }
    while (getDoorSensorState() == 0)
        ; // 等待闭合
    mDelay(500);

    DEBUG_TRACE(ACK_TAG, "factoryMode Door Sense Sensor Step:3"); // 等待门磁触发开启
    state = getDoorSensorState();
    if (state == 0) // 已是开启状态
    {
        while (getDoorSensorState() == 0)
            ; // 等待闭合重新触发
    }
    while (getDoorSensorState() > 0)
        ; // 等待开启
    mDelay(500);

    DEBUG_TRACE(ACK_TAG, "factoryMode Door Sense Sensor Step:4"); // 等待防拆触发按下
    state = getTamperState();
    if (state) // 已是闭合状态
    {
        while (getTamperState() > 0)
            ; // 等待开启重新触发
        state = 0;
    }
    while (getTamperState() == 0)
        ; // 等待闭合
    mDelay(500);

    DEBUG_TRACE(ACK_TAG, "factoryMode Door Sense Sensor Step:5"); // 等待防拆触发松开
    state = getTamperState();
    if (state == 0) // 已是开启状态
    {
        while (getTamperState() == 0)
            ; // 等待闭合重新触发
    }
    while (getTamperState() > 0)
        ; // 等待开启
    mDelay(500);

    DEBUG_TRACE(ACK_TAG, "factoryMode Door Sense Sensor Step:6"); // 等待设备入网
    mDelay(1000);

    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 重新初始化设备
}
