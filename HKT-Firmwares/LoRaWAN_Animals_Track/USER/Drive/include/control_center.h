
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
        u8 tamper_status; // 防拆状态
        u32 light_data;   // 环境光值
        u8 light_level;   // 环境光等级
        u32 temperature;  // 放大1000倍的温度值
        u32 humidity;     // 放大1000倍的湿度值
        u32 latitude;     // 纬度
        u32 longitude;    // 经度
        u32 step;         // 步数
        time_t stamp;
    };

    struct device
    {
        char *device_name;

        u8 communicateAck : 1;   // 通讯应答ACK
        u8 updateBattery : 1;    // 更新电量信息标志
        u8 updateFault : 1;      // 更新故障状态标志
        u8 updateTamper : 1;     // 更新防拆状态标志
        u8 syncLocalTime : 1;    // 同步本地时间
        u8 rtcWakeEvent : 1;     // RTC唤醒事件
        u8 extiWakeEvent : 1;    // 外部触发唤醒事件
        u8 lis3d_int : 1;        // 加速计中断
        u8 sleepState : 1;       // 睡眠状态
        u8 powerState : 1;       // 开关机状态
        u8 factoryEvent : 1;     // 恢复出厂设置
        u8 pressShortEvent : 1;  // 按键短按事件
        u8 pressDoubleEvent : 1; // 按键双击事件
        u8 pressLongEvent : 1;   // 按键长按事件
        u8 syncDeviceState : 1;  // 同步设备状态
        u8 syncState : 1;        // 同步设备状态
        u8 waitDafaultMode : 1;  // 进入等待恢复出厂设置
        u8 bleConnectState : 1;  // 蓝牙连接状态
        u8 sendFailCnt;          // LoRa数据发送失败次数
        u8 sleepDelay;           // 睡眠延时
        u8 sleepInterval;        // 设备睡眠周期
        u16 reportInterval;      // 上报周期
        u16 reportCountdown;     // 上报倒计时
        u16 gpsLocateInterval;   // gps 定位间隔
        struct sensor sensor_t;
        void *user_data;
    };

    extern struct device device_t;
    extern u16 ledRedBlinkTime;
    extern u16 ledGreenBlinkTime;

    void Led_Blink_Process(void);
    void systemLowPowerMode_Config(void);
    void factoryTestMode(void);

#ifdef __cplusplus
}
#endif

#endif
