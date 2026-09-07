
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "cJSON.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "lwrb.h"
#include "real_time.h"
#include "rtc.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

/***********            通讯协议数据结构             ***********/
/*--------------------------------------------------------------
|                                                               |
|   同步头   |   特殊类型   |   包序号   |   数据类型   |   数据   |
|                                                               |
---------------------------------------------------------------*/

const u8 SYNC_HEAD[3] = {0x68, 0x6B, 0x74};

u8 lwrb_write_count;
u8 lwrb_data[lwrb_buffer_size];
lwrb_t lwrbBuff_t;

struct timing_task local_schedule_t;
struct real_time_task real_time_task_t;
time_t dataReportTimestamp;
time_t zeroTimestamp;

u8 sendNum;
u8 sendBuffer[220];

u8 factoryMode;
u8 keepRunFlag;
u8 packSyncNumber;

char tempBuf[500];

/**
 * @brief  快速分割短整型为字符串数组
 * @param  src：源数据
 * @param   hex: 输出字符串数组
 * @retval
 */
void quickSplitShort(u16 src, u8 *hex)
{
    unionDataStruct unionDataValue;
    unionDataValue.unionDataSrc = src;
    hex[0] = unionDataValue.unionData.DataH;
    hex[1] = unionDataValue.unionData.DataL;
}

/**
 * @brief  按规定协议封成数据包
 * @param  type：协议类型
 * @param  dest：接收封包后的数组
 * @retval 整条数据长度
 */
u8 setDataPackage(enum DataType type, u8 *dest)
{
    u8 dataLen = 1;
    /*  数据类型  */
    dest[0] = type;

    switch (type) {
    case DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION: // 设备软硬件版本
        dest[1] = HARDWARE_VER;
        dest[2] = SOFTWARE_VER;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_ADDR_ID: // 设备ID
        memcpy(dest + 1, LoRaWAN.DevEUI + 2, 6);
        dataLen += 6;
        break;
    case DATATYPE_DEVICE_BATTERY_INFO: // 电量信息
        dest[1] = batteryLevel;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SYNC_PERIOD: // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时）
        dest[1] = device_t.report_interval >> 8;
        dest[2] = device_t.report_interval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_STATE:
        dest[1] = device_t.value1_state;
        dest[2] = device_t.insert1_connected;
        dest[3] = device_t.pulse1_count >> 8;
        dest[4] = device_t.pulse1_count & 0xFF;
        dest[5] = device_t.value2_state;
        dest[6] = device_t.insert2_connected;
        dest[7] = device_t.pulse2_count >> 8;
        dest[8] = device_t.pulse2_count & 0xFF;
        dataLen += 8;
        break;
    case DATATYPE_SET_VOL_LEVEL:
        dest[1] = device_t.vol_control_sw;
        dataLen += 1;
        break;
    case DATATYPE_PORT_FUNCTION:
        dest[1] = device_t.port_mode;
        dataLen += 1;
        break;
    case DATATYPE_STABLE_TIME:
        dest[1] = device_t.stable_time;
        dataLen += 1;
        break;
    case DATATYPE_SMART_POWER:
        dest[1] = device_t.power_mode;
        dataLen += 1;
        break;
    case DATATYPE_SET_TIME_ZONE: {
        u8 timezone = 0;
        if (device_t.timezone == 3.5) {
            timezone = 25;
        } else if (device_t.timezone == 5.5) {
            timezone = 26;
        } else if (device_t.timezone < 13) {
            timezone = device_t.timezone;
        } else if (device_t.timezone < 24) {
            timezone = -(device_t.timezone - 12);
        }
        dest[1] = timezone;
        dataLen += 1;
    } break;
    case DATATYPE_DEVICE_POWER: {
        dest[1] = device_t.power_on;
        dataLen += 1;
    } break;
    case DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK: // 设备应答ACK
        dest[1] = 0xFF;
        dataLen += 1;
        break;
    default:
        break;
    }
    return dataLen;
}

ErrorStatus CalCheckSum(uint8_t *buf, int len)
{
    int total = 0;
    uint8_t sum = 0;
    ErrorStatus status = SUCCESS;
    for (int i = 0; i < len - 1; i++) {
        total += buf[i];
    }
    sum = total % 256;
    if (sum != buf[len - 1]) {
        DEBUG_TRACE(WARN_TAG, "cal error sum[%d-%d]", sum, buf[len - 1]);
        status = ERROR;
    }
    return status;
}

uint8_t GetCheckSum(uint8_t *buf, int len)
{
    int total = 0;
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        total += buf[i];
    }
    sum = total % 256;
    return sum;
}

/**
 * @brief  发送填充好的数据给LoRa模块
 * @param
 * @retval
 */
void sendLoRaWANData(void)
{
#if FUNC_LORA_VERSION_LIR
    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
#endif
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
#if FUNC_LORA_VERSION_LIR
    /* 等待模组就绪 */
    while (!getLoRaWANBusyIOLevel())
        ;
#endif

    ADC_BatterySglConvert();

    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
    uartLoRa.sendLen = sendNum + 5;

#if FUNC_LORA_VERSION_LIR
    mDelay(20);
    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
    mDelay(20);
    if (getLoRaWANBusyStat()) {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto __fail;
    }
    if (getLoRaWANSendStat()) {
    __fail:
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        packSyncNumber--;
        device_t.send_fail_cnt++;
        if (device_t.send_fail_cnt >= 5) {
            device_t.send_fail_cnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        return;
    }
    device_t.send_fail_cnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 5 * SEC_DELAY;
    if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
#else
    // 先查询入网状态 再进行数据发送
    AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_getJOIN() == TRUE) {
        AT_LoRaWAN_setATExit();
        mDelay(500);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(500);
    } else {
        AT_LoRaWAN_setATExit();
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        if (packSyncNumber)
            packSyncNumber--;
        device_t.send_fail_cnt++;
        if (device_t.send_fail_cnt >= 3) {
            device_t.send_fail_cnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
            if (device_t.sleep_delay < MAX_WAIT_SLEEP_TIMEOUT)
                device_t.sleep_delay = MAX_WAIT_SLEEP_TIMEOUT;
        }
        return;
    }
    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 6 * SEC_DELAY;
    if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    return true;
#endif
}

