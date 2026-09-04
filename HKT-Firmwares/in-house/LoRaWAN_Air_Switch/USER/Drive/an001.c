#include "an001.h"
#include "communicate.h"
#include "systick.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>

// 空开从机参数地址 功能码 04
const static u8 as_addr[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                             0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
                             0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C};
const static u8 as_addr_2[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                               0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13,
                               0x14, 0x15, 0x16, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
                               0x37, 0x38};

const static u8 as_enter_addr_mode_buf[] = {0x01, 0x06, 0x01, 0xF0, 0xFF, 0x00, 0xC9, 0xF5};
const static u8 as_out_addr_mode_buf[] = {0x01, 0x06, 0x01, 0xF0, 0x00, 0x00, 0xC9, 0xF5};

struct as_an001 as_an001_t;

/*
*********************************************************************************************************
*	函 数 名: u16 crc_cal_value(u8 *data_value,u8 data_length)
*	功能说明: CRC校验码
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
u16 crc_cal_value(u8 *data_value, u16 data_length)
{
    int i;
    u16 crc_value = 0xFFFF;

    while (data_length--) {
        crc_value ^= *data_value++;
        for (i = 0; i < 8; i++) {
            if (crc_value & 0x0001)
                crc_value = (crc_value >> 1) ^ 0xA001;
            else
                crc_value = crc_value >> 1;
        }
    }

    return (crc_value);
}

/*
功能：对指定buf长度的数据进行校验并返回检验结果
输入：缓冲区首地址 校验长度
返回：SUCCESS-校验通过
      ERROR  -校验不通过
*/
#if !FUNC_BOARD_VERSION_CL
ErrStatus as_check_crc16(u8 *buf, u16 len)
#else
ErrorStatus as_check_crc16(u8 *buf, u16 len)
#endif
{
    u16 crc1, crc2;
    if (len < 3)
        return ERROR;
    crc1 = crc_cal_value(buf, len - 3); // 不算最后3个字节，重新计算CRC
    crc2 = ((u16)buf[len - 2] << 8) | ((u16)buf[len - 3]);
    return ((crc1 == crc2) ? SUCCESS : ERROR);
}

// 获取空开数量
u8 as_check_slave_number(void)
{
    u8 bufer[8] = {0x01, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00};
    u32 startTimer, value = 0;
    u16 crc;
    u8 number = 0, i, bit_16_h, bit_16_l;

    crc = crc_cal_value(bufer, 6);
    bufer[6] = (u8)crc;        // CRC低字节
    bufer[7] = (u8)(crc >> 8); // CRC高字节

    AS_SendData(bufer, 8);
    startTimer = get_syspant_ms() + 500;
    while (1) {
        if (!uartAS.recvTimeout && uartAS.recvLen) {
#if HIGH_LEVEL_DEBUG_ENABLE
            DebugHexInfo("AS", uartAS.recvData, uartAS.recvLen);
#endif
            if (uartAS.recvData[1] != 0x01)
                return 0;
            if (SUCCESS == as_check_crc16(uartAS.recvData, uartAS.recvLen + 1)) {
                u8 data_len = uartAS.recvData[2]; // 获取空开实际数据位长度
                if (data_len > 4)                 // 限制最大位数为4bytes
                    data_len = 4;
                for (i = 0; i < data_len; i++) {
                    value = value << 8;
                    value |= uartAS.recvData[3 + i];
                }

                for (i = 0; i < data_len * 8; i++) // 实际空开地址排列
                {
                    if (((value >> i) & 0x01) == 1)
                        number++;
                }

                bit_16_h = (value >> 8) & 0xFF;
                bit_16_l = value & 0xFF;
                DEBUG_TRACE(LOG_TAG, "Air Switch Alive Number 0x%08x,", value);
                for (i = 0; i < DEFAULT_AS_NUMBER; i++) // 软件限制空开地址排列 仅保留16位数据
                {
                    if (i < 8) {
                        if (((bit_16_h >> i) & 0x01) == 1) // 从单个byte的第0位开始
                            as_an001_t.is_alive[i] = 1;
                        else
                            as_an001_t.is_alive[i] = 0;
                    } else {
                        if (((bit_16_l >> (i - 8)) & 0x01) == 1) // 从单个byte的第0位开始
                            as_an001_t.is_alive[i] = 1;
                        else
                            as_an001_t.is_alive[i] = 0;
                    }
                    INFO(" %d", as_an001_t.is_alive[i]);
                }
                memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
                uartAS.recvLen = 0;
                return number;
            } else {
                AS_SendData(bufer, 8);
                startTimer = get_syspant_ms() + 500;
            }
            memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
            uartAS.recvLen = 0;
        }
        // 等待期间及时处理LoRaWAN下行数据
        if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
            DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
            fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
            memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
            uartLoRa.recvLen = 0;
        }
        if (startTimer <= get_syspant_ms()) // 超时未回应直接跳出
        {
            DEBUG_TRACE(ERROR_TAG, "Air Switch No Reply %s", __func__);
            break;
        }
				fwdgt_reload();
    }
    return number;
}

