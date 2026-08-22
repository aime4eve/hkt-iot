
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY  3  // 5秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 60 // 60秒未成功连接强制进入睡眠

struct device {
    char *device_name;

    u8 rtc_wake_event : 1;       // RTC唤醒事件
    u8 key_wake_event : 1;       // 按键触发唤醒事件
    u8 acc_wake_event : 1;       // 加速计触发唤醒事件
    u8 ble_wake_event : 1;       // 蓝牙触发唤醒事件
    u8 lptime_wake_event : 1;    // 低功耗定时器触发唤醒事件
    u8 sleep_state : 1;          // 睡眠状态
    u8 factoryEvent : 1;         // 恢复出厂设置
    u8 press_short_event : 1;    // 按键短按事件
    u8 press_double_event : 1;   // 按键双击事件
    u8 press_long_event : 1;     // 按键长按事件
    u8 sync_state : 1;           // 同步设备状态
    u8 gps_state : 1;            // 同步设备状态
    u8 wait_default_mode : 1;    // 进入等待恢复出厂设置
    u8 power_on : 1;             // 开机标识
    u8 ble_connected : 1;        // 蓝牙连接状态
    u8 angle_offset_mode : 1;    // 角度校准模式
    u8 acc_offset_mode : 1;      // 加速度校准模式
    u8 ht_alarm : 1;             // 高温报警
    u8 is_slant;                 // 设备倾斜状态
    u8 threshold_state;          // 垃圾桶满溢状态
    u8 send_fail_cnt;            // LoRa数据发送失败次数
    u8 sleep_delay;              // 睡眠延时
    u8 ud_det_cnt;               // 超声波测距检测次数
    u16 report_interval;         // 上报周期
    u16 gps_convert_interval;    // GPS转换间隔
    u16 ud_distance;             // 超声波距离 mm
    u16 low_threshold_distance;  // 低阈值
    u16 high_threshold_distance; // 高阈值
    u32 temperature;             // 放大1000倍的温度值
    u32 humidity;                // 放大1000倍的湿度值
    u32 latitude;                // 纬度
    u32 longitude;               // 经度
    u16 battery_voltage;         // 电压
    float angle;                 // 倾斜角

    void *user_data;
};

extern struct device device_t;
extern u16 ledBlinkTime;
extern u16 unsleepTime;

void Led_Blink_Process(void);
void systemLowPowerMode_Config(void);
int convert_Distance(void);
void UD_Period_Event(void);
void Device_SglConvert(void);

#ifdef __cplusplus
}
#endif

#endif
