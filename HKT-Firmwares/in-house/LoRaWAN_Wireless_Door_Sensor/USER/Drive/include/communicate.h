
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define lwrb_buffer_size (23 * 10) // 保存10条标准长度数据

enum DeviceType {
    LoRaWAN_IR_PEOPLE_COUNTER = 1, // LoRaWAN红外线人流量计数器
    LoRaWAN_DOOR_SENSE_SENSOR = 2, // LoRaWAN门磁传感器
};

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
    DATATYPE_DEVICE_BATTERY_INFO = 0x03,              // 电量信息
    DATATYPE_DEVICE_DOOR_STATE = 0x08,                // 门磁开关状态
    DATATYPE_DEVICE_DOOR_RUN_MODE = 0x22,             // 设置门磁运行模式
    DATATYPE_DEVICE_DOOR_COUNT_TIME = 0x23,           // 设置门磁计数时间段
    DATATYPE_DEVICE_DOOR_COUNT_INTERVAL = 0x24,       // 设置门磁计数间隔
    DATATYPE_DEVICE_DOOR_HEART_INTERVAL = 0x25,       // 设置门磁心跳间隔
    DATATYPE_DEVICE_REPORT_DOOR_COUNT = 0x26,         // 上报门磁计数值
                                                      // 2byte 周期内触发开计数
                                                      // 2byte 周期内触发关计数
                                                      // 4byte 总触发开计数
                                                      // 4byte 总触发开计数
    DATATYPE_SERVER_SYNC_TIME = 0x80,                 // 同步时间
    DATATYPE_DEVICE_TAMPER_STATE = 0x84,              // 防拆状态
    DATATYPE_DEVICE_REST_FACTORY = 0x85,              // 恢复默认出厂设置
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

extern u8 factoryMode;
extern u8 keepMode;
extern time_t report_stamp;
extern time_t heart_stamp;
extern struct LoRaParam LoRaWAN;

void quickSplitShort(u16 src, u8 *hex);
u8 setDataPackage(enum DataType type, u8 *dest);
void sendLoRaWANData(void);

ErrorStatus checkDataSyncHead(u8 *data);
void fromLoRaWANDataHandle(u8 *data, u16 len);

void Device_PeriodicReport(void);
void Device_DoorCountReport(void);
void DeviceEvent_Process(void);
void DeviceStateProcess(void);

void fromInfoDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void DeviceSleepEventProcess(void);

void lwrbDataInit(void);

#ifdef __cplusplus
}
#endif

#endif