// 获取设备对应信息
void as_get_slave_info(as_event_t event)
{
    u8 bufer[8] = {0x01, 0x01, 0x00, 0x00, 0x00, DEFAULT_AS_NUMBER, 0x00, 0x00};
    u16 crc, value = 0;
    u32 startTimer;
    int j = 0, p = 0, i, addr_size;
    u8 bit_16_h = 0, bit_16_l = 0;

    if (event == READ_AS_SLAVE_PARAM || event == READ_AS_REAL_TIME_DATA) {
        if (event == READ_AS_SLAVE_PARAM) {
            addr_size = sizeof(as_addr_2) / sizeof(as_addr_2[0]);
            DEBUG_TRACE(LOG_TAG, "Start Read AS Slave Param");
        } else {
            addr_size = sizeof(as_addr) / sizeof(as_addr[0]);
            DEBUG_TRACE(LOG_TAG, "Start Read AS Real Time Data");
        }
        for (p = 0; p < addr_size; p++) {
            bufer[1] = event;
            if (event == READ_AS_SLAVE_PARAM) {
                bufer[2] = as_addr_2[p]; // 参数的地址
            } else if (event == READ_AS_REAL_TIME_DATA) {
                bufer[2] = as_addr[p]; // 参数的地址
            }
            crc = crc_cal_value(bufer, 6);
            bufer[6] = (u8)crc;        // CRC低字节
            bufer[7] = (u8)(crc >> 8); // CRC高字节

            memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
            uartAS.recvLen = 0;
            AS_SendData(bufer, 8);
            startTimer = get_syspant_ms() + 500; // 命令超时时间500ms
            while (1) {
                if (!uartAS.recvTimeout && uartAS.recvLen) {
#if HIGH_LEVEL_DEBUG_ENABLE
                    DebugHexInfo("AS", uartAS.recvData, uartAS.recvLen);
#endif
                    if (SUCCESS == as_check_crc16(uartAS.recvData, uartAS.recvLen + 1)) {
                        u16 *ptr;
                        for (j = 0; j < DEFAULT_AS_NUMBER; j++) {
                            if (event == READ_AS_SLAVE_PARAM) {
                                ptr = &as_an001_t.slave_param[j].max_voltage + p;
                                *ptr = ((u16)uartAS.recvData[3 + 2 * j] << 8) | uartAS.recvData[4 + 2 * j];
                            } else {
                                ptr = &as_an001_t.slave_realtime_data[j].total_voltage + p;
                                *ptr = ((u16)uartAS.recvData[3 + 2 * j] << 8) | uartAS.recvData[4 + 2 * j];
                            }
                        }
                    } else {
                        // DEBUG_TRACE(ERROR_TAG, "Air Switch Check CRC ERROR[%d][%d]", event, as_addr[p]);
                        p--;
                    }

                    memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
                    uartAS.recvLen = 0;
                    break;
                }
                // 等待期间及时处理LoRaWAN下行数据
                if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
                    DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
                    fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
                    memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
                    uartLoRa.recvLen = 0;
                }
                if (startTimer <= get_syspant_ms()) // 超时未回应直接跳出
                {
                    // DEBUG_TRACE(ERROR_TAG, "Air Switch No Reply[%d][%2x]", READ_AS_SLAVE_PARAM, as_addr[p]);
                    break;
                }
								fwdgt_reload();
            }
        }
        if (event == READ_AS_SLAVE_PARAM) {
            DEBUG_TRACE(LOG_TAG, "End Read AS Slave Param");
        } else {
            DEBUG_TRACE(LOG_TAG, "End Read AS Real Time Data");
        }
    } else {

        bufer[1] = event;
        crc = crc_cal_value(bufer, 6);
        bufer[6] = (u8)crc;        // CRC低字节
        bufer[7] = (u8)(crc >> 8); // CRC高字节

        memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
        uartAS.recvLen = 0;

        AS_SendData(bufer, 8);
        startTimer = get_syspant_ms() + 500; // 命令超时时间500ms

        while (1) {
            if (!uartAS.recvTimeout && uartAS.recvLen) {
#if HIGH_LEVEL_DEBUG_ENABLE
                DebugHexInfo("AS", uartAS.recvData, uartAS.recvLen);
#endif
                u8 data_len;
                if (SUCCESS == as_check_crc16(uartAS.recvData, uartAS.recvLen + 1)) {
                    switch (event) {
                    case READ_AS_CONTACT_STATUS:
                        data_len = uartAS.recvData[2]; // 获取空开实际数据位长度
                        if (data_len > 4)              // 限制最大位数为4bytes
                            data_len = 4;
                        for (i = 0; i < data_len; i++) {
                            value = value << 8;
                            value |= uartAS.recvData[3 + i];
                        }
                        bit_16_h = (value >> 8) & 0xFF;
                        bit_16_l = value & 0xFF;
                        DEBUG_TRACE(LOG_TAG, "Air Switch Open State");
                        for (i = 0; i < DEFAULT_AS_NUMBER; i++) // 软件限制空开地址排列 仅保留16位数据
                        {
                            if (i < 8) {
                                if (((bit_16_h >> i) & 0x01) == 1) // 从单个byte的第0位开始
                                    as_an001_t.open_state[i] = 1;
                                else
                                    as_an001_t.open_state[i] = 0;
                            } else {
                                if (((bit_16_l >> (i - 8)) & 0x01) == 1) // 从单个byte的第0位开始
                                    as_an001_t.open_state[i] = 1;
                                else
                                    as_an001_t.open_state[i] = 0;
                            }
                            INFO(" %d", as_an001_t.open_state[i]);
                        }
                        break;
                    case READ_AS_IS_REMOTE_CONTROL:
                        data_len = uartAS.recvData[2]; // 获取空开实际数据位长度
                        if (data_len > 4)              // 限制最大位数为4bytes
                            data_len = 4;
                        for (j = 0; j < data_len; j++) {
                            value = value << 8;
                            value |= uartAS.recvData[3 + j];
                        }
                        bit_16_h = (value >> 8) & 0xFF;
                        bit_16_l = value & 0xFF;
                        DEBUG_TRACE(LOG_TAG, "Air Switch Remote Control State");
                        for (i = 0; i < DEFAULT_AS_NUMBER; i++) // 软件限制空开地址排列 仅保留16位数据
                        {
                            if (i < 8) {
                                if (((bit_16_h >> i) & 0x01) == 0 && as_an001_t.is_alive[i]) // 从单个byte的第0位开始
                                    as_an001_t.is_remote_control[i] = 1;
                                else
                                    as_an001_t.is_remote_control[i] = 0;
                            } else {
                                if (((bit_16_l >> (i - 8)) & 0x01) == 0 && as_an001_t.is_alive[i]) // 从单个byte的第0位开始
                                    as_an001_t.is_remote_control[i] = 1;
                                else
                                    as_an001_t.is_remote_control[i] = 0;
                            }
                            INFO(" %d", as_an001_t.is_remote_control[i]);
                        }
                        break;
                    default:
                        break;
                    }
                    memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
                    uartAS.recvLen = 0;
                    break;
                } else {
                    AS_SendData(bufer, 8);
                    startTimer = get_syspant_ms() + 500;
                }
                memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
                uartAS.recvLen = 0;
            }
            // 等待期间及时处理LoRaWAN下行数据
            if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
                DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
                fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
                memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
                uartLoRa.recvLen = 0;
            }
            if (startTimer <= get_syspant_ms()) // 超时未回应直接跳出
            {
                DEBUG_TRACE(ERROR_TAG, "Air Switch No Reply");
                break;
            }
						fwdgt_reload();
        }
    }
}