/**
 * @brief  校验同步头
 * @param  data：接收到数据的数组
 * @retval  检验状态 1：成功 0：失败
 */
ErrorStatus checkDataSyncHead(u8 *data)
{
    ErrorStatus status = SUCCESS;
    if (memcmp(data, SYNC_HEAD, 3) != 0)
        status = ERROR;
    return status;
}

/**
 * @brief  处理来自LoRaWAN模组的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromLoRaWANDataHandle(u8 *data, u16 len)
{
    /*  确认数据 */
    if (checkDataSyncHead(data) == ERROR) {
        // DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
        // DEBUG_TRACE(ERROR_TAG, "Data Check Sync Head ERROR!");

        getJoinedStatus();
        return;
    }

    if (data[3] & 0x02) { // 通讯启用校验
        if (CalCheckSum(data, len) == ERROR) {
            DEBUG_TRACE(ERROR_TAG, "Data Check Sum ERROR!");
            return;
        }
    }

    // 服务器需要应答
    if ((data[3] & 0x01) == 1) {
        u8 *dataHex = sendBuffer;
        memset(sendBuffer, 0, sizeof(sendBuffer));

        sendNum = setDataPackage(DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK, dataHex);
        dataHex += sendNum;

        memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
        memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
        uartLoRa.sendData[3] = 0;
        uartLoRa.sendData[4] = packSyncNumber++;
        memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
        uartLoRa.sendLen = sendNum + 5;

        ADC_BatterySglConvert();

#if FUNC_LORA_VERSION_LIR
        if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
            setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
#endif
        if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
            setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
#if FUNC_LORA_VERSION_LIR
        /* 等待模组就绪 */
        while (!getLoRaWANBusyIOLevel())
            ;
#endif
        mDelay(20);
#if FUNC_LORA_VERSION_LIR
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(20);
        if (getLoRaWANBusyStat()) {
            DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
            goto __send_fail;
        }
        if (getLoRaWANSendStat()) {
        __send_fail:
            DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
            packSyncNumber--;
            device_t.send_fail_cnt++;
            if (device_t.send_fail_cnt >= 5) {
                device_t.send_fail_cnt = 0;
                LoRaWAN.joinState = 0;
                LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                reconnect_stamp = Timestamp + 30 * 60;
                DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
            }
            return;
        }
        device_t.send_fail_cnt = 0;
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
#else
        AT_LoRaWAN_setATEnter();
        if (AT_LoRaWAN_getJOIN() == TRUE) {
            AT_LoRaWAN_setATExit();
            mDelay(500);
            LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
            DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
            mDelay(500);
        } else {
            AT_LoRaWAN_setATExit();
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

            DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendNum + 1)) {
                lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }
            if (packSyncNumber)
                packSyncNumber--;
            device_t.send_fail_cnt++;
            if (device_t.send_fail_cnt >= 3) {
                device_t.send_fail_cnt = 0;
                LoRaWAN.joinState = 0;
                LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
                if (device_t.sleep_delay < MAX_WAIT_SLEEP_TIMEOUT)
                    device_t.sleep_delay = MAX_WAIT_SLEEP_TIMEOUT;
            }
            return;
        }
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 6 * SEC_DELAY;
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
#endif
    }

    /*  解析数据  */
    u8 dataLen = len - 5;

    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d,", dataLen);
    u8 *srcHex = data + 5;
    while (dataLen) {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : %02x", dataType);
        switch (dataType) {
        case DATATYPE_DEVICE_REST_FACTORY: {
            u8 restFactorySign = srcHex[1];
            if (restFactorySign == 1) {
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                FLASH_Reset_Param();
            }
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(WARN_TAG, "Recv Device Rest Factory CMD : %d", restFactorySign);
        } break;
        case DATATYPE_DEVICE_SYNC_TIME: { // 同步时间
            systime.tm_year = srcHex[1] + 2000;
            systime.tm_mon = srcHex[2];
            systime.tm_mday = srcHex[3];
            systime.tm_hour = srcHex[4];
            systime.tm_min = srcHex[5];
            systime.tm_sec = srcHex[6];
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            set_real_time();
            getTimestamp();
            RTC_TimeSet();
            dataLen -= 7;
            srcHex += 7;
            DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
            local_schedule_init();
            device_t.sync_state = 1;
        } break;
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
                device_t.report_interval = reportInterval;
                FLASH_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.report_interval);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval ERROR: %dmin", device_t.report_interval);
            }
            dataLen -= 3;
            srcHex += 3;
        } break;
        case DATATYPE_SET_LOCAL_SCHEDULE: {
            u8 task_num = srcHex[1];
            u8 *task_index = srcHex + 2;

            // 读取本地任务
            if (dataLen == 1) {
                Device_ReportLocalTaskData();
                return;
            }

            if (task_num > 4 || task_num < 1) {
                DEBUG_TRACE(ERROR_TAG, "Task Num Err[%d]", task_num);
                return;
            }

            for (u8 i = 0; i < task_num; i++) { // 设置本地定时任务
                struct schedule task_t = {0};
                task_t.id = task_index[0];
                task_t.valve = task_index[1];
                task_t.state = task_index[2];
                task_t.pulse_count = task_index[3] << 8 | task_index[4];
                task_t.start_hour = task_index[5];
                task_t.start_min = task_index[6];
                task_t.end_hour = task_index[7];
                task_t.end_min = task_index[8];
                task_t.repeat_duty = task_index[9];
                task_index += 10;

                if (task_t.id > 16 || task_t.id == 0) {
                    DEBUG_TRACE(ERROR_TAG, "Task[%d] ID Err", task_t.id);
                    return;
                }
                if (task_t.valve > 2) {
                    DEBUG_TRACE(ERROR_TAG, "Task[%d] Value Err[%d]", task_t.id, task_t.valve);
                    return;
                }
                if (task_t.state > 1) {
                    DEBUG_TRACE(ERROR_TAG, "Task[%d] State Err[%d]", task_t.id, task_t.state);
                    return;
                }
                if (task_t.start_hour > 23 || task_t.start_min > 59 || task_t.end_hour > 23 || task_t.end_min > 59) {
                    DEBUG_TRACE(ERROR_TAG, "Task[%d] Time Err[%d:%d-%d:%d]", task_t.id, task_t.start_hour, task_t.start_min, task_t.end_hour,
                                task_t.end_min);
                    return;
                }
                if (task_t.repeat_duty > 0x7F || task_t.repeat_duty < 1) {
                    DEBUG_TRACE(ERROR_TAG, "Task[%d] Repeat Err[%d]", task_t.repeat_duty);
                    return;
                }
                task_t.is_vaild = 1;
                memcpy(&local_schedule_t.task_t[task_t.id - 1], &task_t, sizeof(task_t));
            }
            local_schedule_init();
            FLASH_Write_Local_Schedule();

            dataLen -= 3 + (task_num * 10);
            srcHex += 3 + (task_num * 10);
        } break;

        case DATATYPE_DELETE_LOCAL_SCHEDULE: {
            u8 id = srcHex[1];
            if ((id > 16 && id != 0xFF) || id == 0) {
                DEBUG_TRACE(ERROR_TAG, "Delete Task[%d] ID Err", id);
                return;
            }

            // 删除任务时强制关闭当前正在执行的任务
            if (id == 0xFF || local_schedule_t.task_t[id - 1].is_start) {
                local_schedule_init();
            }

            if (id == 0xFF) {
                memset(&local_schedule_t, 0, sizeof(local_schedule_t));
            } else {
                memset(&local_schedule_t.task_t[id - 1], 0, sizeof(struct schedule));
            }
            local_schedule_init();
            FLASH_Write_Local_Schedule();

            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_SET_CLOCK_SCHEDULE: { // 设置实时任务
            // if (local_schedule_t.current->is_start != 1 && !local_schedule_t.current) { // 本地任务执行时无效
            if (local_schedule_t.current->is_start != 1) { // 本地任务执行时无效
                memset(&real_time_task_t, 0, sizeof(real_time_task_t));
                real_time_task_t.valve = srcHex[1];
                real_time_task_t.state = srcHex[2];
                real_time_task_t.time = srcHex[3] << 8 | srcHex[4];
                real_time_task_t.pulse_count = srcHex[5] << 16 | srcHex[6] << 8 | srcHex[7];
                if (real_time_task_t.time > 0)
                    real_time_task_t.task_over_timestamp = Timestamp + real_time_task_t.time;
                else
                    real_time_task_t.task_over_timestamp = 0;
                real_time_task_t.pulse_count_trigger = 0;
                real_time_task_t.is_start = 1;
                INFO("\r\n********************** Start Real Task **********************\r\n");

                // 根据初始状态恢复到原来的状态
                // 根据初始状态执行相反操作
                if (!real_time_task_t.state) {
                    switch (real_time_task_t.valve) {
                    case 1:
                        back_1_control();
                        break;
                    case 2:
                        back_2_control();
                        break;
                    case 0:
                        back_1_control();
                        back_2_control();
                        break;
                    default:
                        break;
                    }
                } else {
                    switch (real_time_task_t.valve) {
                    case 1:
                        forward_1_control();
                        break;
                    case 2:
                        forward_2_control();
                        break;
                    case 0:
                        forward_1_control();
                        forward_2_control();
                        break;
                    default:
                        break;
                    }
                }
            }
            dataLen -= 8;
            srcHex += 8;
        } break;
        case DATATYPE_SET_VOL_LEVEL: {
            u8 level = srcHex[1];
            if (level > 2) {
                DEBUG_TRACE(ERROR_TAG, "Config OutPut Vol Err[%d]", level);
                return;
            }
            device_t.vol_control_sw = level;
            device_t.flash_write_flag = 1;
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_PORT_FUNCTION: {
            u8 port_mode = srcHex[1];
            if ((port_mode & 0x78) > 0) {
                DEBUG_TRACE(ERROR_TAG, "Config Port Function Err[%d]", port_mode);
                return;
            }
            device_t.port_mode = port_mode;
            device_t.flash_write_flag = 1;
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_STABLE_TIME: {
            u8 stable_time = srcHex[1];
            if (stable_time < 1) {
                DEBUG_TRACE(ERROR_TAG, "Config Stable Time Err[%d]", stable_time);
                return;
            }
            device_t.stable_time = stable_time;
            device_t.flash_write_flag = 1;
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_SMART_POWER: {
            u8 power_mode = srcHex[1];
            if (power_mode > 1) {
                DEBUG_TRACE(ERROR_TAG, "Config Stable Time Err[%d]", power_mode);
                return;
            }
            device_t.power_mode = power_mode;
            device_t.flash_write_flag = 1;
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_SET_TIME_ZONE: {
            u8 timezone = srcHex[1];
            if (timezone > 26) {
                DEBUG_TRACE(ERROR_TAG, "Config Time Zone Err[%d]", timezone);
                return;
            }
            if (timezone == 25) {
                device_t.timezone = 3.5;
            } else if (timezone == 26) {
                device_t.timezone = 5.5;
            } else if (timezone < 13) {
                device_t.timezone = timezone;
            } else {
                device_t.timezone = -(timezone - 12);
            }
            DEBUG_TRACE(LOG_TAG, "Config Time Zone [%.1f]", device_t.timezone);
            convertTimestampToSystime();
            set_real_time();
            RTC_TimeSet();
            FLASH_Write_Timezone();
            dataReportTimestamp = Timestamp;
            local_schedule_init();
            device_t.sync_state = 1;
            dataLen -= 2;
            srcHex += 2;
        } break;
        default:
            dataLen -= 2;
            srcHex += 2;
            break;
        }
        if (dataLen > 200) // 长度异常，数据出错，退出数据解析
            break;
    }
    if (device_t.flash_write_flag) {
        device_t.flash_write_flag = 0;
        FLASH_Write_Device_Param();
    }
}

/**
 * @brief  周期同步上报数据
 * @param
 * @retval
 */
void Device_PeriodicReport(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_SET_VOL_LEVEL, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_PORT_FUNCTION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_STABLE_TIME, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_SMART_POWER, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_SET_TIME_ZONE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendNum + 1)) {
        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    } else {
        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
    }
}

