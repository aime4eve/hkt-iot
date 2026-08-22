
#include "control_center.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "communicate.h"
#include "gpio.h"
#include "iic.h"
#include "multi_button.h"
#include "p25q40.h"
#include "qmc5883l.h"
#include "rkb1145d.h"
#include "rtc.h"
#include "spi.h"
#include "stm32l4xx_hal_pwr.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

// 静态函数声明
static bool initialize_and_check(uint32_t *covert_time);
static void process_magnetic_data(void);
static uint16_t calculate_total_magnetic_difference(void);
static uint8_t count_active_magnetic_axes(uint16_t total_diff);
static uint8_t check_axis_activity(int16_t value, int16_t offset, uint16_t threshold, uint16_t threshold_sum);
static void update_magnetic_state(uint8_t axis_act);
static void process_valid_magnetic_state(uint8_t *mag_valid_cnt, uint8_t *mag_invalid_cnt);
static void process_invalid_magnetic_state(uint8_t *mag_valid_cnt, uint8_t *mag_invalid_cnt);
static void process_radar_data(void);
static void calculate_adjust_radar_power(const u16 *power, const u16 *base, u16 *adjust);
static uint32_t calculate_total_radar_power(const u16 *power);
static uint16_t calculate_max_radar_power(const u16 *power);
static uint8_t calculate_max_radar_number(const u16 *power);
static bool is_radar_covered(uint32_t total_power, u16 *power);
static void update_parking_state(void);
static void update_fusion_mode_parking_state(void);
static void update_single_mode_parking_state(void);
static void update_radar_priority_parking_state(void);

struct device device_t;
u16 ledRedBlinkTime;

/**
 * @brief  LED显示控制，在1ms中断函数中执行
 * @param
 * @retval
 */
void Led_Blink_Process(void)
{
    static u32 tick;
    tick++;

    if (!LoRaWAN.joinState) {
        tick++;
        if (tick % 500 == 0)
            LL_GPIO_TogglePin(LED_BLUE_PORT, LED_BLUE_PIN);
    }

    if (ledRedBlinkTime) {
        LED_RED_L;
        ledRedBlinkTime--;
        if (!ledRedBlinkTime)
            LED_RED_H;
    }
}

/**
 * @brief  系统执行低功耗模式配置
 * @param
 * @retval
 */
