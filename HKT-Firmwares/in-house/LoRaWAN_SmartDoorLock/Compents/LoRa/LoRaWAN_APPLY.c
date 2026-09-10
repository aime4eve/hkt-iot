
/* Includes ------------------------------------------------------------------*/
#include "SI12T.h"
#include "W25X40CLSNIG.h"
#include "ZS902R.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "mbi5120.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>
#include <gpio.h>

/* OTAA mode default setting */
// 083FBC0065000001
u8 appEui[] = {0x08, 0x3F, 0xBC, 0x00, 0x65, 0x00, 0x00, 0x01};
// 2D35C7963275D743B4E73BEA91681A3B
u8 appKey[16] = {0x2D, 0x35, 0xC7, 0x96, 0x32, 0x75, 0xD7, 0x43, 0xB4, 0xE7, 0x3B, 0xEA, 0x91, 0x68, 0x1A, 0x3B};

/* ABP mode default setting */
// 41646472
u8 devAddr[4] = {0x41, 0x64, 0x64, 0x72};
// 2D35C7963275D743B4E73BEA91681A3B
u8 appSKey[16] = {0x2D, 0x35, 0xC7, 0x96, 0x32, 0x75, 0xD7, 0x43, 0xB4, 0xE7, 0x3B, 0xEA, 0x91, 0x68, 0x1A, 0x3B};
// 2D35C7963275D743B4E73BEA91681A3B
u8 nwkSKey[16] = {0x2D, 0x35, 0xC7, 0x96, 0x32, 0x75, 0xD7, 0x43, 0xB4, 0xE7, 0x3B, 0xEA, 0x91, 0x68, 0x1A, 0x3B};

u8 jion_fail_cnt;
time_t reconnect_stamp;
struct LoRaParam LoRaWAN;

#define MDK_YEAR ((((__DATE__[7] - '0') * 10 + __DATE__[8] - '0') * 10 + (__DATE__[9] - '0')) * 10 + (__DATE__[10] - '0'))
#define MDK_DAY ((__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0'))
#define MDK_HOUR ((__TIME__[0] - '0') * 10 + __TIME__[1] - '0')
#define MDK_MIN ((__TIME__[3] - '0') * 10 + __TIME__[4] - '0')
#define MDK_SEC ((__TIME__[6] - '0') * 10 + __TIME__[7] - '0')

// 从宏定义__DATE__中获取月份
static u8 MDK_MONTH(void)
{
    u8 month_val;
    switch (__DATE__[2]) {
    case 'n':
        if (__DATE__[1] == 'a')
            month_val = 1;
        else
            month_val = 6;
        break;
    case 'b':
        month_val = 2;
        break;
    case 'r':
        if (__DATE__[1] == 'a')
            month_val = 3;
        else
            month_val = 4;
        break;
    case 'y':
        month_val = 5;
        break;
    case 'l':
        month_val = 7;
        break;
    case 'g':
        month_val = 8;
        break;
    case 'p':
        month_val = 9;
        break;
    case 't':
        month_val = 10;
        break;
    case 'v':
        month_val = 11;
        break;
    case 'c':
        month_val = 12;
        break;
    default:
        month_val = 0;
        break;
    }
    return month_val;
}

void LoRa_DelayMs(u32 delay_time) { tx_thread_sleep(delay_time); }

/**
 * @brief  生成 AppKey
 * @param
 * @retval
 */
void LoRaWAN_CreatAPPKEY(void)
{
    uint32_t Device_Serial0 = *(u32 *)0x1FFFFE80;
    uint32_t Device_Serial1 = *(u32 *)0x1FFFFE84;
    uint32_t Device_Serial2 = *(u32 *)0x1FFFFE88;
    uint32_t Device_Serial3 = *(u32 *)0x1FFFFE8C;

    srand(Device_Serial0 + Device_Serial1);
    u32 t0 = rand();

    srand(Device_Serial2 + Device_Serial3);
    u32 t1 = rand();

    appKey[0] = (t0 >> 24) & 0xFF;
    appKey[1] = (t0 >> 16) & 0xFF;
    appKey[2] = (t0 >> 8) & 0xFF;
    appKey[3] = t0 & 0xFF;

    appKey[4] = (t1 >> 24) & 0xFF;
    appKey[5] = (t1 >> 16) & 0xFF;
    appKey[6] = (t1 >> 8) & 0xFF;
    appKey[7] = t1 & 0xFF;

    memcpy(appKey + 8, LoRaWAN.DevEUI, 8);
    memcpy(LoRaWAN.AppKEY, appKey, 16);
}

#if FUNC_LORA_VERSION_LIR

/**
 * @brief  复位LoRaWAN模组
 * @param
 * @retval
 */
void rstLoRaWANModule(void)
{
    GPIO_ResetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LoRa_DelayMs(5);
    GPIO_SetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LoRa_DelayMs(200);
    LoRaWAN.joinState = 0;
}

