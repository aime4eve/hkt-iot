
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define IR_DEC_DUTY 200

typedef enum people_counter_mode {
    IDLE_MODE,
    ALIGNMENT_MODE,  // 对齐模式
    VALIDATION_MODE, // 验证模式
    COUNTING_MODE,   // 计数模式

} people_counter_mode_t;

typedef enum people_counter_direction {
    PASSAGE_DIRECTION_A_TO_B,
    PASSAGE_DIRECTION_B_TO_A,

} people_counter_direction_t;

struct device_sensor {
    char *device_name;

    u8 communicateAck : 1;     // 通讯应答ACK
    u8 updateBattery : 1;      // 更新电量信息标志
    u8 updateFault : 1;        // 更新故障状态标志
    u8 updateCounter : 1;      // 更新红外计数器
    u8 linkCheckEvent : 1;     // 链路检测事件
    u8 syncTime : 1;           // 同步设备时间
    u8 syncDeviceState : 1;    // 同步设备状态
    u8 faultState : 1;         // 故障状态
    u8 tamperAlarm : 1;        // 防拆报警
    u8 doorState : 1;          // 0 关闭 1开启
    u8 syncRTCTime : 1;        // 同步rtc时间
    u8 reportMode;             // 上报模式
    u8 send_failcnt;           // 发送失败计数
    u8 timeEvent;              // 定时事件 0-未使能 1-定时清零计数
    u16 reportInterval;        // 上报间隔 单位分钟
    u16 reportIRInterval;      // 计数器同步间隔
    u16 reportTotalCount;      // 累计人数
    u32 heartbeatInterval;     // 心跳间隔   单位秒
    u32 heartbeatDelay;        // 心跳发送延时
    u32 mode_timeout;          // 特殊模式停留时间
    u16 counter_a_value;       // 红外计数器a计数值
    u16 counter_b_value;       // 红外计数器b计数值
    u32 counter_a_count;       // 红外计数器a总计数值
    u32 counter_b_count;       // 红外计数器b总计数值
    u32 counter_run_time;      // 红外计数器运行计数时间
    u16 counter_a_sense_count; // 红外计数器a，感应到次数
    u16 counter_b_sense_count; // 红外计数器b，感应到次数
    people_counter_mode_t mode;
    people_counter_direction_t direction;

    void *user_data;
};

extern struct device_sensor device_t;
extern u16 ledGreenBlinkTime;
extern u16 ledBlueBlinkTime;
extern u16 ledOrangeBlinkTime;
extern u16 ledRedBlinkTime;
extern u32 countDelay;

void People_Counter_Event(void);
void People_Counter_Mode_Process(void);
void Run_People_Counter_Handle(void);
void Led_Blink_Process(void);
void factoryTestMode(void);
void People_Counter_Handle(void);

#ifdef __cplusplus
}
#endif

#endif
