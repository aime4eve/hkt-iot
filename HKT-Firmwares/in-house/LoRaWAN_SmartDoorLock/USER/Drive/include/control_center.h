
#ifndef __CONTRL_CENTER_H__
#define __CONTRL_CENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define WAIT_SLEEP_TIME_DELAY  5   // 5秒未有新操作进入睡眠
#define MAX_WAIT_SLEEP_TIMEOUT 150 // 150秒未成功连接强制进入睡眠

#define MAX_SECRET_LEN            10  // 最大密钥长度
#define MAX_SECRET_NUM            100 // 最大密钥条数
#define MAX_SECRET_TIMELINESS_NUM 100 // 最大密钥条数

#define MAIN_BIT_RUN       (1 << 0)
#define MAIN_BIT_SLEEP     (1 << 1)
#define MAIN_BIT_DOOR_OPEN (1 << 2)
#define MAIN_BIT_NO_TOUCH  (1 << 3)

#define MAIN_BIT_DISPLAY_ALL (MAIN_BIT_RUN | MAIN_BIT_SLEEP | MAIN_BIT_DOOR_OPEN | MAIN_BIT_NO_TOUCH)

enum e_systemMode {
    SYSTEM_MODE_WAIT_INPUT_PASSWORD = 1,
    SYSTEM_MODE_WAIT_CHOOSE_MODE,
    SYSTEM_MODE_WAIT_CHOOSE_USER_PASSWORD, // 选择密码方式
    SYSTEM_MODE_WAIT_CHOOSE_USER_CARD,     // 选择卡方式
    SYSTEM_MODE_WAIT_CHOOSE_USER_FINGER,   // 选择指纹方式
    SYSTEM_MODE_WAIT_CHOOSE_SYSTEMCONFIG,  // 进入系统设置
    SYSTEM_MODE_SYSTEM_FACTORY,            // 确认恢复初始化状态
    // SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD,        // 等待录入管理员密码
    // SYSTEM_MODE_WAIT_INPUT_USER_PASSWORD,        // 等待录入用户密码
    // SYSTEM_MODE_WAIT_INPUT_ROOT_CARD,            // 等待录入管理员卡
    // SYSTEM_MODE_WAIT_INPUT_USER_CARD,            // 等待录入用户卡
    // SYSTEM_MODE_WAIT_INPUT_ROOT_FINGER,          // 等待录入管理员指纹
    // SYSTEM_MODE_WAIT_INPUT_USER_FINGER,          // 等待录入用户指纹
    SYSTEM_MODE_WAIT_INPUT_ROOT_PASSWORD_NUMBER, // 等待输入管理员密码编号
    SYSTEM_MODE_WAIT_INPUT_USER_PASSWORD_NUMBER, // 等待输入用户密码编号
    SYSTEM_MODE_WAIT_DELETE_PASSWORD_NUMBER,     // 等待删除用户密码信息编号
    SYSTEM_MODE_WAIT_INPUT_ROOT_CARD_NUMBER,     // 等待输入管理员卡编号
    SYSTEM_MODE_WAIT_INPUT_USER_CARD_NUMBER,     // 等待输入用户卡编号
    SYSTEM_MODE_WAIT_DELETE_CARD_NUMBER,         // 等待删除用户卡信息编号
    SYSTEM_MODE_WAIT_INPUT_ROOT_FINGER_NUMBER,   // 等待输入管理员指纹编号
    SYSTEM_MODE_WAIT_INPUT_USER_FINGER_NUMBER,   // 等待输入用户指纹编号
    SYSTEM_MODE_WAIT_DELETE_FINGER_NUMBER,       // 等待删除用户指纹信息编号
    SYSTEM_MODE_INPUT_ROOT_PASSWORD,             // 录入管理员密码
    SYSTEM_MODE_INPUT_USER_PASSWORD,             // 录入用户密码
    SYSTEM_MODE_DELETE_PASSWORD,                 // 删除用户密码信息
    SYSTEM_MODE_INPUT_ROOT_CARD,                 // 录入管理员卡
    SYSTEM_MODE_INPUT_USER_CARD,                 // 录入用户卡
    SYSTEM_MODE_DELETE_CARD,                     // 删除用户卡信息
    SYSTEM_MODE_INPUT_ROOT_FINGER,               // 录入管理员指纹
    SYSTEM_MODE_INPUT_USER_FINGER,               // 录入用户指纹
    SYSTEM_MODE_DELETE_FINGER,                   // 删除用户指纹信息
    SYSTEM_MODE_INPUT_ROOT_PASSWORD_2,           // 再次录入管理员密码
    SYSTEM_MODE_INPUT_USER_PASSWORD_2,           // 再次录入用户密码
    SYSTEM_MODE_INPUT_ROOT_CARD_2,               // 再次录入管理员卡
    SYSTEM_MODE_INPUT_USER_CARD_2,               // 再次录入用户卡
    SYSTEM_MODE_INPUT_ROOT_FINGER_2,             // 再次录入管理员指纹
    SYSTEM_MODE_INPUT_USER_FINGER_2,             // 再次录入用户指纹
    SYSTEM_MODE_INPUT_ROOT_FINGER_3,             // 再次录入管理员指纹
    SYSTEM_MODE_INPUT_USER_FINGER_3,             // 再次录入用户指纹
    SYSTEM_MODE_INPUT_ROOT_FINGER_4,             // 再次录入管理员指纹
    SYSTEM_MODE_INPUT_USER_FINGER_4,             // 再次录入用户指纹
    SYSTEM_MODE_INPUT_ROOT_FINGER_5,             // 再次录入管理员指纹
    SYSTEM_MODE_INPUT_USER_FINGER_5,             // 再次录入用户指纹
};