/**
 * @brief  获取LoRaWAN模块工作状态
 * @param
 * @retval LoRaWAN模块工作状态
 */
int getLoRaWANMode(void)
{
    if (GPIO_ReadOutputDataBit(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN) |= Bit_RESET)
        return Bit_SET;
    else
        return Bit_RESET;
}

/**
 * @brief  设置LoRaWAN模块工作状态
 * @param
 * @retval
 */
void setLoRaWANMode(int status)
{
    if (!status)
        GPIO_ResetBits(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN);
    else
        GPIO_SetBits(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN);
    LoRa_DelayMs(5);
}

/**
 * @brief  获取LoRaWAN模块睡眠状态
 * @param
 * @retval LoRaWAN模块睡眠状态
 */
int getLoRaWANStatus(void)
{
    if (GPIO_ReadOutputDataBit(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN) |= Bit_RESET)
        return Bit_SET;
    else
        return Bit_RESET;
}

/**
 * @brief  设置LoRaWAN模块睡眠状态
 * @param
 * @retval
 */
void setLoRaWANStatus(int status)
{
    if (!status)
        GPIO_ResetBits(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);
    else
        GPIO_SetBits(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);
    LoRa_DelayMs(5);
}

/**
 * @brief  设置LoRaWAN模块系统复位脚状态
 * @param
 * @retval
 */
void setLoRaWANResetIOLevel(int status)
{
    if (!status)
        GPIO_ResetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    else
        GPIO_SetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LoRa_DelayMs(5);
}

/**
 * @brief  获取LoRaWAN模块busy脚状态
 * @param
 * @retval LoRaWAN模块busy脚状态
 */
int getLoRaWANBusyIOLevel(void)
{
    if (GPIO_ReadInputDataBit(LoRaWAN_BUSY_PORT, LoRaWAN_BUSY_PIN) |= Bit_RESET)
        return Bit_SET;
    else
        return Bit_RESET;
}

/**
 * @brief  获取LoRaWAN模块stat脚状态
 * @param
 * @retval LoRaWAN模块stat脚状态
 */
int getLoRaWANStatIOLevel(void)
{
    if (GPIO_ReadInputDataBit(LoRaWAN_STAT_PORT, LoRaWAN_STAT_PIN) |= Bit_RESET)
        return Bit_SET;
    else
        return Bit_RESET;
}

/**
 * @brief  获取模组数据发送状态
 * @param
 * @retval 0 成功 1失败
 */
int getLoRaWANSendStat(void)
{
    int i, stat;
    LoRa_DelayMs(1000);
    for (i = 0; i < 6; i++) {
        stat = getLoRaWANStatIOLevel();
        if (stat)
            return 0;
        LoRa_DelayMs(1000);
    }
    return 1;
}

/**
 * @brief  获取模组数据发送状态（最大超时时间15s）
 * @param
 * @retval 0 成功 1失败
 */
int getLoRaWANBusyStat(void)
{
    int i, stat;
    for (i = 0; i < 15; i++) {
        LoRa_DelayMs(1000);
        stat = getLoRaWANBusyIOLevel();
        if (stat)
            return 0;
    }
    return 1;
}

/**
 * @brief  LoRaWAN模块初始化
 * @param
 * @retval
 */
void LoRaWAN_Init(void)
{
    char sfre[] = "AS923_AS2";
    char smode[] = "OTAA";
    setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
    setLoRaWANMode(LoRaWAN_CMD_MODE);      // 配置进入命令模式
    rstLoRaWANModule();                    // 复位模组
    LoRa_DelayMs(20);
    AT_LoRaWAN_restFactory(); // 复位模块配置为缺省状态
    AT_LoRaWAN_getDEVEUI();   // 获取地址码
    AT_LoRaWAN_getVer();

    memcpy(LoRaWAN.DevADDR, LoRaWAN.DevEUI + 4, 4);
#if !FUNC_LoRaWAN_SAMPLE
    LoRaWAN_CreatAPPKEY();
#else
    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));
    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));
#endif

    /* 配置LoRaWAN 模块参数 */
#if FUNC_LoRaWAN_ABP
    AT_LoRaWAN_setABP();
    AT_LoRaWAN_setAPPKEY();
    AT_LoRaWAN_setDEVADDR();
    AT_LoRaWAN_setAPPSKEY();
    AT_LoRaWAN_setNwkSKEY();

#else
    AT_LoRaWAN_setOTAA();
    AT_LoRaWAN_setAPPEUI();
    AT_LoRaWAN_setAPPKEY();
#endif

    // AT_LoRaWAN_setDebug(0);
#if FUNC_LoRaWAN_CN470_0
    AT_LoRaWAN_setBAND(6);
    AT_LoRaWAN_setFREQ(1, 8, 470300000);
    AT_LoRaWAN_setFREQ(3, 8, 500300000);
    // AT_LoRaWAN_setRX2(3, 505300000);