void as_period_upload(void)
{
    int j = 0;
    static u16 alarm_state[DEFAULT_AS_NUMBER] = {0};
    static u8 open_state[DEFAULT_AS_NUMBER] = {0};
    static u8 is_remote_control[DEFAULT_AS_NUMBER] = {0};

    if (!as_an001_t.period_refresh_real_time) {
        as_an001_t.period_refresh_real_time = 30;     // 30秒刷新一次实时数据
        as_get_slave_info(READ_AS_REAL_TIME_DATA);    // 读取空开实时数据
        as_get_slave_info(READ_AS_CONTACT_STATUS);    // 读取空开地址状态
        as_get_slave_info(READ_AS_IS_REMOTE_CONTROL); // 读取空开远程使能状态
        // 确认是否存在报警事件
        for (j = 0; j < DEFAULT_AS_NUMBER; j++) {
            if (as_an001_t.is_alive[j] && as_an001_t.slave_realtime_data[j].alarm_t.alarm && as_an001_t.slave_realtime_data[j].alarm_t.alarm != alarm_state[j]) {
                as_an001_t.period_upload_real_timeout = 0; // 触发报警立马上报实时数据
                as_an001_t.change_mask |= (1 << j);        // 标记变化空开
                DEBUG_TRACE(WARN_TAG, "AS[%d] Alarm, Code=0x%04x", j, as_an001_t.slave_realtime_data[j].alarm_t.alarm)
            }
            alarm_state[j] = as_an001_t.slave_realtime_data[j].alarm_t.alarm;
        }

        // 确认是否开合闸状态变更
        for (j = 0; j < DEFAULT_AS_NUMBER; j++) {
            if (as_an001_t.is_alive[j] && as_an001_t.open_state[j] != open_state[j]) {
                as_an001_t.period_upload_real_timeout = 0; // 触发立马上报实时数据
                as_an001_t.change_mask |= (1 << j);        // 标记变化空开
                DEBUG_TRACE(WARN_TAG, "AS[%d] Open State Changed %d", j, as_an001_t.open_state[j])
            }
            open_state[j] = as_an001_t.open_state[j];
        }

        // 确认是否远程控制状态变更
        for (j = 0; j < DEFAULT_AS_NUMBER; j++) {
            if (as_an001_t.is_alive[j] && as_an001_t.is_remote_control[j] != is_remote_control[j]) {
                as_an001_t.period_upload_real_timeout = 0; // 触发立马上报实时数据
                as_an001_t.change_mask |= (1 << j);        // 标记变化空开
                DEBUG_TRACE(WARN_TAG, "AS[%d] Remote Control State Changed %d", j, as_an001_t.is_remote_control[j])
            }
            is_remote_control[j] = as_an001_t.is_remote_control[j];
        }
    }

    if (!as_an001_t.period_upload_real_timeout && LoRaWAN.joinState == 1 && as_an001_t.period_upload_real_time) {
        as_an001_t.period_upload_real_timeout = as_an001_t.period_upload_real_time;

        u16 upload_mask;
        u8 is_event_upload = (as_an001_t.change_mask != 0);
        if (is_event_upload) {
            upload_mask = as_an001_t.change_mask;
            as_an001_t.change_mask = 0; // 清除掩码
        }

        for (j = 0; j < DEFAULT_AS_NUMBER; j++) {
            // 事件触发：仅上传掩码中标记的空开；周期触发：上传所有在线空开
            if (is_event_upload) {
                if (!(upload_mask & (1 << j)))
                    continue;
            } else {
                if (!as_an001_t.is_alive[j])
                    continue;
            }
                u16 *ptr = &as_an001_t.slave_realtime_data[j].total_voltage;
                u16 value = 0;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendNum = 0;

                sendBuffer[sendNum++] = 0xAA;
                sendBuffer[sendNum++] = REPORT_REAL_TIME_DATA_CMD;              // CMD
                sendBuffer[sendNum++] = j;                                      // 设备编号
                sendBuffer[sendNum++] = as_an001_t.slave_param[j].version >> 8; // 设备型号
                sendBuffer[sendNum++] = as_an001_t.slave_param[j].item & 0xFF;  // 空开类型
                sendBuffer[sendNum++] = 59;                                     // 数据长度=29*2+1=59字节

                for (u8 p = 0; p < 29; p++) // 将29个u16类型的实时数据拷贝到填充帧
                {
                    value = *(ptr + p);
                    sendBuffer[sendNum++] = (u8)(value >> 8); // 高字节
                    sendBuffer[sendNum++] = (u8)value;        // 低字节
                }
                sendBuffer[sendNum++] = as_an001_t.is_remote_control[j]; // 能否被远程控制

                value = crc_cal_value(sendBuffer, sendNum); // 求crc
                sendBuffer[sendNum++] = (u8)value;          // CRC低字节
                sendBuffer[sendNum++] = (u8)(value >> 8);   // CRC高字节
                sendBuffer[sendNum++] = 0x55;

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
                DEBUG_TRACE(LOG_TAG, "Lwrb Write Count %d", lwrb_write_count);
                lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        }
    }
}

