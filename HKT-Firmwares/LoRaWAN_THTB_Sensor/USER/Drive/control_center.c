
#include "systick.h"
#include "gpio.h"
#include "adc.h"
#include "tim.h"
#include "uart.h"
#include "control_center.h"
#include "communicate.h"
#include "spi.h"

#include "multi_button.h"
#include "iic.h"
#include "lis2dh12.h"
#include "ens210.h"
#include "rtc.h"
#include "gps.h"

#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

struct device device_t;

u16 ledBlueBlinkTime;

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    // static u32 tick;
    // if (ledBlueBlinkTime)
    // {
    //     ledBlueBlinkTime--;
    //     LL_GPIO_WriteOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN, 1);
    //     if (!ledBlueBlinkTime)
    //         LL_GPIO_WriteOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN, 0);
    // }
    // else if (!LoRaWAN.joinState)
    // {
    //     tick++;
    //     if (tick % 500 == 0)
    //         LL_GPIO_TogglePin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN);
    // }
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

    // 开关机管理
    if (device_t.powerState)
    {
        if (reed_switch_long) // 关机
        {
            reed_switch_long = 0;
            device_t.powerState = 0;
            BEEP_RUN(3000, 50);
            LoRaWAN_Init();
						sensor_stamp = Timestamp;
            goto sleep;
        }

        if (!uartLoRa.sendDelay && !device_t.sleepDelay && !factoryMode && !keepRunFlag &&
            ((LoRaWAN.joinState == 1 && !device_t.syncDeviceState) || (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE)))
        {
            lis2dh12_offset_t.event = ACC_STANDY_BY_EVENT;
            DEBUG_TRACE(LOG_TAG, "Current x offset:%.2f, y offset:%.2f, z offset:%.2f, roll:%.2f, pitch:%.2f",
                        lis2dh12_offset_t.current_x_offset, lis2dh12_offset_t.current_y_offset, lis2dh12_offset_t.current_z_offset, lis2dh12_offset_t.old_roll, lis2dh12_offset_t.old_pitch);
            lpmSleepTimeInit();
            DEBUG_TRACE(LOG_TAG, "Next Temperature Covert: %d, Next Gps Covert: %d, Time Now: %d", sensor_stamp, gps_stamp, Timestamp);
            DEBUG_TRACE(LOG_TAG, ">>>>>>Enter Sleep Mode<<<<<<");
            device_t.sleepState = 1;
            device_t.extiWakeEvent = 0;
            device_t.rtcWakeEvent = 0;
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
            HAL_Delay(20);
            HAL_TIM_Base_DeInit(&htim6);
            HAL_UART_DeInit(&hlpuart1);
            // HAL_UART_DeInit(&huart2);
            HAL_SPI_DeInit(&hspi1);
            HAL_I2C_DeInit(&hi2c1);
            HAL_ADC_DeInit(&hadc);
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

            HAL_Delay(20);
            // 恢复时钟及外设
            TIM6_Init(1000, 4);
            MX_LPUART1_UART_Init();
            MX_USART1_UART_Init();
            MX_USART2_UART_Init();
            MX_SPI1_Init();
            MX_I2C1_Init();
            MX_ADC_Init();
            sysTimeSwitchToRtcTime();
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
                }
                // device_t.updateLowPowerMode = 1; //每次唤醒同步模式
            }
        }
    }
    else
    {
    wake:
        if (getReedSwitchState() == GPIO_PIN_SET)
        {
            // DEBUG_TRACE(LOG_TAG, ">>>>>> Try Open Device <<<<<<");
            for (int p = 0; p < 30; p++)
            {
                if (getReedSwitchState() == GPIO_PIN_RESET)
                {
                    DEBUG_TRACE(LOG_TAG, ">>>>>> Try Open Device Fail <<<<<<");
                    goto sleep;
                }
                HAL_Delay(100);
            }
            DEBUG_TRACE(LOG_TAG, ">>>>>> OPEN Device <<<<<<");
            device_t.powerState = 1;
            BEEP_RUN(1000, 50);
            HAL_Delay(500);
            while (getReedSwitchState())
                ; // 等待干簧管松开
            reed_switch_long = 0;
#if FUNC_ACC_ENABLE
            Lis2dh12_Init();
#endif
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
        }
        else
        {
        sleep:
            HAL_Delay(20);
            DEBUG_TRACE(LOG_TAG, ">>>>>> OFF Device <<<<<<");
#if FUNC_ACC_ENABLE
            Lis2dh12_Init(); // 关闭加速计
#endif
            HAL_GPIO_WritePin(ENS210_PWR_GPIO_Port, ENS210_PWR_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOB, GPS_VBACK_Pin, GPIO_PIN_RESET); // 关闭热启动
            GPS_PWR_CTRL_L;
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
            HAL_Delay(20);
            HAL_TIM_Base_DeInit(&htim6);
            HAL_UART_DeInit(&hlpuart1);
            // HAL_UART_DeInit(&huart1);
            HAL_UART_DeInit(&huart2);
            HAL_SPI_DeInit(&hspi1);
            HAL_I2C_DeInit(&hi2c1);
            HAL_ADC_DeInit(&hadc);
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

            HAL_Delay(20);
            // 恢复时钟及外设
            MX_USART2_UART_Init();
            MX_LPUART1_UART_Init();
            MX_USART1_UART_Init();
            TIM6_Init(1000, 4);
            MX_SPI1_Init();
            MX_I2C1_Init();
            MX_ADC_Init();
            DEBUG_TRACE(LOG_TAG, ">>>>>> Wake UP Device <<<<<<");
            goto wake;
        }
    }
}

/**
 * @brief  工厂测试模式
 * @param
 * @retval
 */
void factoryTestMode(void)
{
    // HAL_Delay(1000);
    // DEBUG_TRACE(ACK_TAG, "factoryMode Car Track Step:1"); //开始测试
    //                                                       //    ledBlueBlinkTime = 5 * SEC_DELAY;                     // LED常亮5秒
    // HAL_Delay(1000);
    // DEBUG_TRACE(ACK_TAG, "factoryMode Car Track Step:2"); //等待门磁触发关闭
    // int state = getDoorSensorState();
    // if (state) //已是闭合状态
    // {
    //     while (getDoorSensorState() > 0)
    //         ; //等待开启重新触发
    //     state = 0;
    // }
    // while (getDoorSensorState() == 0)
    //     ; //等待闭合
    // HAL_Delay(500);

    // DEBUG_TRACE(ACK_TAG, "factoryMode Car Track Step:3"); //等待门磁触发开启
    // state = getDoorSensorState();
    // if (state == 0) //已是开启状态
    // {
    //     while (getDoorSensorState() == 0)
    //         ; //等待闭合重新触发
    // }
    // while (getDoorSensorState() > 0)
    //     ; //等待开启
    // HAL_Delay(500);

    // DEBUG_TRACE(ACK_TAG, "factoryMode Car Track Step:4");
    // HAL_Delay(1000);

    // DEBUG_TRACE(ACK_TAG, "factoryMode Car Track Step:5"); //等待设备入网
    // HAL_Delay(1000);

    // LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; //重新初始化设备
}