/**
 * @brief  同步本地定时任务数据
 * @param
 * @retval
 */
void Device_ReportLocalTaskData(void)
{
    u8 dataLen = 2;
    u8 task_num = 0;

    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;
    int free_number = lwrb_get_free(&lwrbBuff_t);

    sendBuffer[0] = DATATYPE_SET_LOCAL_SCHEDULE;
    for (u8 i = 0; i < 16; i++) {
        if (local_schedule_t.task_t[i].is_vaild) {
            task_num++;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].id;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].valve;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].state;
            sendBuffer[dataLen++] = (local_schedule_t.task_t[i].pulse_count >> 8) & 0xFF;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].pulse_count & 0xFF;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].start_hour;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].start_min;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].end_hour;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].end_min;
            sendBuffer[dataLen++] = local_schedule_t.task_t[i].repeat_duty;

            if (task_num == 4) {
                sendBuffer[1] = task_num;
                sendNum = dataLen;
                free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendNum + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
                dataLen = 2;
                task_num = 0;
            }
        }
    }

    if (task_num > 0) {
        sendBuffer[1] = task_num;
        sendNum = dataLen;
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    }
}

/**
 * @brief  设备事件处理
 * @param
 * @retval
 */
void DeviceEvent_Process(void)
{
    if (device_t.ble_wake_event) {
        device_t.ble_wake_event = 0;
        if (device_t.ble_connected != BLE_CONNECT_STA) {
            mDelay(20);
            if (device_t.ble_connected != BLE_CONNECT_STA) {
                device_t.ble_connected = BLE_CONNECT_STA;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "BLE Connect State Change To %d", device_t.ble_connected);
            }
        }
    }

    if (device_t.insert1_wake_event) {
        device_t.insert1_wake_event = 0;
        if (device_t.insert1_connected != INSERT1_CONNECT_STA) {
            mDelay(20);
            if (device_t.insert1_connected != INSERT1_CONNECT_STA) {
                device_t.insert1_connected = INSERT1_CONNECT_STA;
                device_t.sync_state = 1;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "Insert1 Connect State Change To %d", device_t.insert1_connected);
            }
        }
    }

    if (device_t.insert2_wake_event) {
        device_t.insert2_wake_event = 0;
        if (device_t.insert2_connected != INSERT2_CONNECT_STA) {
            mDelay(20);
            if (device_t.insert2_connected != INSERT2_CONNECT_STA) {
                device_t.insert2_connected = INSERT2_CONNECT_STA;
                device_t.sync_state = 1;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "Insert2 Connect State Change To %d", device_t.insert2_connected);
            }
        }
    }

    if (device_t.insert1_connected != INSERT1_CONNECT_STA) {
        device_t.insert1_wake_event = 1;
    }
    if (device_t.insert2_connected != INSERT2_CONNECT_STA) {
        device_t.insert2_wake_event = 1;
    }

    if (device_t.power_mode) {
        if (device_t.insert1_connected || device_t.insert2_connected) {
            if (!device_t.power_on) {
                device_t.power_on = 1;
                DEBUG_TRACE(LOG_TAG, "Power State Change To %d", device_t.power_on);
            }
        } else {
            if (device_t.power_on) {
                device_t.power_on = 0;
                DEBUG_TRACE(LOG_TAG, "Power State Change To %d", device_t.power_on);
            }
        }
    }

    if (dataReportTimestamp <= Timestamp) {
        dataReportTimestamp = Timestamp + device_t.report_interval * 60;
        device_t.sync_state = 1;
    }

    if (device_t.zero_event) {
        device_t.zero_event = 0;
        DEBUG_TRACE(LOG_TAG, "Device Zero Event");

        local_schedule_init();
        device_t.pulse1_count = 0;
        device_t.pulse2_count = 0;
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
        if (lwrb_write_count) { // 缓冲区内有数据
            memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
            memset(sendBuffer, 0, sizeof(sendBuffer));
            u8 *dataHex = sendBuffer;

            lwrb_read(&lwrbBuff_t, &sendNum, 1);
            lwrb_read(&lwrbBuff_t, dataHex, sendNum);

            sendLoRaWANData();

            if (--lwrb_write_count == 0) {
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }
        } else if (device_t.sync_state) {
            device_t.sync_state = 0;
            Device_PeriodicReport();
            // uartLoRa.sendDelay  = 5 * SEC_DELAY;
            dataReportTimestamp = Timestamp + device_t.report_interval * 60;
        }
    }
    GpioEventProcess();
    DeviceEvent_Process();
}

