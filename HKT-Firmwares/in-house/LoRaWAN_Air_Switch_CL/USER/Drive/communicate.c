
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "an001.h"
#include "flash.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

u8 lwrb_write_count;
u8 lwrb_data[1024 * 2];
lwrb_t lwrbBuff_t;

u8 sendNum;
u8 sendBuffer[100];

u8 send_failcnt;
u8 AS_SeriaNet;

/**
 * @brief  将十六进制字符串转换为原始字节数组
 * @param  hex_str: 十六进制字符串 (如 "AA0055")
 * @param  raw: 输出原始字节缓冲区
 * @param  raw_size: 缓冲区大小
 * @retval >0: 转换后的字节数, -1: 失败
 */
static int hex_str_to_raw(const char *hex_str, uint8_t *raw, int raw_size)
{
    int hex_len = strlen(hex_str);
    if (hex_len % 2 != 0 || raw_size < hex_len / 2)
        return -1;
    u16 result = cmd_get_hexstr2hex(raw, (u16)raw_size, (const u8 *)hex_str, (u16)hex_len);
    return (result > 0) ? (int)result : -1;
}

/**
 * @brief  解析 +radiorx / +macrx 格式的 LoRaWAN 下行数据字符串
 *
 * 支持两种格式:
 *   +radiorx,<rssi>,<snr>,<hex_data>
 *    例: +radiorx,-94,11,31323334353637383930
 *   +macrx,<rssi>,<snr>,<port>,<hex_data>
 *    例: +macrx,-77,9,10,010169554880093f80000017233c00000e10
 *
 * @param  recv: 输入的原始 AT 响应字符串
 * @param  rx_data: 输出的解析结果结构体
 */
void parse_radiorx(const char *recv, RadioRxData *rx_data)
{
    /* 初始化解析结果（默认失败） */
    memset(rx_data, 0, sizeof(RadioRxData));
    rx_data->parse_ok = 0;

    if (recv == NULL || rx_data == NULL) {
        INFO("Error: Invalid input parameter\n");
        return;
    }

    /* 定位 +radiorx 或 +macrx 起始位置 */
    char *valid_data = NULL;
    if (strstr(recv, "+radiorx")) {
        valid_data = strstr(recv, "+radiorx");
    } else if (strstr(recv, "+macrx")) {
        valid_data = strstr(recv, "+macrx");
    } else {
        INFO("Error: Not a radiorx or macrx string\n");
        return;
    }

    char recv_copy[256] = {0};
    strncpy(recv_copy, valid_data, sizeof(recv_copy) - 1);
    char *token = strtok(recv_copy, ",");

    int  field_idx = 0;
    char hex_str[256] = {0}; /* 存储十六进制字符串段 */

    /* +macrx,-77,9,10,010169554880093f80000017233c00000e10 */
    while (token != NULL) {
        switch (field_idx) {
            case 0:
                /* 跳过头部标识字段 "+radiorx" 或 "+macrx" */
                break;

            case 1:
                rx_data->rssi = atoi(token);
                /* 校验 RSSI 合理性（LoRa 的 RSSI 通常在 -148~0 之间） */
                if (rx_data->rssi < -148 || rx_data->rssi > 0) {
                    INFO("Warning: Abnormal RSSI value %d\n", rx_data->rssi);
                }
                break;

            case 2:
                rx_data->snr = atoi(token);
                if (rx_data->snr <= -50 || rx_data->snr > 127) {
                    INFO("Error: Abnormal SNR value %d\n", rx_data->snr);
                    return;
                }
                break;

            case 3:
                if (strstr(recv, "+macrx")) {
                    rx_data->port = atoi(token);
                } else {
                    strncpy(hex_str, token, sizeof(hex_str) - 1);
                }
                break;
            case 4:
                if (strstr(recv, "+macrx")) {
                    strncpy(hex_str, token, sizeof(hex_str) - 1);
                }
                break;
            default: /* 多余字段，忽略 */
                break;
        }
        token = strtok(NULL, ",");
        field_idx++;
    }

    if (field_idx < 4) {
        INFO("Error: Incomplete radiorx fields (only %d fields)\n", field_idx);
        return;
    }

    /* 剥离 hex_str 末尾的 \r \n (AT 响应行尾携带) */
    {
        char *p = hex_str;
        while (*p) {
            if (*p == '\r' || *p == '\n') {
                *p = '\0';
                break;
            }
            p++;
        }
    }

    int raw_len = hex_str_to_raw(hex_str, rx_data->raw_data, sizeof(rx_data->raw_data));
    if (raw_len < 0) {
        INFO("Error: Parse hex string failed\n");
        return;
    }

    rx_data->raw_len = raw_len;
    rx_data->parse_ok = 1;
}

