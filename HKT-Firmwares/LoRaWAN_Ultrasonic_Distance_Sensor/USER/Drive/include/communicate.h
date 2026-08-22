
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define lwrb_buffer_size (1024) // 保存10条标准长度数据

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
    DATATYPE_DEVICE_ADDR_ID,                          // 设备ID
    DATATYPE_DEVICE_BATTERY_INFO,                     // 电量信息
    DATATYPE_DEVICE_TEMPERATURE_INFO = 0x09,          // 温度数据 数据放大1000倍上传
    DATATYPE_DEVICE_HUMIDITY_INFO = 0x0A,             // 湿度数据 数据放大1000倍上传
    DATATYPE_DEVICE_ANGLE_INFO = 0x0E,                // 设备当前俯仰角
    DATATYPE_DEVICE_LATITUDE = 0x10,                  // 纬度
    DATATYPE_DEVICE_LONGITUDE = 0x11,                 // 经度
    DATATYPE_DEVICE_HT_ALARM = 0x28,                  // 高温报警
    DATATYPE_DEVICE_SLANT_STATE = 0x44,               // 倾斜状态
    DATATYPE_DEVICE_GPS_PERIOD = 0x45,                // GPS定位周期
    DATATYPE_DEVICE_UD_DISTANCE = 0x46,               // 超声波距离
    DATATYPE_DEVICE_OVERFLOW_STATE = 0x47,            // 垃圾桶满溢状态
    DATATYPE_DEVICE_OVERFLOW_CONFIG = 0x48,           // 超声波测距告警阈值设置
    DATATYPE_DEVICE_SYNC_TIME = 0x80,                 // 同步时间
    DATATYPE_DEVICE_REST_FACTORY = 0x85,              // 恢复默认出厂设置   1LoRaWAN模块恢复出厂设置 2设备恢复出厂设置（包括LoRaWAN模块）
    DATATYPE_DEVICE_SYNC_PERIOD = 0x86,               // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认20分钟。
    DATATYPE_DEVICE_BATTERY_VOLTAGE = 0x8B,           // 电池电压
    DATATYPE_DEVICE_ACC_REFERENCE = 0x8C,             // ACC校准模式
    DATATYPE_DEVICE_POWER = 0x8D,                     // 开关机状态
    DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF,  // 通讯应答ACK
};

typedef union {
    struct {
        u8 DataL : 8;
        u8 DataH : 8;
    } unionData;
    u16 unionDataSrc;
} unionDataStruct;

extern u8 factoryMode;
extern u8 keepRunFlag;
extern u8 lwrb_write_count;
extern time_t dataReportTimestamp;
extern time_t dataConvertTimestamp;

u8 setDataPackage(enum DataType type, u8 *dest);
ErrorStatus checkDevEUI(u8 *data);

void fromLoRaWANDataHandle(u8 *data, u16 len);
void fromInfoDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void lwrbDataInit(void);
void sendLoRaWANData(void);
void DeviceStateProcess(void);
void Device_PeriodicReport(void);
void lwrbTest(void);

#ifdef __cplusplus
}
#endif

#endif