/**
 * @brief  处理来自debug口的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromInfoDataHandle(u8 *data, u16 len)
{
    char *valid;

    device_t.sleep_delay = 30; // 串口有新命令时刷新进入睡眠时长
    /* 设置数据同步周期 */
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:0")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.report_interval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval: %dm", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") && len >= strlen("setDataSyncPeriod:1")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.report_interval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dmin \r\n OK", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod Param ERROR");
    }
    /* 设置LoRa参数 */
    else if (strstr((const char *)data, "LoRaWANPort") && strstr((const char *)data, "LoRaWANFreq") && strstr((const char *)data, "LoRaWANJoin")) {
        memset(&splitBuf, 0, sizeof(splitBuf));
        char *validStr = strstr((const char *)data, "LoRaWANPort:");
        int validNum = 0, i = 0;
        cmd_split(validStr, ",", splitBuf, &validNum);
        if (validNum == 8) {
            DEBUG_TRACE(ACK_TAG, "setLoRaWANParam OK");

            // LoRaWAN Port
            valid = strstr((const char *)splitBuf[0], ":");
            if (strlen(valid + 1) > 0 && strlen(valid + 1) <= 3) {
                LoRaWAN.port = atoi(valid + 1);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort: %d \r\n OK", LoRaWAN.port);
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort Param ERROR");

            // LoRaWAN Freq
            valid = strstr((const char *)splitBuf[1], ":");
            if (strlen(valid + 1) == 6) {
                char value[] = "868";
                snprintf(value, 3, "%s", valid + 1);
                LoRaWAN.frequency = atoi(value) * 1000;
                DEBUG_TRACE(ACK_TAG, "setLoRaWANFreq: %d \r\n OK", LoRaWAN.frequency);
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort Param ERROR");

            // LoRaWAN Join
            valid = strstr((const char *)splitBuf[2], ":");
            if (strlen(valid + 1) == 7) {
                if (strstr((const char *)valid + 1, "A"))
                    LoRaWAN.class = 0; // class A
                else
                    LoRaWAN.class = 2; // class C
                DEBUG_TRACE(ACK_TAG, "setLoRaWANJoin: %d \r\n OK", LoRaWAN.class);
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANJoin Param ERROR");

            // LoRaWAN APPEUI
            valid = strstr((const char *)splitBuf[3], ":") + 1;
            if (strlen(valid) == 8) {
                /* 转换字符串为hex */
                for (i = 0; i < 8; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppEUI[i]);

                DEBUG_TRACE(LOG_TAG, "AppEUI: %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.AppKEY[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2],
                            LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPEUI \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAppEUI Param ERROR");

            // LoRaWAN APPKEY
            valid = strstr((const char *)splitBuf[4], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppKEY[i]);

                DEBUG_TRACE(LOG_TAG, "APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.AppKEY[0],
                            LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6],
                            LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12],
                            LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPKEY \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPKEY Param ERROR");

            // LoRaWAN DEVADDR
            valid = strstr((const char *)splitBuf[5], ":") + 1;
            if (strlen(valid) == 4) {
                /* 转换字符串为hex */
                for (i = 0; i < 4; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.DevADDR[i]);

                DEBUG_TRACE(LOG_TAG, "DEVADDR: %02x %02x %02x %02x", LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANDEVADDR \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANDEVADDR Param ERROR");

            // LoRaWAN APPSKEY
            valid = strstr((const char *)splitBuf[6], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppSKEY[i]);

                DEBUG_TRACE(LOG_TAG, "APPSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.AppSKEY[0],
                            LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6],
                            LoRaWAN.AppSKEY[7], LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12],
                            LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY Param ERROR");

            // LoRaWAN NWKSKEY
            valid = strstr((const char *)splitBuf[7], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.NwkSKEY[i]);

                DEBUG_TRACE(LOG_TAG, "NWKSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.NwkSKEY[0],
                            LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6],
                            LoRaWAN.NwkSKEY[7], LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12],
                            LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANNWKSKEY \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANNWKSKEY Param ERROR");

            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 重新初始化设备
        } else
            DEBUG_TRACE(ACK_TAG, "LoRaWAN PARAM ERROR");
    } else if (strstr((const char *)data, "getDeviceStatus")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0) {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.report_interval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class A");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2],
                    LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2],
                    LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0],
                    LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6],
                    LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12],
                    LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:");
            sprintf(tempBuf + strlen(tempBuf), ",APPSKEY:");
            sprintf(tempBuf + strlen(tempBuf), ",NWKSKEY:");
        } else {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.report_interval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class C");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2],
                    LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2],
                    LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0],
                    LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6],
                    LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12],
                    LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:%02x%02x%02x%02x", LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2],
                    LoRaWAN.DevADDR[3]);
            sprintf(tempBuf + strlen(tempBuf), ",APPSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppSKEY[0],
                    LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6],
                    LoRaWAN.AppSKEY[7], LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12],
                    LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",NWKSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.NwkSKEY[0],
                    LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6],
                    LoRaWAN.NwkSKEY[7], LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12],
                    LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
        }
        DEBUG_TRACE(ACK_TAG, "%s", tempBuf);
    } else if (strstr((const char *)data, "syncDeviceTimestamp")) { // 同步设备时间
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
        if (stamp != 0) {
            stamp += device_t.timezone * 60 * 60;
            struct tm *timeP;
            timeP = localtime(&stamp); // 转换
            systime.tm_year = timeP->tm_year + 1900;
            systime.tm_mon = timeP->tm_mon;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            set_real_time();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD OK,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
            local_schedule_init();
            device_t.sync_state = 1;
        }
    } else if (strstr((const char *)data, "keepMainRun")) // 保持不进入睡眠
    {
        keepRunFlag = 1;
    } else if (strstr((const char *)data, "leaveMainRun")) {
        keepRunFlag = 0;
    } else {
        // DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY; // 错误命令
    }
}

