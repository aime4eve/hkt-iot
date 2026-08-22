
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define lwrb_buffer_size 2048

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
    DATATYPE_DEVICE_ADDR_ID, // 设备ID
    DATATYPE_DEVICE_BATTERY_INFO, // 电量信息
    DATATYPE_DEVICE_PARK_STATE = 0x3A, // 车位状态
    DATATYPE_DEVICE_PARK_MODE, // 地磁工作模式
    DATATYPE_DEVICE_SYNC_TIME = 0x80, // 同步时间
    DATATYPE_DEVICE_TAMPER_STATE = 0x84, // 防拆状态 0设备已安装 1设备未安装
    DATATYPE_DEVICE_REST_FACTORY = 0x85, // 恢复默认出厂设置   1LoRaWAN模块恢复出厂设置 2设备恢复出厂设置（包括LoRaWAN模块）
    DATATYPE_DEVICE_SYNC_PERIOD, // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认20分钟。
    DATATYPE_DEVICE_POWER = 0x8D, // 开关机状态
    DATATYPE_DEVICE_MAG_X = 0x5D, // X轴磁场值 (2 bytes, int16, 仅上行)
    DATATYPE_DEVICE_MAG_Y = 0x5E, // Y轴磁场值 (2 bytes, int16, 仅上行)
    DATATYPE_DEVICE_MAG_Z = 0x5F, // Z轴磁场值 (2 bytes, int16, 仅上行)
    DATATYPE_DEVICE_RADAR_SPECTRUM = 0x60, // 雷达频谱值 (20 bytes, 10×uint16, 仅上行)
    DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF, // 通讯应答ACK
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
extern time_t dataReportTimestamp;
extern u8 BLEDebugMode;

u8 setDataPackage(enum DataType type, u8 *dest);
ErrorStatus checkDevEUI(u8 *data);

void fromLoRaWANDataHandle(u8 *data, u16 len);
void fromInfoDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void lwrbDataInit(void);
bool sendLoRaWANData(u8 *buf, u8 txlen);
void DeviceStateProcess(void);
void Device_PeriodicReport(void);

#ifdef __cplusplus
}
#endif

#endif