#elif FUNC_LoRaWAN_CN470_8
    AT_LoRaWAN_setBAND(6);
    AT_LoRaWAN_setFREQ(1, 8, 471900000);
    AT_LoRaWAN_setFREQ(3, 8, 501900000);
    AT_LoRaWAN_setRX2(3, 501900000);
#elif FUNC_LoRaWAN_CN470A
    AT_LoRaWAN_setBAND(6);
    AT_LoRaWAN_setFREQ(1, 8, 475300000);
    AT_LoRaWAN_setFREQ(3, 8, 505300000);
    AT_LoRaWAN_setRX2(3, 505300000);
#elif FUNC_LoRaWAN_IN865
    AT_LoRaWAN_setFREQ_IN865();
    AT_LoRaWAN_setRX2(2, 866550000);
#elif FUNC_LoRaWAN_EU868
    AT_LoRaWAN_setFREQ(1, 3, 868100000);
    AT_LoRaWAN_setRX2(0, 869525000);
#elif FUNC_LoRaWAN_AS923_AS1
    AT_LoRaWAN_setBAND(93);
    // AT_LoRaWAN_setFREQBW(1, 8, 922000000, 200); // 上下同频
    // AT_LoRaWAN_setRX2(2, 923200000);
#elif FUNC_LoRaWAN_AS923_AS2
    AT_LoRaWAN_setBAND(93);
    // AT_LoRaWAN_setFREQBW(1, 8, 923200000, 200); // 上下同频
    // AT_LoRaWAN_setRX2(2, 923200000);
#endif
    AT_LoRaWAN_setClass(LoRaWAN_DEFAULT_CLASS);
    AT_LoRaWAN_setPORT(LoRaWAN_DEFAULT_PORT);
    AT_LoRaWAN_setConfirm(1, 3);
    // AT_LoRaWAN_setConfirm(0, 3);

    AT_LoRaWAN_setJOIN();
    // AT_LoRaWAN_setCSMA();
    // AT_LoRaWAN_setBaudTimeout(50);
    // AT_LoRaWAN_getBaudTimeout();
    // AT_LoRaWAN_getCSMA();

#if FUNC_LoRaWAN_EU868_TO_US915
    AT_LoRaWAN_setBAND(90);
#endif

#if FUNC_LoRaWAN_EU868_TO_AU915
    AT_LoRaWAN_setBAND(91);
#endif

#if FUNC_LoRaWAN_AU915 || FUNC_LoRaWAN_US915
#if FUNC_LoRaWAN_AU915_0
		AT_LoRaWAN_setCHMASK(0xFF00);
#else
    AT_LoRaWAN_setCHMASK(0x00FF);
#endif
#endif

    AT_LoRaWAN_saveEEPROM(); // 保存配置参数
    LoRa_DelayMs(20);

#if FUNC_LoRaWAN_AU915 || FUNC_LoRaWAN_US915
    AT_LoRaWAN_getCHMASK();
#endif

    AT_LoRaWAN_getAPPKEY();
    AT_LoRaWAN_getAPPEUI();

    AT_LoRaWAN_getDEVADDR();
    AT_LoRaWAN_getAPPSKEY();
    AT_LoRaWAN_getNwkSKEY();

    AT_LoRaWAN_getClass();
    AT_LoRaWAN_getPORT();

    AT_LoRaWAN_getDataRate();
    // AT_LoRaWAN_getFREQ();
    AT_LoRaWAN_getRX2();

    AT_LoRaWAN_getConfirm();
    AT_LoRaWAN_getBAND();

    AT_LoRaWAN_saveEEPROM(); // 保存配置参数
    LoRa_DelayMs(20);
#if !FUNC_LoRaWAN_SAMPLE
    LoRaWAN_CreatAPPKEY();
    memcpy(LoRaWAN.DevADDR, LoRaWAN.DevEUI + 4, 4);
#endif

    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

#if FUNC_LoRaWAN_CN470_0
    snprintf(sfre, sizeof(sfre), "CN470_0");
#elif FUNC_LoRaWAN_CN470_8
    snprintf(sfre, sizeof(sfre), "CN470_8");
#elif FUNC_LoRaWAN_CN470A
    snprintf(sfre, sizeof(sfre), "CN470A");
#elif FUNC_LoRaWAN_IN865
    snprintf(sfre, sizeof(sfre), "IN865");
#elif FUNC_LoRaWAN_EU868
    snprintf(sfre, sizeof(sfre), "EU868");
#elif FUNC_LoRaWAN_US915
    snprintf(sfre, sizeof(sfre), "US915");
#elif FUNC_LoRaWAN_AU915
    snprintf(sfre, sizeof(sfre), "AU915");
