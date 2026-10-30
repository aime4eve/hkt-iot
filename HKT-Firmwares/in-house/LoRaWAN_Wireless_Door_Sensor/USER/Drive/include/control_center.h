
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY 3 // 3秒未有新操作进入睡眠

    struct dcounter_time
    {
        u8 start_hour; //开始时间小时
        u8 start_min; //开始时间分钟
        u8 end_hour;   //结束时间小时
        u8 end_min;   //结束时间分钟
    };

    struct device_sensor
    {
        char *device_name;

        u8 communicateAck : 1;  //通讯应答ACK
        u8 updateBattery : 1;   //更新电量信息标志
        u8 updateFault : 1;     //更新故障状态标志
        u8 syncLocalTime : 1;   //同步本地时间
        u8 rtcWakeEvent : 1;    // RTC唤醒事件
        u8 extiWakeEvent : 1;   //外部触发唤醒事件
        u8 sleepState : 1;      //睡眠状态
        u8 linkCheckEvent : 1;  //链路检测事件
        u8 syncDoorCount : 1;   //同步门磁计数值
        u8 syncDeviceState : 1; //同步设备状态
        u8 doorEvent : 1;       //门磁触发事件
        u8 tamperEvent : 1;     //防拆触发事件
        u8 touchEvent : 1;      //触发事件
        u8 tamperState : 1;     //防拆状态 0设备已安装 1设备未安装
        u8 doorState : 1;       // 0 关闭 1开启
        u8 run_mode;            //模式1 区间时间内累计计数，并周期上报  模式2 实时上报
        u16 counter_intervel;   //门磁计数上报间隔(模式一 )
        u8 heater_intervel;     //心跳间隔  0：24小时  1：12小时  2：8小时
        u8 send_failcnt;       // 发送失败计数

        u32 doorOpenCountIntervel;
        u32 doorOffCountIntervel;

        u32 doorOpenCount;
        u32 doorOffCount;

        u32 sleepDelay;

        struct dcounter_time c_time;
    };

    extern struct device_sensor device_t;
    extern u16 ledBlueBlinkTime;

    void Led_Blink_Process(void);
    void systemLowPowerMode_Config(void);
    void factoryTestMode(void);

#ifdef __cplusplus
}
#endif

#endif