void as_write_cmd(u8 event, u8 addr, u8 cmd)
{
    u8 bufer[8] = {0x01, 0x05, 0x00, 0x00, 0xff, 0x00, 0x8c, 0x3a};
    u32 startTimer;
    u16 crc = 0;
    u8 f_num = 0;

    switch (event) {
    case CMD_AS_ENTER_ADDR_MODE:
        memcpy(bufer, as_enter_addr_mode_buf, 8);
        break;
    case CMD_AS_OUT_ADDR_MODE:
        memcpy(bufer, as_out_addr_mode_buf, 8);
        break;
    case CMD_AS_SET_CONTACT:
        bufer[1] = 0x05;
        bufer[3] = addr;
        if (cmd)
            bufer[4] = 0xff;
        else
            bufer[4] = 0x00;
        break;
    case CMD_AS_RESET_FACTORY_MODE_2:
        bufer[1] = 0x06;
        bufer[2] = 0x49;
        bufer[3] = f_num;
        break;
    case CMD_AS_LEAKAGE_PROTECTION:
        bufer[1] = 0x06;
        bufer[2] = 0x08;
        bufer[3] = addr;
        bufer[4] = 0;
        bufer[5] = 0x5A;
        break;
    default:
        break;
    }
send:
    memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
    uartAS.recvLen = 0;

    crc = crc_cal_value(bufer, 6);
    bufer[6] = (u8)crc;        // CRC低字节
    bufer[7] = (u8)(crc >> 8); // CRC高字节
    AS_SendData(bufer, 8);
    startTimer = get_syspant_ms() + 500;
		
		memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
		uartLoRa.recvLen = 0;

    while (1) {
        if (!uartAS.recvTimeout && uartAS.recvLen) {
#if HIGH_LEVEL_DEBUG_ENABLE
            DebugHexInfo("AS", uartAS.recvData, uartAS.recvLen);
#endif
            if (uartAS.recvData[0] != 0x01)
                return;
            if (SUCCESS == as_check_crc16(uartAS.recvData, uartAS.recvLen + 1)) {
                switch (event) {
                case CMD_AS_ENTER_ADDR_MODE:
                    as_an001_t.run_mode = 1;
                    as_an001_t.addr_mode_timeout = 0;
                    memset(as_an001_t.is_remote_control, 0, sizeof(as_an001_t.is_remote_control)); // 清除远程使能位
                    DEBUG_TRACE(LOG_TAG, "AS Enter Addr Mode Success");
                    return;
                case CMD_AS_OUT_ADDR_MODE:
                    as_an001_t.run_mode = 0;
                    as_an001_t.addr_mode_timeout = 0;
                    DEBUG_TRACE(LOG_TAG, "AS Out Addr Mode Success");
                    return;
                case CMD_AS_SET_CONTACT:
                    DEBUG_TRACE(LOG_TAG, "AS Set Contact %s Success %d", cmd ? "Open" : "Off", addr);
                    return;
                case CMD_AS_RESET_FACTORY_MODE_2:
                    DEBUG_TRACE(LOG_TAG, "AS Reset Factory Mode Success %d", f_num);
                    if (++f_num >= DEFAULT_AS_NUMBER)
                        return;
                    bufer[3] = f_num;
                    goto send;
                case CMD_AS_LEAKAGE_PROTECTION:
                    DEBUG_TRACE(LOG_TAG, "AS Leakage Protection %d Success", addr);
                    return;
                default:
                    break;
                }
            } else {
                AS_SendData(bufer, 8);
                startTimer = get_syspant_ms() + 500;
            }
            memset(uartAS.recvData, 0, sizeof(uartAS.recvData));
            uartAS.recvLen = 0;
        }
        // 等待期间及时处理LoRaWAN下行数据
        if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
            DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
            fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
            memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
            uartLoRa.recvLen = 0;
        }
        if (startTimer <= get_syspant_ms()) // 超时未回应直接跳出
        {
            switch (event) {
            case CMD_AS_ENTER_ADDR_MODE:
                DEBUG_TRACE(LOG_TAG, "AS Enter Addr Mode Timeout")
                break;
            case CMD_AS_OUT_ADDR_MODE:
                DEBUG_TRACE(LOG_TAG, "AS Out Addr Mode Timeout")
                break;
            case CMD_AS_SET_CONTACT:
                DEBUG_TRACE(LOG_TAG, "AS Set Contact Timeout");
                break;
            default:
                break;
            }
            return;
        }
				fwdgt_reload();
    }
}