#if FUNC_LORA_VERSION_LIR
/**
 * @brief  发送填充好的数据给LoRa模块
 * @param
 * @retval
 */
void sendLoRaWANData(void)
{
    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
    // /* 等待模组就绪 */
    // while (!getLoRaWANBusyIOLevel())
    //     ;
    // delay_1ms(20);

    uartLoRa.sendLen = sendNum;
    memcpy(uartLoRa.sendData, sendBuffer, uartLoRa.sendLen);
    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
#if HIGH_LEVEL_DEBUG_ENABLE
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
#endif
    if (getLoRaWANBusyStat()) {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto __fail;
    }
    if (getLoRaWANSendStat()) {
    __fail:

        // 检查剩余空间，不足时丢弃最旧数据
        while (lwrb_get_free(&lwrbBuff_t) < (size_t)(1 + sendNum)) {
            u8 old_len;
            if (lwrb_read(&lwrbBuff_t, &old_len, 1) == 1) {
                lwrb_skip(&lwrbBuff_t, old_len);
            } else {
                lwrb_reset(&lwrbBuff_t);
                break;
            }
            if (lwrb_write_count > 0)
                lwrb_write_count--;
        }
        lwrb_write_count++;

        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        lwrb_write(&lwrbBuff_t, &uartLoRa.sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, uartLoRa.sendData, uartLoRa.sendLen);

        send_failcnt++;
        if (send_failcnt >= 5) {
            send_failcnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            jion_fail_cnt++;
            reconnect_stamp = Timestamp + 10 * 60;

            lwrb_write_count = 0; // 离线后清除缓冲区
            memset(lwrb_data, 0, sizeof(lwrb_data));
            lwrb_reset(&lwrbBuff_t);
        }
        return;
    }
    send_failcnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 3 * SEC_DELAY;
}

#else

/**
 * @brief  发送填充好的数据给LoRa模块
 * @param
 * @retval
 */
void sendLoRaWANData(void)
{
    char upload_data[256] = {0};
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);

    uartLoRa.sendLen = sendNum;
    memcpy(uartLoRa.sendData, sendBuffer, uartLoRa.sendLen);

    // 先查询入网状态 再进行数据发送
    // AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_getJOIN() == TRUE) {
        // AT_LoRaWAN_setATExit();
        delay_1ms(1000);

        int convert_len = cmd_get_hex2hexstr((uint8_t *)upload_data, sizeof(upload_data), uartLoRa.sendData, uartLoRa.sendLen);
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        if (AT_LoRaWAN_macTxHex("cnf", 10, upload_data) != TRUE) {
            DEBUG_TRACE(LOG_TAG, "LoRa SendData Fail!!!");
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
            uartLoRa.sendDelay = 6 * SEC_DELAY;

            /* 检查剩余空间，不足时丢弃最旧数据 */
            while (lwrb_get_free(&lwrbBuff_t) < (size_t)(1 + uartLoRa.sendLen)) {
                u8 old_len;
                if (lwrb_read(&lwrbBuff_t, &old_len, 1) == 1) {
                    lwrb_skip(&lwrbBuff_t, old_len);
                } else {
                    lwrb_reset(&lwrbBuff_t);
                    break;
                }
                if (lwrb_write_count > 0)
                    lwrb_write_count--;
            }
            lwrb_write_count++;
            lwrb_write(&lwrbBuff_t, &uartLoRa.sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, uartLoRa.sendData, uartLoRa.sendLen);
            return;
        }
        // LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    } else {
        // AT_LoRaWAN_setATExit();
        // setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        LoRaWAN.joinState = 0;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        reconnect_stamp = Timestamp + 30 * 60;
        DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        uartLoRa.sendDelay = 6 * SEC_DELAY;

        /* 检查剩余空间，不足时丢弃最旧数据 */
        while (lwrb_get_free(&lwrbBuff_t) < (size_t)(1 + uartLoRa.sendLen)) {
            u8 old_len;
            if (lwrb_read(&lwrbBuff_t, &old_len, 1) == 1) {
                lwrb_skip(&lwrbBuff_t, old_len);
            } else {
                lwrb_reset(&lwrbBuff_t);
                break;
            }
            if (lwrb_write_count > 0)
                lwrb_write_count--;
        }
        lwrb_write_count++;
        lwrb_write(&lwrbBuff_t, &uartLoRa.sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, uartLoRa.sendData, uartLoRa.sendLen);
        return;
    }
    // setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 6 * SEC_DELAY;
    return;
}

