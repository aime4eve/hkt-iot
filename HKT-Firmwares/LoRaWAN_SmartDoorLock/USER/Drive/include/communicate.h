
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lwrb.h"
#include <config.h>

#define lwrb_buffer_size (4096)

enum DataType {
    DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION = 0x01,    // 设备软硬件版本
    DATATYPE_DEVICE_ADDR_ID,                             // 设备ID
    DATATYPE_DEVICE_BATTERY_INFO,                        // 电量信息
    DATATYPE_DEVICE_MANAGE_CODED_LOCK = 0x2E,            // 管理开锁密码
    DATATYPE_DEVICE_REMOTE_UNLOCK = 0x2F,                // 远程开锁
    DATATYPE_DEVICE_REPORT_UNLOCK_RECORD = 0x30,         // 开锁记录上报
    DATATYPE_DEVICE_MANAGE_CARD_FINGER = 0x31,           // 管理卡和指纹状态
    DATATYPE_DEVICE_MANAGE_TEMP_PASSWORD = 0x32,         // 临时密码
    DATATYPE_DEVICE_MANAGE_CODED_LOCK_TIMELINESS = 0x4E, // 管理开锁密码(带时效)
    DATATYPE_DEVICE_MANAGE_CARD_LOCK_TIMELINESS,         // 管理开锁卡片(带时效)
    DATATYPE_DEVICE_MANAGE_FINGER_LOCK_TIMELINESS,       // 管理开锁指纹(带时效)
    DATATYPE_DEVICE_MANAGE_FACE_LOCK_TIMELINESS,         // 管理开锁人脸(带时效)
    DATATYPE_DEVICE_OPEN_MODE,                           // 常开模式
    DATATYPE_DEVICE_LOCK_AUTO_BACK_TIME,                 // 门锁自动回锁时间
    DATATYPE_DEVICE_TIMESTAMP,                           // 时间戳
    DATATYPE_DEVICE_BODY_SENSING_MODE,                   // 人体感应模式
    DATATYPE_DEVICE_USER_BIND,                           // 绑定状态
    DATATYPE_DEVICE_VOLUME_SET,                          // 音量设置
    DATATYPE_DEVICE_SYNC_TIME = 0x80,                    // 同步时间
    DATATYPE_DEVICE_POWER_WAY,                           // 设备供电方式 0电池 1市电
    DATATYPE_DEVICE_TAMPER_STATE = 0x84,                 // 防拆状态
    DATATYPE_DEVICE_REST_FACTORY = 0x85,                 // 恢复默认出厂设置  2设备恢复出厂设置
    DATATYPE_DEVICE_SYNC_PERIOD,                         // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认8小时。
    DATATYPE_DEVICE_TIME_ZONE = 0x8A,                    // 设备时区设置
    DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK = 0xFF,     // 设备应答ACK
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
extern time_t dataReportTimestamp;
extern time_t batteryTimestamp;
extern time_t wakeupTimestamp;
extern u8 lwrb_write_count;
extern lwrb_t lwrbBuff_t;
extern const u8 SYNC_HEAD[3];
extern u8 packSyncNumber;

u8 setDataPackage(enum DataType type, u8 *dest);
ErrorStatus checkDevEUI(u8 *data);

void fromLoRaWANDataHandle(u8 *data, u16 len);
void fromInfoDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void lwrbDataInit(void);
bool sendLoRaWANData(u8 *buf, u8 txlen);
void DeviceStateProcess(void);
void Device_SensorDataReport(void);
void Device_PeriodicReport(void);
void thread_communicate(ULONG thread_input);
void thread_uart(ULONG thread_input);
void thread_bt(ULONG thread_input);
void thread_deepsleep(ULONG thread_input);

#ifdef __cplusplus
}
#endif

#endif
