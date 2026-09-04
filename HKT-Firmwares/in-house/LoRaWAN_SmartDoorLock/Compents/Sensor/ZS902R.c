#include "ZS902R.h"
#include "W25X40CLSNIG.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "lwrb.h"
#include "si12t.h"
#include "sound.h"
#include "systick.h"
#include "uart.h"

#if FUNC_FINGERPRINT_ENABLE

TX_EVENT_FLAGS_GROUP zs_display_event_group;

static const u8 pack_header[2] = {0xEF, 0x01};
// 校验和是从包标识至校验和之间所有字节之和，包含包标识不包含校验和，超出 2 字节的进位忽略
static u16 zs_cal_sum(u8 *buf, int len)
{
    u16 sum = 0;
    for (int i = 6; i < len - 2; i++) {
        sum += buf[i];
    }
    return sum;
}

// 校验数据
bool check_recv_pack(u8 *buf, int len)
{
    u16 sum, buf_sum;

    sum = zs_cal_sum(buf, len);
    buf_sum = buf[len - 2] << 8 | buf[len - 1];
    if (sum == buf_sum && !memcmp(buf, pack_header, 2)) {
        return true;
    }
    return false;
}

// 获取确认码
u8 get_validation_code(u8 *vbuf, u16 timeout)
{
    u16 time_slice = 20;
    u16 retry = 0;
    u8 validation_code;
    u8 pack_len = 0;
    while (1) {
        if (uartZs.recvLen && !uartZs.recvTimeout) {
            // DebugHexInfo("Recv Finger", uartZs.recvData, uartZs.recvLen);
            // DEBUG_TRACE(LOG_TAG, "Recv Uart Finger Data: %s", uartZs.recvData);
            if (check_recv_pack(uartZs.recvData, uartZs.recvLen) == true) {
                validation_code = uartZs.recvData[9];
                pack_len = uartZs.recvData[8];
                if (vbuf != NULL)
                    memcpy(vbuf, uartZs.recvData + 9, pack_len);
                memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
                uartZs.recvLen = 0;
                return validation_code;
            }
            memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
            uartZs.recvLen = 0;
        }
        if (time_slice * retry > timeout) {
            memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
            uartZs.recvLen = 0;
            DEBUG_TRACE(WARN_TAG, "Get Validation Code Timeout");
            return 0xFF;
        }
        retry++;
        tx_thread_sleep(time_slice);
    }
}

// 获取指纹图像
u8 get_fingerprint_image(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;
    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = 0xFF;
    u8 retry = 0;
    while (1) {
        if (uartZs.recvLen && !uartZs.recvTimeout) {
            if (check_recv_pack(uartZs.recvData, uartZs.recvLen) == true) {
                // 包标识与长度均匹配
                if (uartZs.recvData[6] == 7 && uartZs.recvData[8] == 3) {
                    validation_code = uartZs.recvData[9];
                    break;
                } else {
                    validation_code = ZS_VALIDATION_CODE_PACK_RECV_ERROR;
                    DebugHexInfo(__func__, uartZs.recvData, uartZs.recvLen);
                    break;
                }
            }
            memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
            uartZs.recvLen = 0;
        }

        if (20 * retry > 500) {
            memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
            uartZs.recvLen = 0;
            break;
        }
        retry++;
        tx_thread_sleep(20);
    }
    tx_semaphore_put(&Semaphore);
    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_NO_FINGER:
        // DEBUG_TRACE(WARN_TAG, "\"%s\" No Finger %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 生成指纹特征
u8 creat_fingerprint_characteristic(u8 bufferID)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x01, 0x00, 0x08};
    buf[10] = bufferID;

    u16 sum = zs_cal_sum(buf, sizeof(buf));
    buf[11] = sum >> 8;
    buf[12] = sum & 0xFF;

    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_SPANNING_CHARACTERISTICS_FAIL_3:
    case ZS_VALIDATION_CODE_SPANNING_CHARACTERISTICS_FAIL_4:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Spanning Characteristics Fail %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FINGER_NO_MATCH:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Finger Not Match %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_MERGE_CHARACTERISTICS_FAIL:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Merge Characteristics Fail %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_MERGE_IMAGE_FAIL:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Merge Image Fail %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FINGER_CHARACTERISTICS_RELEVANT:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Finger Characteristics Relevant %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 精确比对模板缓冲区中的特征文件或者模板