void systemLowPowerMode_Process(void)
{
    static uint16_t sleep_cnt;

    if (device_t.ble_connected && !device_t.ble_sleep_mode)
        return;

    if (!device_t.ble_wake_event && !uartLoRa.sendDelay && !device_t.sleep_delay && !factoryMode && !keepRunFlag &&
        (LoRaWAN.joinState == 1 || (LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE) ||
         !device_t.power_on)) {

    __sleep:
        if (getLoRaWANStatus() == LoRaWAN_WAKE_STATUS)
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

        magnetometer_standby();
#if NOR_FLASH_ENABLE
        p25q40_power_down();
#endif
        DEBUG_TRACE(LOG_TAG, "Enter DeepSleep Mode, Time Now: %04d-%02d-%02d:%02d:%02d:%02d, Timestamp :%ld, Report Timestamp :%ld", systime.tm_year,
                    systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp, dataReportTimestamp);
        if (device_t.ble_connected) {
            BLE_SendCMD("EnterSleep !!!");
        }
        if (device_t.power_on) {
            LPTIM1_Continue();
        }

        LL_TIM_DeInit(TIM16);
        // App_Uart2_Deinit();
        App_Uart3_Deinit();
        // App_Uart1_Deinit();
        // App_LpUart1DeInit();
        // I2C1_DeInit();
        Spi3Deint();

        // PWR_CTRL_L;
        LED_RED_H;
        LED_BLUE_H;

        // LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOA |
        //                       LL_AHB2_GRP1_PERIPH_GPIOB |
        //                       LL_AHB2_GRP1_PERIPH_GPIOC |
        //                       LL_AHB2_GRP1_PERIPH_GPIOD |
        //                       LL_AHB2_GRP1_PERIPH_GPIOH);

        device_t.sleep_state = 1;
        device_t.mag_wake_event = 0;
        device_t.vcs_wake_event = 0;
        device_t.rtc_wake_event = 0;
        device_t.lptime_wake_event = 0;

        // HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
        EnterSleepMode();
        // while (1) {
        //     LL_IWDG_ReloadCounter(IWDG);
        //     if (device_t.rtc_wake_event || device_t.vcs_wake_event || device_t.mag_wake_event || device_t.ble_wake_event ||
        //     device_t.lptime_wake_event)
        //         break;
        // }

        // Delay_Us(20);
        Systick_Init();
        SystemClock_Config();
        PeriphCommonClock_Config();
        App_PortCfg();
        App_Uart1Cfg();
        App_LpUart1Cfg();
        App_Uart2Cfg();
        App_Uart3Cfg();
        I2C1_Init();
        TIM16_Init(1000, 16);
        Spi3Init();
        LPTIM1_Stop();
        LL_IWDG_ReloadCounter(IWDG);
        sysTimeSwitchToRtcTime();
        // RTC睡眠唤醒事件,重新使能串口和时钟
        if (device_t.rtc_wake_event || device_t.vcs_wake_event || device_t.mag_wake_event || device_t.ble_wake_event || device_t.lptime_wake_event) {
            if (device_t.rtc_wake_event) {
                device_t.rtc_wake_event = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
            }
            if (device_t.mag_wake_event) {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From MAG");
                device_t.mag_wake_event = 0;
            }
            if (device_t.vcs_wake_event) {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From VCS");
                device_t.vcs_wake_event = 0;
            }
            if (device_t.ble_wake_event) {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From BLE");
            }
            if (device_t.lptime_wake_event) {
                device_t.lptime_wake_event = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From LpTime");
            }
        } else {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From Unknown");
        }

        magnetometer_init();
        convert_light_sensor();

        if (((dataReportTimestamp <= Timestamp || device_t.sync_state) && LoRaWAN.joinState) ||
            (!LoRaWAN.joinState && reconnect_stamp <= Timestamp)) {
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        } else {
            if (!device_t.ble_wake_event && device_t.power_on) {
                if (device_t.park_mode == 2) {
                    /* 雷达优先模式: 每3个LPTIM周期唤醒雷达一次
                     * 序列: mag → mag → mag+radar → mag → mag → mag+radar (16s周期) */
                    device_t.radar_wake_phase = (device_t.radar_wake_phase + 1) % RKB_RADAR_WAKE_INTERVAL;

                    if (magnetometer_is_actived()) {
                        /* 磁力计检测到变化 */
                        if (device_t.park_state == 1) {
                            /* 当前有车：快速雷达确认是否真的离场，避免误唤醒 */
                            bool still_parked = false;
                            if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
                                PWR_CTRL_H;
                                mDelay(500);
                            }
                            if (rkb_search_distance_cal()) {
                                calculate_adjust_radar_power(rkb1145d_t.power, rkb1145d_t.base_power, rkb1145d_t.adjust_power);
                                uint16_t max_val = rkb1145d_t.power[0];
                                uint8_t max_idx = 0;
                                for (uint8_t i = 1; i < 10; i++) {
                                    if (rkb1145d_t.power[i] > max_val) {
                                        max_val = rkb1145d_t.power[i];
                                        max_idx = i;
                                    }
                                }
                                bool covered = (max_idx < 2 && max_val >= RKB_COVERED_MIN && max_val <= RKB_COVERED_MAX);
                                bool low_chassis = (max_idx < 2 && max_val > RKB_COVERED_MAX);
                                bool vehicle = low_chassis;
                                for (uint8_t i = 2; i <= 7 && !vehicle; i++) {
                                    if (rkb1145d_t.adjust_power[i] > RKB_VEHICLE_THRESHOLD)
                                        vehicle = true;
                                }
                                /* 雷达仍检测到车辆或被覆盖 → 状态未变，继续睡 */
                                if (vehicle || covered) {
                                    still_parked = true;
                                }
                            }
                            if (still_parked) {
                                DEBUG_TRACE(LOG_TAG, "Mode2: Mag Actived but Radar Still Parked, Sleep");
                                App_Uart2_Deinit();
                                PWR_CTRL_L;
                                goto __sleep;
                            }
                        }
                        /* 确认有状态变化，保持唤醒 */
                        sleep_cnt = 0;
                        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                        DEBUG_TRACE(LOG_TAG, "Mode2: Mag Actived, Stay Awake");
                    } else if (device_t.radar_wake_phase == 0) {
                        /* 雷达巡检周期到：快速读一次雷达，有变化才正式唤醒 */
                        bool radar_changed = false;
                        if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
                            PWR_CTRL_H;
                            mDelay(500);
                        }
                        /* 读取雷达频谱数据 */
                        if (rkb_search_distance_cal()) {
                            calculate_adjust_radar_power(rkb1145d_t.power, rkb1145d_t.base_power, rkb1145d_t.adjust_power);
                            /* 找频谱峰值通道及能量 */
                            uint8_t max_idx = 0;
                            uint16_t max_val = rkb1145d_t.power[0];
                            for (uint8_t i = 1; i < 10; i++) {
                                if (rkb1145d_t.power[i] > max_val) {
                                    max_val = rkb1145d_t.power[i];
                                    max_idx = i;
                                }
                            }
                            /* 覆盖：峰值在0-20cm 且 峰值能量在[600,1500]水反射范围 */
                            bool covered = (max_idx < 2 &&
                                            max_val >= RKB_COVERED_MIN &&
                                            max_val <= RKB_COVERED_MAX);
                            /* 低底盘车辆：近场峰值>1500(金属强反射) */
                            bool low_chassis = (max_idx < 2 && max_val > RKB_COVERED_MAX);
                            /* 车辆检测: 低底盘车 或 adjust_power[2]~[7]有能量 */
                            bool vehicle = low_chassis;
                            for (uint8_t i = 2; i <= 7 && !vehicle; i++) {
                                if (rkb1145d_t.adjust_power[i] > RKB_VEHICLE_THRESHOLD)
                                    vehicle = true;
                            }
                            /* 与当前车位状态比较：状态翻转才正式唤醒 */
                            if (device_t.park_state == 1) {
                                /* 当前有车 → 雷达无车或被覆盖 → 可能离场，唤醒确认 */
                                if (!vehicle || covered) {
                                    radar_changed = true;
                                    DEBUG_TRACE(LOG_TAG, "Mode2: Radar Exit Detect, Stay Awake");
                                }
                            } else {
                                /* 当前无车 → 雷达有车且未覆盖 → 可能来车，唤醒确认 */
                                if (vehicle && !covered) {
                                    radar_changed = true;
                                    DEBUG_TRACE(LOG_TAG, "Mode2: Radar Entry Detect, Stay Awake");
                                }
                            }
                        }
                        if (radar_changed) {
                            sleep_cnt = 0;
                            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                        } else {
                            /* 无变化：关闭雷达省电，继续睡眠 */
                            App_Uart2_Deinit();
                            PWR_CTRL_L;
                            goto __sleep;
                        }
                    } else {
                        goto __sleep;
                    }
                } else {
                    // 固定周期唤醒检测雷达波模块       1200 / 8 = 150 20min
                    if (magnetometer_is_actived() || ++sleep_cnt >= 150) { // 磁力计存在变化,延时进入睡眠
                        sleep_cnt = 0;
                        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                        // PWR_CTRL_H;
                        // Delay_Ms(200);
                        DEBUG_TRACE(LOG_TAG, "Mag Is Actived And Sleep Delay");
                    } else {
                        goto __sleep;
                    }
                }
            }
        }
