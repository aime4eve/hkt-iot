
#ifndef __LoRAWAN_APPLY_H__
#define __LoRAWAN_APPLY_H__

///** 使用ADR功能，模块在运行过程中，服务器会根据ADR机制动态调整模块的发送功率，通讯速率

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define LoRaWAN_PASSTHROUGH_MODE 0 // 透传模式
#define LoRaWAN_CMD_MODE 1         // 命令模式

#define LoRaWAN_SLEEP_STATUS 0 // 睡眠状态
#define LoRaWAN_WAKE_STATUS 1  // 激活状态

    typedef enum sfType
    {
        SF6,
        SF7,
        SF8,
        SF9,
        SF10,
        SF11,
        SF12
    } sfType_t;
    typedef enum bwType
    {
        BW62K,
        BW125K,
        BW250K,
        BW500K
    } bwType_t;
    typedef enum crType
    {
        CR4_5,
        CR4_6,
        CR4_7,
        CR4_8
    } crType_t;

    typedef union
    {
        struct
        {
            u8 FreqL : 8;
            u8 FreqM : 8;
            u8 FreqH : 8;
            u8 FreqX : 8;
        } freq;
        unsigned long Freq;
    } FreqStruct;

    typedef enum LoRaWAN_Status
    {
        LoRaWAN_STATUS_NORMAL,
        LoRaWAN_STATUS_NORESPONSES,
        LoRaWAN_STATUS_NET_ERROR,
    } LoRaWAN_Status_t;

    typedef enum LoRaWAN_JoinEvent
    {
        LoRaWAN_JOINEVENT_IDLE,
        LoRaWAN_JOINEVENT_INIT,     // 初始化
        LoRaWAN_JOINEVENT_REGISTER, // 注册
        LoRaWAN_JOINEVENT_JOINED,   // 完成入网
        LoRaWAN_JOINEVENT_ERROR,    // 入网失败

    } LoRaWAN_JoinEvent_t;

    struct LoRaParam
    {
        u8 joinState : 1;    // 入网状态
        u8 waitJoin : 1;     // 等待设备入网
        u8 connectState : 1; // 通讯状态
        u8 port;
        u8 class;     // 0 class A    2 class C
        u8 DevEUI[8]; // OTAA入网方式使用
        u8 AppEUI[8];
        u8 AppKEY[16];

        u8 DevADDR[4]; // ABP入网方式可直接使用如下三个参数
        u8 AppSKEY[16];
        u8 NwkSKEY[16];

        LoRaWAN_Status_t status;       // LoRa模块状态 0 正常 1入网失败
        LoRaWAN_JoinEvent_t joinEvent; // LoRa入网事件

        u16 bandWidth; // unit: KHz
        u32 frequency; // unit: KHz

        char outputPower; // unit: dBm   range: 2-20 [2dBm~+20dBm]

        enum sfType SFSel; // unit: none, range: SF6~SF12
        enum bwType BWSel;
        enum crType CRSel;

        FreqStruct FrequencyValue;
    };

    extern u8 appEui[];
    extern u8 appKey[];
    extern u8 appSKey[];
    extern u8 nwkSKey[];
    extern u8 devAddr[];
    extern u8 jion_fail_cnt;
    extern time_t reconnect_stamp;
    extern struct LoRaParam LoRaWAN;

    void LoRa_DelayMs(u32 delay_time);
    void rstLoRaWANModule(void);
    int getLoRaWANMode(void);
    void setLoRaWANMode(int status);
    int getLoRaWANStatus(void);
    void setLoRaWANStatus(int status);
    void setLoRaWANResetIOLevel(int status);
    int getLoRaWANBusyIOLevel(void);
    int getLoRaWANStatIOLevel(void);
    int getLoRaWANSendStat(void);
    int getLoRaWANBusyStat(void);
    void LoRaWAN_CreatAPPKEY(void);

    void LoRaWAN_Init(void);
    void LoRaWAN_JoinInit(void);

#ifdef __cplusplus
}
#endif

#endif