void callback_BLEQuery(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_POWER, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_SET_VOL_LEVEL, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_PORT_FUNCTION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_STABLE_TIME, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_SMART_POWER, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_SET_TIME_ZONE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEAck(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEPower(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_POWER, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEValue(u8 state)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    sendBuffer[0] = 0xF9;
    dataHex += 1;
    sendNum += 1;

    sendBuffer[4] = state;

    dataHex += 4;
    sendNum += 4;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLETimestamp(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    sendBuffer[0] = 0xFA;
    dataHex += 1;
    sendNum += 1;

    sendBuffer[1] = (Timestamp >> 24) & 0xFF;
    sendBuffer[2] = (Timestamp >> 16) & 0xFF;
    sendBuffer[3] = (Timestamp >> 8) & 0xFF;
    sendBuffer[4] = Timestamp & 0xFF;

    dataHex += 4;
    sendNum += 4;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEInfo(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    sendBuffer[0] = 0xFB;
    dataHex += 1;
    sendNum += 1;

    dataLen = setDataPackage(DATATYPE_DEVICE_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEJoinData(void)
{
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    u16 power[10];
    uint8_t i = 0;

    sendBuffer[sendNum++] = 0xFC;

    sendBuffer[sendNum++] = LoRaWAN.joinState;

    sendBuffer[sendNum++] = (LoRaWAN.SNR >> 24) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.SNR >> 16) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.SNR >> 8) & 0xFF;
    sendBuffer[sendNum++] = LoRaWAN.SNR & 0xFF;

    sendBuffer[sendNum++] = (LoRaWAN.RSSI >> 24) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.RSSI >> 16) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.RSSI >> 8) & 0xFF;
    sendBuffer[sendNum++] = LoRaWAN.RSSI & 0xFF;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

/**
 * @brief  处理来自蓝牙的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromBleDataHandle(u8 *data, u16 len)
{
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:10")) {
        char *valid = strstr((const char *)data, ":");
        u16 report_interval = atoi(valid + 1);
        if (report_interval <= 1440 && report_interval >= 10) {
            device_t.report_interval = report_interval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    } else if (strstr((const char *)data, "getDevEui")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "DEVEUI:%02X%02X%02X%02X%02X%02X%02X%02X", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3],
                LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
        BLE_SendCMD(tempBuf);
        mDelay(100);
    } else if (strstr((const char *)data, "getProctName")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "ProctName:%s", PROCT_NAME);
        BLE_SendCMD(tempBuf);
        mDelay(100);
    } else if (strstr((const char *)data, "syncDeviceTimestamp")) { // 同步设备时间
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
        if (stamp != 0) {
            stamp += device_t.timezone * 60 * 60;
            struct tm *timeP;
            timeP = localtime(&stamp); // 转换
            systime.tm_year = timeP->tm_year + 1900;
            systime.tm_mon = timeP->tm_mon;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            set_real_time();
            RTC_TimeSet();
            DEBUG_TRACE(LOG_TAG, "Recv BLE Sync Time CMD, %04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "hkt") && data[5] == 1) { // 更新固件
        FLASH_Write_Update_Flag();
        mDelay(100);
        NVIC_SystemReset(); // 复位
    } else if (strstr((const char *)data, "reboot")) { // 重启
        BLE_SendCMD("reboot on");
        NVIC_SystemReset(); // 复位
    } else if (strstr((const char *)data, "switchPowerON")) { // 激活
        if (!device_t.power_on) {
            device_t.power_on = 1;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
            FLASH_Write_Device_Param();
        }
        BLE_SendCMD("PowerON");
    } else if (strstr((const char *)data, "switchPowerOFF")) { // 关机
        if (device_t.power_on) {
            device_t.power_on = 0;
            FLASH_Write_Device_Param();
        }
        BLE_SendCMD("PowerOFF");
    } else if (strstr((const char *)data, "getDeviceStatus")) { // 获取设备信息
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "HARDWARE_VER: 0x%02X ,SOFTWARE_VER: 0x%02X", HARDWARE_VER, SOFTWARE_VER);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "batteryLevel: %d", batteryLevel);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "powerStatus: %d", device_t.power_on);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "dataSyncPeriod: %d", device_t.report_interval);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "LoRaWANJoin: %s", "Class A");
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "DEVEUI: %02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3],
                LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "APPEUI: %02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3],
                LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "APPKEY: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1],
                LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8],
                LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14],
                LoRaWAN.AppKEY[15]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "Join State: %d", LoRaWAN.joinState);
        BLE_SendCMD(tempBuf);

        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "Value1 State[%d] Insert1[%d] Pulse1[%d]", device_t.value1_state, device_t.insert1_connected, device_t.pulse1_count);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "Value2 State[%d] Insert2[%d] Pulse2[%d]", device_t.value2_state, device_t.insert2_connected, device_t.pulse2_count);
        BLE_SendCMD(tempBuf);
        mDelay(100);
    } else if (strstr((const char *)data, "hkt") && (data[5] == 5 || data[5] == 9)) { // APP操作指令
        DebugHexInfo("RevBLE", data, len);
        u8 packnum = data[6];
        // APP查询指令
        if (data[6] == 0xFF) {
            callback_BLEQuery();
        } else if (data[6] == 0xFC) {
            callback_BLEJoinData();
        } else if (data[6] == 0xFB) {
            callback_BLEInfo();
        } else if (data[6] == 0x09 || data[6] == 0x0A) { // APP开关机控制
            if (data[6] == 0x09) {
                if (!device_t.power_on) {
                    device_t.power_on = 1;
                    FLASH_Write_Device_Param();
                    DEBUG_TRACE(LOG_TAG, "Recv BLE Power ON");
                }
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
            } else {
                if (device_t.power_on) {
                    device_t.power_on = 0;
                    FLASH_Write_Device_Param();
                    DEBUG_TRACE(LOG_TAG, "Recv BLE Power OFF");
                }
            }
            callback_BLEPower();
        } else if (data[6] == 0xF9) { // APP控制阀门开关
            if (data[10] == 0x01) {
                GPIO_12V_PWR_H;
                GPIO_BOOST_PWR_H;
                GPIO_CON1_EN_L;
                GPIO_CON1_A_H;
                GPIO_CON1_B_L;
                GPIO_CON2_EN_L;
                GPIO_CON2_A_H;
                GPIO_CON2_B_L;
                callback_BLEValue(1);
            } else {
                GPIO_CON1_EN_L;
                GPIO_CON1_A_L;
                GPIO_CON1_B_L;
                GPIO_CON2_EN_L;
                GPIO_CON2_A_L;
                GPIO_CON2_B_L;
                GPIO_BOOST_PWR_L;
                GPIO_12V_PWR_L;
                callback_BLEValue(0);
            }
        } else if (data[6] == 0xFA) {
            // APP执行时间同步
            time_t stamp = data[7] << 24 | data[8] << 16 | data[9] << 8 | data[10];
            if (stamp != 0) {
                stamp += device_t.timezone * 60 * 60;
                struct tm *timeP;
                timeP = localtime(&stamp); // 转换
                systime.tm_year = timeP->tm_year + 1900;
                systime.tm_mon = timeP->tm_mon;
                systime.tm_mday = timeP->tm_mday;
                systime.tm_hour = timeP->tm_hour;
                systime.tm_min = timeP->tm_min;
                systime.tm_sec = timeP->tm_sec;
                systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
                getTimestamp();
                set_real_time();
                RTC_TimeSet();
                DEBUG_TRACE(LOG_TAG, "Recv BLE Sync Time[%d], %04d-%02d-%02d-%d:%d:%d", stamp, systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                            systime.tm_hour, systime.tm_min, systime.tm_sec);
            }
            callback_BLETimestamp();
        } else if (data[6] == 0xFE) {
            // APP开关机控制
            if (data[10] == 0x01) {
                if (!device_t.power_on) {
                    device_t.power_on = 1;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                    FLASH_Write_Device_Param();
                }
            } else if (data[10] == 0x00) {
                if (device_t.power_on) {
                    device_t.power_on = 0;
                    FLASH_Write_Device_Param();
                }
            }
            DEBUG_TRACE(LOG_TAG, "Recv BLE Power CMD [%02X]", data[10]);
            callback_BLEAck();
        } else if (data[7] == 1) { // APP更新指令
            FLASH_Write_Update_Flag();
            mDelay(100);
            NVIC_SystemReset(); // 复位
        }
    } else if (strstr((const char *)data, "hkt") && data[5] == 1) { // 更新固件
        FLASH_Write_Update_Flag();
        mDelay(100);
        NVIC_SystemReset(); // 复位
    } else if (strstr((const char *)data, "hkt") && data[5] == 4 && data[6] == 6) { // 同步设备时间
        time_t stamp = data[7] << 24 | data[8] << 16 | data[9] << 8 | data[10];
        if (stamp != 0) {
            stamp += device_t.timezone * 60 * 60;
            struct tm *timeP;
            timeP = localtime(&stamp); // 转换
            systime.tm_year = timeP->tm_year + 1900;
            systime.tm_mon = timeP->tm_mon;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            set_real_time();
            RTC_TimeSet();
            callback_BLEAck();
            DEBUG_TRACE(LOG_TAG, "Recv BLE Sync Time[%d], %04d-%02d-%02d-%d:%d:%d", stamp, systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "hkt") && data[5] == 8 && data[6] == 2) { // APP通用配置
        u8 volOut = data[7];
        u8 valveMode = data[8];
        u8 stableTime = data[9];
        u8 autoPower = data[10];
        u8 timezone = data[11];
        u16 reportInterval = data[12] << 8 | data[13];
        if (timezone > 26) {
            DEBUG_TRACE(ERROR_TAG, "Config Time Zone Err[%d]", timezone);
            return;
        }
        if ((reportInterval >= 1 && reportInterval <= 1440) || reportInterval == 0) {
            device_t.report_interval = reportInterval;
            device_t.vol_control_sw = volOut;
            device_t.port_mode = valveMode;
            device_t.stable_time = stableTime;
            device_t.power_mode = autoPower;

            FLASH_Write_Device_Param();
            FLASH_Write_Report_Interval();

            if (timezone == 25) {
                device_t.timezone = 3.5;
            } else if (timezone == 26) {
                device_t.timezone = 5.5;
            } else if (timezone < 13) {
                device_t.timezone = timezone;
            } else {
                device_t.timezone = -(timezone - 12);
            }
            DEBUG_TRACE(LOG_TAG, "Config Time Zone [%.1f]", device_t.timezone);
            convertTimestampToSystime();
            set_real_time();
            RTC_TimeSet();
            FLASH_Write_Timezone();
            dataReportTimestamp = Timestamp;
            local_schedule_init();
            device_t.sync_state = 1;

            DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.report_interval);
        } else {
            DEBUG_TRACE(ERROR_TAG, "Update Device Sync Interval ERROR: %dmin", device_t.report_interval);
        }
        callback_BLEAck(); // 复位
    } else if (strstr((const char *)data, "hkt") && data[5] == 7 && data[6] == 3) { // 实时任务
        // if (local_schedule_t.current->is_start != 1 && !local_schedule_t.current) { // 本地任务执行时无效
        if (local_schedule_t.current->is_start != 1) { // 本地任务执行时无效

            callback_BLEAck();

            memset(&real_time_task_t, 0, sizeof(real_time_task_t));
            u8 *srcHex = data + 6;
            real_time_task_t.valve = srcHex[1];
            real_time_task_t.state = srcHex[2];
            real_time_task_t.time = srcHex[3] << 8 | srcHex[4];
            real_time_task_t.pulse_count = srcHex[5] << 16 | srcHex[6] << 8 | srcHex[7];
            if (real_time_task_t.time > 0)
                real_time_task_t.task_over_timestamp = Timestamp + real_time_task_t.time;
            else
                real_time_task_t.task_over_timestamp = 0;
            real_time_task_t.pulse_count_trigger = 0;
            real_time_task_t.is_start = 1;

            INFO("\r\n********************** Start Real Task **********************\r\n");
            // 根据初始状态恢复到原来的状态
            // 根据初始状态执行相反操作
            if (!real_time_task_t.state) {
                switch (real_time_task_t.valve) {
                case 1:
                    back_1_control();
                    break;
                case 2:
                    back_2_control();
                    break;
                case 0:
                    back_1_control();
                    back_2_control();
                    break;
                default:
                    break;
                }
            } else {
                switch (real_time_task_t.valve) {
                case 1:
                    forward_1_control();
                    break;
                case 2:
                    forward_2_control();
                    break;
                case 0:
                    forward_1_control();
                    forward_2_control();
                    break;
                default:
                    break;
                }
            }
            device_t.sync_state = 1;
        }
    } else if (strstr((const char *)data, "hkt") && data[5] == 11 && data[6] == 4) { // 配置定时任务
        u16 tempTime = 0;
        u8 *task_index = data + 7;
        struct schedule task_t = {0};
        task_t.id = task_index[0];
        task_t.valve = task_index[1];
        task_t.state = task_index[2];
        task_t.pulse_count = task_index[3] << 8 | task_index[4];
        tempTime = task_index[5] << 8 | task_index[6];
        task_t.start_hour = tempTime / 60;
        task_t.start_min = tempTime % 60;
        tempTime = task_index[7] << 8 | task_index[8];
        task_t.end_hour = tempTime / 60;
        task_t.end_min = tempTime % 60;
        task_t.repeat_duty = task_index[9];

        if (task_t.id > 16 || task_t.id == 0) {
            DEBUG_TRACE(ERROR_TAG, "Task[%d] ID Err", task_t.id);
            return;
        }
        if (task_t.valve > 2) {
            DEBUG_TRACE(ERROR_TAG, "Task[%d] Value Err[%d]", task_t.id, task_t.valve);
            return;
        }
        if (task_t.state > 1) {
            DEBUG_TRACE(ERROR_TAG, "Task[%d] State Err[%d]", task_t.id, task_t.state);
            return;
        }
        if (task_t.start_hour > 23 || task_t.start_min > 59 || task_t.end_hour > 23 || task_t.end_min > 59) {
            DEBUG_TRACE(ERROR_TAG, "Task[%d] Time Err[%d:%d-%d:%d]", task_t.id, task_t.start_hour, task_t.start_min, task_t.end_hour, task_t.end_min);
            return;
        }
        if (task_t.repeat_duty > 0x7F) {
            DEBUG_TRACE(ERROR_TAG, "Task[%d] Repeat Err[%d]", task_t.id, task_t.repeat_duty);
            return;
        }
        task_t.is_vaild = 1;
        memcpy(&local_schedule_t.task_t[task_t.id - 1], &task_t, sizeof(task_t));
        local_schedule_init();
        FLASH_Write_Local_Schedule();
        callback_BLEAck();
    } else if (strstr((const char *)data, "hkt") && data[5] == 2 && data[6] == 5) { // 删除定时任务
        u8 id = data[7];
        if ((id > 16 && id != 0xFF) || id == 0) {
            DEBUG_TRACE(ERROR_TAG, "Delete Task[%d] ID Err", id);
            return;
        }
        // 删除任务时强制关闭当前正在执行的任务
        if (id == 0xFF || local_schedule_t.task_t[id - 1].is_start) {
            local_schedule_init();
        }
        if (id == 0xFF) {
            memset(&local_schedule_t, 0, sizeof(local_schedule_t));
        } else {
            memset(&local_schedule_t.task_t[id - 1], 0, sizeof(struct schedule));
        }
        local_schedule_init();
        FLASH_Write_Local_Schedule();
        callback_BLEAck();
    }
    // else {
    //     BLE_SendCMD(data);
    // }
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromUartDataHandle(void)
{
    if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
        DebugHexInfo("LoRaWAN Recv", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartBle.recvTimeout && uartBle.recvLen) {
        DebugHexInfo("BLE Recv", uartBle.recvData, uartBle.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart BLE Data: %s", uartBle.recvData);
        fromBleDataHandle(uartBle.recvData, uartBle.recvLen);
        memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
        uartBle.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug

        // DebugHexInfo("Info Recv", uartInfo.recvData, uartInfo.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartInfo.recvData);

        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

/******************************************************************************/
/******************************************************************************/

/**
 * @brief  环形缓冲区事件打印函数
 * @param
 * @retval
 */
void my_buff_evt_fn(lwrb_t *buff, lwrb_evt_type_t type, size_t len)
{
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
