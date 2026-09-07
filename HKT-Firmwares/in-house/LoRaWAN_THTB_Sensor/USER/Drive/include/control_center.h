
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>
#include "time.h"

#define WAIT_SLEEP_TIME_DELAY 10  // 10秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 60 // 60秒未成功连接强制进入睡眠

    struct sensor
    {
        u8 tamper_status; //防拆状态
        u32 light_data;   // 环境光值
        u8 light_level;   // 环境光等级
        u8 state;
        u8 move_state;
        int temperature; // 放大1000倍的温度值
        int humidity;    // 放大1000倍的湿度值
        u32 latitude;    //纬度
        u32 longitude;   // 经度
        time_t stamp;
    };

    struct system_config
    {
        int system_isable;  //系统运行使能
        int battery_isable; //电量检测使能
        int move_isable;    //运动检测使能

        int angle_alarm_threshold;            //角度变化报警阈值
        int temperature_alarm_low_threshold;  //温度过低报警阈值
        int temperature_alarm_high_threshold; //温度过高报警阈值

        int humidity_alarm_low_threshold;  //湿度过低报警阈值
        int humidity_alarm_high_threshold; //湿度过高报警阈值
    };

    struct device
    {
        char *device_name;

        u8 communicateAck : 1;  //通讯应答ACK
        u8 updateBattery : 1;   //更新电量信息标志
        u8 updateFault : 1;     //更新故障状态标志
        u8 syncLocalTime : 1;   //同步本地时间
        u8 rtcWakeEvent : 1;    // RTC唤醒事件
        u8 extiWakeEvent : 1;   //外部触发唤醒事件
        u8 sleepState : 1;      //睡眠状态
        u8 syncDeviceState : 1; //同步设备状态
        u8 syncGpsState : 1;
        u8 powerIn : 1;
        u8 flash_data : 1;     // flash数据标志位
        u8 tamperAlarm : 1;    // 防拆状态
        u8 powerState : 1;     // 开机标识
        u8 accIntState : 1;    // 加速计中断
        u8 getGpsInfo : 1;     // 获取GPS数据标志
        u8 send_failcnt;       // 发送失败计数
        u8 sleepDelay;         // 睡眠延时
        u16 reportInterval;    // 上报周期
        u16 reportCountdown;   // 上报倒计时
        u16 gpsLocateInterval; // gps 定位间隔

        struct sensor sensor_t;
        struct system_config system_config_t;

        void *user_data;
    };

    extern struct device device_t;
    extern u16 ledBlueBlinkTime;

    void Led_Blink_Process(void);
    void systemLowPowerMode_Config(void);
    void factoryTestMode(void);

#ifdef __cplusplus
}
#endif

#endif
