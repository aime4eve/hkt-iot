
#ifndef __control_CENTER_H__
#define __control_CENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY 5   // 5秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 60 // 60秒未成功连接强制进入睡眠

struct device {
    char *device_name;
    u8 rtc_wake_event : 1;     // RTC唤醒事件
    u8 vcs_wake_event : 1;     // 磁开关触发唤醒事件
    u8 pulse1_wake_event : 1;  // 脉冲输入1触发唤醒事件
    u8 pulse2_wake_event : 1;  // 脉冲输入2触发唤醒事件
    u8 insert1_wake_event : 1; // 插入检测1触发唤醒事件
    u8 insert2_wake_event : 1; // 插入检测2触发唤醒事件
    u8 ble_wake_event : 1;     // 蓝牙触发唤醒事件
    u8 lptime_wake_event : 1;  // 低功耗定时器触发唤醒事件
    u8 lora_wake_event : 1;    // LoRaWAN触发唤醒事件
    u8 sleep_state : 1;        // 睡眠状态
    u8 factoryEvent : 1;       // 恢复出厂设置
    u8 press_short_event : 1;  // 按键短按事件
    u8 press_double_event : 1; // 按键双击事件
    u8 press_long_event : 1;   // 按键长按事件
    u8 sense_long_event : 1;   // 磁开关长按事件
    u8 sense_hold_event : 1;   // 磁开关长按事件
    u8 sync_state : 1;         // 同步设备状态
    u8 wait_default_mode : 1;  // 进入等待恢复出厂设置
    u8 power_on : 1;           // 开机标识
    u8 ble_connected : 1;      // 蓝牙连接状态
    u8 insert1_connected : 1;  // 接口1连接状态
    u8 insert2_connected : 1;  // 接口2连接状态
    u8 value1_state : 1;       // 接口1阀门状态
    u8 value2_state : 1;       // 接口2阀门状态
    u8 vol_control_sw;         // 电压控制 0 12v 1 9v 2 5v
    u8 port_mode;              // 脉冲模式/阀门开关检测模式
    u8 stable_time;            // 稳定时间 单位S
    u8 power_mode;             // 自动开关机模式
    u8 zero_event;             // 0点时间
    u8 send_fail_cnt;          // LoRa数据发送失败次数
    u8 flash_write_flag;       // 写Flash数据标志
    u8 sleep_delay;            // 睡眠延时
    u16 pulse_delay;           // 计数延时
    u16 report_interval;       // 上报周期
    u16 pulse1_count;          // 当日接口1脉冲总数
    u16 pulse2_count;          // 当日接口2脉冲总数
    float timezone;            // 时区 取值范围-11-11 西11区到东11区
    void *user_data;
};

extern struct device device_t;
extern u16 ledRedBlinkTime;

void Led_Blink_Process(void);
void systemLowPowerMode_Config(void);

void local_schedule_event(void);
void local_schedule_init(void);

#ifdef __cplusplus
}
#endif

#endif