#endif

/**
 * @brief  回应ACK数据
 * @param
 * @retval
 */
void ACK_Server(u8 cmd, u8 addr) 
{
    u16 crc;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendBuffer[0] = 0xAA;               // 帧头
    sendBuffer[1] = cmd;                // 上传应答命令码
    sendBuffer[2] = addr;               // 把下发的空开地址作为应答的空开地址
    sendBuffer[3] = 0x00;               // 数据长度
    crc = crc_cal_value(sendBuffer, 4); // 计算CRC
    sendBuffer[4] = (u8)crc;            // CRC低字节
    sendBuffer[5] = (u8)(crc >> 8);     // CRC高字节
    sendBuffer[6] = 0x55;               // 帧尾
    sendNum = 7;

    sendLoRaWANData();
}

void ACK_Server1(u8 cmd, u8 addr)
{
    u16 crc, value;
    u8 bit_16_h = 0, bit_16_l = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));

    // for (u8 j = 0; j < DEFAULT_AS_NUMBER; j++)
    // {
    //     if (j < 8)  //存在问题多移一位！！！ 市机关版本 保留错误
    //     {
    //         if (as_an001_t.open_state[j] == 1)
    //             bit_16_h |= 0x01;
    //         bit_16_h = bit_16_h << 1;
    //     }
    //     else
    //     {
    //         if (as_an001_t.open_state[j] == 1)
    //             bit_16_l |= 0x01;
    //         bit_16_l = bit_16_l << 1;
    //     }
    // }
    // value = bit_16_h << 8 | bit_16_l;

    for (u8 j = 0; j < 15; j++) {
        if (j < DEFAULT_AS_NUMBER) {
            if (as_an001_t.open_state[j] == 1)
                value |= 0x01;
        }
        value = value << 1;
    }

    sendBuffer[0] = 0xAA;               // 帧头
    sendBuffer[1] = cmd;                // 上传应答命令码
    sendBuffer[2] = addr;               // 把下发的空开地址作为应答的空开地址
    sendBuffer[3] = 0x02;               // 数据长度
    sendBuffer[4] = (u8)(value >> 8);   // 高字节
    sendBuffer[5] = (u8)value;          // 低字节
    crc = crc_cal_value(sendBuffer, 6); // 计算CRC
    sendBuffer[6] = (u8)crc;            // CRC低字节
    sendBuffer[7] = (u8)(crc >> 8);     // CRC高字节
    sendBuffer[8] = 0x55;               // 帧尾

    sendNum = 9;
    sendLoRaWANData();
}

void ACK_Server2(u8 cmd, u8 addr, u16 value)
{
    u16 crc;
    u8 bit_16_h = 0, bit_16_l = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));

    sendBuffer[0] = 0xAA;               // 帧头
    sendBuffer[1] = cmd;                // 上传应答命令码
    sendBuffer[2] = addr;               // 把下发的空开地址作为应答的空开地址
    sendBuffer[3] = 0x02;               // 数据长度
    sendBuffer[4] = (u8)(value >> 8);   // 高字节
    sendBuffer[5] = (u8)value;          // 低字节
    crc = crc_cal_value(sendBuffer, 6); // 计算CRC
    sendBuffer[6] = (u8)crc;            // CRC低字节
    sendBuffer[7] = (u8)(crc >> 8);     // CRC高字节
    sendBuffer[8] = 0x55;               // 帧尾

    sendNum = 9;
    sendLoRaWANData();
}

