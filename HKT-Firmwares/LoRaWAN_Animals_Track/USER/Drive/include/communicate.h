
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define lwrb_buffer_size (100 * 10) // 保存10条标准长度数据

    enum DeviceType
    {
        LoRaWAN_IR_PEOPLE_COUNTER = 1,     // LoRaWAN红外线人流量计数器
        LoRaWAN_DOOR_SENSE_SENSOR = 2,     // LoRaWAN门磁传感器
        LoRaWAN_AIR_MONITORING_SENSOR = 3, // LoRaWAN室内空气质量传感器
        LoRaWAN_CATTLE_SHEEP_TRACK = 4,    // LoRaWAN牛羊追踪器
    };

    enum DataType
    {
        DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, // 设备软硬件版本
        DATATYPE_DEVICE_ADDR_ID,                          // 设备ID
        DATATYPE_DEVICE_BATTERY_INFO,                     // 电量信息
        DATATYPE_DEVICE_IR_REPORT_MODE,                   // 红外线人流量计数器上报模式
        DATATYPE_DEVICE_IR_SET_REPORT_INTERVAL,           // 红外线人流量计数器时间间隔设定
        DATATYPE_DEVICE_IR_THRESHOLD_TOTAL_COUNT,         // 红外线人流量计数器累计人数阈值设定
        DATATYPE_DEVICE_IR_REPORT_COUNT,                  // 红外线人流量计数器上报人数
        DATATYPE_DEVICE_DOOR_SENSOR_STATE,                // 门磁状态 0关 1开
        DATATYPE_DEVICE_TEMPERATURE_INFO,                 // 温度数据 数据放大1000倍上传
        DATATYPE_DEVICE_HUMIDITY_INFO,                    // 湿度数据 数据放大1000倍上传
        DATATYPE_DEVICE_X_ACC_VALUE,                      // x轴加速度值
        DATATYPE_DEVICE_Y_ACC_VALUE,                      // y轴加速度值
        DATATYPE_DEVICE_Z_ACC_VALUE,                      // z轴加速度值
        DATATYPE_DEVICE_ANGLE_INFO,                       // 设备当前俯仰角
        DATATYPE_DEVICE_UTC_TIMESTAMP,                    // UTC时间戳（GPS）
        DATATYPE_DEVICE_LATITUDE,                         // 纬度
        DATATYPE_DEVICE_LONGITUDE,                        // 经度
        DATATYPE_DEVICE_GPS_MOVE_SPEED,                   // GPS移动速度
        DATATYPE_DEVICE_GPS_AZIMUTH,                      // GPS方位角
        DATATYPE_DEVICE_GPS_ALTITUDE,                     // 海拔
        DATATYPE_DEVICE_STEP_NUMBER,                      // 步数
        DATATYPE_DEVICE_BLE_RSSI,                         // 蓝牙信号强度
        DATATYPE_DEVICE_BLE_TAG_NUMBER,                   // 蓝牙标签数量
        DATATYPE_DEVICE_BLE_MAJOR_MINOR,                  // 蓝牙Major,Minor值
        DATATYPE_DEVICE_PRESSURE,                         // 大气压
        DATATYPE_DEVICE_PIR_SIGNAL,                       // PIR信号
        DATATYPE_DEVICE_AMBIENT_LIGHT_LEVEL,              // 环境光强度等级
        DATATYPE_DEVICE_PM2_5,                            // 标准大气压环境下，空气中PM2.5浓度
        DATATYPE_DEVICE_PM10,                             // 标准大气压环境下，空气中PM10浓度
        DATATYPE_DEVICE_HCHO_INFO,                        // 空气中甲醛浓度 数据放大1000倍上传
        DATATYPE_DEVICE_O3_LEVEL,                         // 空气中臭氧含量等级
        DATATYPE_DEVICE_CO2_INFO,                         // 空气中二氧化碳浓度
        DATATYPE_DEVICE_VOC_LEVEL,                        // 空气中VOC含量等级
        DATATYPE_DEVICE_SYNC_DATA = 0x32,                 // 设备主动同步数据
        DATATYPE_DEVICE_RUN_CONFIG = 0x39,                // 运行模式配置
        DATATYPE_DEVICE_SYNC_TIME = 0x80,                 // 同步时间
        DATATYPE_DEVICE_POWER_WAY,                        // 设备供电方式 0电池 1市电
        DATATYPE_DEVICE_WORK_MODE,                        // 设备工作模式 0正常模式 1节能模式（低功耗模式）
        DATATYPE_DEVICE_FAULT_STATE,                      // 故障状态 0故障解除 1设备故障
        DATATYPE_DEVICE_TAMPER_STATE,                     // 防拆状态 0设备已安装 1设备未安装
        DATATYPE_DEVICE_REST_FACTORY,                     // 恢复默认出厂设置   1LoRaWAN模块恢复出厂设置 2设备恢复出厂设置（包括LoRaWAN模块）
        DATATYPE_DEVICE_SYNC_PERIOD,                      // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认20分钟。
        DATATYPE_DEVICE_HEARTBEAT_INTERVAL,               // 心跳间隔
        DATATYPE_DEVICE_HEARTBEAT_PACK,                   // 心跳包

        DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF, // 通讯应答ACK

        DATATYPE_DEVICE_LoRaWAN_JION_WAY = 0xA0, // LoRaWAN 激活方式
        DATATYPE_DEVICE_LoRaWAN_JION_APPSKEY,
        DATATYPE_DEVICE_LoRaWAN_JION_NWKSKEY,
        DATATYPE_DEVICE_LoRaWAN_DEVEUI,
        DATATYPE_DEVICE_LoRaWAN_JION_APPKEY,
        DATATYPE_DEVICE_LoRaWAN_JION_APPEUI,
        DATATYPE_DEVICE_LoRaWAN_DATA_PORT,      // LoRaWAN 数据端口
        DATATYPE_DEVICE_LoRaWAN_WORK_FREQUENCY, // LoRaWAN 工作频段
        DATATYPE_DEVICE_LoRaWAN_HEARTBEAT_PORT, // LoRaWAN 心跳端口
    };

    enum xz_DataType
    {
        XZ_CHANNEL_0X01_DEVICE_BATTERY_INFO = 0x75, // 电量信息
        XZ_CHANNEL_0X03_DEVICE_DOOR_STATE = 0x00,   // 门磁状态
        XZ_CHANNEL_0X04_DEVICE_TAMPER = 0x00,       // 防拆状态

        XZ_CHANNEL_0XFF_DEVICE_PROTOCOL_VERSION = 0x01, // 协议版本
        XZ_CHANNEL_0XFF_DEVICE_SN_ID = 0x08,            // 设备SN
        XZ_CHANNEL_0XFF_DEVICE_HARDWARE_VERSION = 0x09, // 硬件版本
        XZ_CHANNEL_0XFF_DEVICE_BIN_VERSION = 0x0A,      // 固件版本
        XZ_CHANNEL_0XFF_DEVICE_NODE_TYPE = 0x0F,        // 节点类型
    };

    typedef union
    {
        struct
        {
            u8 DataL : 8;
            u8 DataH : 8;
        } unionData;
        u16 unionDataSrc;
    } unionDataStruct;

    struct run_mode
    {
        u8 runMode;
        u16 reportInterval[2];
        u8 start_hour[2];
        u8 start_min[2];
        u8 end_hour[2];
        u8 end_min[2];
        u16 idleReportInterval;
    };

    extern u8 factoryMode;
    extern u8 keepRunFlag;
    extern struct run_mode system_schedule_t;

    u8 setDataPackage(enum DataType type, u8 *dest);
    ErrorStatus checkDevEUI(u8 *data);

    void fromLoRaWANDataHandle(u8 *data, u16 len);
    void fromInfoDataHandle(u8 *data, u16 len);
    void fromUartDataHandle(void);
    void lwrbDataInit(void);
    void sendLoRaWANData(void);
    void DeviceStateProcess(void);
    void Device_PeriodicReport(void);

#ifdef __cplusplus
}
#endif

#endif