#elif FUNC_LoRaWAN_AU915A
    snprintf(sfre, sizeof(sfre), "AU915A");
#elif FUNC_LoRaWAN_AS923_AS1
    snprintf(sfre, sizeof(sfre), "AS923_AS1");
#elif FUNC_LoRaWAN_AS923_AS2
    snprintf(sfre, sizeof(sfre), "AS923_AS2");
#else
    snprintf(sfre, sizeof(sfre), "ERR");
#endif
#if FUNC_LoRaWAN_ABP
    snprintf(smode, sizeof(smode), "ABP");
#else
    snprintf(smode, sizeof(smode), "OTAA");
#endif
    INFO("\r\n****************************************\r\n");
    INFO("\r\nSoftVer:V%d.%d_%s_%s_Class%s_LIR_%d.%d.%d_%d:%d:%d\r\n", HARDWARE_VER, SOFTWARE_VER, sfre, smode, LoRaWAN_DEFAULT_CLASS ? "C" : "A",
         MDK_YEAR, MDK_MONTH(), MDK_DAY, MDK_HOUR, MDK_MIN, MDK_SEC);
    INFO("\r\nDevEui:%02x%02x%02x%02x%02x%02x%02x%02x\r\n", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3],
         LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
    INFO("\r\nAppKey:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n", LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2],
         LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9],
         LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
    INFO("\r\nAppEui:%02x%02x%02x%02x%02x%02x%02x%02x\r\n", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3],
         LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
    INFO("\r\n****************************************\r\n");
}

/**
 * @brief  LoRaWAN模块入网初始化
 * @param
 * @retval
 */
void LoRaWAN_JoinInit(void)
{
    static u32 startTimer;
    static u16 waitRegisterTick;

    if (startTimer > get_syspant_ms())
        return;
    startTimer = get_syspant_ms() + 500;
    switch (LoRaWAN.joinEvent) {
    case LoRaWAN_JOINEVENT_IDLE:
        if (reconnect_stamp <= Timestamp && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE && LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR) {
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
            if (device_t.sleepDelay < MAX_WAIT_SLEEP_TIMEOUT)
                device_t.sleepDelay = MAX_WAIT_SLEEP_TIMEOUT;
        }
        break;
    case LoRaWAN_JOINEVENT_INIT:
        LoRaWAN.joinState = 0;
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
        rstLoRaWANModule();                    // 复位模组
        LoRa_DelayMs(20);
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE); // 配置进入透传模式
        LoRa_DelayMs(100);
        waitRegisterTick = 0;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_REGISTER;
        break;
    case LoRaWAN_JOINEVENT_REGISTER:
        if (getLoRaWANBusyIOLevel() && getLoRaWANStatIOLevel()) {
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_JOINED;
            if (factoryMode) {
                DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:LoRa Register Success"); // 设备入网成功
            }
        } else {
            waitRegisterTick++;
            DEBUG_TRACE(LOG_TAG, "LoRaWAN Wait Enter In Net %d", waitRegisterTick);
            if (waitRegisterTick >= 240) // 120秒
            {
                waitRegisterTick = 0; // 入网失败

                /* 入网超时 */
                DEBUG_TRACE(WARN_TAG, "LoRaWAN Enter In Net Timeout!");
                LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_ERROR;
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            }
        }
        break;
    case LoRaWAN_JOINEVENT_JOINED:
        if (factoryMode) {
            factoryMode = 0;
            LoRa_DelayMs(1000);
            DEBUG_TRACE(ACK_TAG, "factoryMode Air Monitoring Sensor Step:Finish"); // 测试完成
            LoRa_DelayMs(500);
        }

        LoRaWAN.joinState = 1;
        jion_fail_cnt = 0;
        waitRegisterTick = 0;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        LoRaWAN.status = LoRaWAN_STATUS_NORMAL;
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;

        // 入网后同步一次数据
        device_t.syncDeviceState = 1;

        {
            struct lock_log exc_lock_info_t;
            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
            exc_lock_info_t.u_exec.isLocal = 1;
            exc_lock_info_t.u_exec.joined = 1;
            exc_lock_info_t.type = 0x64;
            exc_lock_info_t.stamp = Timestamp;
            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
        }
        break;
    case LoRaWAN_JOINEVENT_ERROR:
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        if (reconnect_stamp <= Timestamp) {
            /*
                首次入网失败，间隔半小时后唤醒重新入网
                再次失败后,后续入网时间翻倍
                第一次失败间隔：30分钟
                第二次失败间隔：1小时
                第三次失败间隔：2小时
                第四次失败间隔：8小时
                第五次失败间隔：24小时（最大值）
            */
            jion_fail_cnt++;
            if (jion_fail_cnt > 5)
                jion_fail_cnt = 5;
            switch (jion_fail_cnt) {
            case 1:
                reconnect_stamp = Timestamp + 30 * 60;
                break;
            case 2:
                reconnect_stamp = Timestamp + 60 * 60;
                break;
            case 3:
                reconnect_stamp = Timestamp + 60 * 60 * 2;
                break;
            case 4:
                reconnect_stamp = Timestamp + 60 * 60 * 8;
                break;
            case 5:
                reconnect_stamp = Timestamp + 60 * 60 * 24;
                break;
            default:
                reconnect_stamp = Timestamp + 60 * 60 * 24;
                break;
            }
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS); // 进入睡眠
        break;
    default:
        break;
    }
}
#else