/**
 * @brief  将指定空开阈值上传给服务器
 * @param
 * @retval
 */
void reportAirSwitchThresholdValue(u8 cmd, u8 addr)
{
    u16 *ptr;
    u16 crc, value;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    sendBuffer[sendNum++] = 0xAA;
    sendBuffer[sendNum++] = cmd;                                       // CMD
    sendBuffer[sendNum++] = addr;                                      // 设备编号
    sendBuffer[sendNum++] = as_an001_t.slave_param[addr].version >> 8; // 设备型号
    sendBuffer[sendNum++] = as_an001_t.slave_param[addr].item & 0xFF;  // 空开类型
    sendBuffer[sendNum++] = 40;                                        // 数据长度=20*2=40字节

    ptr = &as_an001_t.slave_param[addr].max_voltage;
    for (u8 j = 0; j < 6; j++) {
        value = *(ptr + j);
        sendBuffer[sendNum++] = (u8)(value >> 8); // 高字节
        sendBuffer[sendNum++] = (u8)value;        // 低字节
    }

    ptr = &as_an001_t.slave_param[addr].max_a_electricity;
    for (u8 j = 0; j < 14; j++) {
        value = *(ptr + j);
        sendBuffer[sendNum++] = (u8)(value >> 8); // 高字节
        sendBuffer[sendNum++] = (u8)value;        // 低字节
    }
    crc = crc_cal_value(sendBuffer, sendNum); // 求crc
    sendBuffer[sendNum++] = (u8)crc;          // CRC低字节
    sendBuffer[sendNum++] = (u8)(crc >> 8);   // CRC高字节
    sendBuffer[sendNum++] = 0x55;

    sendLoRaWANData();
}

/**
 * @brief  将指定空开的实时数据发送到服务端
 * @param
 * @retval
 */
void reportAirSwitchRealTimeData(u8 cmd, u8 addr)
{
    u16 *ptr = &as_an001_t.slave_realtime_data[addr].total_voltage; // 获取指定空开的实时数据结构体中第1个变量地址
    u16 crc, value;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    sendBuffer[sendNum++] = 0xAA;
    sendBuffer[sendNum++] = cmd;                                       // CMD
    sendBuffer[sendNum++] = addr;                                      // 设备编号
    sendBuffer[sendNum++] = as_an001_t.slave_param[addr].version >> 8; // 设备型号
    sendBuffer[sendNum++] = as_an001_t.slave_param[addr].item & 0xFF;  // 空开类型
    sendBuffer[sendNum++] = 59;                                        // 数据长度=29*2+1=59字节

    for (u8 j = 0; j < 29; j++) // 将29个u16类型的实时数据拷贝到填充帧
    {
        value = *(ptr + j);
        sendBuffer[sendNum++] = (u8)(value >> 8); // 高字节
        sendBuffer[sendNum++] = (u8)value;        // 低字节
    }

    sendBuffer[sendNum++] = as_an001_t.is_remote_control[addr]; // 能否被远程控制

    crc = crc_cal_value(sendBuffer, sendNum); // 求crc
    sendBuffer[sendNum++] = (u8)crc;          // CRC低字节
    sendBuffer[sendNum++] = (u8)(crc >> 8);   // CRC高字节
    sendBuffer[sendNum++] = 0x55;

    sendLoRaWANData();
}

/**
 * @brief  上报空开在线状态
 * @param
 * @retval
 */