struct lock_log {
    union // 操作方式
    {
        struct
        {
            u16 unlockSuccess : 1; // 开锁开锁成功
            u16 unlockFail : 1;    // 开锁失败
            u16 enterRoot : 1;     // 进入管理员模式
            u16 inputAction : 1;   // 录入操作
            u16 deleteAction : 1;  // 删除操作
            u16 readAction : 1;    // 读操作
            u16 joined : 1;        // 入网
            u16 offline : 1;       // 入网后离线
            u16 isLocal : 1;       // 1 本地操作 0远程操作
            u16 tamperAlarm : 1;   // 防拆报警
            u16 tamperRecover : 1; // 防拆恢复
            u16 factory : 1;       // 恢复出厂设置

            u16 reserved_1 : 1;
            u16 reserved_2 : 1;
            u16 reserved_3 : 1;
            u16 reserved_4 : 1;
            u16 reserved_5 : 1;
        };
        u16 execAction;
    } u_exec;

    u8     type;     // 类型 0密码 1指纹 2卡 3蓝牙 0x64入网
    u8     number;   // 用户ID
    char   word[10]; // 密码或者指纹ID
    time_t stamp;    // 操作时间戳
};

struct secret_key {
    u8   isable;               // 是否有效
    u8   event;                // 同步事件
    char word[MAX_SECRET_LEN]; // 密码
};

// 15 size
struct temp_key {
    u8     isable;               // 是否有效
    time_t time;                 // 有效时长
    char   word[MAX_SECRET_LEN]; // 密码
};

struct coded_lock {
    u8                total_num_pass;         // 实际有效密码数量
    u8                total_num_card;         // 实际有效卡数量
    u8                total_num_finger;       // 实际有效指纹数量
    struct secret_key pass[MAX_SECRET_NUM];   // 密码
    struct secret_key card[MAX_SECRET_NUM];   // 已存储卡UID
    struct secret_key finger[MAX_SECRET_NUM]; // 已存储指纹
    struct temp_key   temp_pass;              // 临时密码
};

// 23 size
struct secret_key_timeliness {
    u8     user_num;             // 序号
    u8     isable;               // 0-冻结，1-生效
    u8     len;                  // 密码长度
    char   word[MAX_SECRET_LEN]; // 密码
    time_t start_timestamp;
    time_t end_timestamp;
    u16    valid_cnt; // 有效次数
};

// 32 size
struct secret_card_timeliness {
    u8     user_num;   // 序号
    u8     isable;     // 0-冻结，1-生效
    u8     id[4];      // 卡号
    u8     id_key[16]; // 密钥
    time_t start_timestamp;
    time_t end_timestamp;
    u16    valid_cnt; // 有效次数
};