#if NOR_FLASH_ENABLE
        p25q40_release();
#endif
    }
}

void park_process(void)
{
    static uint32_t covert_time = 0;

    if (!initialize_and_check(&covert_time))
        return;

    /* 雷达优先模式走独立磁力计处理路径 */
    if (device_t.park_mode != 2) {
        process_magnetic_data();
    }
    process_radar_data();
    update_parking_state();
}

static bool initialize_and_check(uint32_t *covert_time)
{
    /* 雷达优先模式(park_mode==2)不依赖磁力计校准 */
    if (!device_t.power_on || device_t.park_delay)
        return false;
    if (device_t.park_mode != 2) {
        if (!qmc5883l_t.is_calibration || !device_t.mag_valid)
            return false;
    }

    if (!check_delay_expired(*covert_time, DATA_COLLECT_INTERVAL))
        return false;

    *covert_time = get_syspant_ms();
    return true;
}

static void process_magnetic_data(void)
{
    uint8_t axis_act = 0;
    uint16_t total_diff = 0;

    device_t.mag_valid = 0;

    total_diff = calculate_total_magnetic_difference();
    axis_act = count_active_magnetic_axes(total_diff);

    update_magnetic_state(axis_act);
}

static uint16_t calculate_total_magnetic_difference(void)
{
    uint16_t total_diff = 0;
    total_diff += abs(qmc5883l_t.offset.x - qmc5883l_t.xyz.x);
    total_diff += abs(qmc5883l_t.offset.y - qmc5883l_t.xyz.y);
    total_diff += abs(qmc5883l_t.offset.z - qmc5883l_t.xyz.z);
    return total_diff;
}

