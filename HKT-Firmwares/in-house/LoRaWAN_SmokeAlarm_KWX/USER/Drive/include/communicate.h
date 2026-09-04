
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
    DATATYPE_DEVICE_BATTERY_INFO = 0x03,              // 电量信息
    DATATYPE_DEVICE_TEMPERATURE_INFO = 0x09,          // 温度
    DATATYPE_DEVICE_SMOKE_ALARM = 0x27,               // 烟雾报警
    DATATYPE_DEVICE_TEMPERATURE_ALARM,                // 温度报警
    DATATYPE_DEVICE_FAULT_STATE = 0x83,               // 故障状态 0故障解除 1设备故障
    DATATYPE_DEVICE_TAMPER_STATE,                     // 防拆状态 0设备已安装 1设备未安装
    DATATYPE_DEVICE_REST_FACTORY,                     // 恢复默认出厂设置   1LoRaWAN模块恢复出厂设置 2设备恢复出厂设置（包括LoRaWAN模块）
    DATATYPE_DEVICE_SYNC_PERIOD,                      // 设备数据同步周期 取值范围：10- 1440 （1分钟到24小时），默认24小时。
    DATATYPE_DEVICE_SYNC_BATTERY_STATE = 0x89,        // 电量状态 0低电量 1正常
    DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF,  // 通讯应答ACK

};

typedef struct communicate {
    u8 dat1_event : 1;
    u8 dat2_event : 1;
    u8 aue_event : 1;
    u8 rtc_event : 1;
    u8 tamper_event : 1;

    u8 battery_state : 1; // 电量状态  0低 1高
    u8 sync_state : 1;    // 同步设备状态
    u8 temp_state : 1;    // 温度状态
    u8 alarm_state : 1;   // 报警状态
    u8 tamper_state : 1;  // 防拆状态

    float temperature; // 温度

    u32 report_interval;       // 数据上报间隔 单位分钟
    u32 report_interval_delay; // 上报延时
    u16 sleep_delay;           // 睡眠延时

} _communicate;

extern struct communicate device_t;

void sendLoRaWANData(void);
void Device_PeriodicReport(void);
void Device_alarm_stateReport(void);
void Device_temp_stateReport(void);
void Device_TampStateReport(void);
void Device_fault_alarmReport(void);

void fromLoRaWANDataHandle(u8 *data, u16 len);
void fromInfoDataHandle(u8 *data, u16 len);
void fromBTDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void DeviceStateProcess(void);
void DeviceSleepEventProcess(void);
void my_buff_evt_fn(lwrb_t *buff, lwrb_evt_type_t type, size_t len);
void lwrbDataInit(void);

#ifdef __cplusplus
}
#endif

#endif
