
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define lwrb_buffer_size (23 * 10) //保存10条标准长度数据

    enum DeviceType
    {
        LoRaWAN_IR_PEOPLE_COUNTER = 1, // LoRaWAN红外线人流量计数器
        LoRaWAN_DOOR_SENSE_SENSOR = 2, // LoRaWAN门磁传感器
    };

    enum DataType
    {
        DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01, //设备软硬件版本
        DATATYPE_DEVICE_ADDR_ID,                          //设备ID
        DATATYPE_DEVICE_BATTERY_INFO,                     //电量信息
        DATATYPE_DEVICE_IR_REPORT_MODE,                   //红外线人流量计数器上报模式
        DATATYPE_DEVICE_IR_SET_REPORT_INTERVAL,           //红外线人流量计数器时间间隔设定
        DATATYPE_DEVICE_IR_THRESHOLD_TOTAL_COUNT,         //红外线人流量计数器累计人数阈值设定
        DATATYPE_DEVICE_IR_REPORT_COUNT,                  //红外线人流量计数器上报人数
        DATATYPE_DEVICE_DOOR_SENSOR_STATE,                //门磁状态 0关 1开

        DATATYPE_DEVICE_TIME_EVENT = 0x7F,   //定时事件
        DATATYPE_DEVICE_SYNC_TIME,          //同步时间
        DATATYPE_DEVICE_POWER_WAY,          //设备供电方式 0电池 1市电
        DATATYPE_DEVICE_WORK_MODE,          //设备工作模式 0正常模式 1节能模式（低功耗模式）
        DATATYPE_DEVICE_FAULT_STATE,        //故障状态 0故障解除 1设备故障
        DATATYPE_DEVICE_TAMPER_STATE,       //防拆状态 0设备已安装 1设备未安装
        DATATYPE_DEVICE_REST_FACTORY,       //恢复默认出厂设置   1LoRaWAN模块恢复出厂设置 2设备恢复出厂设置（包括LoRaWAN模块）
        DATATYPE_DEVICE_SYNC_PERIOD,        //设备数据同步周期 取值范围：10- 1440 （1分钟到24小时），默认8小时。
        DATATYPE_DEVICE_HEARTBEAT_INTERVAL, //心跳间隔
        DATATYPE_DEVICE_HEARTBEAT_PACK,     //心跳包

        DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF, //通讯应答ACK
    };

    enum ReportMode
    {
        PERIOD_REPORT_MODE = 0x01,            //周期上报模式
        REPORT_COUNT_MORETHAN_SETTING = 0x02, //计数值超过预设值
        REPORT_COUNT_CHANGED = 0x04,          //计数值改变
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

    extern u8 factoryMode;
    extern u8 keepMode;
    extern time_t heart_stamp;
    extern struct LoRaParam LoRaWAN;

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