// 5540 size
struct coded_lock_timeliness {
    u8 total_num_pass; // 实际有效密码数量
    u8 total_num_card; // 实际有效卡数量
    // u8 total_num_bt;                                                  // 实际有效蓝牙密码数量
    struct secret_key_timeliness  pass[MAX_SECRET_TIMELINESS_NUM + 1]; // 密码
    struct secret_card_timeliness card[MAX_SECRET_TIMELINESS_NUM];     // 已存储卡UID
    struct temp_key               temp_pass;                           // 临时密码
    // struct secret_key_timeliness bt_pass[MAX_SECRET_TIMELINESS_NUM];  // 密码
};

struct coded_lock_open_mode {
    u8  mode;       // 实际有效密码数量
    u16 lock_delay; // 延时上锁时间（常开模式2使用），单位秒
    u8  loop_duty;
    u8  loop_cnt;
    u8  loop_start_hour;
    u8  loop_start_min;
    u8  loop_start_sec;
    u8  loop_over_hour;
    u8  loop_over_min;
    u8  loop_over_sec;
};

struct device {
    char *device_name;
    u8    communicateAck : 1;  // 通讯应答ACK
    u8    updateBattery : 1;   // 更新电量信息标志
    u8    updateTamper : 1;    // 更新防拆状态标志
    u8    syncLocalTime : 1;   // 同步本地时间
    u8    rstKeyWakeEvent : 1; // 复位按键触发唤醒事件
    u8    lptWakeEvent : 1;    // 低功耗定时器触发唤醒事件
    u8    rtcWakeEvent : 1;    // RTC触发唤醒事件
    u8    touchWakeEvent : 1;  // 触摸按键触发唤醒事件
    u8    cardWakeEvent : 1;   // 读卡器触发唤醒事件
    u8    fingerWakeEvent : 1; // 指纹触发唤醒事件
    u8    loraWakeEvent : 1;   // lora唤醒事件
    u8    tamperWakeEvent : 1; // 防拆触发唤醒事件
    u8    tamperAlarm : 1;     // 防拆报警
    u8    sleepState : 1;      // 睡眠状态
    u8    sending : 1;         // 数据发送中
    u8    syncDeviceState : 1; // 同步设备状态
    u8    syncSensorData : 1;  // 同步传感器数据
    u8    bleConnectState : 1; // 蓝牙连接状态
    u8    factoryFlag : 1;     // 复位标志
    u8    isTouchInit : 1;     // 初始化触摸传感器标志
    u8    isLedShow : 1;       // LED恢复显示
    u8    isSyncBattery : 1;   // 需要采样电池信息
    u8    isBand;              // 绑定状态
    u8    network;             // 联网状态
    u8    sendFailCnt;         // LoRa数据发送失败次数
    u8    openFailCnt;         // 连续开锁失败次数
    u8    enterRootFailCnt;    // 连续进入管理员模式失败
    u8    enterRootType;       // 进入管理员模式方式 0 密码 1卡 2指纹
    u8    enterRootNum;        // 进入管理员模式编号
    u8    auto_lock_time;      // 门锁自动回锁时间
    u8    volume;              // 门锁音量
    u16   sleepDelay;          // 睡眠延时
    u16   reportInterval;      // 数据同步周期
    u32   lockDoorTimeout;     // 上锁延时
    u8    lockMode;            // 锁模式
    u16   inputNum;            // 录入编号
    u16   lockTimeout;         // 系统锁定超时
    u16   operateTimeout;      // 操作超时
    float timezone;            // 时区 取值范围-11-11 西11区到东11区
    void *user_data;
};

extern struct device device_t;
#if FUNC_OPERATIONAL_VERSION_ENABLE
extern struct coded_lock_timeliness coded_lock_t;
extern struct coded_lock_open_mode  coded_open_mode_t;
#else
extern struct coded_lock coded_lock_t;
#endif

extern TX_EVENT_FLAGS_GROUP main_event_group;

void systemLowPowerMode_Config(void);
void FuncInit(void);
void sensorReadAndConvert(void);

#ifdef __cplusplus
}
#endif

#endif
