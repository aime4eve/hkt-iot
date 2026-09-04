
#include "acc.h"
#include "adc.h"
#include "gpio.h"
#include "iic.h"
#include "p25q40.h"
#include "rtc.h"
#include "sht40x.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "control_center.h"

struct device device_t;

u16 ledBlinkTime;
u16 unsleepTime;

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    static u16 tick;

    if (!device_t.ble_connected) {
        GPIO_LED_H;
    } else if (!LoRaWAN.joinState) {
        tick++;
        if (tick % 500 == 0)
            LL_GPIO_TogglePin(GPIO_LED_PORT, GPIO_LED_PIN);
    } else {
        GPIO_LED_L;
    }
    if (ledBlinkTime) {
        GPIO_LED_L;
        ledBlinkTime--;
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
    if (device_t.ble_connected)
        return;
    // 5分钟未睡眠强制重启
    if (unsleepTime > (5 * 60)) {
        NVIC_SystemReset();
    }

    if (!device_t.ble_wake_event && !uartLoRa.sendDelay && !device_t.sleep_delay && !factoryMode && !keepRunFlag && !device_t.press_long_event &&
        !device_t.ud_det_cnt &&
        ((LoRaWAN.joinState && !lwrb_write_count && !device_t.sync_state) ||
         (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE) || !device_t.power_on)) {
        if (getLoRaWANStatus() == LoRaWAN_WAKE_STATUS)
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

#if NOR_FLASH_ENABLE
        p25q40_power_down();
#endif

        if (device_t.power_on) {
            lpmSleepTimeInit();
        } else {
            DEBUG_TRACE(LOG_TAG, "Enter DeepSleep Mode, Time Now: %04d-%02d-%02d:%02d:%02d:%02d", systime.tm_year, systime.tm_mon + 1,
                        systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }

        acc_power_down();
        LL_TIM_DeInit(TIM16);
        // Spi2Deint();
        Spi3Deint();
        // App_Uart2_Deinit();
        // App_Uart3_Deinit();

        GPIO_GPS_PWR_CTRL_L;
        GPIO_LED_H;
        FLASH_WP_L;

        device_t.sleep_state = 1;
        
        device_t.key_wake_event = 0;
        device_t.rtc_wake_event = 0;
        device_t.lptime_wake_event = 0;
        unsleepTime = 0;
        EnterSleepMode();

        // Delay_Us(20);
        // SystemClock_Config();
        // App_PortCfg();
        // App_Uart1Cfg();
        // App_LpUart1Cfg();
        App_Uart3Cfg();
        // I2C1_Init();
        TIM16_Init(1000, 16);
        // Spi3Init();
        Acc_Init();
        LL_IWDG_ReloadCounter(IWDG);
        // RTC睡眠唤醒事件,重新使能串口和时钟
        if (device_t.rtc_wake_event) {
            device_t.rtc_wake_event = 0;
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
        } else if (device_t.key_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Key");
            device_t.key_wake_event = 0;
            if (!LoRaWAN.joinState && device_t.power_on)
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 离线状态下主动按按键进行重新入网
        } else if (device_t.acc_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From ACC");
            device_t.acc_wake_event = 0;
        } else if (device_t.ble_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From BLE");
        } else {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Unknown");
        }
        sysTimeSwitchToRtcTime();
        ADC_BatterySglConvert(); // 每次唤醒转换电量信息
        Device_SglConvert();
        Angle_SglConvert();

        // device_t.ud_det_cnt = 5 * UD_CONVERT_CNT;
    }
}

/**
 * @brief  转换超声波模块距离
 * @param
 * @retval
 */
int convert_Distance(void)
{
    u8 txdata = 0xFF;
    u16 ud_distance;
    u16 retry = 0;
    u16 time_slice = 100;
    u32 recovert_time = 0;
    memset(uartUd.recvData, 0, sizeof(uartUd.recvData));
    uartUd.recvLen = 0;
    LL_IWDG_ReloadCounter(IWDG);
    while (1) {
        if (!uartUd.recvTimeout && uartUd.recvLen) {
            if (uartUd.recvData[0] == 0xFF) {
                u16 sum = uartUd.recvData[0] + uartUd.recvData[1] + uartUd.recvData[2];
                if ((sum & 0x00FF) == uartUd.recvData[3]) {
                    ud_distance = uartUd.recvData[1] << 8 | uartUd.recvData[2];
                    if (ud_distance > 0)
                        return ud_distance;
                }
            }
            memset(uartUd.recvData, 0, sizeof(uartUd.recvData));
            uartUd.recvLen = 0;
        }
        if (retry > 10) {
            /* 指令超时 */
            return -1;
        }

        if (check_delay_expired(recovert_time, time_slice)) {
            UD_SendData(&txdata, 1);
            retry++;
            recovert_time = get_syspant_ms();
        }
    }
}

void UD_Period_Event(void)
{
    static u8 cnt;
    static u8 threshold_state;
    static u16 ud_distance[UD_CONVERT_CNT + 1];
    static u32 covert_time;
    int temperature, humidity;

    if (!uartUd.recvTimeout && uartUd.recvLen) {
        if (uartUd.recvData[0] == 0xFF) {
            u16 sum = uartUd.recvData[0] + uartUd.recvData[1] + uartUd.recvData[2];
            if ((sum & 0x00FF) == uartUd.recvData[3]) {
                ud_distance[cnt++] = uartUd.recvData[1] << 8 | uartUd.recvData[2];
            }
        }
        memset(uartUd.recvData, 0, sizeof(uartUd.recvData));
        uartUd.recvLen = 0;
    }

    if (cnt >= UD_CONVERT_CNT) {
        cnt = 0;
        quick_sort((short *)ud_distance, 0, sizeof(ud_distance) / sizeof(unsigned short) - 1);
        device_t.ud_distance = getAccRaw((short *)ud_distance, UD_CONVERT_CNT);
        if (device_t.ud_distance < 30 || device_t.ud_distance > 4500) {
            threshold_state = 0xFF;
            device_t.ud_distance = 0;
        } else if (device_t.ud_distance < device_t.low_threshold_distance) {
            threshold_state = 1;
        } else if (device_t.ud_distance > device_t.high_threshold_distance && device_t.high_threshold_distance > 0) {
            threshold_state = 2;
        } else {
            threshold_state = 0;
        }
        if (device_t.threshold_state != threshold_state) {
            device_t.threshold_state = threshold_state;
            device_t.sync_state = 1;
            uartLoRa.sendDelay = 3 * SEC_DELAY;
        }
        DEBUG_TRACE(LOG_TAG, "Convert UD Distance: %dmm, Threshold State: %d", device_t.ud_distance, device_t.threshold_state);
        if (convertTemperature_Result(&temperature, &humidity) == 0) {
            // 数据有效
            if (temperature >= 0)
                device_t.temperature = temperature;
            else
                device_t.temperature = (-temperature) | 0x800000;
            device_t.humidity = humidity;
            DEBUG_TRACE(LOG_TAG, "SHT40X Read Temperature: %0.2fC, Humidity: %0.2f%RH", temperature / 1000.0f, humidity / 1000.0f);
        }
    }

    if (!device_t.power_on)
        return;
    if (!check_delay_expired(covert_time, 100))
        return;
    covert_time = get_syspant_ms();

    if (device_t.ud_det_cnt) {
        device_t.ud_det_cnt--;
        UD_SendData(NULL, 1);
    }
}

void Device_SglConvert(void)
{
    static u8 threshold_state;
    int temperature, humidity;
    dataConvertTimestamp = Timestamp + 5 * 60; // 固定5分钟周期唤醒转换
    int ud_distance = convert_Distance();
    if (!device_t.is_slant) {
        if (ud_distance == -1) {
            ud_distance = 0;
        }
        device_t.ud_distance = ud_distance;
        if (device_t.ud_distance < 30 || device_t.ud_distance > 4500) {
            threshold_state = 0xFF;
            device_t.ud_distance = 0;
        } else if (device_t.ud_distance < device_t.low_threshold_distance) {
            threshold_state = 1;
        } else if (device_t.ud_distance > device_t.high_threshold_distance && device_t.high_threshold_distance > 0) {
            threshold_state = 2;
        } else {
            threshold_state = 0;
        }

        if (device_t.threshold_state != threshold_state) {
            device_t.threshold_state = threshold_state;
            device_t.sync_state = 1;
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        }
        DEBUG_TRACE(LOG_TAG, "Convert UD Distance: %dmm, Threshold State: %d", ud_distance, device_t.threshold_state);
    }
    if (convertTemperature_Result(&temperature, &humidity) == 0) {
        // 数据有效
        if (temperature >= 0)
            device_t.temperature = temperature;
        else
            device_t.temperature = (-temperature) | 0x800000;
        device_t.humidity = humidity;
        if (temperature >= 50000 && device_t.ht_alarm == 0) {
            device_t.sync_state = 1;
            device_t.ht_alarm = 1;
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        } else if (temperature < 50000 && device_t.ht_alarm == 1) {
            device_t.sync_state = 1;
            device_t.ht_alarm = 0;
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        }
        DEBUG_TRACE(LOG_TAG, "Convert Temperature: %0.2fC, Humidity: %0.2f%RH, HT Alarm: %d", temperature / 1000.0f, humidity / 1000.0f,
                    device_t.ht_alarm);
    }
}