void as_event(void)
{
    static u32 start_timer;
    static u8 cnt;

    if (as_an001_t.run_mode == 1) {
        if (start_timer < get_syspant_ms()) {
            start_timer = get_syspant_ms() + 1000;
            if (++as_an001_t.addr_mode_timeout >= 60) {
                as_an001_t.addr_mode_timeout = 0;
                as_an001_t.run_mode = 0;
                as_write_cmd(CMD_AS_OUT_ADDR_MODE, 0, 0);
            }
        }
    }

    if (!as_an001_t.number) {
        if (start_timer < get_syspant_ms()) {
            start_timer = get_syspant_ms() + 3000; // 3S 获取一次状态
            u8 number = as_check_slave_number();
            if (number > 0) {
                as_an001_t.number = number;
                DEBUG_TRACE(LOG_TAG, "Get Air Switch Number: %d", as_an001_t.number);

                as_get_slave_info(READ_AS_CONTACT_STATUS);    // 读取空开地址状态
                as_get_slave_info(READ_AS_IS_REMOTE_CONTROL); // 读取空开远程使能状态
                as_get_slave_info(READ_AS_REAL_TIME_DATA);    // 读取空开实时数据
                as_get_slave_info(READ_AS_SLAVE_PARAM);       // 读取所有空开的配置参数
                cnt = 0;
            } else {
                cnt++;
                DEBUG_TRACE(LOG_TAG, "Wait Air Switch Stable: %d", cnt);
            }
        }
    } else {
        as_period_upload();
    }
}

