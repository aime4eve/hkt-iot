
#include "adc.h"
#include "gpio.h"
#include "iic.h"
#include "p25q40.h"
#include "real_time.h"
#include "rtc.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "control_center.h"

struct device device_t;

u16 ledRedBlinkTime;

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    static u16 tick;

    if (!device_t.ble_connected) {
        GPIO_BLUE_LED_L;
    } else if (!LoRaWAN.joinState) {
        tick++;
        if (tick % 500 == 0)
            LL_GPIO_TogglePin(GPIO_BLUE_LED_PORT, GPIO_BLUE_LED_PIN);
    } else {
        GPIO_BLUE_LED_H;
    }
    if (ledRedBlinkTime) {
        GPIO_RED_LED_L;
        ledRedBlinkTime--;
        if (!ledRedBlinkTime)
            GPIO_RED_LED_H;
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

    if (!device_t.ble_wake_event && !uartLoRa.sendDelay && !device_t.sleep_delay && !factoryMode && !keepRunFlag && !device_t.press_long_event && !device_t.ble_connected &&
        ((LoRaWAN.joinState && !lwrb_write_count && !device_t.sync_state) || (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE) || !device_t.power_on)) {

        if (getLoRaWANStatus() == LoRaWAN_WAKE_STATUS)
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

#if NOR_FLASH_ENABLE
        p25q40_power_down();
#endif

        if (device_t.power_on) {
            lpmSleepTimeInit();
        } else {
            DEBUG_TRACE(LOG_TAG, "Enter DeepSleep Mode, Time Now: %04d-%02d-%02d:%02d:%02d:%02d",
                        systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }

        LL_TIM_DeInit(TIM16);
        App_Uart3_Deinit();
        // App_Uart1_Deinit();
        // App_LpUart1DeInit();
        // I2C1_DeInit();
        Spi3Deint();

        GPIO_CON1_EN_L;
        GPIO_CON2_EN_L;
        GPIO_CON1_A_L;
        GPIO_CON1_B_L;
        GPIO_CON2_A_L;
        GPIO_CON2_B_L;

        GPIO_BOOST_PWR_L;
        GPIO_RED_LED_H;
        GPIO_BLUE_LED_H;
        GPIO_BEEP_L;
        FLASH_WP_L;

        device_t.sleep_state       = 1;
        device_t.vcs_wake_event    = 0;
        device_t.rtc_wake_event    = 0;
        device_t.lptime_wake_event = 0;

        // HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
        EnterSleepMode();
        // while (1) {
        //     LL_IWDG_ReloadCounter(IWDG);
        //     if (device_t.rtc_wake_event || device_t.vcs_wake_event || device_t.ble_wake_event)
        //         break;
        // }

        // Delay_Us(20);
        // SystemClock_Config();
        // App_PortCfg();
        // App_Uart1Cfg();
        // App_LpUart1Cfg();
        App_Uart3Cfg();
        // I2C1_Init();
        TIM16_Init(1000, 16);
        // LPTIM1_Init();
        Spi3Init();
        LL_IWDG_ReloadCounter(IWDG);
        // RTC睡眠唤醒事件,重新使能串口和时钟
        if (device_t.rtc_wake_event) {
            device_t.rtc_wake_event = 0;
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
        } else if (device_t.vcs_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From VCS");
            device_t.vcs_wake_event = 0;
            if (!LoRaWAN.joinState && device_t.power_on)
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 离线状态下主动按按键进行重新入网
        } else if (device_t.pulse1_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Pulse1");
            device_t.pulse1_wake_event = 0;
        } else if (device_t.pulse2_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Pulse2");
            device_t.pulse2_wake_event = 0;
        } else if (device_t.insert1_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Insert1");
            device_t.insert1_wake_event = 0;
        } else if (device_t.insert2_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Insert2");
            device_t.insert2_wake_event = 0;
        } else if (device_t.ble_wake_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From BLE");
        } else if (device_t.lora_wake_event) {
            device_t.lora_wake_event = 0;
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From LoRa Mode");
        } else {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Unknown");
        }
        if (!device_t.power_on) {
            ledRedBlinkTime = SEC_DELAY / 2;
        } else {
            read_real_time();
            DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d, week: %02d, Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                        systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, systime.tm_wday, Timestamp);
        }
    }
}