void reportAirSwitchAliveState(u8 cmd, u8 addr)
{
    u16 crc, value = 0;
    u8 bit_16_h = 0, bit_16_l = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    // for (u8 j = 0; j < DEFAULT_AS_NUMBER; j++)
    // {
    //     if (j < 8)  //市机关版本 保留错误
    //     {
    //         if (as_an001_t.is_alive[j] == 1)
    //             bit_16_h |= 0x01;
    //         bit_16_h = bit_16_h << 1;
    //     }
    //     else
    //     {
    //         if (as_an001_t.is_alive[j] == 1)
    //             bit_16_l |= 0x01;
    //         bit_16_l = bit_16_l << 1;
    //     }
    // }
    // value = bit_16_h << 8 | bit_16_l;

    for (u8 j = 0; j < 15; j++) {
        if (j < DEFAULT_AS_NUMBER) {
            if (as_an001_t.is_alive[j] == 1)
                value |= 0x01;
        }
        value = value << 1;
    }

    sendBuffer[0] = 0xAA;               // 帧头
    sendBuffer[1] = cmd;                // 上传应答命令码
    sendBuffer[2] = addr;               // 把下发的空开地址作为应答的空开地址
    sendBuffer[3] = 0x02;               // 数据长度
    sendBuffer[4] = (u8)(value >> 8);   // 高字节
    sendBuffer[5] = (u8)value;          // 低字节
    crc = crc_cal_value(sendBuffer, 6); // 计算CRC
    sendBuffer[6] = (u8)crc;            // CRC低字节
    sendBuffer[7] = (u8)(crc >> 8);     // CRC高字节
    sendBuffer[8] = 0x55;               // 帧尾

    sendNum = 9;
    sendLoRaWANData();
}

/**
 * @brief  上报空开分合闸状态远程使能状态
 * @param
 * @retval
 */
void reportAirSwitchRemoteState(u8 cmd, u8 addr)
{
    u16 crc, value = 0;
    u8 bit_16_h = 0, bit_16_l = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    // for (u8 j = 0; j < DEFAULT_AS_NUMBER; j++)
    // {
    //     if (j < 8)  //市机关版本 保留错误
    //     {
    //         if (as_an001_t.is_alive[j] == 1)
    //             bit_16_h |= 0x01;
    //         bit_16_h = bit_16_h << 1;
    //     }
    //     else
    //     {
    //         if (as_an001_t.is_alive[j] == 1)
    //             bit_16_l |= 0x01;
    //         bit_16_l = bit_16_l << 1;
    //     }
    // }
    // value = bit_16_h << 8 | bit_16_l;

    for (u8 j = 0; j < 15; j++) {
        if (j < DEFAULT_AS_NUMBER) {
            if (as_an001_t.open_state[j] == 1)
                value |= 0x01;
        }
        value = value << 1;
    }

    sendBuffer[0] = 0xAA;             // 帧头
    sendBuffer[1] = cmd;              // 上传应答命令码
    sendBuffer[2] = addr;             // 把下发的空开地址作为应答的空开地址
    sendBuffer[3] = 0x04;             // 数据长度
    sendBuffer[4] = (u8)(value >> 8); // 高字节
    sendBuffer[5] = (u8)value;        // 低字节

    // for (u8 j = 0; j < DEFAULT_AS_NUMBER; j++)
    // {
    //     if (j < 8)  //市机关版本 保留错误
    //     {
    //         if (as_an001_t.is_remote_control[j] == 1)
    //             bit_16_h |= 0x01;
    //         bit_16_h = bit_16_h << 1;
    //     }
    //     else
    //     {
    //         if (as_an001_t.is_remote_control[j] == 1)
    //             bit_16_l |= 0x01;
    //         bit_16_l = bit_16_l << 1;
    //     }
    // }
    // value = bit_16_h << 8 | bit_16_l;

    value = 0;
    for (u8 j = 0; j < 15; j++) {
        if (j < DEFAULT_AS_NUMBER) {
            if (as_an001_t.is_remote_control[j] == 1)
                value |= 0x01;
        }
        value = value << 1;
    }

    sendBuffer[6] = (u8)(value >> 8);   // 高字节
    sendBuffer[7] = (u8)value;          // 低字节
    crc = crc_cal_value(sendBuffer, 8); // 计算CRC
    sendBuffer[8] = (u8)crc;            // CRC低字节
    sendBuffer[9] = (u8)(crc >> 8);     // CRC高字节
    sendBuffer[10] = 0x55;              // 帧尾

    sendNum = 11;
    sendLoRaWANData();
}

