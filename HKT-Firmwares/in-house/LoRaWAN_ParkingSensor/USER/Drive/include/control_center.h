
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY  5 // 5秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 60 // 60秒未成功连接强制进入睡眠

struct device {
    char *device_name;

    u8 communicate_ack : 1; // 通讯应答ACK
    u8 sync_battery : 1; // 更新电量信息标志
    u8 sync_local_time : 1; // 同步本地时间
    u8 lptime_wake_event : 1;
    u8 rtc_wake_event : 1; // RTC唤醒事件
    u8 vcs_wake_event : 1; // 外部触发唤醒事件
    u8 mag_wake_event : 1; // 磁力计触发唤醒事件
    u8 ble_wake_event : 1; // 蓝牙触发唤醒事件
    u8 sleep_state : 1; // 睡眠状态
    u8 sync_state : 1; // 同步设备状态
    u8 tamper_alarm : 1; // 防拆状态
    u8 power_on : 1; // 开机标识
    u8 ble_connected : 1; // 蓝牙连接状态
    u8 ble_sleep_mode : 1; // 蓝牙连接时是否进入睡眠
    u8 mag_valid : 1; // mag数据有效
    u8 park_state; // 车位状态 0无车 1有车 ff异常/被水覆盖
    u8 park_mode; // 车位检测模式 0融合 1仅磁力计 2雷达优先
    u8 radar_wake_phase : 2; // 模式2: 雷达唤醒相位计数(0-2)
    u8 radar_covered_flag : 1; // 模式2: 雷达被覆盖标志
    u8 radar_detected : 1; // 模式2: 雷达检测到车辆
    u8 radar_miss_counter : 4; // 模式2: 雷达连续未检测计数
    u8 sleep_delay; // 睡眠延时
    u8 send_fail_cnt; // LoRa数据发送失败次数
    u8 sleep_interval; // 设备睡眠周期
    u16 report_interval; // 上报周期
    u16 park_delay; // 车位状态感应延时
    
} __attribute__((aligned(4)));

extern struct device device_t;
extern u16 ledRedBlinkTime;

void Led_Blink_Process(void);
void systemLowPowerMode_Process(void);
void park_process(void);
void system_debug(void);

#ifdef __cplusplus
}
#endif

#endif
