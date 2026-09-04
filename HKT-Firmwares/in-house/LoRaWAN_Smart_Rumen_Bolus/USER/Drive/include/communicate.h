
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define lwrb_buffer_size (5*512)

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
    DATATYPE_DEVICE_BATTERY_INFO = 0x03,              // 电量信息
    DATATYPE_DEVICE_TEMPERATURE_INFO = 0x09,          // 温度数据 数据放大1000倍上传
    DATATYPE_DEVICE_GASTRIC_MOTILITY = 0x49,          // 胃动量
    DATATYPE_DEVICE_X_ACC_VALUE,                      // x轴加速度值(g)
    DATATYPE_DEVICE_Y_ACC_VALUE,                      // y轴加速度值(g)
    DATATYPE_DEVICE_Z_ACC_VALUE,                      // z轴加速度值(g)
    DATATYPE_DEVICE_TEMPERATURE_GROUP_INFO,           // 多组温度数据 数据放大100倍上传
    DATATYPE_DEVICE_TIMESTAMP,
    DATATYPE_DEVICE_SYNC_PERIOD = 0x86,               // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认20分钟。
    DATATYPE_DEVICE_BATTERY_VOLTAGE = 0x8B,           // 电池电压
    DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF,  // 通讯应答ACK
};

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
void zn_sendLoRaWANData(void);
void DeviceStateProcess(void);
void Device_PeriodicReport(void);

#ifdef __cplusplus
}
#endif

#endif