// 01 06 09 F0 BA 00 F9 05  //01 06 02 05 01 79 B7                      //读取模组 版本号
// 01 06 01 F0 FF 00 C9 F5  //01 06 01 F0 FF 00 C9 F5                   //设置空开 地址
// 01 06 01 F0 00 00 88 05  //01 06 01 F0 00 00 88 05                   //退出地址 设置
// 01 06 03 F0 BA 03 BA DC                                              //波特率修 改
// 01 03 00 00 00 04 44 09  //01 03 08 00 E1 00 41 00 81 00 E1 C8 B6    //读取总电压
// 01 03 01 00 00 04 45 F5  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //读取漏电流
// 01 03 02 00 00 04 45 B1  //01 03 08 00 EA 00 73 00 73 00 E0 2B 81    //读取功率
// 01 03 03 00 00 04 44 4D  //01 03 08 01 1F 01 0E 01 04 01 08 92 A1    //读取模块温度
// 01 03 04 00 00 04 45 39  //01 03 08 00 68 00 21 00 22 00 66 E0 36    //读取电流
// 01 03 05 00 00 04 44 C5  //01 03 08 00 00 20 00 20 00 00 00 99 77    //读取告警位
// 01 03 06 00 00 04 44 81  //01 03 08 00 B0 00 56 00 55 00 A7 3D BA    //读取电量
// 01 03 07 00 00 04 45 7D  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //电量高没有数据
// 01 03 08 00 00 04 46 69  //01 03 08 00 00 00 72 00 72 00 00 0D C7    //A 相电压
// 01 03 09 00 00 04 47 95  //01 03 08 00 00 00 62 00 62 00 00 6C 1F    //B 相电压
// 01 03 0A 00 00 04 47 D1  //01 03 08 00 00 00 61 00 84 00 00 68 36    //C 相电压
// 01 03 0B 00 00 04 46 2D  //01 03 08 00 00 00 65 00 66 00 00 39 C0    //A 相电流
// 01 03 0C 00 00 04 47 59  //01 03 08 00 00 00 63 00 65 00 00 41 C0    //B 相电流
// 01 03 0D 00 00 04 46 A5  //01 03 08 00 00 00 63 00 66 00 00 B1 C0    //C 相电流
// 01 03 0E 00 00 04 46 E1  //01 03 08 00 00 00 65 00 00 00 00 D9 DF    //N 相电流
// 01 03 0F 00 00 04 47 1D  //01 03 08 00 00 00 55 00 55 00 00 89 CB    //A 相功率
// 01 03 10 00 00 04 40 C9  //01 03 08 00 00 00 53 00 54 00 00 50 0B    //B 相功率
// 01 03 11 00 00 04 41 35  //01 03 08 00 00 00 54 00 55 00 00 B4 0B    //C 相功率
// 01 03 12 00 00 04 41 71  //01 03 08 00 00 28 00 20 00 00 00 98 3F    //A 相告警
// 01 03 13 00 00 04 40 8D  //01 03 08 00 00 28 00 20 00 00 00 98 3F    //B 相告警
// 01 03 14 00 00 04 41 F9  //01 03 08 00 00 28 00 20 00 00 00 98 3F    //C 相告警
// 01 03 15 00 00 04 40 05  //01 03 08 5A 00 A5 0E A5 0E 5A 00 18 0F    //分合闸状态
// 01 03 16 00 00 04 40 41  //01 03 08 00 00 7F FF 7F FF 7F FF C3 98    //A相功率因数
// 01 03 17 00 00 04 41 BD  //01 03 08 00 00 7F FF 7F FF 00 00 A3 E8    //B相功率因数
// 01 03 18 00 00 04 42 A9  //01 03 08 00 00 7F FF 7F FF 00 00 A3 E8    //C相功率因数
// 01 03 19 00 00 04 43 55  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //端子温度 A
// 01 03 1A 00 00 04 43 11  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //端子温度 B
// 01 03 1B 00 00 04 42 ED  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //端子温度 C
// 01 03 1C 00 00 04 43 99  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //端子温度 N
// 01 03 30 00 00 04 4B 09  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //漏电电流
// 01 03 31 00 00 04 4A F5  //01 03 08 00 00 28 00 20 00 00 00 98 3F    //总告警位
// 01 03 32 00 00 04 4A B1  //01 03 08 00 00 00 A8 00 4C 00 00 35 D8    //读取带值电压
// 01 03 33 00 00 04 4B 4D  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //读取带值电流
// 01 03 34 00 00 04 4A 39  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //读取带值功率
// 01 03 35 00 00 04 4B C5  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //读取带值温度
// 01 03 36 00 00 04 4B 81  //01 03 08 00 00 28 00 20 00 00 00 98 3F    //读取带值A告警
// 01 03 37 00 00 04 4A 7D  //01 03 08 00 00 00 62 00 00 00 00 6C 1F    //A 电压
// 01 03 38 00 00 04 49 69  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //A 电流
// 01 03 39 00 00 04 48 95  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //A 功率
// 01 03 3A 00 00 04 48 D1  //01 03 08 00 00 01 2C 01 2C 00 00 C5 F5    //A 温度
// 01 03 3B 00 00 04 49 2D  //01 03 08 00 00 28 00 20 00 00 00 98 3F    //B 告警
// 01 03 3C 00 00 04 48 59  //01 03 08 00 00 00 61 00 00 00 00 28 1F    //B 电压
// 01 03 3D 00 00 04 49 A5  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //B 电流
// 01 03 3E 00 00 04 49 E1  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //B 功率
// 01 03 3F 00 00 04 48 1D  //01 03 08 00 00 01 2C 01 2C 00 00 C5 F5    //B 温度
// 01 03 40 00 00 04 51 C9  //01 03 08 00 00 28 00 20 00 00 00 98 3F    //C 告警
// 01 03 41 00 00 04 50 35  //01 03 08 00 00 00 61 00 84 00 00 68 36    //C 电压
// 01 03 42 00 00 04 50 71  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //C 电流
// 01 03 43 00 00 04 51 8D  //01 03 08 00 00 00 00 00 00 00 00 95 D7    //C 功率
// 01 03 44 00 00 04 50 F9  //01 03 08 00 00 01 2C 01 2C 00 00 C5 F5    //C 温度
// 01 04 00 00 00 04 F1 C9  //01 04 08 01 04 01 02 01 08 01 04 58 7D    //电压上限
// 01 04 01 00 00 04 F0 35  //01 04 08 00 64 00 64 00 64 00 AF 71 A0    //电压下限
// 01 04 02 00 00 04 F0 71  //01 04 08 03 E8 01 2C 01 2C 00 00 5C 34    //功率上限