/**
 * @brief  处理来自LoRaWAN模组的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromLoRaWANDataHandle(u8 *data, u16 len)
{
    if ((data[0] == 0xAA) && (data[len - 1] == 0x55) && (SUCCESS == as_check_crc16(data, len))) // 帧头、帧尾及CRC校验 通过
    {

        u8 cmd = data[1];
        u8 addr = data[2]; // 空开地址
        switch (cmd)       // 根据下发的指令码分类处理
        {
        case 0x00:        // 设置空开地址
            if (len == 8) // 帧长度判断
            {
                if (data[4] == 0x01)
                    as_write_cmd(CMD_AS_ENTER_ADDR_MODE, 0, 0); // 设置空开地址-已确认，空开执行指令完成之后再应答服务器
                else
                    as_write_cmd(CMD_AS_OUT_ADDR_MODE, 0, 0); // 设置空开地址-已确认，空开执行指令完成之后再应答服务器
                ACK_Server(cmd, 0x00);                        // 应答服务器，地址字节默认为0x00，主机不用关心
            }
            break;
        case 0x01:        // 设置上传实时数据间隔时间
            if (len == 9) // 帧长度判断
            {
                u32 interval = ((u16)data[4] << 8) | data[5]; // 秒
                if (interval > 36000)                         // 上限限定 10小时
                    interval = 36000;
                else if ((interval < 300) && (interval > 0)) // 下限限定5分钟，为0时不上传实时参数
                    interval = 300;
                as_an001_t.period_upload_real_time = interval;
                as_an001_t.period_upload_real_timeout = 0;
                ACK_Server(cmd, 0x00); // 应答服务器，地址字节默认为0x00，主机不用关心
                fmc_program_report_interval();
            }
            break;
        case 0x03:        // 设置指定空开通断(注意：每次设置地址完成后，首次必须手动合闸才能置位远程控制使能位，否则无法执行远程控制指令)
            if (len == 8) // 帧长度判断
            {
                u8 action = data[4];          // 动作值
                if (addr < DEFAULT_AS_NUMBER) // 空开地址范围 0~9 表示操作单个空开
                {
                    // 如果此空开在线 且 远程控制已使能 且 此空开支持远程控制
                    if (as_an001_t.is_alive[addr] && as_an001_t.is_remote_control[addr]) {
                        as_write_cmd(CMD_AS_SET_CONTACT, addr, action);
                        ACK_Server(cmd, addr); // 应答服务器，地址字节默认为0x00，主机不用关心
                    }
                } else if (addr == 0xFF) // 操作所有空开
                {
                    for (u8 j = 0; j < DEFAULT_AS_NUMBER; j++) {
                        if (as_an001_t.is_alive[j] && as_an001_t.is_remote_control[j]) {
                            as_write_cmd(CMD_AS_SET_CONTACT, j, action); // 设置此空开状态
                        }
                    }
                    ACK_Server(cmd, addr); // 应答服务器
                }
            }
            break;
        case 0x04:        // 恢复出厂设置
            if (len == 7) // 帧长度判断
            {
                as_write_cmd(CMD_AS_RESET_FACTORY_MODE_2, 0, 0); // 设置此空开状态
                ACK_Server(cmd, 0x00);                           // 应答服务器
            }
            break;
        case 0x05:        // 读取指定空开阈值
            if (len == 7) // 帧长度判断
            {
                if (addr < DEFAULT_AS_NUMBER) // 空开地址0~9
                {
                    reportAirSwitchThresholdValue(cmd, addr);
                    DEBUG_TRACE(LOG_TAG, "Report Air Switch Threshold Value %d", addr);
                }
            }
            break;
        case 0x06:        // 读取指定空开实时数据
            if (len == 7) // 帧长度判断
            {
                if (addr < DEFAULT_AS_NUMBER) // 空开地址0~9
                {
                    reportAirSwitchRealTimeData(cmd, addr);    // 发送指定空开实时数据
                    as_an001_t.period_upload_real_timeout = 0; // 手动查询上报实时数据后，计数器清零，防止主动上报实时数据进程中马上又发送实时数据

                    DEBUG_TRACE(LOG_TAG, "Report Air Switch Real Time Data %d", addr);
                }
            }
            break;
        case 0x08:        // 读取空开是否存在
            if (len == 7) // 帧长度判断
            {
                reportAirSwitchAliveState(cmd, 0x00);
                DEBUG_TRACE(LOG_TAG, "Report Air Switch Alive State");
            }
            break;
        case 0x09:        // 控制多个空开通断
            if (len == 9) // 帧长度判断
            {
                u8 flag = 0;
                u8 action;
                u16 state = ((u16)data[4] << 8) | data[5]; // 要操作的空开位设定状态

                for (u8 j = 0; j < DEFAULT_AS_NUMBER; j++) {
                    if (as_an001_t.is_alive[j] && as_an001_t.is_remote_control[j]) {
                        if ((state << j) & 0x8000)
                            action = 1;
                        else
                            action = 0;
                        as_write_cmd(CMD_AS_SET_CONTACT, j, action); // 设置此空开状态
                        flag = 1;
                    }
                }
                if (flag) // 批量操作至少有一个操作成功都应答，如果批量操作的空开都不在线，则不应答
                {
                    as_get_slave_info(READ_AS_CONTACT_STATUS);
                    ACK_Server1(cmd, addr); // 应答服务器(应答包中含有执行结果)
                }
            }
            break;
        case 0x0A:        // 批量查询空开分合闸状态及远程状态
            if (len == 7) // 帧长度判断
            {
                as_get_slave_info(READ_AS_CONTACT_STATUS);
                as_get_slave_info(READ_AS_IS_REMOTE_CONTROL);
                reportAirSwitchRemoteState(cmd, addr);
                DEBUG_TRACE(LOG_TAG, "Report Air Switch Remote Control State");
            }
            break;
        case 0x0B:        // 批量查询空开分合闸状态及远程状态
            if (len == 7) // 帧长度判断
            {
                u8 flag = 0;
                for (u8 j = 0; j < DEFAULT_AS_NUMBER; j++) {
                    if (as_an001_t.is_alive[j] && as_an001_t.open_state[j]) {
                        as_write_cmd(CMD_AS_LEAKAGE_PROTECTION, j, 0);
                        flag = 1;
                    }
                }
                ACK_Server2(cmd, 0, flag);
                DEBUG_TRACE(LOG_TAG, "Exec Air Switch Leakage Protection %d", flag);
            }
            break;
        case 0xEE:       // 透传指令
            if (len > 7) // 帧长度判断
            {
                AS_SendData(data + 2, len - 5); // 去掉帧头、命令码、地址、数据长度、CRC和帧尾剩余的部分透传给AS
                AS_SeriaNet = 1;
            }
            break;
        default:
            break;
        }
    }
    // else if (data[0] == 0x54 && data[1] == 0x53 && data[len - 1] == 0x45) // 调试使用透传指令
    // {
    //     AS_SendData(data + 2, len - 3);
    //     AS_SeriaNet = 1;
    // }
    else {
        DEBUG_TRACE(WARN_TAG, "Recv LoRaWAN Data Crc Error");
    }
}

/**
 * @brief  处理数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromASDataHandle(u8 *data, u16 len)
{
    if (AS_SeriaNet && (len < 100 - 3)) {
        AS_SeriaNet = 0;
        memset(sendBuffer, 0, sizeof(sendBuffer)); // 将接收到的数据写入缓冲区
        // memcpy(sendBuffer, data, len);
        // sendNum = len;

        u8 index = 0;
        sendBuffer[index++] = 0xAA; // 帧头
        sendBuffer[index++] = 0xEE; // 命令码
        memcpy(&sendBuffer[2], data, len);
        index += len;
        u16 crc = crc_cal_value(sendBuffer, index); // 求crc
        sendBuffer[index++] = crc & 0xFF;           // CRC低字节
        sendBuffer[index++] = (crc >> 8) & 0xFF;    // CRC高字节
        sendBuffer[index++] = 0x55;
        sendNum = index;

        // 检查剩余空间，不足时丢弃最旧数据
        while (lwrb_get_free(&lwrbBuff_t) < (size_t)(1 + sendNum)) {
            u8 old_len;
            if (lwrb_read(&lwrbBuff_t, &old_len, 1) == 1) {
                lwrb_skip(&lwrbBuff_t, old_len);
            } else {
                lwrb_reset(&lwrbBuff_t);
                break;
            }
            if (lwrb_write_count > 0)
                lwrb_write_count--;
        }

        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    }
}

/**
 * @brief  处理来自debug口的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromInfoDataHandle(u8 *data, u16 len)
{
    // AS_SendCMD((const char *)data);
    // AS_SendCMD("\r\n");
#if 0
    AS_SendData(data, len);
#else
    memset(sendBuffer, 0, sizeof(sendBuffer)); // 将接收到的数据写入缓冲区
    memcpy(sendBuffer, data, len);
    sendNum = len;

    /* 检查剩余空间，不足时丢弃最旧数据 */
    while (lwrb_get_free(&lwrbBuff_t) < (size_t)(1 + sendNum)) {
        u8 old_len;
        if (lwrb_read(&lwrbBuff_t, &old_len, 1) == 1) {
            lwrb_skip(&lwrbBuff_t, old_len);
        } else {
            lwrb_reset(&lwrbBuff_t);
            break;
        }
        if (lwrb_write_count > 0)
            lwrb_write_count--;
    }
    lwrb_write_count++;
    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