/**
 * @brief  复位LoRaWAN模组
 * @param
 * @retval
 */
void rstLoRaWANModule(void)
{
    GPIO_ResetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LoRa_DelayMs(5);
    GPIO_SetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LoRa_DelayMs(3200);
    LoRaWAN.joinState = 0;
}

/**
 * @brief  获取LoRaWAN模块睡眠状态
 * @param
 * @retval LoRaWAN模块睡眠状态
 */
int getLoRaWANStatus(void)
{
    if (GPIO_ReadOutputDataBit(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN) != Bit_RESET)
        return Bit_SET;
    else
        return Bit_RESET;
}

/**
 * @brief  设置LoRaWAN模块睡眠状态
 * @param
 * @retval
 */
void setLoRaWANStatus(int status)
{
    if (!status) {
        // 标准ClassC版本不允许控制进入睡眠
#if (!FUNC_STANDARD_CLASSC_ENABLE)
        GPIO_ResetBits(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);
#endif
    } else
        GPIO_SetBits(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);
    LoRa_DelayMs(5);
}

/**
 * @brief  LoRaWAN模块初始化
 * @param
 * @retval
 */
void LoRaWAN_Init(void)
{
    char sfre[] = "AS923_AS2";
    char smode[] = "OTAA";

    setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
    rstLoRaWANModule();                    // 复位模组
    LoRa_DelayMs(20);
    AT_LoRaWAN_setATEnter();

    //    AT_LoRaWAN_restFactory(); // 复位模块配置为缺省状态
    //    LoRa_DelayMs(3000);
    //    AT_LoRaWAN_setATEnter();
    // AT_LoRaWAN_setComMode();

    AT_LoRaWAN_getDEVEUI(); // 获取地址码
    AT_LoRaWAN_getVer();

    memcpy(LoRaWAN.DevADDR, LoRaWAN.DevEUI + 4, 4);
#if !FUNC_LoRaWAN_SAMPLE
    LoRaWAN_CreatAPPKEY();
#else
    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));
    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));
#endif

    /* 配置LoRaWAN 模块参数 */
#if FUNC_LoRaWAN_ABP
    AT_LoRaWAN_setJOIN(1);
    AT_LoRaWAN_setAPPKEY();
    AT_LoRaWAN_setDEVADDR();
    AT_LoRaWAN_setAPPSKEY();
    AT_LoRaWAN_setNwkSKEY();
#else
    // AT_LoRaWAN_setJOIN(0);
    AT_LoRaWAN_setAPPEUI();
    AT_LoRaWAN_setAPPKEY();
#endif

#if FUNC_LoRaWAN_CN470_0
    AT_LoRaWAN_setDataRate(5);
    AT_LoRaWAN_setBand("cn470");
    // ret|=AT_LoRaWAN_setRX2(5, 505300000);
    AT_LoRaWAN_setRX2(5, 508500000);
    AT_LoRaWAN_setCHMASK(470300000);
    AT_LoRaWAN_setADR(0);
#elif FUNC_LoRaWAN_EU868
    AT_LoRaWAN_setDataRate(5);
    AT_LoRaWAN_setBand("eu868");
    AT_LoRaWAN_setRX2(5, 869525000);
    AT_LoRaWAN_setCHMASK(867100000);
    AT_LoRaWAN_setADR(0);
#elif FUNC_LoRaWAN_US915
    AT_LoRaWAN_setBand("us915");
    AT_LoRaWAN_setRX2(8, 923300000);
    AT_LoRaWAN_setCHMASK(903900000);
    AT_LoRaWAN_setDataRate(3);
    AT_LoRaWAN_setADR(0);
    AT_LoRaWAN_setJoinAdrOFF();
#elif FUNC_LoRaWAN_AU915
    AT_LoRaWAN_setBand("au915");
    AT_LoRaWAN_setRX2(8, 923300000);
    AT_LoRaWAN_setCHMASK(916800000);
    AT_LoRaWAN_setDataRate(3);
    AT_LoRaWAN_setADR(0);
    AT_LoRaWAN_setJoinAdrOFF();