static uint8_t count_active_magnetic_axes(uint16_t total_diff)
{
    uint8_t axis_act = 0;
    uint16_t threshold_sum = qmc5883l_t.threshold.x + qmc5883l_t.threshold.y + qmc5883l_t.threshold.z;

    if (total_diff >= threshold_sum * 0.85) {
        axis_act++;
    }

    axis_act += check_axis_activity(qmc5883l_t.xyz.x, qmc5883l_t.offset.x, qmc5883l_t.threshold.x, threshold_sum);
    axis_act += check_axis_activity(qmc5883l_t.xyz.y, qmc5883l_t.offset.y, qmc5883l_t.threshold.y, threshold_sum);
    axis_act += check_axis_activity(qmc5883l_t.xyz.z, qmc5883l_t.offset.z, qmc5883l_t.threshold.z, threshold_sum);

    return axis_act;
}

static uint8_t check_axis_activity(int16_t value, int16_t offset, uint16_t threshold, uint16_t threshold_sum)
{
    uint8_t activity = 0;
    int16_t diff = abs(value - offset);

    if (diff >= threshold) {
        activity++;
        if (diff >= threshold_sum)
            activity++;
    }

    return activity;
}

static void update_magnetic_state(uint8_t axis_act)
{
    static uint8_t mag_valid_cnt = 0, mag_invalid_cnt = 0;

    if (axis_act >= 2) {
        process_valid_magnetic_state(&mag_valid_cnt, &mag_invalid_cnt);
    } else {
        process_invalid_magnetic_state(&mag_valid_cnt, &mag_invalid_cnt);
    }
}

static void process_valid_magnetic_state(uint8_t *mag_valid_cnt, uint8_t *mag_invalid_cnt)
{
    if (*mag_invalid_cnt)
        (*mag_invalid_cnt)--;

    if (qmc5883l_t.is_changed != 1) {
        (*mag_valid_cnt)++;
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }

    if (*mag_valid_cnt > MAG_CHANGED_CNT) {
        // *mag_valid_cnt   = 0;
        *mag_invalid_cnt = 0;
        if (qmc5883l_t.is_changed != 1) {
            qmc5883l_t.is_changed = 1;
            if (device_t.park_mode == 0) {
                if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
                    PWR_CTRL_H;
                    mDelay(500);
                }
            }
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, ">>>>>>>> MAG Changed State [%02X] <<<<<<<<", qmc5883l_t.is_changed);
        }
    }
}

static void process_invalid_magnetic_state(uint8_t *mag_valid_cnt, uint8_t *mag_invalid_cnt)
{
    if (*mag_valid_cnt)
        (*mag_valid_cnt)--;

    if (qmc5883l_t.is_changed == 1) {
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        (*mag_invalid_cnt)++;
    }

    if (*mag_invalid_cnt > MAG_CHANGED_CNT) {
        // *mag_invalid_cnt = 0;
        *mag_valid_cnt = 0;
        if (qmc5883l_t.is_changed == 1) {
            qmc5883l_t.is_changed = 0;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, ">>>>>>>> MAG Changed State [%02X] <<<<<<<<", qmc5883l_t.is_changed);
        }
    }
}

