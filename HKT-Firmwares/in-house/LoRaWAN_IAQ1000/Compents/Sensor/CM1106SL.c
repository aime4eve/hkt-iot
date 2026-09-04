
#include "CM1106SL.h"
#include "control_center.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

/*
1、350～450ppm：同一般室外环境。 　　
2、350～1000ppm：空气清新，呼吸顺畅。 　　
3、1000～2000ppm：感觉空气浑浊，并开始觉得昏昏欲睡。 　　
4、2000～5000ppm：感觉头痛、嗜睡、呆滞、注意力无法集中、心跳加速、轻度恶心。
5、大于5000ppm：可能导致严重缺氧，造成永久性脑损伤、昏迷、甚至死亡。
*/

/**
 * @brief  数据校验
 * @param buf: 待发送数据内容
 * @param len: 待发送数据长度
 * @retval 计算好的校验和，取低8位
 */
u8 cm_cal_sum(u8 *buf, int len)
{
    int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += buf[i];
    }
    sum = -sum;
    return (sum & 0xFF);
}

/**
 * @brief 指令执行
 * @param cmd :待执行命令
 * @param len:待执行命令长度
 * @param time: 命令响应超时时间
 * @param result: 返回结果
 * @retval 执行结果
 */
bool cm_exec_cmd(u8 *cmd, u8 len, u16 timeout, u16 *result)
{
    const u16 time_slice  = 100;
    const u16 max_retries = timeout / time_slice;
    u16       retry       = 0;

    /* 清除缓存数据 */
    memset(uartCM.recvData, 0, sizeof(uartCM.recvData));
    uartCM.recvLen = 0;

    if (!CO2_PWR_CTRL_STA) {
        CO2_PWR_CTRL_H;
        mDelay(20);
    }
    /* 发送指令 */
    CM_SendData(cmd, len);

    /* 等待接收足够的数据 */
    while (uartCM.recvLen < 3 && retry < max_retries) {
        mDelay(time_slice);
        retry++;
    }

    if (retry >= max_retries) {
        DEBUG_TRACE(WARN_TAG, "\"0x%02x\" cm cmd exec timeout", cmd[2]);
        return false;
    }

    mDelay(20); // 等待串口数据接收完

    /* 验证数据有效性 */
    if (uartCM.recvData[0] != 0x16 ||
        uartCM.recvData[uartCM.recvLen - 1] != cm_cal_sum(uartCM.recvData, uartCM.recvLen - 1)) {
        DEBUG_TRACE(ERROR_TAG, "\"0x%02x\" cm cmd exec error", cmd[2]);
        return false;
    }

    DEBUG_TRACE(LOG_TAG, "\"0x%02x\" cm cmd exec success", cmd[2]);

    u8 data_len = uartCM.recvData[1];
    u8 cmd_num  = uartCM.recvData[2];
    switch (cmd_num) {
    case 0x01: // CO2查询结果
    {
        uint8_t  i;
        uint16_t co2 = uartCM.recvData[3] << 8 | uartCM.recvData[4];
        *result      = co2;
    } break;
    case 0x03: // CO2 浓度值单点校准
    {
        DEBUG_TRACE(LOG_TAG, "CM1106SL CO2 Forced Finish");
    } break;
    case 0x0F: // 模块 ABC 参数查询
    {
        u8  able   = uartCM.recvData[4];
        u8  period = uartCM.recvData[5];
        u16 ppm    = uartCM.recvData[6] << 8 | uartCM.recvData[7];
        DEBUG_TRACE(LOG_TAG, "CM1106SL Read Cal Able[%d], Period[%d], PPM[%d]", able, period, ppm);
    } break;
    case 0x10: // 模块 ABC 参数设置
    {
        DEBUG_TRACE(LOG_TAG, "CM1106SL Set Cal Success");
    } break;
    case 0x11: // 模块 发送ABC校准存储数据命令
    {
        DEBUG_TRACE(LOG_TAG, "CM1106SL Cal Store Success");
    } break;
    case 0x1E: // 模块 软件版本号
    {
        DEBUG_TRACE(LOG_TAG, "CM1106SL Version: %s", uartCM.recvData + 3);
    } break;
    case 0x1F: // 模块 SN
    {
        DEBUG_TRACE(LOG_TAG, "CM1106SL Version: %02X%02X%02X%02X%02X", uartCM.recvData[3], uartCM.recvData[4], uartCM.recvData[5], uartCM.recvData[6], uartCM.recvData[7]);
    } break;
    case 0x50: // 设置/查询传感器测量周期及平滑数据个数
    {
        if (data_len <= 4) {
            DEBUG_TRACE(LOG_TAG, "CM1106SL Set Out Success");
        } else {
            u16 period = uartCM.recvData[3] << 8 | uartCM.recvData[4];
            u8  sm_num = uartCM.recvData[5];
            DEBUG_TRACE(LOG_TAG, "CM1106SL Read Out Peiod[%d], Smoothed Data Count[%d]", period, sm_num);
        }
    } break;
    case 0x51: // 设置/查询传感器工作状态 (持续供电、间歇供电工作状态)
    {
        if (data_len <= 4) {
            DEBUG_TRACE(LOG_TAG, "CM1106SL Set Work Mode Success");
        } else {
            u8 mode = uartCM.recvData[3];
            DEBUG_TRACE(LOG_TAG, "CM1106SL Read Work Mode[%d]", mode);
        }
    } break;
    default:
        DEBUG_TRACE(WARN_TAG, "CM1106SL Recv Unknown");
        break;
    }

    return true;
}

/**
 * @brief  读CO2浓度
 * @param
 * @retval
 */