/**
 * @brief  本地定时任务
 * @param
 * @retval
 */
void local_schedule_event(void)
{
    u8     i;
    time_t raw;
    time_t zonestamp = (device_t.timezone * 60 * 60);
    // if (!local_schedule_t.total_task_single_day)
    // return;

    // 处于任务执行状态
    if (local_schedule_t.current->is_start == 1 && local_schedule_t.current) {
        // 到达结束时间 或者满足脉冲数则关闭阀门
        if (Timestamp >= local_schedule_t.task_over_timestamp ||
            (local_schedule_t.pulse_count >= local_schedule_t.current->pulse_count && local_schedule_t.current->pulse_count > 0)) {
            local_schedule_t.current->is_start = 0;
            local_schedule_t.pulse_count       = 0;
            INFO("\r\n********************** End Task **********************\r\n");

            // 根据初始状态恢复到原来的状态
            if (!local_schedule_t.current->state) {
                switch (local_schedule_t.current->valve) {
                case 1:
                    forward_1_control();
                    break;
                case 2:
                    forward_2_control();
                    break;
                case 0:
                    forward_1_control();
                    forward_2_control();
                    break;
                default:
                    break;
                }
            } else {
                switch (local_schedule_t.current->valve) {
                case 1:
                    back_1_control();
                    break;
                case 2:
                    back_2_control();
                    break;
                case 0:
                    back_1_control();
                    back_2_control();
                    break;
                default:
                    break;
                }
            }
            device_t.sync_state = 1;

            // 下一个待执行的任务
            for (i = ++local_schedule_t.next_task_index; i < local_schedule_t.total_task_single_day; i++) {
                if (local_schedule_t.task_sort_id_valid[local_schedule_t.next_task_index]) {
                    local_schedule_t.next_task_index = i;
                    local_schedule_t.current         = &local_schedule_t.task_t[local_schedule_t.task_sort_id_valid[i] - 1];
                    if (local_schedule_t.current->is_vaild) {
                        struct tm time = {0};
                        time.tm_year   = systime.tm_year - 1900;
                        time.tm_mon    = systime.tm_mon;
                        time.tm_mday   = systime.tm_mday;
                        time.tm_hour   = local_schedule_t.current->start_hour;
                        time.tm_min    = local_schedule_t.current->start_min;
                        time.tm_sec    = 0;
                        time.tm_wday   = systime.tm_wday;
                        time.tm_isdst  = -1;
                        // local_schedule_t.task_start_timestamp = mktime(&time) - (device_t.timezone * 60 * 60);

                        // time.tm_hour = local_schedule_t.current->end_hour;
                        // time.tm_min = local_schedule_t.current->end_min;
                        // local_schedule_t.task_over_timestamp = mktime(&time) - (device_t.timezone * 60 * 60);

                        raw                                   = mktime(&time);
                        local_schedule_t.task_start_timestamp = raw - zonestamp;

                        time.tm_hour = local_schedule_t.current->end_hour;
                        time.tm_min  = local_schedule_t.current->end_min;

                        raw                                  = mktime(&time);
                        local_schedule_t.task_over_timestamp = raw - zonestamp;

                        INFO("\r\n********************** Next Task Info **********************\r\n");
                        INFO("ID[%d], Value[%d], State[%d], Pulse[%d], Repeat[%d]\n",
                             local_schedule_t.current->id, local_schedule_t.current->valve, local_schedule_t.current->state, local_schedule_t.current->pulse_count, local_schedule_t.current->repeat_duty);
                        INFO("Start Time: %02d-%02d\n", local_schedule_t.current->start_hour, local_schedule_t.current->start_min);
                        INFO("Over Time: %02d-%02d\n", local_schedule_t.current->end_hour, local_schedule_t.current->end_min);
                        INFO("Task Index[%d], Task Sort[%d], Task Timestamp[%d-%d]",
                             local_schedule_t.next_task_index, local_schedule_t.task_sort_id_valid[local_schedule_t.next_task_index], local_schedule_t.task_start_timestamp, local_schedule_t.task_over_timestamp);
                        INFO("\r\n*************************************************************\r\n");
                        break;
                    }
                }
            }
            if (i >= local_schedule_t.total_task_single_day) {
                INFO("\r\n********************** No Task **********************\r\n");
                local_schedule_t.task_start_timestamp = 0;
                local_schedule_t.task_over_timestamp  = 0;
                local_schedule_t.current              = NULL;
            }
        }
    } else if (real_time_task_t.is_start) {
        if ((real_time_task_t.task_over_timestamp <= Timestamp && real_time_task_t.task_over_timestamp > 0) || (real_time_task_t.pulse_count_trigger >= real_time_task_t.pulse_count && real_time_task_t.pulse_count > 0)) {
            INFO("\r\n real time pulse cout %d\r\n", real_time_task_t.pulse_count_trigger);
            // 根据初始状态恢复到原来的状态
            if (!real_time_task_t.state) {
                switch (real_time_task_t.valve) {
                case 1:
                    forward_1_control();
                    break;
                case 2:
                    forward_2_control();
                    break;
                case 0:
                    forward_1_control();
                    forward_2_control();
                    break;
                default:
                    break;
                }
            } else {
                switch (real_time_task_t.valve) {
                case 1:
                    back_1_control();
                    break;
                case 2:
                    back_2_control();
                    break;
                case 0:
                    back_1_control();
                    back_2_control();
                    break;
                default:
                    break;
                }
            }
            memset(&real_time_task_t, 0, sizeof(real_time_task_t));
            device_t.sync_state = 1;
            INFO("\r\n********************** End Real Task **********************\r\n");
        }
    } else {
        // 准备执行新任务
        if (Timestamp >= local_schedule_t.task_start_timestamp && local_schedule_t.task_start_timestamp > 0) {
            // local_schedule_t.task_start_timestamp = 2100000000; // 开始执行任务后将时间戳设置为最大 2036-07-18 21:20:00
            local_schedule_t.current->is_start = 1;
            // local_schedule_t.pulse_count = 0;
            // 根据状态执行对应操作
            if (!local_schedule_t.current->state) {
                switch (local_schedule_t.current->valve) {
                case 1:
                    back_1_control();
                    break;
                case 2:
                    back_2_control();
                    break;
                case 0:
                    back_1_control();
                    back_2_control();
                    break;
                default:
                    break;
                }
            } else {
                switch (local_schedule_t.current->valve) {
                case 1:
                    forward_1_control();
                    break;
                case 2:
                    forward_2_control();
                    break;
                case 0:
                    forward_1_control();
                    forward_2_control();
                    break;
                default:
                    break;
                }
            }
            device_t.sync_state = 1;
            INFO("\r\n********************** Current Task Info **********************\r\n");
            INFO("ID[%d], Value[%d], State[%d], Pulse[%d], Repeat[%d]\n",
                 local_schedule_t.current->id, local_schedule_t.current->valve, local_schedule_t.current->state, local_schedule_t.current->pulse_count, local_schedule_t.current->repeat_duty);
            INFO("Start Time: %02d-%02d\n", local_schedule_t.current->start_hour, local_schedule_t.current->start_min);
            INFO("Over Time: %02d-%02d\n", local_schedule_t.current->end_hour, local_schedule_t.current->end_min);
            INFO("Task Index[%d], Task Sort[%d], Task Timestamp[%d-%d]",
                 local_schedule_t.next_task_index, local_schedule_t.task_sort_id_valid[local_schedule_t.next_task_index], local_schedule_t.task_start_timestamp, local_schedule_t.task_over_timestamp);
            INFO("\r\n*************************************************************\r\n");
        }
    }
}