/*
设置断路器地址
第1字节       帧头AA
第2字节       CMD(00代表断路器地址设置)
第3字节       断路器地址(此字节无意义，默认为00)
第4字节       数据长度(1个字节)
第5字节       数据字节(01-进入地址设置模式 02-退出地址设置模式)
第6、7字节    校验码，低字节在前
第8字节       帧尾55
返回帧：上传应答信号
注：模组接收到此命令会有60秒时间等用户设置地址，如果在60秒内设完地址且发送了退出指令则直接退出地址设置模式，如果60秒后没有接收到退出指令则自动退出地址设置模式，重新设完地址后，必须将断路器的电源模块断电重启，因为地址顺序改变后，原来上电时的阈值参数在内存中的存储状态并未改变，重启后才会重新更新。
示例帧如下：
进入地址设置模式：AA00000101FC4855
退出地址设置模式：AA00000102BC4955


控制指定断路器的开和关
第1字节       帧头AA
第2字节       CMD(03代表动作指定断路器)
第3字节       断路器地址(00~09，00代表第1个断路器，01代表第2个断路器，FF代表全部)
第4字节       数据长度(1个字节)
第5字节       断路器状态(00分闸，01合闸)
第6、7字节    校验码，低字节在前
第8字节       帧尾55
返回帧：上传应答信号(服务器收到应答表示断路器动作已执行)
示例帧如下：
断路器1断闸：  AA030001003DCC55
断路器1合闸：  AA03000101FC0C55
断路器2断闸：  AA030101006C0C55
断路器2合闸：  AA03010101ADCC55
断路器3断闸：  AA030201009C0C55
断路器3合闸：  AA030201015DCC55
断路器4断闸：  AA03030100CDCC55
断路器4合闸：  AA030301010C0C55
断路器5断闸：  AA030401007C0D55
断路器5合闸：  AA03040101BDCD55
断路器6断闸：  AA030501002DCD55
断路器6合闸：  AA03050101EC0D55
所有断路器断闸：AA03FF01000DFC55
所有断路器合闸：AA03FF0101CC3C55
*/
