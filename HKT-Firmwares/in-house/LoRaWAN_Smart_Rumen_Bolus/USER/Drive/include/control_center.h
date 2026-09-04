
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY 3   // 3秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 60 // 60秒未成功连接强制进入睡眠

struct sensor {
    u8 ax;
    u8 ay;
    u8 az;
    u32 gastricMotility; // 胃动量
    u32 temperature;     // 放大1000倍的温度值
    u32 humidity;        // 放大1000倍的湿度值
    u32 step;            // 步数
    u16 vol;             // 电池电压
    time_t stamp;
};

struct device {
    char *device_name;
    u8 rtcWakeEvent : 1;    // RTC唤醒事件
    u8 extiWakeEvent : 1;   // 外部触发唤醒事件
    u8 da_int : 1;          // 加速计中断
    u8 sleepState : 1;      // 睡眠状态
    u8 syncDeviceState : 1; // 同步设备状态
    u8 sendFailCnt;         // LoRa数据发送失败次数
    u8 sleepDelay;          // 睡眠延时
    u16 reportInterval;     // 上报周期
    u8 temperature_group;   // 温度组数
    u16 temperature[10];    // 温度偏移值
    struct sensor sensor_t;
    void *user_data;
};

extern struct device device_t;
extern u16 ledBlinkTime;

void Led_Blink_Process(void);
void systemLowPowerMode_Config(void);
void factoryTestMode(void);

#ifdef __cplusplus
}
#endif

#endif