void cm_search_co2(u8 save)
{
    u8  txbuf[] = {0x11, 0x01, 0x01, 0xED};
    u16 i, co2;

    if (cm_exec_cmd(txbuf, sizeof(txbuf), 500, &co2) == true) {
        if (co2 == 0) {
            co2 = device_t.sensor_t.co2; // 读取失败使用上一次的值
            DEBUG_TRACE(ERROR_TAG, "CM1106SL Read CO2 Invalid, skipping.");
            goto __co_history;
        } else {
        __co_history:
            device_t.sensor_t.co2 = co2;
            DEBUG_TRACE(LOG_TAG, "CM1106SL Read CO2: %d", co2);

            if (save == 0)
                return;
            
            for (i = 0; i < 36; i++) {
                if (device_t.co2_history_buf[i] == 0)
                    break;
            }
            if (i < 36) {
                device_t.co2_history_buf[i] = co2;
            } else {
                for (i = 0; i < 35; i++) {
                    device_t.co2_history_buf[i] = device_t.co2_history_buf[i + 1];
                }
                device_t.co2_history_buf[35] = co2;
            }
        }
    } else {
        DEBUG_TRACE(ERROR_TAG, "CM1106SL Read CO2 Invalid, skipping.");
    }
}

// 进行 CO2 单点校准之前，请确认当前环境 CO2 值为单点校准目标值，稳定时间最少 2 分钟以上
// 单点校准目标值范围为（400 ～ 1500 ppm）
bool cm_forced(u16 forced)
{
    u8 txbuf[] = {0x11, 0x03, 0x03, 0x00, 0x00, 0x00};

    txbuf[3] = (forced >> 8) & 0xFF;
    txbuf[4] = forced & 0xFF;
    txbuf[5] = cm_cal_sum(txbuf, sizeof(txbuf) - 1);

    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

bool cm_read_version(void)
{
    u8 txbuf[] = {0x11, 0x01, 0x1E, 0xD0};
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

bool cm_read_sn(void)
{
    u8 txbuf[] = {0x11, 0x01, 0x1F, 0xCF};
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

/**
 * @brief 开启/关闭零点自校准以及零点自校准参数设置
 * @param able :校准使能（0：开启；2：关闭）
 * @param period :校准周期（1——30 可选，默认为 7，上电 24h+7 天）
 * @param ppm  :基准值
 * @retval 执行结果
 */
bool cm_set_cal(u8 able, u8 period, u16 ppm)
{
    u8 txbuf[] = {0x11, 0x07, 0x10, 0x64, 0x00, 0x07, 0x01, 0x90, 0x64, 0x78};

    txbuf[4] = able;
    txbuf[5] = period;
    txbuf[6] = (ppm >> 8) & 0xFF;
    txbuf[7] = ppm & 0xFF;
    txbuf[9] = cm_cal_sum(txbuf, sizeof(txbuf) - 1);
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

bool cm_search_cal(void)
{
    u8 txbuf[] = {0x11, 0x01, 0x0F, 0xDF};
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

/**
 * @brief 设置/查询传感器测量周期及平滑数据个数
 * @param period :测量周期(s)
 * @param sm_num  :平滑数据个数
 * @retval 执行结果
 */
bool cm_set_out(u16 period, u8 sm_num)
{
    u8 txbuf[] = {0x11, 0x04, 0x50, 0x00, 0x78, 0x0F, 0x14};

    txbuf[3] = (period >> 8) & 0xFF;
    txbuf[4] = period & 0xFF;
    txbuf[5] = sm_num;
    txbuf[6] = cm_cal_sum(txbuf, sizeof(txbuf) - 1);
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

bool cm_search_out(void)
{
    u8 txbuf[] = {0x11, 0x01, 0x50, 0xDF};
    txbuf[3]   = cm_cal_sum(txbuf, sizeof(txbuf) - 1);
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

/**
 * @brief 设置/查询传感器工作状态 (持续供电、间歇供电工作状态)
 * @param mode :0 间歇式供电工作模式（模式 A）  1 连续供电工作模式（模式 B）
 * @retval 执行结果
 */
bool cm_set_mode(u8 mode)
{
    u8 txbuf[] = {0x11, 0x02, 0x51, 0x01, 0x98};

    txbuf[3] = mode;
    txbuf[4] = cm_cal_sum(txbuf, sizeof(txbuf) - 1);
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

bool cm_search_mode(void)
{
    u8 txbuf[] = {0x11, 0x01, 0x51, 0xDF};
    txbuf[3]   = cm_cal_sum(txbuf, sizeof(txbuf) - 1);
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

// 发送ABC校准存储数据命令
bool cm_cal_store(void)
{
    u8 txbuf[] = {0x11, 0x01, 0x11, 0xDF};
    txbuf[3]   = cm_cal_sum(txbuf, sizeof(txbuf) - 1);
    return cm_exec_cmd(txbuf, sizeof(txbuf), 500, NULL);
}

/**
 * @brief	初始化二氧化碳传感器
 * @param
 * @retval
 */
void CM1106SL_Init(void)
{
    Uartx_Init(UART3, 9600);

    CO2_PWR_CTRL_H;
    tx_thread_sleep(20);

    cm_set_cal(0, 7, 400);
    cm_set_out(5, 15);
    // cm_set_out(180, 15);
    // cm_set_mode(0); // 间歇工作模式（上电一次读一次）
    cm_set_mode(1); // 持续工作模式（周期）

    cm_read_version();
    cm_read_sn();
    cm_search_cal();
    cm_search_out();
    cm_search_mode();
}

void CM1106SL_Sleep(void)
{
    cm_set_out(180, 15);
}