#elif (FUNC_LoRaWAN_AS923_AS1 || FUNC_LoRaWAN_AS923_AS2)
    AT_LoRaWAN_setBand("as923");
    AT_LoRaWAN_setRX2(2, 923200000);
    AT_LoRaWAN_setCHMASK(923200000);
    AT_LoRaWAN_setDataRate(3);
    AT_LoRaWAN_setADR(0);
    AT_LoRaWAN_setJoinAdrOFF();
#elif FUNC_LoRaWAN_AS923_3
    AT_LoRaWAN_setBand("as923");
    AT_LoRaWAN_setRX2(2, 916600000);
    AT_LoRaWAN_setCHMASK(916600000);
    AT_LoRaWAN_setDataRate(3);
    AT_LoRaWAN_setADR(0);
    AT_LoRaWAN_setJoinAdrOFF();
#endif

    AT_LoRaWAN_setIrq();
    AT_LoRaWAN_setPORT(LoRaWAN_DEFAULT_PORT);

#if (LoRaWAN_DEFAULT_CLASS == 2)
    AT_LoRaWAN_setTranscnf(0);
    AT_LoRaWAN_setClass(LoRaWAN_DEFAULT_CLASS);
    AT_LoRaWAN_setSleep();
#else
    AT_LoRaWAN_setTranscnf(1);
    AT_LoRaWAN_setClass(0);
    AT_AT_LoRaWAN_setSleepLight();
#endif

#if FUNC_OPERATIONAL_VERSION_ENABLE
    AT_LoRaWAN_setRxPrLen();
#endif
    AT_LoRaWAN_setLoRaWANMode();

#if (LOCAL_LOCK_ENABLE)
    AT_LoRaWAN_setJOIN_Close(0);
#else
    if (device_t.network == 0) {
        AT_LoRaWAN_setJOIN_Close(0);
    }
#endif

    AT_LoRaWAN_setJOIN_Close(0);

    AT_LoRaWAN_reboot(); // 保存配置参数
    LoRa_DelayMs(3000);

    AT_LoRaWAN_setATEnter();

#if (!LOCAL_LOCK_ENABLE)
    AT_LoRaWAN_getAPPEUI();
    AT_LoRaWAN_getAPPKEY();
    AT_LoRaWAN_getDEVADDR();
    AT_LoRaWAN_getClass();
    AT_LoRaWAN_getPORT();
    AT_LoRaWAN_getDataRate();
    AT_LoRaWAN_getCHMASK();
    AT_LoRaWAN_getSleep();
    AT_LoRaWAN_getIrq();
    AT_LoRaWAN_getRX2();
    AT_LoRaWAN_getBand();
    AT_LoRaWAN_getLpcpin();
    AT_LoRaWAN_getRx1();
    AT_LoRaWAN_getMacMode();
    AT_LoRaWAN_getRxChext();
#endif

    AT_LoRaWAN_getParam();
    LoRa_DelayMs(20);

    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

#if FUNC_LoRaWAN_CN470_0
    snprintf(sfre, sizeof(sfre), "CN470_0");
#elif FUNC_LoRaWAN_CN470_8
    snprintf(sfre, sizeof(sfre), "CN470_8");
#elif FUNC_LoRaWAN_CN470A
    snprintf(sfre, sizeof(sfre), "CN470A");
#elif FUNC_LoRaWAN_IN865
    snprintf(sfre, sizeof(sfre), "IN865");
#elif FUNC_LoRaWAN_EU868
    snprintf(sfre, sizeof(sfre), "EU868");
#elif FUNC_LoRaWAN_US915
    snprintf(sfre, sizeof(sfre), "US915");
#elif FUNC_LoRaWAN_AU915
    snprintf(sfre, sizeof(sfre), "AU915");
#elif FUNC_LoRaWAN_AU915A
    snprintf(sfre, sizeof(sfre), "AU915A");
#elif FUNC_LoRaWAN_AS923_AS1
    snprintf(sfre, sizeof(sfre), "AS923_AS1");
#elif FUNC_LoRaWAN_AS923_AS2
    snprintf(sfre, sizeof(sfre), "AS923_AS2");
#elif FUNC_LoRaWAN_AS923_3
    snprintf(sfre, sizeof(sfre), "AS923_3");
#else
    snprintf(sfre, sizeof(sfre), "ERR"); 
#endif
#if FUNC_LoRaWAN_ABP
    snprintf(smode, sizeof(smode), "ABP");
#else
    snprintf(smode, sizeof(smode), "OTAA");
#endif
    INFO("\r\n****************************************\r\n");
    INFO("\r\nSoftVer:V%d.%d_%s_%s_Class%s_OC_%d.%d.%d_%d:%d:%d\r\n", HARDWARE_VER, SOFTWARE_VER, sfre, smode, LoRaWAN_DEFAULT_CLASS ? "C" : "A",
         MDK_YEAR, MDK_MONTH(), MDK_DAY, MDK_HOUR, MDK_MIN, MDK_SEC);
    INFO("\r\nDevEui:%02x%02x%02x%02x%02x%02x%02x%02x\r\n", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3],
         LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
    INFO("\r\nAppKey:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n", LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2],
         LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9],
         LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
    INFO("\r\nAppEui:%02x%02x%02x%02x%02x%02x%02x%02x\r\n", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3],
         LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
    INFO("\r\n****************************************\r\n");
}