u8 match_fingerprint(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x03, 0x00, 0x07};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FINGER_NO_MATCH:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Finger Not Match %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FUNC_SECRET_LEVEL_NOT_MATCH:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Func Not Match %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 搜索指纹
u8 search_fingerprint(u16 *id_num)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x04, 0x01, 0x00, 0x00, 0x00, 0x82, 0x00, 0x07};
    u8 rbuf[9] = {0};

    u16 sum = zs_cal_sum(buf, sizeof(buf));
    buf[15] = sum >> 8;
    buf[16] = sum & 0xFF;
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(rbuf, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH: {
        u16 num = rbuf[1] << 8 | rbuf[2];
        u16 score = rbuf[3] << 8 | rbuf[4]; // 得分
        *id_num = num;
        DEBUG_TRACE(WARN_TAG, "\"%s\" Match Finger Success %d, ID[%03d], Score[%d]", __func__, validation_code, num, score);
    } break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FINGER_NOT_FIND:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Finger Not Find %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FINGER_NOT_MOVE:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Finger Not Move %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FUNC_SECRET_LEVEL_NOT_MATCH:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Func Not Match %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 合并特征
u8 merge_characteristics(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x05, 0x00, 0x09};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_MERGE_CHARACTERISTICS_FAIL:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Merge Fail %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 存储模板
u8 save_template(u16 location_num)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x06, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00};

    buf[11] = location_num >> 8;
    buf[12] = location_num & 0xFF;

    u16 sum = zs_cal_sum(buf, sizeof(buf));
    buf[13] = sum >> 8;
    buf[14] = sum & 0xFF;
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 2000);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_ADDR_FAIL:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Address Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_WR_FLASH_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Write Flash Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FUNC_SECRET_LEVEL_NOT_MATCH:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Func Not Match %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 读取模板
u8 read_template(u16 location_num)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x06, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00};

    buf[11] = location_num >> 8;
    buf[12] = location_num & 0xFF;

    u16 sum = zs_cal_sum(buf, sizeof(buf));
    buf[13] = sum >> 8;
    buf[14] = sum & 0xFF;
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_READ_TEMPLATE_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Read Template Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_ADDR_FAIL:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Address Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FINGER_TEMPLATE_INVALID:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Finger Template Invalid %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 上传模板 暂不实现
// 下载模板 暂不实现

// 删除模板
u8 delete_template(u16 page, u16 delete_num)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    buf[10] = page >> 8;
    buf[11] = page & 0xFF;

    buf[12] = delete_num >> 8;
    buf[13] = delete_num & 0xFF;

    u16 sum = zs_cal_sum(buf, sizeof(buf));
    buf[14] = sum >> 8;
    buf[15] = sum & 0xFF;
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_DELETE_TEMPLATE_FAIL:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Delete Template Fail %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 清除指纹库
void clear_fingerprint_maps(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0D, 0x00, 0x11};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));

    u8 validation_code = get_validation_code(NULL, 2000);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_CLEAR_FINGER_FAIL:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Clear Fail %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
}

