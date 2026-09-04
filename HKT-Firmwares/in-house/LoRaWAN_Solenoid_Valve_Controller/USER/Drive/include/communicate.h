
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define lwrb_buffer_size (100 * 10) // 保存10条标准长度数据

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
    DATATYPE_DEVICE_ADDR_ID,                          // 设备ID
    DATATYPE_DEVICE_BATTERY_INFO,                     // 电量信息
    DATATYPE_DEVICE_STATE = 0x3C,                     // 设备状态
    DATATYPE_SET_LOCAL_SCHEDULE = 0x3D,               // 电磁阀设置本地任务
    DATATYPE_DELETE_LOCAL_SCHEDULE = 0x3E,            // 电磁阀删除本地任务
    DATATYPE_SET_CLOCK_SCHEDULE = 0x3F,               // 电磁阀设置实时任务
    DATATYPE_SET_VOL_LEVEL = 0x40,                    // 电磁阀设置输出电压等级
    DATATYPE_PORT_FUNCTION = 0x41,                    // 电磁阀接口功能
    DATATYPE_STABLE_TIME = 0x42,                      // 接口稳定时长（仅开关状态模式有效）
    DATATYPE_SMART_POWER = 0x43,                      // 自动开关机
    DATATYPE_DEVICE_SYNC_TIME = 0x80,                 // 同步时间
    DATATYPE_DEVICE_REST_FACTORY = 0x85,              // 恢复默认出厂设置
    DATATYPE_DEVICE_SYNC_PERIOD = 0x86,               // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认60分钟。
    DATATYPE_SET_TIME_ZONE = 0x8A,                    // 设置时区
    DATATYPE_DEVICE_POWER = 0x8D,                     // 开关机状态
    DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF,  // 通讯应答ACK
};

typedef union {
    struct
    {
        u8 DataL : 8;
        u8 DataH : 8;
    } unionData;
    u16 unionDataSrc;
} unionDataStruct;

struct schedule {
    u8 id;           // 任务ID
    u8 valve;        // 阀门号
    u8 state;        // 初始状态
    u8 start_hour;   // 开始时间 小时
    u8 start_min;    // 开始时间 分钟
    u8 end_hour;     // 结束时间 小时
    u8 end_min;      // 结束时间 分钟
    u8 repeat_duty;  // 重复周期
    u8 is_vaild;     // 是否有效
    u8 is_start;     // 执行状态
    u16 pulse_count; // 脉冲数
};

struct timing_task {
    struct schedule *current;   // 当前执行任务
    struct schedule task_t[16]; // 本地任务
    u8 total_task;              // 任务总数
    u8 total_task_single_day;   // 当天有效任务总数
    u8 next_task_index;         // 下一个任务的执行位置
    u8 task_sort_id[16];        // 当天任务ID排序 根据开始时间 从高到低
    u8 task_sort_id_valid[16];  // 当天有效任务ID排序  从高到低
    u16 pulse_count;            // 脉冲数
    time_t task_start_timestamp;
    time_t task_over_timestamp;
};

struct real_time_task {
    u8 valve;                // 阀门号
    u8 state;                // 初始状态
    u8 is_start;             // 执行状态
    u16 pulse_count;         // 脉冲数
    u16 pulse_count_trigger; // 脉冲数
    u32 time;                // 持续时间 S
    time_t task_over_timestamp;
};

extern u8 factoryMode;
extern u8 keepRunFlag;
extern u8 lwrb_write_count;
extern time_t dataReportTimestamp;
extern time_t zeroTimestamp;
extern struct timing_task local_schedule_t;
extern struct real_time_task real_time_task_t;

u8 setDataPackage(enum DataType type, u8 *dest);
ErrorStatus checkDevEUI(u8 *data);

void fromLoRaWANDataHandle(u8 *data, u16 len);
void fromInfoDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void lwrbDataInit(void);
void sendLoRaWANData(void);
void DeviceStateProcess(void);
void Device_ReportLocalTaskData(void);
void Device_PeriodicReport(void);

#ifdef __cplusplus
}
#endif

#endif