// /**
//  * @brief  LoRaWAN模块入网初始化
//  * @param
//  * @retval
//  */
// void LoRaWAN_JoinInit(void)
// {
//     static u32 startTimer;
//     static u16 waitRegisterTick;
//     static u8 rejoin_cnt;

//     if (startTimer > get_syspant_ms())
//         return;
//     startTimer = get_syspant_ms() + 1000;
//     if (device_t.network != 1) {
//         return;
//     }
//     switch (LoRaWAN.joinEvent) {
//     case LoRaWAN_JOINEVENT_IDLE:
//         if (reconnect_stamp <= Timestamp && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE && LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR) {
//             LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
//             if (device_t.sleepDelay < MAX_WAIT_SLEEP_TIMEOUT)
//                 device_t.sleepDelay = MAX_WAIT_SLEEP_TIMEOUT;
//         }
//         break;
//     case LoRaWAN_JOINEVENT_INIT:
//         LoRaWAN.joinState = 0;
//         setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
//         AT_LoRaWAN_setATEnter();
//         LoRa_SendCMD("at+join\r");
//         LoRa_DelayMs(20);
//         waitRegisterTick = 0;
//         LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_REGISTER;
//         break;
//     case LoRaWAN_JOINEVENT_REGISTER:
//         if (AT_LoRaWAN_getJOIN_1() == TRUE) {
//             LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_JOINED;
//             AT_LoRaWAN_setATExit();
//         } else {
//             waitRegisterTick++;
//             DEBUG_TRACE(LOG_TAG, "LoRaWAN Wait Enter In Net %d", waitRegisterTick);
//             if (!s_touch.timeout && waitRegisterTick == 10) {
//                 MBI5120_Display(0); // 关闭显示
// #if FUNC_FINGERPRINT_ENABLE
//                 tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
//                 // finger_led_control(3, 0, 0, 0); // 关闭灯光
// #endif
//             }
//             if (waitRegisterTick >= 100) { // 120秒
//                 waitRegisterTick = 0;      // 入网失败
//                 /* 入网超时 */
//                 DEBUG_TRACE(WARN_TAG, "LoRaWAN Enter In Net Timeout!");
//                 LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
//                 LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_ERROR;
//                 device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
//                 AT_LoRaWAN_setJOIN_Close(0);
//             }
//         }
//         break;
//     case LoRaWAN_JOINEVENT_JOINED:

//         LoRaWAN.joinState = 1;
//         jion_fail_cnt = 0;
//         waitRegisterTick = 0;
//         LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
//         LoRaWAN.status = LoRaWAN_STATUS_NORMAL;
//         device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
//         if (!s_touch.timeout) {
//             MBI5120_Display(0); // 关闭显示
// #if FUNC_FINGERPRINT_ENABLE
//             tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
//             // finger_led_control(3, 0, 0, 0); // 关闭灯光
// #endif
//         }

//         // LoRa_DelayMs(6000);
//         // LoRa_SendCMD("123456"); // 发送数据
//         // LoRa_DelayMs(2000);

//         // 入网后同步一次数据
//         device_t.syncDeviceState = 1;
//         uartLoRa.sendDelay = 6 * SEC_DELAY;
//         setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

//         {
//             struct lock_log exc_lock_info_t;
//             memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
//             exc_lock_info_t.u_exec.isLocal = 1;
//             exc_lock_info_t.u_exec.joined = 1;
//             exc_lock_info_t.type = 0x64;
//             exc_lock_info_t.stamp = Timestamp;
//             nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
//         }