// 读模组基本参数
u8 read_mode_param(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0F, 0x00, 0x13};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 读有效模板个数
u8 read_valid_template_num(u16 *num)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x1D, 0x00, 0x21};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));

    u8 rbuf[4] = {0};
    u8 validation_code = get_validation_code(rbuf, 500);
    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        *num = rbuf[0] << 8 | rbuf[1];
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 注册用获取图像
u8 get_enroll_image(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x29, 0x00, 0x2D};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Recv Pack Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_NO_FINGER:
        DEBUG_TRACE(WARN_TAG, "\"%s\" No Finger %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 进入休眠模式
u8 enter_sleep_mode(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x33, 0x00, 0x37};
    u8 retry = 0;
    u8 validation_code;

    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    while (retry < 3) {
        Zs_SendData(buf, sizeof(buf));
        validation_code = get_validation_code(NULL, 500);

        switch (validation_code) {
        case ZS_VALIDATION_CODE_EXEC_FINSH:
            DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
            return validation_code;
        case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
            DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Error %d", __func__, validation_code);
            return validation_code;
        case 0xFF:
            DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
            break;
        default:
            break;
        }
        retry++;
        tx_thread_sleep(1000);
    }
    return validation_code;
}

// 取消指令 取消自动注册模板和自动验证指纹
u8 cancel_cmd(void)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x30, 0x00, 0x34};
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;

    Zs_SendData(buf, sizeof(buf));
    u8 validation_code = get_validation_code(NULL, 500);

    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Error %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FUNC_SECRET_LEVEL_NOT_MATCH:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Func Not Match %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// LED 控制灯指令
// mode 1-普通呼吸灯，2-闪烁灯，3-常开灯，4-常闭灯，5-渐开灯，6-渐闭灯
// color 0x01_蓝灯亮，0x02_绿灯亮，0x04_红灯亮，0x06_红绿灯亮，0x05_红蓝灯亮，0x03_绿蓝灯亮，0x07_红绿蓝灯亮，0x00_全灭
// cycle 循环次数
u8 finger_led_control(u8 mode, u8 start_color, u8 end_color, u8 cycle)
{
#if !FUNC_OPERATIONAL_VERSION_ENABLE
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    u16 retry = 0;
    u8 validation_code;

    buf[10] = mode;
    buf[11] = start_color;
    buf[12] = end_color;
    buf[13] = cycle;

    u16 sum = zs_cal_sum(buf, sizeof(buf));
    buf[14] = sum >> 8;
    buf[15] = sum & 0xFF;
    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;
    while (retry < 3) {
        Zs_SendData(buf, sizeof(buf));
        validation_code = get_validation_code(NULL, 800);
        tx_semaphore_put(&Semaphore);
        switch (validation_code) {
        case ZS_VALIDATION_CODE_EXEC_FINSH:
            DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
            return validation_code;
        case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
            DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Error %d", __func__, validation_code);
            return validation_code;
        case 0xFF:
            DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
            break;
        default:
            break;
        }
        retry++;
    }
    return validation_code;
#endif
}

// 自动验证指纹(全局搜索)
u8 auto_identify(u16 *id_num)
{
    u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x32, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0x02, 0x0E};

    u16 sum = zs_cal_sum(buf, sizeof(buf));
    buf[15] = sum >> 8;
    buf[16] = sum & 0xFF;

    memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
    uartZs.recvLen = 0;
    Zs_SendData(buf, sizeof(buf));

    u8 rbuf[9] = {0};
    u8 validation_code = get_validation_code(NULL, 2000);
    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
        break;
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
    default:
        return validation_code;
    }

    validation_code = get_validation_code(rbuf, 2000);
    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        if (rbuf[0] == 1) // 获取图像成功
            DEBUG_TRACE(WARN_TAG, "\"%s\" Get Image Success %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_TIMEOUT:
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
    default:
        return validation_code;
    }

    validation_code = get_validation_code(rbuf, 2000);
    switch (validation_code) {
    case ZS_VALIDATION_CODE_EXEC_FINSH:
        if (rbuf[0] == 5) // 已注册指纹比对
        {
            u16 num = rbuf[1] << 8 | rbuf[2];
            u16 score = rbuf[3] << 8 | rbuf[4]; // 得分
            *id_num = num;
            DEBUG_TRACE(WARN_TAG, "\"%s\" Match Finger Success %d, ID[%03d], Score[%d]", __func__, validation_code, num, score);
        }
        break;
    case ZS_VALIDATION_CODE_FINGER_NOT_FIND:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Not Find Finger %d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_FINGER_INVALID: // 指纹库为空
        DEBUG_TRACE(WARN_TAG, "\"%s\" Finger Invalid%d", __func__, validation_code);
        break;
    case ZS_VALIDATION_CODE_TIMEOUT:
    case 0xFF:
        DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
        break;
    default:
        break;
    }
    return validation_code;
}

