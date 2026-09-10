
/* Includes ------------------------------------------------------------------*/
#include "rkb1145d.h"
#include "control_center.h"
#include "gpio.h"
#include "qmc5883l.h"
#include "systick.h"
#include "uart.h"

struct rkb1145d rkb1145d_t;

/**
 * @brief  雷达数据校验
 * @param buf: 待发送数据内容
 * @param len: 待发送数据长度-2
 * @retval 计算好的校验和，取低8位
 */
u8 rkb_cal_sum(u8 *buf, int len)
{
    int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += buf[i];
    }
    return (sum & 0xFF);
}

/**
 * @brief RD指令执行
 * @param cmd :待执行命令
 * @param len:待执行命令长度
 * @param time: 命令响应超时时间
 * @retval 执行结果
 */
bool rd_exec_cmd(u8 *cmd, u8 len, u16 timeout)
{
    u16 retry = 0;
    u16 time_slice = 60;

    if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
        /* 模式错误 */
        PWR_CTRL_H;
        mDelay(500);
    }

    /* 清除缓存数据 */
    memset(uartRd.recvData, 0, sizeof(uartRd.recvData));
    uartRd.recvLen = 0;

    /* 发送指令 */
    RD_SendData(cmd, len);

    while (uartRd.recvLen < 3) {
        if (time_slice * retry > timeout) {
            /* 指令超时 */
            DEBUG_TRACE(WARN_TAG, "\"0x%02x\" rd cmd exec timeout", cmd[2]);
            return false;
        }
        mDelay(time_slice);
        retry++;
    }

    mDelay(20); // 等待串口数据接收完

    // if (uartRd.recvData[0] == 0x52 && uartRd.recvData[uartRd.recvLen - 1] == 0x46 && cmd[2] == uartRd.recvData[2] &&
    //     uartRd.recvData[uartRd.recvLen - 2] == rkb_cal_sum(uartRd.recvData, uartRd.recvLen - 2)) {
    //     DEBUG_TRACE(LOG_TAG, "\"0x%02x\" rd cmd exec success", cmd[2]);
    //     return true;
    // }

    if (uartRd.recvData[0] == 0x52 && uartRd.recvData[uartRd.recvLen - 1] == 0x46 &&
        uartRd.recvData[uartRd.recvLen - 2] == rkb_cal_sum(uartRd.recvData, uartRd.recvLen - 2)) {
        DEBUG_TRACE(LOG_TAG, "\"0x%02x\" rd cmd exec success", cmd[2]);

        if (uartRd.recvData[2] == 0x03) {
            u8 max_num;
            u16 max_power = 0;
            bool data_valid = true;
            u8 *valid = uartRd.recvData + 3;
            for (u8 i = 0; i < 10; i++) {
                rkb1145d_t.power[i] = valid[0] << 8 | valid[1];
                valid += 2;
                /* 数据合理性校验：功率值超过10000视为UART数据损坏 */
                if (rkb1145d_t.power[i] > 10000) {
                    data_valid = false;
                }
                if (max_power < rkb1145d_t.power[i]) {
                    max_power = rkb1145d_t.power[i];
                    max_num = i;
                }
            }
            if (!data_valid) {
                DEBUG_TRACE(WARN_TAG, "\"0x%02x\" rd data invalid, discard", cmd[2]);
                memset(uartRd.recvData, 0, sizeof(uartRd.recvData));
                uartRd.recvLen = 0;
                return false;
            }
            rkb1145d_t.max_power_distance = max_num * 10;
            INFO("\r\n[");
            for (u8 i = 0; i < 10; i++) {
                INFO(" %d", rkb1145d_t.power[i]);
            }
            INFO(" ] [%d] [%d] power, max, distance", max_power, rkb1145d_t.max_power_distance);
        }
        /* 清除缓存数据 */
        memset(uartRd.recvData, 0, sizeof(uartRd.recvData));
        uartRd.recvLen = 0;
        return true;
    }

    /* 指令执行错误 */
    DEBUG_TRACE(ERROR_TAG, "\"0x%02x\" rd cmd exec error", cmd[2]);
    /* 清除缓存数据 */
    memset(uartRd.recvData, 0, sizeof(uartRd.recvData));
    uartRd.recvLen = 0;
    return false;
}

/**
 * @brief  查询雷达模块距离或频谱
 * @param
 * @retval
 */
