
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define lwrb_buffer_size (4096)

#define LoRaWAN_AIR_MONITORING_SENSOR = 3, // LoRaWAN室内空气质量传感器

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
    DATATYPE_DEVICE_ADDR_ID,                          // 设备ID
    DATATYPE_DEVICE_BATTERY_INFO,                     // 电量信息
    DATATYPE_DEVICE_SENSOR_REPORT_INTERVAL = 0x05,    // 传感器数据上报间隔
    DATATYPE_DEVICE_TEMPERATURE_INFO = 0x09,          // 温度数据 数据放大100倍上传
    DATATYPE_DEVICE_HUMIDITY_INFO,                    // 湿度数据 数据放大100倍上传
    DATATYPE_DEVICE_GPS_ALTITUDE = 0x14,              // 海拔
    DATATYPE_DEVICE_PRESSURE = 0x19,                  // 大气压
    DATATYPE_DEVICE_PIR_SIGNAL,                       // PIR信号
    DATATYPE_DEVICE_AMBIENT_LIGHT_LEVEL,              // 环境光强度等级
    DATATYPE_DEVICE_PM2_5,                            // 标准大气压环境下，空气中PM2.5浓度
    DATATYPE_DEVICE_PM10,                             // 标准大气压环境下，空气中PM10浓度
    DATATYPE_DEVICE_HCHO_INFO,                        // 空气中甲醛浓度 数据放大1000倍上传
    DATATYPE_DEVICE_O3_LEVEL,                         // 空气中臭氧含量等级
    DATATYPE_DEVICE_CO2_INFO,                         // 空气中二氧化碳浓度
    DATATYPE_DEVICE_VOC_LEVEL,                        // 空气中TVOC含量等级
    DATATYPE_DEVICE_LED_SHOW_MODE = 0x29,             // LED显示模式
    DATATYPE_DEVICE_BEEP_STATUS,                      // 蜂鸣器启用状态
    DATATYPE_DEVICE_TEMPERATURE_SHOW_WAY,             // 温度显示单位
    DATATYPE_DEVICE_EPD_SHOW_MODE,                    // 电子纸显示模式
    DATATYPE_DEVICE_EPD_OVERTURN_MODE,                // 电子纸翻转显示
    DATATYPE_DEVICE_CO2_CALIBRATION = 0x58,           // 强制校准CO2
    DATATYPE_DEVICE_TEMPERATURE_OFFSET = 0x5B,        // 校准温度值
    DATATYPE_DEVICE_SENSOR_TASK_PERIOD = 0x5C,        // 传感器任务配置
    DATATYPE_DEVICE_PIR_TASK_PERIOD = 0x5D,           // PIR触发与状态同步间隔配置
    DATATYPE_DEVICE_SYNC_TIME = 0x80,                 // 同步时间
    DATATYPE_DEVICE_POWER_WAY,                        // 设备供电方式 0电池 1市电
    DATATYPE_DEVICE_REST_FACTORY = 0x85,              // 恢复默认出厂设置   1LoRaWAN模块恢复出厂设置 2设备恢复出厂设置（包括LoRaWAN模块）
    DATATYPE_DEVICE_SYNC_PERIOD,                      // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认8小时。
    DATATYPE_DEVICE_REBOOT = 0xF0,                    // 远程重启设备
    DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF,  // 设备应答ACK
};

typedef union {
    struct {
        u8 DataL : 8;
        u8 DataH : 8;
    } unionData;
    u16 unionDataSrc;
} unionDataStruct;

extern u8 factoryMode;
extern time_t dataReportTimestamp;
extern time_t sensorConvertTimestamp;
extern time_t pirConvertTimestamp;
extern time_t getRTCTimestamp;

u8 setDataPackage(enum DataType type, u8 *dest);
ErrorStatus checkDevEUI(u8 *data);

void fromLoRaWANDataHandle(u8 *data, u16 len);
void fromInfoDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void lwrbDataInit(void);
void sendLoRaWANData(void);
void DeviceStateProcess(void);
void Device_SensorDataReport(void);
void Device_PeriodicReport(void);

void thread_communicate(ULONG thread_input);
void thread_sensor(ULONG thread_input);
void thread_tvoc(ULONG thread_input);
void thread_deepsleep(ULONG thread_input);

#ifdef __cplusplus
}
#endif

#endif