static void process_radar_data(void)
{
    static uint8_t rkb_cover_cnt = 0;
    static uint8_t rkb_valid_cnt = 0;
    static uint8_t rkb_invalid_cnt = 0;
    static uint8_t rkb_restart_cnt = 0;
    // 电源未开启时不处理
    if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN))
        return;
    calculate_adjust_radar_power(rkb1145d_t.power, rkb1145d_t.base_power, rkb1145d_t.adjust_power);

    /* 共用计算：最大能量位置与距离 */
    uint32_t total_power = calculate_total_radar_power(rkb1145d_t.adjust_power);
    uint16_t max_power = calculate_max_radar_power(rkb1145d_t.adjust_power);
    uint8_t max_num = calculate_max_radar_number(rkb1145d_t.adjust_power);

    /* 雷达优先模式(park_mode==2): 专用覆盖/车辆检测逻辑 */
    if (device_t.park_mode == 2) {
        static uint8_t rkb_cover_m2_cnt = 0;
        static uint8_t rkb_uncover_m2_cnt = 0;
        static uint8_t rkb_vehicle_m2_cnt = 0;
        static uint8_t rkb_novehicle_m2_cnt = 0;

        /* 覆盖：峰值在0-20cm 且 峰值能量在[600,1500]水反射范围 */
        bool covered = (max_num < 2 &&
                        max_power >= RKB_COVERED_MIN &&
                        max_power <= RKB_COVERED_MAX);
        if (covered) {
            rkb_cover_m2_cnt++;
            rkb_uncover_m2_cnt = 0;
            if (rkb_cover_m2_cnt >= RD_CHANGED_CNT) {
                device_t.radar_covered_flag = 1;
            }
        } else {
            rkb_uncover_m2_cnt++;
            rkb_cover_m2_cnt = 0;
            if (rkb_uncover_m2_cnt >= RD_CHANGED_CNT) {
                device_t.radar_covered_flag = 0;
            }
        }

        /* 低底盘车辆：近场峰值>1500(金属强反射) */
        bool low_chassis = (max_num < 2 && max_power > RKB_COVERED_MAX);
        /* 车辆检测: 低底盘车 或 adjust_power[2]~[7]有能量 */
        bool vehicle = low_chassis;
        for (uint8_t i = 2; i <= 7 && !vehicle; i++) {
            if (rkb1145d_t.adjust_power[i] > RKB_VEHICLE_THRESHOLD) {
                vehicle = true;
                break;
            }
        }
        if (vehicle) {
            rkb_vehicle_m2_cnt++;
            rkb_novehicle_m2_cnt = 0;
            if (rkb_vehicle_m2_cnt >= RD_CHANGED_CNT) {
                device_t.radar_detected = 1;
            }
        } else {
            rkb_novehicle_m2_cnt++;
            rkb_vehicle_m2_cnt = 0;
            /* 出场去抖更长(×3)，防止雷达读数波动导致误报离场 */
            if (rkb_novehicle_m2_cnt >= RD_CHANGED_CNT * 3) {
                device_t.radar_detected = 0;
            }
        }

        INFO("\r\n[M2] power[");
        for (uint8_t i = 0; i < 10; i++) {
            INFO("%d", rkb1145d_t.power[i]);
            if (i < 9) INFO(" ");
        }
        INFO("] adj[");
        for (uint8_t i = 0; i < 10; i++) {
            INFO("%d", rkb1145d_t.adjust_power[i]);
            if (i < 9) INFO(" ");
        }
        INFO("] max=%d dist=%dcm covered=%d vehicle=%d",
             max_power, max_num * 10, device_t.radar_covered_flag, device_t.radar_detected);

        /* 模式2不继续走原有的is_changed逻辑 */
        return;
    }

    /* 判断雷达数据 */
    if (is_radar_covered(total_power, rkb1145d_t.adjust_power) && max_num < RKB_MIN_DISTANCE_THRESHOLD) { // 雷达波模块被覆盖
        rkb_cover_cnt++;
        if (rkb_valid_cnt)
            rkb_valid_cnt--;
        if (rkb_invalid_cnt)
            rkb_invalid_cnt--;

        if (rkb_cover_cnt > RD_CHANGED_CNT) {
            // rkb_cover_cnt   = 0;
            rkb_valid_cnt = 0;
            rkb_invalid_cnt = 0;

            // 有地磁感应，雷达波模块未正常感应到时，重启雷达波模块 最大重试3次
            if (rkb_restart_cnt < RD_CHANGED_CNT && qmc5883l_t.is_changed) {
                rkb_restart_cnt++;
                PWR_CTRL_L;
                // App_Uart2_Deinit();
                LL_IWDG_ReloadCounter(IWDG);
                mDelay(3000);
                LL_IWDG_ReloadCounter(IWDG);
                // App_Uart2Cfg();
                PWR_CTRL_H;
                mDelay(500);
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            }

            if (rkb1145d_t.is_changed != 0xFF) {
                rkb1145d_t.is_changed = 0xFF;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, ">>>>>>>> RKB Changed State [%02X] <<<<<<<<", rkb1145d_t.is_changed);
            }
        }
    } else { // 雷达波模块未被覆盖

        if (rkb_cover_cnt) {
            rkb_cover_cnt--;
        }
        if (!rkb_cover_cnt && rkb1145d_t.is_changed == 0xFF) {
            rkb1145d_t.is_changed = 0;
        }

        // 正常距离
        if (max_num >= RKB_MIN_DISTANCE_THRESHOLD) {
            // 雷达波模块感应到车辆
            if (max_power >= RKB_INDUCTION_CAR_THRESHOLD) {
                rkb_valid_cnt++;
                if (rkb_invalid_cnt)
                    rkb_invalid_cnt--;

                if (rkb_valid_cnt > RD_CHANGED_CNT) {
                    rkb_cover_cnt = 0;
                    // rkb_valid_cnt   = 0;
                    rkb_invalid_cnt = 0;
                    rkb_restart_cnt = 0;
                    if (rkb1145d_t.is_changed != 1) {
                        rkb1145d_t.is_changed = 1;
                        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                        DEBUG_TRACE(LOG_TAG, ">>>>>>>> RKB Changed State [%02X] <<<<<<<<", rkb1145d_t.is_changed);
                    }
                }
            } else {
                if (rkb_valid_cnt)
                    rkb_valid_cnt--;

                if (!rkb_valid_cnt && rkb1145d_t.is_changed) {
                    rkb1145d_t.is_changed = 0;
                    rkb_restart_cnt = 0;
                    if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                    DEBUG_TRACE(LOG_TAG, ">>>>>>>> RKB Changed State [%02X] <<<<<<<<", rkb1145d_t.is_changed);
                }

                // 发现异常信号 主动重启雷达波模块
                if (max_power >= RKB_UNUSUAL_THRESHOLD) {
                    rkb_invalid_cnt++;
                    if (rkb_invalid_cnt > RD_CHANGED_CNT) {
                        rkb_invalid_cnt = 0;

                        // 有地磁感应，雷达波模块未正常感应到时，重启雷达波模块 最大重试3次
                        if (rkb_restart_cnt < RD_CHANGED_CNT && qmc5883l_t.is_changed) {
                            rkb_restart_cnt++;
                            PWR_CTRL_L;
                            // App_Uart2_Deinit();
                            LL_IWDG_ReloadCounter(IWDG);
                            mDelay(5000);
                            LL_IWDG_ReloadCounter(IWDG);
                            // App_Uart2Cfg();
                            PWR_CTRL_H;
                            mDelay(500);
                            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                        }
                    }
                } else {
                    if (rkb_invalid_cnt)
                        rkb_invalid_cnt--;
                }
            }
        }
    }
}