/**
 * @brief  本地定时任务初始化
 * @param
 * @retval
 */
void local_schedule_init(void)
{
    u8              i, j;
    time_t          raw;
    time_t          zonestamp       = (device_t.timezone * 60 * 60);
    struct schedule temp_task_t[16] = {0};
    memcpy(&temp_task_t, &local_schedule_t.task_t, sizeof(local_schedule_t.task_t));

    memset(local_schedule_t.task_sort_id_valid, 0, sizeof(local_schedule_t.task_sort_id_valid));
    memset(local_schedule_t.task_sort_id, 0, sizeof(local_schedule_t.task_sort_id));

    // 处于任务执行状态直接关闭当前任务
    // if (local_schedule_t.current->is_start == 1 && local_schedule_t.current && Timestamp > local_schedule_t.task_over_timestamp) {
    if (local_schedule_t.current->is_start == 1 && local_schedule_t.current) {
        local_schedule_t.current->is_start = 0;
        local_schedule_t.pulse_count       = 0;
        INFO("\r\n********************** End Task **********************\r\n");
        // 根据初始状态恢复到原来的状态
        if (!local_schedule_t.current->state) {
            switch (local_schedule_t.current->valve) {
            case 1:
                forward_1_control();
                break;
            case 2:
                forward_2_control();
                break;
            case 0:
                forward_1_control();
                forward_2_control();
                break;
            default:
                break;
            }
        } else {
            switch (local_schedule_t.current->valve) {
            case 1:
                back_1_control();
                break;
            case 2:
                back_2_control();
                break;
            case 0:
                back_1_control();
                back_2_control();
                break;
            default:
                break;
            }
        }
    }

    local_schedule_t.total_task            = 0;
    local_schedule_t.pulse_count           = 0;
    local_schedule_t.next_task_index       = 0;
    local_schedule_t.total_task_single_day = 0;

    j = 0;
    for (i = 0; i < 16; i++) {
        if (local_schedule_t.task_t[i].id) {
            local_schedule_t.total_task++;
        }
    }

    // 排序
    u8 is_order = 1;
    for (i = 0; i < 16; i++) {
        is_order = 1;
        for (j = 0; j < 15 - i; j++) {
            if ((temp_task_t[j].start_hour * 60 + temp_task_t[j].start_min) > (temp_task_t[j + 1].start_hour * 60 + temp_task_t[j + 1].start_min) || temp_task_t[j].is_vaild != 1) {
                struct schedule task;
                memcpy(&task, &temp_task_t[j], sizeof(task));
                memcpy(&temp_task_t[j], &temp_task_t[j + 1], sizeof(task));
                memcpy(&temp_task_t[j + 1], &task, sizeof(task));
                is_order = 0;
            }
        }
        if (is_order) {
            break;
        }
    }

    j = 0;
    for (i = 0; i < 16; i++) {
        if (temp_task_t[i].id)
            local_schedule_t.task_sort_id[j++] = temp_task_t[i].id;
    }

    INFO("\r\n********************** Local Task Info **********************");
    for (i = 0; i < 16; i++) {
        INFO("\nID[%d], Value[%d], State[%d]\n",
             local_schedule_t.task_t[i].id, local_schedule_t.task_t[i].valve, local_schedule_t.task_t[i].state);
        INFO("Start Time: %02d-%02d\n", local_schedule_t.task_t[i].start_hour, local_schedule_t.task_t[i].start_min);
        INFO("Over Time: %02d-%02d\n", local_schedule_t.task_t[i].end_hour, local_schedule_t.task_t[i].end_min);
        INFO("Repeat[%d], Vaild[%d], Pulse[%d]\n",
             local_schedule_t.task_t[i].repeat_duty, local_schedule_t.task_t[i].is_vaild, local_schedule_t.task_t[i].pulse_count);
    }
    INFO("\nID Sort[");
    for (i = 0; i < 16; i++) {
        INFO("%d ", local_schedule_t.task_sort_id[i]);
    }
    INFO("]\r\n");

    j = 0;
    // 当日任务初始化
    for (i = 0; i < 16; i++) {
        if (local_schedule_t.task_t[local_schedule_t.task_sort_id[i] - 1].is_vaild &&
            ((local_schedule_t.task_t[local_schedule_t.task_sort_id[i] - 1].repeat_duty >> ((systime.tm_wday ? systime.tm_wday : 7) - 1)) & 0x01)) {
            if ((local_schedule_t.task_t[local_schedule_t.task_sort_id[i] - 1].start_hour * 60 + local_schedule_t.task_t[local_schedule_t.task_sort_id[i] - 1].start_min) >= (systime.tm_hour * 60 + systime.tm_min)) {
                local_schedule_t.task_sort_id_valid[j++] = local_schedule_t.task_sort_id[i];
                local_schedule_t.total_task_single_day++;
            }
        }
    }

    INFO("Single Day Valid ID Sort[");
    for (i = 0; i < 16; i++) {
        INFO("%d ", local_schedule_t.task_sort_id_valid[i]);
    }
    INFO("]\r\n");
    INFO("Total Task Count[%d], Single Day Task Count[%d]", local_schedule_t.total_task, local_schedule_t.total_task_single_day);
    INFO("\r\n*************************************************************\r\n");

    // 下一个待执行的任务
    for (i = 0; i < local_schedule_t.total_task_single_day; i++) {
        local_schedule_t.current = &local_schedule_t.task_t[local_schedule_t.task_sort_id_valid[i] - 1];
        // 任务有效，且时间大于当前时间
        if (local_schedule_t.current->is_vaild && (local_schedule_t.current->start_hour * 60 + local_schedule_t.current->start_min >= systime.tm_hour * 60 + systime.tm_min)) {
            break;
        }
    }

    if (i < local_schedule_t.total_task_single_day) {
        struct tm time = {0};
        time.tm_year   = systime.tm_year - 1900;
        time.tm_mon    = systime.tm_mon;
        time.tm_mday   = systime.tm_mday;
        time.tm_hour   = local_schedule_t.current->start_hour;
        time.tm_min    = local_schedule_t.current->start_min;
        time.tm_sec    = 0;
        time.tm_wday   = systime.tm_wday;
        time.tm_isdst  = -1;

        raw                                   = mktime(&time);
        local_schedule_t.task_start_timestamp = raw - zonestamp;

        time.tm_hour = local_schedule_t.current->end_hour;
        time.tm_min  = local_schedule_t.current->end_min;

        raw                                  = mktime(&time);
        local_schedule_t.task_over_timestamp = raw - zonestamp;
        // local_schedule_t.task_over_timestamp = mktime(&time) - (device_t.timezone * 60 * 60);

        INFO("\r\n********************** Next Task Info **********************\r\n");
        INFO("ID[%d], Value[%d], State[%d], Pulse[%d], Repeat[%d]\n",
             local_schedule_t.current->id, local_schedule_t.current->valve, local_schedule_t.current->state, local_schedule_t.current->pulse_count, local_schedule_t.current->repeat_duty);
        INFO("Start Time: %02d-%02d\n", local_schedule_t.current->start_hour, local_schedule_t.current->start_min);
        INFO("Over Time: %02d-%02d\n", local_schedule_t.current->end_hour, local_schedule_t.current->end_min);
        INFO("Task Index[%d], Task Sort[%d], Task Timestamp[%d-%d]",
             local_schedule_t.next_task_index, local_schedule_t.task_sort_id_valid[local_schedule_t.next_task_index], local_schedule_t.task_start_timestamp, local_schedule_t.task_over_timestamp);
        INFO("\r\n*************************************************************\r\n");
    } else {
        local_schedule_t.task_start_timestamp = 0;
        local_schedule_t.task_over_timestamp  = 0;
        local_schedule_t.current              = NULL;
        INFO("\r\n********************** No Task **********************\r\n");
    }
}