//         break;
//     case LoRaWAN_JOINEVENT_ERROR:
//         device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
//         LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
//         LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
// #if 0
//         if (rejoin_cnt >= 3) {
//             reconnect_stamp = Timestamp + 60 * 60 * 24;
//             rejoin_cnt = 0;
//         } else {
//             reconnect_stamp = Timestamp;
//             device_t.sleepDelay = MAX_WAIT_SLEEP_TIMEOUT;
//         }
// #else
//         if (reconnect_stamp <= Timestamp) {
//             /*
//                 首次入网失败，间隔半小时后唤醒重新入网
//                 再次失败后,后续入网时间翻倍
//                 第一次失败间隔：30分钟
//                 第二次失败间隔：1小时
//                 第三次失败间隔：2小时
//                 第四次失败间隔：8小时
//                 第五次失败间隔：24小时（最大值）
//             */
//             jion_fail_cnt++;
//             if (jion_fail_cnt > 5)
//                 jion_fail_cnt = 5;
//             switch (jion_fail_cnt) {
//             case 1:
//                 reconnect_stamp = Timestamp + 30 * 60;
//                 break;
//             case 2:
//                 reconnect_stamp = Timestamp + 60 * 60;
//                 break;
//             case 3:
//                 reconnect_stamp = Timestamp + 60 * 60 * 2;
//                 break;
//             case 4:
//                 reconnect_stamp = Timestamp + 60 * 60 * 8;
//                 break;
//             case 5:
//                 reconnect_stamp = Timestamp + 60 * 60 * 24;
//                 break;
//             default:
//                 reconnect_stamp = Timestamp + 60 * 60 * 24;
//                 break;
//             }
//             DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
//         }
// #endif
//         setLoRaWANStatus(LoRaWAN_SLEEP_STATUS); // 进入睡眠
//         break;
//     default:
//         break;
//     }
// }

/**
 * @brief  LoRaWAN模块入网初始化
 * @param
 * @retval
 */
void LoRaWAN_JoinInit(void)
{
    static u32 startTimer;
    static u16 waitRegisterTick;
    static u8 rejoin_cnt;
    static u8 joinAttempts; // 新增：记录入网尝试次数

    if (startTimer > get_syspant_ms())
        return;
    startTimer = get_syspant_ms() + 1000;
    if (device_t.network != 1) {
        return;
    }
    switch (LoRaWAN.joinEvent) {
    case LoRaWAN_JOINEVENT_IDLE:
        if (reconnect_stamp <= Timestamp && LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR) {
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
            rstLoRaWANModule();
            if (device_t.sleepDelay < 30)
                device_t.sleepDelay = 30;
            joinAttempts = 0; // 重置尝试次数
        }
        break;
    case LoRaWAN_JOINEVENT_INIT:
        LoRaWAN.joinState = 0;
        // 检查尝试次数是否小于3
        if (joinAttempts < 3) {
            joinAttempts++; // 增加尝试次数
            DEBUG_TRACE(LOG_TAG, "LoRaWAN Join Attempt %d/3", joinAttempts);
            LoRaWAN.joinState = 0;
            setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
            AT_LoRaWAN_setATEnter();
            LoRa_SendCMD("at+join\r");
            LoRa_DelayMs(20);
            waitRegisterTick = 0;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_REGISTER;
        } else {
            // 3次尝试都失败
            DEBUG_TRACE(WARN_TAG, "LoRaWAN Join Failed after 3 attempts");
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_ERROR;
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            AT_LoRaWAN_setJOIN_Close(0);
        }
        break;
    case LoRaWAN_JOINEVENT_REGISTER:
        if (AT_LoRaWAN_getJOIN_1() == TRUE) {
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_JOINED;
            AT_LoRaWAN_setATExit();
        } else {
            waitRegisterTick++;
            DEBUG_TRACE(LOG_TAG, "LoRaWAN Wait Enter In Net %d", waitRegisterTick);
            if (!s_touch.timeout && waitRegisterTick == 10) {
                MBI5120_Display(0); // 关闭显示
#if FUNC_FINGERPRINT_ENABLE
                tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
            }
            if (waitRegisterTick >= 10) {
                waitRegisterTick = 0; // 入网失败
                /* 入网超时 */
                DEBUG_TRACE(WARN_TAG, "LoRaWAN Enter In Net Timeout!");
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                if (device_t.sleepDelay < 10)
                    device_t.sleepDelay = 10;
            }
        }
        break;
    case LoRaWAN_JOINEVENT_JOINED:

        LoRaWAN.joinState = 1;
        jion_fail_cnt = 0;
        waitRegisterTick = 0;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        LoRaWAN.status = LoRaWAN_STATUS_NORMAL;
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        if (!s_touch.timeout) {
            MBI5120_Display(0); // 关闭显示
#if FUNC_FINGERPRINT_ENABLE
            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
            // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
        }

        // LoRa_DelayMs(6000);
        // LoRa_SendCMD("123456"); // 发送数据
        // LoRa_DelayMs(2000);

        // 入网后同步一次数据

        device_t.syncDeviceState = 1;
        uartLoRa.sendDelay = 6 * SEC_DELAY;
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

        {
            struct lock_log exc_lock_info_t;
            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
            exc_lock_info_t.u_exec.isLocal = 1;
            exc_lock_info_t.u_exec.joined = 1;
            exc_lock_info_t.type = 0x64;
            exc_lock_info_t.stamp = Timestamp;
            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
        }

        break;
    case LoRaWAN_JOINEVENT_ERROR:
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        if (reconnect_stamp <= Timestamp) {
            reconnect_stamp = Timestamp + 60 * 60 * 72;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS); // 进入睡眠
        break;
    default:
        break;
    }
}

#endif
