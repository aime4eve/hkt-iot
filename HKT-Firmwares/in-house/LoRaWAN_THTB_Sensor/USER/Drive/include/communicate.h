
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

    enum DataType
    {
        DATATYPE_DEVICE_INTERVAL_READING = 0x01,   //
        DATATYPE_DEVICE_LOW_THRESHOLD_TEMP_ALARM,  // 温度过低报警
        DATATYPE_DEVICE_HIGH_THRESHOLD_TEMP_ALARM, //  温度过高报警
        DATATYPE_DEVICE_LOW_THRESHOLD_HUMI_ALARM,  // 湿度过低报警
        DATATYPE_DEVICE_HIGH_THRESHOLD_HUMI_ALARM, //  湿度过高报警
        DATATYPE_DEVICE_BATTERY_LOW,
        DATATYPE_DEVICE_MOVE_DETECTION,      //移动检测
        DATATYPE_DEVICE_TAMPER_ALARM,        //防拆报警
        DATATYPE_DEVICE_SYSTEM_IS_RUN = 9,       //运行 停止
        DATATYPE_DEVICE_GPS_REPORT_INTERVAL, // GPS上报间隔
        DATATYPE_DEVICE_ANGLE_THRESHOLD,     //变化角度
        DATATYPE_DEVICE_OTA_RESET,           //空中复位
        DATATYPE_DEVICE_GPS_INFO = 21,       // GPS位置信息上报
        DATATYPE_DEVICE_CACHED_TEMP = 22,    //缓存温湿度数据
        DATATYPE_DEVICE_101 = 101,
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
    extern u8 keepRunFlag;
    extern u16 packSyncNumber;
    extern time_t sensor_stamp;

    ErrorStatus checkDevEUI(u8 *data);

    void fromLoRaWANDataHandle(u8 *data, u16 len);
    void fromInfoDataHandle(u8 *data, u16 len);
    void fromUartDataHandle(void);
    void lwrbDataInit(void);
    void sendLoRaWANData(char *data);
    void sendPackFromSlave(int event);
    void SendConfigInfo(void);
    void DeviceStateProcess(void);
    void Device_PeriodicReport(void);

#ifdef __cplusplus
}
#endif

#endif