// 恢复出厂设定(未完善)
// u8 reset_factory(void)
// {
//     u8 buf[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x3B, 0x00, 0x3F};
//         memset(uartZs.recvData, 0, sizeof(uartZs.recvData));
// uartZs.recvLen = 0;

// Zs_SendData(buf, sizeof(buf));
//      u8 validation_code = get_validation_code(NULL, 500);

//     switch (validation_code)
//     {
//     case ZS_VALIDATION_CODE_EXEC_FINSH:
//         DEBUG_TRACE(LOG_TAG, "\"%s\" Exec Success %d", __func__, validation_code);
//         break;
//     case ZS_VALIDATION_CODE_PACK_RECV_ERROR:
//         DEBUG_TRACE(WARN_TAG, ""\"%s\" Exec Error %d", __func__, validation_code);
//         break;
//     case 0xFF:
//         DEBUG_TRACE(WARN_TAG, "\"%s\" Exec Timeout %d", __func__, validation_code);
//         break;
//     default:
//         break;
//     }
//     return validation_code;
// }

/**
 * @brief  指纹模块事件
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_fingerprint(ULONG thread_input)
{
    (void)thread_input;
    ULONG actual_events;
    u16 userNum, action;

    u8 ret;
    u8 characteristics_number = 0;

    u8 buffer[100];
    u8 sendLen;
    u8 repeat_cnt = 0;

    ULONG zs902r_bit;
    ULONG main_bit;
    UINT status = tx_event_flags_create(&zs_display_event_group, "zs_display_event_group");
    if (status) {
        DEBUG_TRACE(ERROR_TAG, "zs_display_event_group Creat Fail !!! %d", status);
    }

    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_BLUE, TX_OR);

    FINGER_POWER_H;
    tx_thread_sleep(3000);
    // clear_fingerprint_maps();

    while (1) {
    __back:

        // 等待处于运行状态
        tx_event_flags_get(&main_event_group, MAIN_BIT_RUN, TX_OR, &main_bit, TX_WAIT_FOREVER);
        tx_thread_sleep(5);

        if (!device_t.operateTimeout && device_t.lockMode) { // 超时退出管理员模式
            device_t.lockMode = 0;
            if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_FINGER || device_t.lockMode == SYSTEM_MODE_INPUT_USER_FINGER ||
                (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER_2 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_5)) {
                cancel_cmd();
            }
        }

        status = tx_event_flags_get(&zs_display_event_group, FINGER_BIT_DISPLAY_ALL, TX_OR_CLEAR, &zs902r_bit, TX_NO_WAIT);
        if (status == TX_SUCCESS) {
            if (zs902r_bit & FINGER_BIT_DISPLAY_STOP) {
                finger_led_control(3, 0, 0, 0); // 关闭灯光
            } else if (zs902r_bit & FINGER_BIT_DISPLAY_GREEN) {
                finger_led_control(3, 2, 2, 0); // 绿色常亮
            } else if (zs902r_bit & FINGER_BIT_DISPLAY_BLUE) {
                finger_led_control(3, 1, 1, 0); // 蓝色常亮
            } else if (zs902r_bit & FINGER_BIT_DISPLAY_RED) {
                finger_led_control(3, 4, 4, 0); // 红色常亮
            }

            if (zs902r_bit & FINGER_BIT_SLEEP) {
                cancel_cmd();
                enter_sleep_mode();
                tx_event_flags_set(&zs_display_event_group, FINGER_BIT_SLEEP_DONE, TX_OR);
                goto __back;
            }
        }

        tx_event_flags_set(&zs_display_event_group, FINGER_BIT_IDLE, TX_OR);
        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &main_bit, TX_NO_WAIT);
        if ((main_bit & MAIN_BIT_SLEEP) || (main_bit & MAIN_BIT_DOOR_OPEN)) {
            goto __back;
        }

        tx_thread_sleep(495);

        tx_event_flags_get(&zs_display_event_group, FINGER_BIT_IDLE, TX_OR_CLEAR, NULL, TX_NO_WAIT);
        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &main_bit, TX_NO_WAIT);
        if ((main_bit & MAIN_BIT_SLEEP) || (main_bit & MAIN_BIT_DOOR_OPEN)) {
            goto __back;
        }

        if (device_t.sleepState || device_t.lockDoorTimeout) // 睡眠时停止检测
            goto __back;

        if (!device_t.lockMode ||
            device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_PASSWORD ||
            (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER) ||
            (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER_2 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_5)) {
            ret = get_fingerprint_image(); // 获取图像
            if (ret != 0)
                goto __back;
        } else
            goto __back;

        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;

        if (device_t.lockTimeout) { // 锁定状态
            DEBUG_TRACE(WARN_TAG, "The Lock is Locked, %s", __func__);
            memset(&s_touch, 0, sizeof(s_touch));
            characteristics_number = 0;
            /* 语音 */
            // 失败提示音
            Sound_Insert_Number(0, VOICE_TONE_FAIL);
            // 系统已锁定
            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
            finger_led_control(3, 4, 4, 0); // 红色常亮
            tx_thread_sleep(1500);          // 失败后强制延时播放语音
            finger_led_control(3, 0, 0, 0); // 关闭灯光
            goto __back;
        }

        if (device_t.lockMode) {
            device_t.operateTimeout = 20 * SEC_DELAY;
            /* 录入指纹信息 */
            if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_FINGER || device_t.lockMode == SYSTEM_MODE_INPUT_USER_FINGER) {
                characteristics_number = 1;
                ret = creat_fingerprint_characteristic(characteristics_number); // 生成指纹特征
                if (ret != 0)
                    goto __fingerprint_fail;

                /* 语音 */
                // 请再次输入
                Sound_Insert_Number(0, VOICE_INPUT_SECOND);
                finger_led_control(3, 2, 2, 0); // 绿色常亮
                tx_thread_sleep(1000);
                finger_led_control(3, 1, 1, 0); // 蓝色常亮
                device_t.lockMode += 7;
                DEBUG_TRACE(LOG_TAG, "Exec Success, Characteristics Number[%d], Lock Mode %d", characteristics_number, device_t.lockMode);
            }
            /* 录入指纹信息 */
            else if (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER_2 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_5) {
                ret = creat_fingerprint_characteristic(++characteristics_number); // 生成指纹特征
                if (ret != 0)
                    goto __fingerprint_fail;
                if (device_t.lockMode >= SYSTEM_MODE_INPUT_ROOT_FINGER_2 && device_t.lockMode <= SYSTEM_MODE_INPUT_USER_FINGER_4) {
                    device_t.lockMode += 2;
                    /* 语音 */
                    // 请再次输入
                    Sound_Insert_Number(0, VOICE_INPUT_SECOND);
                    finger_led_control(3, 2, 2, 0); // 绿色常亮
                    tx_thread_sleep(1000);
                    finger_led_control(3, 1, 1, 0); // 蓝色常亮
                    DEBUG_TRACE(LOG_TAG, "Exec Success, Characteristics Number[%d], Lock Mode %d", characteristics_number, device_t.lockMode);
                } else {
                    ret = merge_characteristics(); // 合并特征
                    if (ret != 0)
                        goto __fingerprint_fail;
                    ret = save_template(device_t.inputNum); // 存储模板
                    if (ret != 0)
                        goto __fingerprint_fail;
                    device_t.lockMode = 0;
                    coded_lock_t.total_num_finger++;
                    coded_lock_t.finger[(u8)device_t.inputNum].isable = 1;

                    set_fingerprint(device_t.inputNum, 1);
                    DEBUG_TRACE(LOG_TAG, "Exec Success, Finger Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);

                    /* 语音 */
                    // 操作成功
                    Sound_Insert_Number(0, VOICE_EXCE_SUCCESS);
                    finger_led_control(3, 2, 2, 0); // 绿色常亮
                    tx_thread_sleep(1000);
                    finger_led_control(3, 1, 1, 0); // 蓝色常亮

                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.inputAction = 1;
                    exc_lock_info_t.type = 1;
                    exc_lock_info_t.number = device_t.inputNum;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
            }
            /* 验证管理员密码 */
            else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_PASSWORD) {
                userNum = 0xFF;
                if (coded_lock_t.total_num_finger > 0) {
                    // __repeat_verify:
                    //     if (repeat_cnt != 0) {
                    //         ret = get_fingerprint_image(); // 获取图像
                    //         if (ret != 0)
                    //             goto __back;
                    //     }
                    //     repeat_cnt++;

                    ret = creat_fingerprint_characteristic(1); // 生成指纹特征
                    if (ret != 0)
                        goto __back;
                    ret = search_fingerprint(&userNum); // 搜索指纹

                    if (ret == ZS_VALIDATION_CODE_EXEC_FINSH && (userNum) < 10 && coded_lock_t.finger[userNum].isable) {
                        repeat_cnt = 0;
                        device_t.tamperAlarm = 0;
                        device_t.enterRootNum = userNum;
                        device_t.enterRootType = 2;
                        device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                        DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Choose Mode, Lock Mode %d", device_t.lockMode);
                        /* 语音 */
                        // 操作成功
                        // 1 密码用户
                        // 2 卡用户
                        // 3 指纹用户
                        // 4 系统设置
                        Sound_Insert_Number(0, VOICE_EXCE_SUCCESS);
                        Sound_Insert_Number(1, VOICE_NUM_1);
                        Sound_Insert_Number(1, VOICE_PASSWORD_USER);
                        Sound_Insert_Number(1, VOICE_NUM_2);
                        Sound_Insert_Number(1, VOICE_CARD_USER);
                        Sound_Insert_Number(1, VOICE_NUM_3);
                        Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
                        Sound_Insert_Number(1, VOICE_NUM_4);
                        Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);

                        device_t.openFailCnt = 0;
                        device_t.enterRootFailCnt = 0;
                        finger_led_control(3, 2, 2, 0); // 绿色常亮
                        tx_thread_sleep(1500);
                        finger_led_control(3, 0, 0, 0); // 关闭灯光
                    } else {
                    //     if (repeat_cnt < 3)
                    //         goto __repeat_verify;
                    //     repeat_cnt = 0;
                    __fingerprint_fail:
                        DEBUG_TRACE(WARN_TAG, "Exec Fail, Finger Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                        /* 语音 */
                        // 操作失败
                        // 请输入管理员信息
                        Sound_Insert_Number(0, VOICE_EXCE_FAIL);
                        if (++device_t.enterRootFailCnt >= 5) {
                            device_t.enterRootFailCnt = 0;
                            device_t.lockTimeout = 100 * SEC_DELAY;

                            /* 语音 */
                            // 系统已锁定
                            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                            finger_led_control(3, 0, 0, 0); // 关闭灯光
                        } else {
                            Sound_Insert_Number(1, VOICE_PLS_INPUT_ROOT_INFO);
                        }
                        finger_led_control(3, 4, 4, 0); // 红色常亮
                        tx_thread_sleep(1500);
                        if (!device_t.lockTimeout)
                            finger_led_control(3, 1, 1, 0); // 蓝色常亮
                        else
                            finger_led_control(3, 0, 0, 0); // 关闭灯光
                    }
                } else if (!coded_lock_t.total_num_finger && !coded_lock_t.total_num_card && !coded_lock_t.total_num_pass) {
                    device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                    DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Choose Mode, Lock Mode %d", device_t.lockMode);
                    /* 语音 */
                    // 操作成功
                    // 1 密码用户
                    // 2 卡用户
                    // 3 指纹用户
                    // 4 系统设置
                    Sound_Insert_Number(0, VOICE_EXCE_SUCCESS);
                    Sound_Insert_Number(1, VOICE_NUM_1);
                    Sound_Insert_Number(1, VOICE_PASSWORD_USER);
                    Sound_Insert_Number(1, VOICE_NUM_2);
                    Sound_Insert_Number(1, VOICE_CARD_USER);
                    Sound_Insert_Number(1, VOICE_NUM_3);
                    Sound_Insert_Number(1, VOICE_FINGERPRINT_USER);
                    Sound_Insert_Number(1, VOICE_NUM_4);
                    Sound_Insert_Number(1, VOICE_SYSTEM_CONFIG);

                    device_t.openFailCnt = 0;
                    device_t.enterRootFailCnt = 0;
                    finger_led_control(3, 2, 2, 0); // 绿色常亮
                    tx_thread_sleep(1500);
                    finger_led_control(3, 0, 0, 0); // 关闭灯光
                } else {
                    goto __fingerprint_fail;
                }

                // 写入操作记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.enterRoot = 1;
                exc_lock_info_t.type = 1;
                exc_lock_info_t.number = userNum;
                if (userNum < 100 || userNum == 0xFF)
                    exc_lock_info_t.u_exec.unlockSuccess = 1;
                else
                    exc_lock_info_t.u_exec.unlockFail = 1;
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            } else {
                userNum = 0xFF;
                DEBUG_TRACE(WARN_TAG, "Exec Fail, Finger Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                /* 语音 */
                // 操作失败
                // 请输入管理员信息
                Sound_Insert_Number(0, VOICE_EXCE_FAIL);
                if (++device_t.enterRootFailCnt >= 5) {
                    device_t.enterRootFailCnt = 0;
                    device_t.lockTimeout = 100 * SEC_DELAY;

                    /* 语音 */
                    // 系统已锁定
                    Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                    finger_led_control(3, 0, 0, 0); // 关闭灯光
                } else {
                    Sound_Insert_Number(1, VOICE_PLS_INPUT_ROOT_INFO);
                }
                finger_led_control(3, 4, 4, 0); // 红色常亮
                tx_thread_sleep(1500);
                if (!device_t.lockTimeout)
                    finger_led_control(3, 1, 1, 0); // 蓝色常亮
                else
                    finger_led_control(3, 0, 0, 0); // 关闭灯光

                // 写入操作记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.enterRoot = 1;
                exc_lock_info_t.type = 1;
                exc_lock_info_t.number = 0xFF;
                exc_lock_info_t.u_exec.unlockFail = 1;
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }
        } else {
            if (coded_lock_t.total_num_finger == 0) {
                device_t.openFailCnt++;
                userNum = 0xFF;
                action = 0xC1; // 指纹报警
                DEBUG_TRACE(WARN_TAG, "Fingerprint Not Match!!!");
                if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;

                /* 语音 */
                // 失败提示音
                Sound_Insert_Number(0, VOICE_TONE_FAIL);

                if (device_t.openFailCnt >= 5) {
                    device_t.lockTimeout = 100 * SEC_DELAY;
                    DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                    /* 语音 */
                    // 系统已锁定
                    Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                } else {
                    /* 语音 */
                    // 开锁失败
                    Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
                }

                finger_led_control(3, 4, 4, 0); // 红色常亮
                tx_thread_sleep(1500);
                if (!device_t.lockTimeout)
                    finger_led_control(3, 1, 1, 0); // 蓝色常亮
                else
                    finger_led_control(3, 0, 0, 0); // 关闭灯光

                memset(buffer, 0, sizeof(buffer));
                sendLen = 7;

                buffer[0] = 0x30;
                buffer[1] = action;  // 操作记录
                buffer[2] = userNum; // 用户编号
                buffer[3] = (Timestamp >> 24) & 0xFF;
                buffer[4] = (Timestamp >> 16) & 0xFF;
                buffer[5] = (Timestamp >> 8) & 0xFF;
                buffer[6] = Timestamp & 0xFF;

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendLen + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(WARN_TAG, "Lwrb Already Full");
                }

                if (action == 0xC1) // 写入开锁失败记录
                {
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockFail = 1;
                    exc_lock_info_t.type = 1;
                    exc_lock_info_t.number = userNum;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
            } else {
            __repeat_search:
                if (repeat_cnt != 0) {
                    ret = get_fingerprint_image(); // 获取图像
                    if (ret != 0)
                        goto __back;
                }

                ret = creat_fingerprint_characteristic(1); // 生成指纹特征
                if (ret != 0)
                    goto __back;
                ret = search_fingerprint(&userNum); // 搜索指纹

                if (ret == ZS_VALIDATION_CODE_EXEC_FINSH && coded_lock_t.finger[userNum].isable) {
                    /* 指纹验证成功 执行开锁 */
                    tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                    action = 0x01;
                    repeat_cnt = 0;
                    finger_led_control(3, 2, 2, 0); // 绿色常亮
                    tx_thread_sleep(1500);          // 成功后强制延时播放语音
                    finger_led_control(3, 0, 0, 0); // 关闭灯光
                } else {
                    // if (repeat_cnt < 3) {
                    //     repeat_cnt++;
                    //     goto __repeat_search;
                    // }
                    repeat_cnt = 0;
                    device_t.openFailCnt++;
                    userNum = 0xFF;
                    action = 0xC1; // 指纹报警
                    DEBUG_TRACE(WARN_TAG, "Fingerprint Not Match!!!");
                    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;

                    /* 语音 */
                    // 失败提示音
                    Sound_Insert_Number(0, VOICE_TONE_FAIL);
                    if (device_t.openFailCnt >= 5) {
                        device_t.lockTimeout = 100 * SEC_DELAY;
                        DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                        /* 语音 */
                        // 系统已锁定
                        Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                    } else {
                        /* 语音 */
                        // 开锁失败
                        Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
                    }

                    finger_led_control(3, 4, 4, 0); // 红色常亮
                    tx_thread_sleep(1500);
                    if (!device_t.lockTimeout)
                        finger_led_control(3, 1, 1, 0); // 蓝色常亮
                    else
                        finger_led_control(3, 0, 0, 0); // 关闭灯光
                }
                memset(buffer, 0, sizeof(buffer));
                sendLen = 7;

                buffer[0] = 0x30;
                buffer[1] = action;  // 操作记录
                buffer[2] = userNum; // 用户编号
                buffer[3] = (Timestamp >> 24) & 0xFF;
                buffer[4] = (Timestamp >> 16) & 0xFF;
                buffer[5] = (Timestamp >> 8) & 0xFF;
                buffer[6] = Timestamp & 0xFF;

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendLen + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(WARN_TAG, "Lwrb Already Full");
                }

                if (action == 0xC1) // 写入开锁失败记录
                {
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockFail = 1;
                    exc_lock_info_t.type = 1;
                    exc_lock_info_t.number = userNum;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                } else if (action == 0x01)                    // 写入开锁成功记录
                {
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockSuccess = 1;
                    exc_lock_info_t.type = 1;
                    exc_lock_info_t.number = userNum;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }

                /* 指纹开锁解除防拆报警*/
                if (device_t.tamperAlarm && action == 0x01) {
                    device_t.tamperAlarm = 0;
                    tamperio_state = READ_FUNC_KEY_STATUS;

                    sendLen = 2;
                    buffer[0] = 0x84;
                    buffer[1] = 0;
                    int free_number = lwrb_get_free(&lwrbBuff_t);
                    if (free_number >= (sendLen + 1)) {
                        lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                        lwrb_write(&lwrbBuff_t, buffer, sendLen);
                        lwrb_write_count++;
                    } else {
                        DEBUG_TRACE(WARN_TAG, "Lwrb Already Full");
                    }

                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.tamperRecover = 1;
                    exc_lock_info_t.type = 1;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                        strcat(exc_lock_info_t.word, "123456");
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
            }
        }
    }
}
#endif