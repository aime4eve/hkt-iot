
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY  10   // 10秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 150 // 150秒未成功连接强制进入睡眠

struct sensor {
    u16 pressure; // 大气压
    u16 altitude; // 海拔

    u16 tvoc;      //   ug/m^3
    u8 tvoc_level; // tvoc 含量等级
    u16 co2;
    u32 light_data;  // 环境光值
    u8 light_level;  // 环境光等级
    int temperature; // 放大1000倍的温度值
    int humidity;    // 放大1000倍的湿度值
    u32 pir_signal;  // PIR信号
    u8 air_level;    // 空气质量等级

    // 9合一使用
    u16 atmos_pm_1_0, atmos_pm_2_5, atmos_pm_10_0; // PM2.5值
    u16 o3_aqi;                                    // 臭氧 aqi
    u8 o3_level;                                   // 臭氧含量等级
    u32 hcho_ppm;                                  // 放大1000倍的甲醛值
    time_t stamp;
};

struct device {
    char *device_name;

    u8 communicateAck : 1;      // 通讯应答ACK
    u8 updateBattery : 1;       // 更新电量信息标志
    u8 updateFault : 1;         // 更新故障状态标志
    u8 updateLowPowerMode : 1;  // 同步节能模式
    u8 syncLocalTime : 1;       // 同步本地时间
    u8 rtcWakeEvent : 1;        // RTC唤醒事件
    u8 pirEvent : 1;            // PIR触发事件
    u8 extiWakeEvent : 1;       // 外部触发唤醒事件
    u8 sleepState : 1;          // 睡眠状态
    u8 syncDeviceState : 1;     // 同步设备状态
    u8 syncSensorData : 1;      // 同步传感器数据
    u8 syncLedShowMode : 1;     // 同步Led显示模式
    u8 syncBeepEnable : 1;      // 同步蜂鸣器使能状态
    u8 syncTempShowWay : 1;     // 同步温度显示单位
    u8 syncEpdOverturnMode : 1; // 同步电子纸翻转状态
    u8 syncEpdShowMode : 1;     // 同步电子纸显示模式
    u8 epdInit : 1;             // 同步电子纸显示模式
    u8 powerIn : 1;             // 电源输入方式
    u8 energyMode : 1;          // 节能模式
    u8 beepEnable : 1;          // 蜂鸣器报警使能
    u8 sleepLock : 1;           // 睡眠锁定
    u8 send_failcnt;            // LoRa数据发送失败次数
    u8 ledShowMode;             // LED显示模式 0闪烁 1常闭 2常亮
    u8 timeShow;                // 屏幕时间部分显示内容
    u8 epdShowMode;             // 电子纸显示模式
    u8 epdOverturnShow;         // 电子纸翻转显示
    u8 epdOldImageShow;         // 电子旧图像显示
    u8 epdShowTempWay;          // 电子纸温度显示单位
    u8 epdShowDuCnt;            // 电子纸局刷次数
    u8 co2_calibration_mode;    // CO2强制校准模式
    char temperature_offset;    // 温度偏移值
    u16 co2_calibration_value;  // CO2强制校准数值
    u16 sleepDelay;             // 睡眠延时

    u16 reportInterval;     // 数据同步周期
    u16 sensorInterval;     // 传感器上报周期(时间段外)
    u16 sensorIntervalTask; // 时间段内上报周期
    u16 pirInterval; // 时间段内上报周期
    u8 start_hour;
    u8 start_min;
    u8 end_hour;
    u8 end_min;

    int epdShowModeWriteDelay; // 显示模式写入延时
    int ledShowModeWriteDelay; // LED显示模式写入延时
    u16 co2_history_buf[40];
    struct sensor sensor_t;

    void *user_data;
};

extern struct device device_t;

void factoryTestMode(void);
void FuncInit(void);
void sensorReadAndConvert(void);

#ifdef __cplusplus
}
#endif

#endif
