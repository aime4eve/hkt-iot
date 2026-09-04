
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY 10   // 10秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 150 // 150秒未成功连接强制进入睡眠

struct device {
    char *device_name;
    u8 rtcWakeEvent : 1;  // RTC触发唤醒事件
    u8 extiWakeEvent : 1; // 按键唤醒事件
    u8 loraWakeEvent : 1; // lora唤醒事件
    u8 sleepState : 1;    // 睡眠状态
    u16 sleepDelay;       // 睡眠延时
    void *user_data;
};

extern struct device device_t;

void sendLoRaWANData(void);
void fromLoRaWANDataHandle(u8 *data, u16 len);
void fromInfoDataHandle(u8 *data, u16 len);
void fromBTDataHandle(u8 *data, u16 len);
void fromUartDataHandle(void);
void DeviceStateProcess(void);
void systemLowPowerMode_Config(void);
void my_buff_evt_fn(lwrb_t *buff, lwrb_evt_type_t type, size_t len);
void lwrbDataInit(void);

#ifdef __cplusplus
}
#endif

#endif