static void calculate_adjust_radar_power(const u16 *power, const u16 *base, u16 *adjust)
{
    uint32_t total = calculate_total_radar_power(base);
    for (int i = 0; i < 10; i++) {
        if (total) {
            adjust[i] = abs(power[i] - base[i]);
        } else {
            adjust[i] = power[i];
        }
    }
}

static uint32_t calculate_total_radar_power(const u16 *power)
{
    uint32_t total_power = 0;
    // 使用指针算术来优化循环
    const u16 *end = power + 10;
    const u16 *p = power + 3;

    while (p < end) {
        total_power += *p++;
    }
    return total_power;
}

static uint16_t calculate_max_radar_power(const u16 *power)
{
    uint16_t max_power = power[0];
    // 使用指针算术来优化循环
    const u16 *end = power + 10;
    const u16 *p = power + 1;

    while (p < end) {
        if (*p > max_power) {
            max_power = *p;
        }
        p++;
    }
    return max_power;
}

static uint8_t calculate_max_radar_number(const u16 *power)
{
    uint32_t max_power = power[0];
    uint8_t max_num = 0;
    // 使用指针算术来优化循环
    const u16 *end = power + 10;
    const u16 *p = power + 1;
    uint32_t current_index = 1;

    while (p < end) {
        if (*p > max_power) {
            max_power = *p;
            max_num = current_index;
        }
        p++;
        current_index++;
    }
    rkb1145d_t.max_power_distance = max_num * 10;
    return max_num;
}

static bool is_radar_covered(uint32_t total_power, u16 *power)
{
    // return total_power <= 50 ||
    //        (rkb1145d_t.power[0] > RKB_INDUCTION_THRESHOLD ||
    //         rkb1145d_t.power[1] > RKB_INDUCTION_THRESHOLD ||
    //         rkb1145d_t.power[2] > RKB_INDUCTION_THRESHOLD);

    // return total_power <= 50 ||
    //        (rkb1145d_t.power[0] > RKB_INDUCTION_THRESHOLD ||
    //         rkb1145d_t.power[1] > RKB_INDUCTION_THRESHOLD);

    return (power[0] > RKB_INDUCTION_THRESHOLD || power[1] > RKB_INDUCTION_THRESHOLD);
}