void rkb_search_distance(void)
{
    u8 txbuf[] = {0x53, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x46};
    // u8 rxbuf[100] = {0};
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    RD_SendData(txbuf, sizeof(txbuf));
    // rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

bool rkb_search_distance_cal(void)
{
    u8 txbuf[] = {0x53, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x46};
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    return rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

/**
 * @brief  设置检测最小距离命令
 * @param   set: 1设置 0查询
 * @param   distance: 最小检测距离值 单位:厘米
 * @retval
 */
void rkb_set_min_distance(bool set, u16 distance)
{
    u8 txbuf[] = {0x53, 0x08, 0x11, 0x00, 0x00, 0x00, 0x00, 0x46};
    txbuf[3] = set;
    txbuf[4] = distance << 8;
    txbuf[5] = distance & 0xFF;
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

/**
 * @brief  设置检测最大距离命令
 * @param   set: 1设置 0查询
 * @param   distance: 最大检测距离值 单位:厘米
 * @retval
 */
void rkb_set_max_distance(bool set, u16 distance)
{
    u8 txbuf[] = {0x53, 0x08, 0x12, 0x00, 0x00, 0x00, 0x00, 0x46};
    txbuf[3] = set;
    txbuf[4] = distance << 8;
    txbuf[5] = distance & 0xFF;
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

/**
 * @brief  设置雷达串口的波特率
 * @param   set: 1设置 0查询
 * @param  baud: 0x01 115200
                 0x02 57600
                 0x03 38400
                 0x04 28800
                 0x05 19200
                 0x06 14400
                 0x07 9600
                 0x08 4800
                 0x09 2400
                 0x0A 1200
 * @retval
 */
void rkb_set_buad_rate(bool set, u8 baud)
{
    u8 txbuf[] = {0x53, 0x08, 0x13, 0x00, 0x00, 0x00, 0x00, 0x46};
    txbuf[3] = set;
    txbuf[5] = baud;
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

/**
 * @brief  设置雷达发送数据形式
 * @param   set: 1设置 0查询
 * @param   power: 1发送全部频谱能量 00 发送最大能量最大目标距离和能量值
 * @retval
 */
void rkb_set_data_mode(bool set, bool power)
{
    // u8 txbuf[] = {0x53, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x46};
    u8 txbuf[] = {0x53, 0x08, 0x20, 0x01, 0x00, 0x00, 0x00, 0x46};
    txbuf[3] = set;
    txbuf[5] = power;
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

/**
 * @brief  设定各距离的判断阈值(距离模式)
 * @param   cmd: 0x31  <50cm
                 0x32  50cm - 100cm
                 0x33  100cm - 150cm
                 0x34  150cm - 200cm
                 0x35  200cm - 250cm
                 0x36  >250cm

 * @param   set: 1设置 0查询
 * @param   value: 阈值
 * @retval
 */
void rkb_set_distance_mode(u8 cmd, bool set, u16 value)
{
    u8 txbuf[] = {0x53, 0x08, 0x31, 0x00, 0x00, 0x00, 0x00, 0x46};
    txbuf[2] = cmd;
    txbuf[3] = set;
    txbuf[4] = value << 8;
    txbuf[5] = value & 0xFF;
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

void rkb_search_version(void)
{
    // u8 txbuf[] = {0x53, 0x08, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x46};
    u8 txbuf[] = {0x53, 0x08, 0xAA, 0x01, 0x00, 0x00, 0x06, 0x46};
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

bool rkb_start_calibration(void)
{
    u8 txbuf[] = {0x53, 0x08, 0x55, 0x01, 0x00, 0x02, 0xb3, 0x46};
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    return rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

void rkb_open_calibration_data_out(void)
{
    u8 txbuf[] = {0x53, 0x08, 0x55, 0x01, 0x00, 0x01, 0xb2, 0x46};
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    return rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

void rkb_off_calibration_data_out(void)
{
    u8 txbuf[] = {0x53, 0x08, 0x55, 0x01, 0x00, 0x00, 0xb1, 0x46};
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);
    return rd_exec_cmd(txbuf, sizeof(txbuf), 500);
}

/**
 * @brief  初始化雷达模块
 * @param
 * @retval
 */
void rkb_init(void)
{
    rkb_set_data_mode(1, 1); // 使用频谱模式
    // rkb_set_min_distance(1, 10);
    rkb_set_min_distance(1, 0);
    rkb_set_max_distance(1, 100);
    // rkb_set_max_distance(1, 500);

    rkb_set_data_mode(0, 1);
    rkb_set_min_distance(0, 0);
    // rkb_set_max_distance(0, 100);
    rkb_set_max_distance(0, 500);
    rkb_search_version();
}

/**
 * @brief  雷达模块周期事件
 * @param
 * @retval
 */
void rkb_period_event(void)
{
    static u32 covert_time;
    static u8 event;

    // if (!device_t.power_on || !qmc5883l_t.is_calibration || rkb1145d_t.is_calibration)
    //     return;
    // if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) // 电源未开启时不处理
    //     return;
    if (!check_delay_expired(covert_time, DATA_COLLECT_INTERVAL))
        return;
    covert_time = get_syspant_ms();
    // rkb_search_distance();
    rkb_search_distance_cal();
}

void rkb_calibration_event(void)
{
    static u32 cal_time;
    static u8 fail_cnt, success_cnt;
    static u8 event;

    if (!device_t.power_on || !rkb1145d_t.is_calibration)
        return;
    if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
        PWR_CTRL_H;
    }
    if (!check_delay_expired(cal_time, DATA_COLLECT_INTERVAL))
        return;
    cal_time = get_syspant_ms();

    switch (event) {
    case 0: // 发送校准命令
        if (rkb_start_calibration() == true) {
            event = 1;
            fail_cnt = 0;
        } else {
            fail_cnt++;
            if (fail_cnt >= 5) {
                fail_cnt = 0;
                event = 3;
            }
        }
        break;
    case 1: // 发送周期查询命令
        if (rkb_search_distance_cal() == true) {
            success_cnt++;
            if (success_cnt >= 10) {
                event = 2;
                fail_cnt = 0;
            }
        } else {
            fail_cnt++;
            if (fail_cnt >= 20) {
                event = 3;
            }
        }
        break;
    case 2: {
        event = 0;
        fail_cnt = 0;
        success_cnt = 0;
        rkb1145d_t.is_calibration = 0;
        DEBUG_TRACE(LOG_TAG, "RKB Calibration Success");
        if (device_t.ble_connected) {
            BLE_SendCMD("RKB Calibration Success");
        }
    } break;
    case 3: {
        event = 0;
        fail_cnt = 0;
        success_cnt = 0;
        rkb1145d_t.is_calibration = 0;
        DEBUG_TRACE(ERROR_TAG, "RKB Calibration Fail");
        if (device_t.ble_connected) {
            BLE_SendCMD("RKB Calibration Fail");
        }
    } break;
    default:
        break;
    }
}

void rkb_adjust_offset(void)
{
    u16 power[10][10];
    u32 total = 0;
    u32 max = 0, min = 65535;
    u8 i, j;
    if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
        PWR_CTRL_H;
    }

    for (i = 0; i < 10; i++) {
        if (rkb_search_distance_cal() != true) {
            if (i) {
                i--;
            }
        } else {
            for (j = 0; j < 10; j++) {
                power[j][i] = rkb1145d_t.power[j];
            }
        }
        LL_IWDG_ReloadCounter(IWDG);
    }

    for (i = 0; i < 10; i++) {
        total = 0;
        max = 0;
        min = 65535;
        for (j = 0; j < 10; j++) {
            total += power[i][j];
            max = (power[i][j] < max) ? max : power[i][j];
            min = (power[i][j] > min) ? min : power[i][j];
        }
        rkb1145d_t.base_power[i] = (total - max - min) / 8;
    }

    INFO("\r\n[");
    for (i = 0; i < 10; i++) {
        INFO(" %d", rkb1145d_t.base_power[i]);
        j = (rkb1145d_t.base_power[i] < max) ? j : i;
    }
    rkb1145d_t.max_power_distance = j * 10;
    INFO(" ] [%d] [%d] base power, max, distance", max, rkb1145d_t.max_power_distance);
}

/**
 * @brief  雷达测距命令响应时间测试
 *         连续发送 N 次测距命令，统计最小/最大/平均响应时间
 * @param
 * @retval
 */
void rkb_response_time_test(void)
{
#define TEST_ROUNDS 50           // 测试轮数
#define POLL_SLICE_MS 1          // 轮询粒度 1ms
#define TEST_CMD_TIMEOUT_MS 1000 // 单次超时

    u8 txbuf[] = {0x53, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x46};
    txbuf[6] = rkb_cal_sum(txbuf, sizeof(txbuf) - 2);

    u32 t_min = 0xFFFFFFFF;
    u32 t_max = 0;
    u32 t_sum = 0;
    u8 success_cnt = 0;
    u8 fail_cnt = 0;

    // 确保雷达已上电
    if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
        PWR_CTRL_H;
        mDelay(500);
    }

    INFO("\r\n========================================");
    INFO("\r\n  RKB Radar Response Time Test (%d rounds)", TEST_ROUNDS);
    INFO("\r\n  Poll granularity: %d ms, Timeout: %d ms", POLL_SLICE_MS, TEST_CMD_TIMEOUT_MS);
    INFO("\r\n========================================\r\n");

    for (u8 round = 0; round < TEST_ROUNDS; round++) {
        /* 清除接收缓存 */
        memset(uartRd.recvData, 0, sizeof(uartRd.recvData));
        uartRd.recvLen = 0;

        /* 记录发送时刻 */
        u32 t_start = get_syspant_ms();

        /* 发送测距命令 */
        RD_SendData(txbuf, sizeof(txbuf));

        /* 细粒度轮询等待响应 */
        u32 elapsed = 0;
        bool timed_out = false;
        while (uartRd.recvLen < 3) {
            mDelay(POLL_SLICE_MS);
            elapsed = get_syspant_ms() - t_start;
            if (elapsed > TEST_CMD_TIMEOUT_MS) {
                timed_out = true;
                break;
            }
        }

        /* 等待剩余数据接收完 */
        mDelay(20);

        u32 t_recv = get_syspant_ms() - t_start;

        if (timed_out) {
            fail_cnt++;
            INFO("  [%02d] TIMEOUT (>%d ms)\r\n", round + 1, TEST_CMD_TIMEOUT_MS);
            continue;
        }

        /* 校验响应 */
        if (uartRd.recvData[0] == 0x52 &&
            uartRd.recvData[uartRd.recvLen - 1] == 0x46 &&
            uartRd.recvData[uartRd.recvLen - 2] == rkb_cal_sum(uartRd.recvData, uartRd.recvLen - 2)) {

            success_cnt++;
            t_sum += t_recv;

            if (t_recv < t_min)
                t_min = t_recv;
            if (t_recv > t_max)
                t_max = t_recv;

            INFO("  [%02d] %d ms\r\n", round + 1, t_recv);
        } else {
            fail_cnt++;
            INFO("  [%02d] CHECKSUM ERROR (recv_len=%d, elapsed=%d ms)\r\n",
                 round + 1, uartRd.recvLen, t_recv);
        }

        LL_IWDG_ReloadCounter(IWDG);

        /* 轮次间隔 */
        if (round < TEST_ROUNDS - 1) {
            mDelay(DATA_COLLECT_INTERVAL);
        }
    }

    /* 统计输出 */
    INFO("\r\n========================================");
    INFO("\r\n  Test Result:");
    INFO("\r\n    Success: %d / %d", success_cnt, TEST_ROUNDS);
    INFO("\r\n    Fail:    %d", fail_cnt);
    if (success_cnt > 0) {
        INFO("\r\n    Min:     %d ms", t_min);
        INFO("\r\n    Max:     %d ms", t_max);
        INFO("\r\n    Avg:     %d ms", t_sum / success_cnt);
    }
    INFO("\r\n========================================\r\n");
}

/**
 * @brief  雷达周期性读取测试
 *         每秒读取一次距离/频谱数据并打印，共 N 次
 *         适用于验证雷达持续工作稳定性
 * @param
 * @retval
 */
void rkb_period_read_test(void)
{
#define PERIOD_TEST_ROUNDS 0    // 测试次数，0 表示无限循环
#define PERIOD_INTERVAL_MS 1000 // 读取间隔 1 秒

    u8 round = 0;
    u32 next_tick = 0;

    // 确保雷达上电
    if (!ReadOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)) {
        PWR_CTRL_H;
        mDelay(500);
    }

    INFO("\r\n========================================");
    INFO("\r\n  RKB Radar Periodic Read Test");
    INFO("\r\n  Interval: %d ms", PERIOD_INTERVAL_MS);
#if PERIOD_TEST_ROUNDS > 0
    INFO("\r\n  Rounds:   %d", PERIOD_TEST_ROUNDS);
#else
    INFO("\r\n  Rounds:   infinite (power-off to stop)");
#endif
    INFO("\r\n========================================\r\n");

    while (1) {
#if PERIOD_TEST_ROUNDS > 0
        if (round >= PERIOD_TEST_ROUNDS)
            break;
#endif
        mDelay(PERIOD_INTERVAL_MS);
        round++;

        /* 读取雷达数据 */
        bool ok = rkb_search_distance_cal();
        if (!ok) {
            INFO("[%04d] READ FAIL\r\n", round);
            continue;
        }

        /* 计算统计值 */
        u16 max_power = rkb1145d_t.power[0];
        u8 max_idx = 0;
        for (u8 i = 1; i < 10; i++) {
            if (rkb1145d_t.power[i] > max_power) {
                max_power = rkb1145d_t.power[i];
                max_idx = i;
            }
        }

        /* 打印: [序号] 距离(cm) | 最大能量 | 10通道频谱 */
        INFO("[%04d] dist=%3d cm | peak=%4d | [",
             round, max_idx * 10, max_power);
        for (u8 i = 0; i < 10; i++) {
            INFO("%d", rkb1145d_t.power[i]);
            if (i < 9)
                INFO(" ");
        }
        INFO("]\r\n");

        LL_IWDG_ReloadCounter(IWDG);
    }

    INFO("\r\n==== Period Read Test End ====\r\n");
}