#endif
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromUartDataHandle(void)
{
    if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
        DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

#if FUNC_BOARD_VERSION_CL
        /*
         * CL 版本: LoRaWAN 模组下行数据为 AT 字符串格式.
         * 仅处理 +radiorx / +macrx 下行数据帧, 其他 AT 响应
         * (如 OK, +JOIN 等) 已由 AT 命令层阻塞式处理.
         */
        if (strstr((const char *)uartLoRa.recvData, "+radiorx") ||
            strstr((const char *)uartLoRa.recvData, "+macrx")) {
            RadioRxData rx_data;
            parse_radiorx((const char *)uartLoRa.recvData, &rx_data);
            if (rx_data.parse_ok) {
                DEBUG_TRACE(LOG_TAG, "LoRa RX: RSSI=%d, SNR=%d, Port=%d, Len=%d",
                            rx_data.rssi, rx_data.snr, rx_data.port, rx_data.raw_len);
                fromLoRaWANDataHandle(rx_data.raw_data, rx_data.raw_len);
            }
        }
#else
        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
#endif
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartAS.recvTimeout && uartAS.recvLen) {
        DebugHexInfo("AS", uartAS.recvData, uartAS.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart AS Data: %s", uartAS.recvData);

        fromASDataHandle(uartAS.recvData, uartAS.recvLen);
        memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
        uartAS.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        DebugHexInfo("Info", uartInfo.recvData, uartInfo.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart Info Data: %s", uartInfo.recvData);

        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{
    if (LoRaWAN.joinState && !uartLoRa.sendDelay) {
        /* 检测缓冲器是否存在待发射数据 */
        if (lwrb_write_count) // 缓冲区内有数据
        {
            uartLoRa.sendLen = 0;
            memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
            memset(sendBuffer, 0, sizeof(sendBuffer));
            u8 *dataHex = sendBuffer;

            lwrb_read(&lwrbBuff_t, &sendNum, 1);
            lwrb_read(&lwrbBuff_t, dataHex, sendNum);
            lwrb_write_count--;
            DEBUG_TRACE(LOG_TAG, "Lwrb Write Count %d", lwrb_write_count);

            sendLoRaWANData();

            if (lwrb_write_count == 0) {
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }
        }
    }
}

/**
 * @brief  环形缓冲区事件打印函数
 * @param
 * @retval
 */
void my_buff_evt_fn(lwrb_t *buff, lwrb_evt_type_t type, size_t len)
{
#if HIGH_LEVEL_DEBUG_ENABLE
    switch (type) {
    case LWRB_EVT_RESET:
        DEBUG_TRACE(LWRB_TAG, "[EVT] Buffer reset event!");
        break;
    case LWRB_EVT_READ:
        DEBUG_TRACE(LWRB_TAG, "[EVT] Buffer read event: %d byte(s)!", (int)len);
        break;
    case LWRB_EVT_WRITE:
        DEBUG_TRACE(LWRB_TAG, "[EVT] Buffer write event: %d byte(s)!", (int)len);
        break;
    default:
        break;
    }
#endif
}

/**
 * @brief  环形缓冲区数据初始化
 * @param
 * @retval
 */
void lwrbDataInit(void)
{
    lwrb_init(&lwrbBuff_t, lwrb_data, sizeof(lwrb_data));
    lwrb_set_evt_fn(&lwrbBuff_t, my_buff_evt_fn);
}