static void update_parking_state(void)
{
    if (device_t.park_mode == 0) {
        update_fusion_mode_parking_state();
    } else if (device_t.park_mode == 1) {
        update_single_mode_parking_state();
    } else if (device_t.park_mode == 2) {
        update_radar_priority_parking_state();
    }
}

static void update_fusion_mode_parking_state(void)
{
    if (!qmc5883l_t.is_changed) {
//         if (device_t.park_state != 0xFF && rkb1145d_t.is_changed == 0xFF) {
//             device_t.park_state = 0xFF;
//             device_t.sync_state = 1;
//             device_t.park_delay = 5 * SEC_DELAY;
//             if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
//                 device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
//             DEBUG_TRACE(WARN_TAG, "******** Cover !!! ********");
//             if (device_t.ble_connected) {
//                 BLE_SendCMD("Cover !!!");
//             }
// #if NOR_FLASH_ENABLE
//             nor_flash_write(&tsdb);
// #endif
//         } else if (rkb1145d_t.is_changed != 0xFF && device_t.park_state == 0xFF) {
//             device_t.park_state = 0;
//             device_t.sync_state = 1;
//             device_t.park_delay = 5 * SEC_DELAY;
//             if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
//                 device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
//             DEBUG_TRACE(LOG_TAG, "******** Not Park !!! ********");
//             if (device_t.ble_connected) {
//                 BLE_SendCMD("Not Park !!!");
//             }
// #if NOR_FLASH_ENABLE
//             nor_flash_write(&tsdb);
// #endif
//         }
        if (device_t.park_state == 1) {
            device_t.park_state = 0;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, "******** Not Park !!! ********");
            if (device_t.ble_connected) {
                BLE_SendCMD("Not Park !!!");
            }
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        }
    } else {
        if (device_t.park_state != 1) {
            if (rkb1145d_t.is_changed > 0) {
                device_t.park_state = 1;
                device_t.sync_state = 1;
                device_t.park_delay = 5 * SEC_DELAY;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "******** Parking !!! ********");
                if (device_t.ble_connected) {
                    BLE_SendCMD("Parking !!!");
                }
#if NOR_FLASH_ENABLE
                nor_flash_write(&tsdb);
#endif
            }
        } else {
            if (rkb1145d_t.is_changed == 0) {
                device_t.park_state = 0;
                device_t.sync_state = 1;
                device_t.park_delay = 5 * SEC_DELAY;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "******** Not Park !!! ********");
                if (device_t.ble_connected) {
                    BLE_SendCMD("Not Park !!!");
                }
#if NOR_FLASH_ENABLE
                nor_flash_write(&tsdb);
#endif
            }
        }
    }
}

/**
 * @brief  雷达优先模式车位状态更新 (park_mode == 2)
 *         雷达主导判决，磁力计辅助。
 *         覆盖时回退到磁力计，雷达检测到车辆直接确认。
 * @param
 * @retval
 */
static void update_radar_priority_parking_state(void)
{
    /* 雷达被覆盖：回退到纯磁力计判决 */
    if (device_t.radar_covered_flag) {
        if (qmc5883l_t.is_changed == 1) {
            /* 磁力计检测到车 → 有车（来车或持续有车） */
            if (device_t.park_state != 1) {
                device_t.park_state = 1;
                device_t.sync_state = 1;
                device_t.park_delay = 5 * SEC_DELAY;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "******** [RadarPriority+Covered] Parking (Mag) !!! ********");
                if (device_t.ble_connected) BLE_SendCMD("Parking !!!");
#if NOR_FLASH_ENABLE
                nor_flash_write(&tsdb);
#endif
            }
        } else if (device_t.park_state == 1) {
            /* 之前有车，磁力计现在无车 → 离场 */
            device_t.park_state = 0;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, "******** [RadarPriority+Covered] Not Park (Mag) !!! ********");
            if (device_t.ble_connected) BLE_SendCMD("Not Park !!!");
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        } else if (device_t.park_state != 0xFF) {
            /* 无车且未被覆盖记录 → 上报0xFF异常覆盖 */
            device_t.park_state = 0xFF;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(WARN_TAG, "******** [RadarPriority+Covered] Abnormal (0xFF) !!! ********");
            if (device_t.ble_connected) BLE_SendCMD("Cover !!!");
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        }
        device_t.radar_miss_counter = 0;
        return;
    }

    /* 未被覆盖：雷达说了算，磁力计不单独确认来车 */
    if (device_t.park_state != 1) {
        if (device_t.radar_detected) {
            /* 仅雷达确认来车 */
            device_t.park_state = 1;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, "******** [RadarPriority] Parking (Radar) !!! ********");
            if (device_t.ble_connected) BLE_SendCMD("Parking !!!");
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        } else if (device_t.park_state == 0xFF) {
            /* 从0xFF恢复正常 */
            device_t.park_state = 0;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, "******** [RadarPriority] Recover From 0xFF !!! ********");
            if (device_t.ble_connected) BLE_SendCMD("Not Cover !!!");
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        }
        device_t.radar_miss_counter = 0;
    } else {
        /* 当前有车 → 无车判定 */
        if (!device_t.radar_detected && qmc5883l_t.is_changed == 0) {
            /* 雷达和磁力计都认为无车 */
            device_t.park_state = 0;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, "******** [RadarPriority] Not Park !!! ********");
            if (device_t.ble_connected) {
                BLE_SendCMD("Not Park !!!");
            }
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
            device_t.radar_miss_counter = 0;
        } else if (!device_t.radar_detected && qmc5883l_t.is_changed == 1) {
            /* 雷达看不到但磁力计还认为有车 → 累计miss计数后信任磁力计 */
            device_t.radar_miss_counter++;
            if (device_t.radar_miss_counter >= 5) {
                device_t.park_state = 0;
                device_t.sync_state = 1;
                device_t.park_delay = 5 * SEC_DELAY;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "******** [RadarPriority] Not Park (Timeout) !!! ********");
                if (device_t.ble_connected) {
                    BLE_SendCMD("Not Park !!!");
                }
#if NOR_FLASH_ENABLE
                nor_flash_write(&tsdb);
#endif
                device_t.radar_miss_counter = 0;
            }
        }
    }
}

static void update_single_mode_parking_state(void)
{
    if (device_t.park_state != 1) {
        if (qmc5883l_t.is_changed == 1) {
            device_t.park_state = 1;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, "******** Parking !!! ********");
            if (device_t.ble_connected) {
                BLE_SendCMD("Parking !!!");
            }
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        }
    } else {
        if (qmc5883l_t.is_changed == 0) {
            device_t.park_state = 0;
            device_t.sync_state = 1;
            device_t.park_delay = 5 * SEC_DELAY;
            if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(LOG_TAG, "******** Not Park !!! ********");
            if (device_t.ble_connected) {
                BLE_SendCMD("Not Park !!!");
            }
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        }
    }
}

/**
 * @brief  设备debug处理
 * @param
 * @retval
 */
void system_debug(void)
{
    static u32 covert_time;
    char output[200];

    if (!BLEDebugMode)
        return;
    if (!device_t.power_on)
        return;

    if (device_t.ble_sleep_mode) {
        if (!check_delay_expired(covert_time, 500))
            return;
    } else {
        if (!check_delay_expired(covert_time, 3000))
            return;
    }
    covert_time = get_syspant_ms();

    convert_light_sensor();
    memset(output, 0, sizeof(output));
    snprintf(output, sizeof(output),
             "mag diff[%d %d %d],mag base[%d %d %d],mag cur[%d %d %d],rkb power[%d %d %d %d %d %d %d %d %d %d],light[%d],mag state[%d],rkb "
             "state[%d],park state[%d],park mode[%d]",
             qmc5883l_t.offset.x - qmc5883l_t.xyz.x, qmc5883l_t.offset.y - qmc5883l_t.xyz.y, qmc5883l_t.offset.z - qmc5883l_t.xyz.z,
             qmc5883l_t.offset.x, qmc5883l_t.offset.y, qmc5883l_t.offset.z, qmc5883l_t.xyz.x, qmc5883l_t.xyz.y, qmc5883l_t.xyz.z,
             rkb1145d_t.adjust_power[0], rkb1145d_t.adjust_power[1], rkb1145d_t.adjust_power[2], rkb1145d_t.adjust_power[3],
             rkb1145d_t.adjust_power[4], rkb1145d_t.adjust_power[5], rkb1145d_t.adjust_power[6], rkb1145d_t.adjust_power[7],
             rkb1145d_t.adjust_power[8], rkb1145d_t.adjust_power[9], convertedValue, qmc5883l_t.is_changed, rkb1145d_t.is_changed,
             device_t.park_state, device_t.park_mode);
    BLE_SendCMD(output);
    DEBUG_TRACE(LOG_TAG, "%s", output);
}
