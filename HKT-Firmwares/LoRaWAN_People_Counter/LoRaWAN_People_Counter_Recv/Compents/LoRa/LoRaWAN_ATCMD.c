
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

char *splitBuf[30] = {0};

/**
 * @brief  字符串转16进制
 * @param  str：输入字符串
 * @param  strhex：输出16进制
 * @retval
 */
void cmd_string_to_hex(char *str, unsigned char *strhex)
{
    uint8_t i, cnt = 0;
    char *p = str;             // 直针p初始化为指向str
    uint8_t len = strlen(str); // 获取字符串中的字符个数

    while (*p != '\0') {          // 结束符判断
        for (i = 0; i < len; i++) // 循环判断当前字符是数字还是小写字符还是大写字母
        {
            if ((*p >= '0') && (*p <= '9'))    // 当前字符为数字0~9时
                strhex[cnt] = *p - '0' + 0x30; // 转为十六进制

            if ((*p >= 'A') && (*p <= 'Z'))    // 当前字符为大写字母A~Z时
                strhex[cnt] = *p - 'A' + 0x41; // 转为十六进制

            if ((*p >= 'a') && (*p <= 'z'))    // 当前字符为小写字母a~z时
                strhex[cnt] = *p - 'a' + 0x61; // 转为十六进制

            p++; // 指向下一个字符
            cnt++;
        }
    }
}

/**
 * @brief  16进制转变为16进制字符串(大写字母)
 * @param  hexstr：16进制字符串数据缓冲区
 * @param  hexstr_size：16进制字符串数据缓冲区大小
 * @param  hex：16进制数据
 * @param  hexlen：16进制数据长度
 * @retval 获取的16进制字符串大小
 */
u16 cmd_get_hex2hexstr(u8 *hexstr, u16 hexstr_size, u8 *hex, u16 hexlen)
{
    if (hex == NULL || hexstr == NULL || hexlen == 0 || hexstr_size < (2 * hexlen)) {
        return 0;
    }
    u8 temp[5] = {0};
    u16 i = 0;
    for (i = 0; i < hexlen; i++) {
        memset(temp, 0, sizeof(temp));
        sprintf((char *)temp, "%02X", *(hex + i));
        strcat((char *)hexstr, (const char *)temp);
        // memcpy((hexstr + i * 2),temp,2);
    }
    return (2 * hexlen);
}

/**
 * @brief  16进制字符串转16进制
 * @param  hex：缓冲区(转换后的16进制字符串)
 * @param  hex_size：缓冲区大小
 * @param  hexstr：需要转换的16进制字符串
 * @param  hexstr_len：转换的16进制字符串的大小
 * @retval 转换后16进制数组长度
 */
u16 cmd_get_hexstr2hex(u8 *hex, u16 hex_size, const u8 *hexstr, u16 hexstr_len)
{
    if (hex == NULL || hexstr == NULL || hexstr_len == 0 || hex_size == 0 || hexstr_len % 2 != 0 || hex_size < (hexstr_len / 2))
        return 0;
    u16 offset = 0;
    u16 buf_len = 0;
    for (offset = 0; offset < hexstr_len; offset++) {
        if ((*(hexstr + offset) >= 'a' && *(hexstr + offset) <= 'f'))
            *(hex + buf_len) = 16 * (*(hexstr + offset) - 'a' + 10);
        else if ((*(hexstr + offset) >= 'A' && *(hexstr + offset) <= 'F'))
            *(hex + buf_len) = 16 * (*(hexstr + offset) - 'A' + 10);
        else if ((*(hexstr + offset) >= '0' && *(hexstr + offset) <= '9'))
            *(hex + buf_len) = 16 * (*(hexstr + offset) - '0');
        else
            return 0;
        offset++;
        if ((*(hexstr + offset) >= 'a' && *(hexstr + offset) <= 'f'))
            *(hex + buf_len) = *(hex + buf_len) + *(hexstr + offset) - 'a' + 10;
        else if ((*(hexstr + offset) >= 'A' && *(hexstr + offset) <= 'F'))
            *(hex + buf_len) = *(hex + buf_len) + *(hexstr + offset) - 'A' + 10;
        else if ((*(hexstr + offset) >= '0' && *(hexstr + offset) <= '9'))
            *(hex + buf_len) = *(hex + buf_len) + *(hexstr + offset) - '0';
        else
            return 0;
        buf_len++;
    }
    return buf_len;
}

/**
 * @brief  字符串分割函数
 * @param  src：源字符串的首地址(buf的地址)
 * @param  separator：指定的分割字符
 * @param  dest：接收子字符串的数组
 * @param  num：分割后的子字符串的个数
 * @retval
 */
void cmd_split(char *src, const char *separator, char **dest, int *num)
{
    char *pNext;
    int count = 0;
    if (src == NULL || strlen(src) == 0) // 如果传入的地址为空或长度为0，直接终止
        return;
    if (separator == NULL || strlen(separator) == 0) // 如未指定分割的字符串，直接终止
        return;
    pNext = (char *)strtok(src, separator); // 必须使用(char *)进行强制类型转换(虽然不写有的编译器中不会出现指针错误)
    while (pNext != NULL) {
        *dest++ = pNext;
        ++count;
        pNext = (char *)strtok(NULL, separator); // 必须使用(char *)进行强制类型转换
    }
    *num = count;
}

/**
 * @brief  AT指令执行
 * @param  None
 * @retval None
 */
AT_Rsp_Typedef AT_Exec_SendCMD(const char *cmd, u16 timeout, const char *keyword)
{
    u16 retry = 0;
    u16 time_slice = 100;

    if (getLoRaWANMode() == LoRaWAN_PASSTHROUGH_MODE) {
        /* 模式错误 */
        DEBUG_TRACE(ERROR_TAG, "LoRaWAN Mode ERROR!");
        return AT_ERROR;
    }

    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS) {
        /* 状态错误 */
        DEBUG_TRACE(ERROR_TAG, "LoRaWAN Status ERROR!");
        return AT_ERROR;
    }

    // if (getLoRaWANBusyIOLevel() == FALSE)
    // {
    //     /* 状态错误 */
    //     DEBUG_TRACE(ERROR_TAG, "LoRaWAN Busy IO Level ERROR!");
    //     return AT_ERROR;
    // }

    IWDG_Feed();

    /* 清除缓存数据 */
    memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
    uartLoRa.recvLen = 0;

    /* 发送指令 */
    LoRa_SendCMD(cmd);
    LoRa_SendCMD("\r\n");
    char *value = (char *)uartLoRa.recvData;
    if (strstr((const char *)keyword, "+LINK INFO:") != NULL || strstr((const char *)keyword, "+RTC INFO:") != NULL) {
        value += 7;
    }
    while (strstr((const char *)value, (const char *)keyword) == NULL) {
        if (strstr((const char *)value, "ERROR")) {
            /* 指令执行错误 */
            DEBUG_TRACE(ERROR_TAG, "\"%s\" at cmd exec error", cmd);
            return AT_ERROR;
        } else if (strstr((const char *)value, "BAD PARM")) {
            /* 指令执行参数错误 */
            DEBUG_TRACE(ERROR_TAG, "\"%s\" at cmd exec bad parm", cmd);
            return AT_ERROR;
        }

        if (time_slice * retry > timeout) {
            /* 指令超时 */
            DEBUG_TRACE(WARN_TAG, "\"%s\" at cmd exec timeout", cmd);
            return AT_TIMEOUT;
        }
        LoRa_DelayMs(time_slice);
        retry++;
    }
    DEBUG_TRACE(LOG_TAG, "\"%s\" at cmd exec success", cmd);
    return AT_OK;
}

/**
 * @brief  获取关键字所在行的内容
 * @param  dst_buf：获取的关键字所在行数据缓冲区
 * @param  bufSize：获取的关键字所在行数据缓冲区大小
 * @param  src_buf：被获取的源数据
 * @param  keyword：关键字
 * @retval 获取的数据长度
 */
u16 AT_Get_Keyword_Line(u8 *dst_buf, u16 bufSize, u8 *src_buf, u8 *keyword)
{
    if (dst_buf == NULL || src_buf == NULL) {
        return 0;
    }
    u8 *head = (u8 *)strstr((const char *)src_buf, (const char *)keyword);
    if (head == NULL) {
        return 0;
    }
    u8 *end = (u8 *)strstr((const char *)head, "\r\n");

    if (head != NULL && end != NULL) {
        if (bufSize >= end - head) {
            memcpy((char *)dst_buf, head, end - head);
            return (end - head);
        } else {
            return 0;
        }
    }
    return 0;
}

/**
 * @brief  删除关键字行
 * @param  src_buf：接收缓冲区源数据
 * @param  keyword：等待数据的关键字
 * @retval 获取的数据长度
 */
u16 AT_Remove_Keyword_Line(u8 *buf, u8 *keyword)
{
    u16 lineLength = 0, tailLen = 0;
    if (buf == NULL) {
        return 0;
    }
    u8 *head = (u8 *)strstr((const char *)buf, (const char *)keyword);
    if (head == NULL) {
        return 0;
    }
    u8 *end = (u8 *)strstr((const char *)head, "\r\n") + 2;
    if (head != NULL && end != NULL) {
        lineLength = end - head;
        memset(head, 0, lineLength);
        tailLen = strlen((const char *)end);
        memcpy(head, end, tailLen);
        return lineLength;
    }
    return 0;
}

/**
 * @brief  等待模组返回指令的内容
 * @param  buf：接收缓冲区
 * @param  keyword：等待数据的关键字
 * @param  timeout：等待超时时间
 * @retval
 */
AT_Rsp_Typedef AT_Wait_Rsp_Keyword(u8 *buf, u8 *keyword, u16 timeout)
{
    u16 retry = 0;
    u16 time_slice = 100;
    if (buf == NULL || keyword == NULL) {
        return AT_ERROR;
    }
    while (strstr((const char *)buf, (const char *)keyword) == NULL) {
        if (retry * time_slice > timeout) {
            return AT_TIMEOUT;
        }
        LoRa_DelayMs(time_slice);
        retry++;
    }
    return AT_OK;
}

/*******************************************************************************/
/****************************      AT CMD      ********************************/
/*******************************************************************************/
/**
 * @brief  设备是否响应
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_checkAlive(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT", 5000, "OK")) {
            DEBUG_TRACE(LOG_TAG, "Device Is Alive");
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    DEBUG_TRACE(ERROR_TAG, "Device Is Dead");
    return FALSE;
}

/**
 * @brief  复位模组参数为缺省状态
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_restFactory(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+FACTORY", 1500, "OK")) {
            LoRaWAN.joinState = 0;
            while (getLoRaWANBusyIOLevel() == 0)
                ; // 等待设备重启完成
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  获取模组版本信息
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getVer(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+VER?", 1500, "+VER")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+VER"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  模组DEBUG模式，默认关闭
 * @param  value : 1 开启debug 0关闭debug
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setDebug(u8 value)
{
    u16 retry = 0;
    char at_cmd[] = "AT+DEBUG=1";
    sprintf(at_cmd, "AT+DEBUG=%d", value);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(at_cmd, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  保存模组当前配置信息到EEPROM
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_saveEEPROM(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+SAVE", 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  系统复位 RESET
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_systemReset(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+RESET", 3000, "OK")) {
            /* 清除缓存数据 */
            memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
            uartLoRa.recvLen = 0;
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置串口波特率 BAUD
 * @param  baud: 待设置波特率

 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setBaud(char *baud)
{
    u16 retry = 0;
    char valid[] = "AT+BAUD=115200";
    sprintf(valid, "AT+BAUD=%s", baud);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取串口波特率 BAUD
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getBaud(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+BAUD?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+BAUD:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置串口分帧时间 TIMEOUT
 * @param  timeout: 超时时间 ,取值范围：0 - 65535

 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setBaudTimeout(int timeout)
{
    u16 retry = 0;
    char valid[] = "AT+TIMEOUT=65535";
    sprintf(valid, "AT+TIMEOUT=%d", timeout);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取串口分帧时间 TIMEOUT
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getBaudTimeout(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+TIMEOUT?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+AT+TIMEOUT:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置上行传输类型 CONFIRM
 * @param  dataRate: 默认值为3,取值范围：

 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setConfirm(int type, int sendCnt)
{
    u16 retry = 0;
    char valid[] = "AT+CONFIRM=1,3";
    sprintf(valid, "AT+CONFIRM=%d,%d", type, sendCnt);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取上行传输类型 CONFIRM
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getConfirm(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+CONFIRM?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+CONFIRM:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置激活模式 OTAA
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setOTAA(void)
{
    u16 retry = 0;
    while (retry < 3) {
        // if (AT_OK == AT_Exec_SendCMD("AT+OTAA=1,1", 1500, "OK")) //设置为OTAA激活，开启热启动
        if (AT_OK == AT_Exec_SendCMD("AT+OTAA=1,0", 1500, "OK")) // 设置为OTAA激活，不开启热启动
        {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置激活模式 OTAA
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getOTAA(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+OTAA?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+OTAA:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置激活模式 ABP
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setABP(void)
{
    u16 retry = 0;
    while (retry < 3) {
        // if (AT_OK == AT_Exec_SendCMD("AT+OTAA=0,1", 1500, "OK")) //设置为ABP激活，开启热启动
        if (AT_OK == AT_Exec_SendCMD("AT+OTAA=0,0", 1500, "OK")) // 设置为ABP激活，不开启热启动
        {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 DEVEUI
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getDEVEUI(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+DEVEUI?", 1500, "+DEVEUI:")) // 设置为OTAA激活，开启热启动
        {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+DEVEUI:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData, "+DEVEUI:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&LoRaWAN.DevEUI[i], 1, (const u8 *)splitBuf[i + 1], 2);
                i++;
            }

            DEBUG_TRACE(LOG_TAG, "DevEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
                        LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 DEVEUI
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setDEVEUI(void)
{
    u16 retry = 0;
    char DevEUI_Str[] = "AT+DEVEUI= FF FF FE 00 00 00 00 01, D3 91 01 02 20 10 28 16, 1";
    sprintf(DevEUI_Str, "AT+DEVEUI= %02x %02x %02x %02x %02x %02x %02x %02x, D3 91 01 02 20 10 28 16, 1",
            LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(DevEUI_Str, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 APPEUI
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setAPPEUI(void)
{
    u16 retry = 0;
    char AppEUI_Str[] = "AT+APPEUI= FF FF FE 00 00 00 00 01";
    sprintf(AppEUI_Str, "AT+APPEUI= %02x %02x %02x %02x %02x %02x %02x %02x",
            LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(AppEUI_Str, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 APPEUI
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getAPPEUI(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+APPEUI?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+APPEUI:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData, "+APPEUI:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&LoRaWAN.AppEUI[i], 1, (const u8 *)splitBuf[i + 1], 2);
                i++;
            }

            DEBUG_TRACE(LOG_TAG, "APPEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
                        LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 APPKEY
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setAPPKEY(void)
{
    u16 retry = 0;
    char AppKEY_Str[] = "AT+APPKEY= 2B 7E 15 16 28 AE D2 A6 AB F7 15 88 09 CF 4F 3C,0";
#if 1
    /* 设置为密钥可读模式 */
    sprintf(AppKEY_Str, "AT+APPKEY= %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x,0",
            LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
            LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
#else
    /* 设置为密钥不可读模式 */
    sprintf(AppKEY_Str, "AT+APPKEY= %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x,1",
            LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
            LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
#endif
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(AppKEY_Str, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 APPKEY，该指令只用于OTAA激活方式（ABP方式无该参数）
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getAPPKEY(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+APPKEY?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+APPKEY:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData, "+APPKEY:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&LoRaWAN.AppKEY[i], 1, (const u8 *)splitBuf[i + 1], 2);
                i++;
            }

            DEBUG_TRACE(LOG_TAG, "APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                        LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
                        LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 DEVADDR
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setDEVADDR(void)
{
    u16 retry = 0;
    char DevADDR[] = "AT+DEVADDR= FF FF FE 00";
    sprintf(DevADDR, "AT+DEVADDR= %02x %02x %02x %02x",
            LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(DevADDR, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 DEVADDR
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getDEVADDR(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+DEVADDR?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+DEVADDR:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData, "+DEVADDR:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&LoRaWAN.DevADDR[i], 1, (const u8 *)splitBuf[i + 1], 2);
                i++;
            }

            DEBUG_TRACE(LOG_TAG, "DEVADDR: %02x %02x %02x %02x",
                        LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 APPSKEY
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setAPPSKEY(void)
{
    u16 retry = 0;
    char AppSKEY_Str[] = "AT+APPSKEY= 2B 7E 15 16 28 AE D2 A6 AB F7 15 88 09 CF 4F 3C";

    sprintf(AppSKEY_Str, "AT+APPSKEY= %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
            LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7],
            LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(AppSKEY_Str, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 APPSKEY，该指令只对“秘钥可见模式”有效
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getAPPSKEY(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+APPSKEY?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+APPSKEY:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData, "+APPSKEY:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&LoRaWAN.AppSKEY[i], 1, (const u8 *)splitBuf[i + 1], 2);
                i++;
            }

            DEBUG_TRACE(LOG_TAG, "APPSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                        LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7],
                        LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 NWKSKEY
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setNwkSKEY(void)
{
    u16 retry = 0;
    char AppSKEY_Str[] = "AT+NWKSKEY= 2B 7E 15 16 28 AE D2 A6 AB F7 15 88 09 CF 4F 3C";

    sprintf(AppSKEY_Str, "AT+NWKSKEY= %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
            LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7],
            LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(AppSKEY_Str, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 NWKSKEY，该指令只对“秘钥可见模式”有效
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getNwkSKEY(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+NWKSKEY?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+NWKSKEY:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData, "+NWKSKEY:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&LoRaWAN.NwkSKEY[i], 1, (const u8 *)splitBuf[i + 1], 2);
                i++;
            }

            DEBUG_TRACE(LOG_TAG, "NWKSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                        LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7],
                        LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 PORT
 * @param  port: 端口号,取值范围：0x01~0xFE
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setPORT(int port)
{
    u16 retry = 0;
    char portStr[] = "AT+PORT=20";
    sprintf(portStr, "AT+PORT=%02x", port);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(portStr, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 PORT
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getPORT(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+PORT?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+PORT:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 CSQ
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setCSQ(int freq)
{
    u16 retry = 0;
    char valid[] = "AT+CSQ=0";
    sprintf(valid, "AT+CSQ=%d", freq);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 CSQ，该功能尚未完全实现，仅支持调试打印
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getCSQ(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+CSQ?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+CSQ:"));
            /*
                转换功能未实现
            */
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置设备类型 CLASS
 * @param  class: 0 – ClassA  2 – ClassC
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setClass(int class)
{
    u16 retry = 0;
    char classStr[] = "AT+CLASS=0";
    sprintf(classStr, "AT+CLASS=%d", class);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(classStr, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取设备类型 CLASS
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getClass(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+CLASS?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+CLASS:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 ADR (ADR功能开启后，服务器将根据当前网络状态,优化节点的速率与功率)
 * @param  adr: 0 - ADR不使能  1 - ADR使能 默认值为1
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setADR(int adr)
{
    u16 retry = 0;
    char adrStr[] = "AT+ADR=1";
    sprintf(adrStr, "AT+ADR=%d", adr);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(adrStr, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 ADR
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getADR(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+ADR?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+ADR:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置发送功率 POWER
 * @param  power: 默认值为0,取值范围：
            0 - 20dBm
            1 - 18dBm
            2 - 16dBm
            3 - 14dBm
            4 - 12dBm
            5 - 10dBm
            6 - 8dBm
            7 - 6dBm

 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setPower(int power)
{
    u16 retry = 0;
    char powerStr[] = "AT+POWER=0";
    sprintf(powerStr, "AT+POWER=%d", power);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(powerStr, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取发送功率 POWER
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getPower(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+POWER?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+POWER:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置通信速率 DATARATE
 * @param  dataRate: 默认值为3,取值范围：
            0 - SF12，BW125
            1 - SF11，BW125
            2 - SF10，BW125
            3 - SF9，BW125
            4 - SF8，BW125
            5 - SF7，BW125
            6 – SF7，BW250
            7 – FSK50

 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setDataRate(int dataRate)
{
    u16 retry = 0;
    char valid[] = "AT+DATARATE=0";
    sprintf(valid, "AT+DATARATE=%d", dataRate);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取通信速率 DATARATE
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getDataRate(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+DATARATE?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+DATARATE:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置注册入网信息 JOIN,清除注册上下文，并执行重启模块AT+RESET指令（需要切换至透传模式开始新一轮JOIN）
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setJOIN(void)
{
    u16 retry = 0;
    while (retry < 3) {
        // if (AT_OK == AT_Exec_SendCMD("AT+JOIN=1", 1500, "OK"))
        if (AT_OK == AT_Exec_SendCMD("AT+JOIN=1,1,12,10", 1500, "OK")) // 120秒
        {
            memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
            uartLoRa.recvLen = 0;
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 CSMA(若开启模块CSMA功能，模块每次发送数据前，会先检查空口是否空闲，若检查到空口空闲，则发送数据，否则随机退避后，重新执行上述流程。最大退避次数缺省为4)
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setCSMA(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+CSMA=1", 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 CSMA
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getCSMA(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+CSMA?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+CSMA:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取状态寄存器 Comm_Status_Reg
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getStatus(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+STATUS?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+STATUS0:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 Link Check
 * @param  checkLink: 默认值为0,取值范围：
            0 – 关闭link check
            1 - 执行一次Link Check
            2 - 模块自动在每次上行数据包中携带Link Check指令

 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setCheckLink(int checkLink)
{
    u16 retry = 0;
    char valid[] = "AT+LINK CHECK=1";
    sprintf(valid, "AT+LINK CHECK=%d", checkLink);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 5000, "+LINK INFO:")) {
            DEBUG_TRACE(LOG_TAG, "%s", uartLoRa.recvData);
            if (strstr((const char *)uartLoRa.recvData + 7, "+LINK INFO: 0")) {
                /* 清除缓存数据 */
                memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
                uartLoRa.recvLen = 0;
                return TRUE;
            }
        }
        retry++;
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
        LoRa_DelayMs(1000);
    }
    /* 清除缓存数据 */
    memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
    uartLoRa.recvLen = 0;
    return FALSE;
}

/**
 * @brief  读取 Link Check
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getCheckLink(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+LINK CHECK?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+LINK INFO:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 RX2
 * @param  dataRate: 默认值为0,取值范围：
            0 - DR0 (SF12 BW125)
            1 - DR1 (SF11 BW125)
            2 - DR2 (SF10 BW125)
            3 - DR3 (SF9 BW125)
            4 - DR4 (SF8 BW125)
            5 - DR5 (SF7 BW125)
            6 – DR6 (SF7 BW250) 7 – DR7 (FSK50)
 * @param  rx2: 通信频率
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setRX2(int dataRate, int rx2)
{
    u16 retry = 0;
    char valid[] = "AT+RX2=0,869525000";
    sprintf(valid, "AT+RX2=%d,%d", dataRate, rx2);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 RX2
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getRX2(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+RX2?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+RX2:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 FREQ    若用户产品遵循标准EU868，无需设置该参数;若要兼容非标准EU868，可通过该指令，来满足预期频率表的设定
 * @param  param: 默认值为0,取值范围：
            0 - 清除之前频率配置，重启模块后，以出厂默认参数运行。
            1 - 设置上行频率表，频率表按照根据起始频率与信道带宽,信道个数，自动生成
            2 - 设置上行频率表，单独设置逻辑信道对应的指定频率
            3 - 设置下行频率表，频率表按照根据起始频率与信道带宽,信道个数，自动生成。若未设置该项，则默认下行频率 = 上行频率
 * @param  channel_num:   配置信道数量
 * @param  freq:   配置上行工作频率
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setFREQ(int param, int channel_num, int freq)
{
    u16 retry = 0;
    char valid[] = "AT+FREQ=1,20,868100000";
    sprintf(valid, "AT+FREQ=%d,%d,%d", param, channel_num, freq);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

bool AT_LoRaWAN_setFREQBW(int param, int channel_num, int freq, int bw)
{
    u16 retry = 0;
    char valid[50] = {0};
    sprintf(valid, "AT+FREQ=%d,%d,%d,%d", param, channel_num, freq, bw);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置IN865版本 FREQ
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setFREQ_IN865(void)
{
    u16 retry = 0;
    char valid[] = "AT+FREQ=2,3,865062500,865402500,865985000";
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 1500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 FREQ
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getFREQ(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+FREQ?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+FREQ:"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  设置 band
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setBAND(int band)
{
    u16 retry = 0;
    char valid[12];
    sprintf(valid, "AT+BAND=%d", band);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 2500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 BAND
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getBAND(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+BAND?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+BAND"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  同步时间
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_syncTime(void)
{
    u16 retry = 0;
    char valid[] = "AT+TIMESYNC";
    while (retry < 5) {
        if (AT_OK == AT_Exec_SendCMD(valid, 10000, "+RTC INFO:")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData + 8, "+RTC INFO:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData + 8, "+RTC INFO:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            if (validNum < 6)
                return FALSE;

            u8 time_buf[6];
            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&time_buf[i], 1, (const u8 *)splitBuf[i + 2], 2);
                i++;
            }

            systime.tm_year = 2000 + __LL_RTC_CONVERT_BCD2BIN(time_buf[0]);
            systime.tm_mon = __LL_RTC_CONVERT_BCD2BIN(time_buf[1]) - 1;
            systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(time_buf[2]);
            systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(time_buf[3]);
            systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(time_buf[4]);
            systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(time_buf[5]);
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            /* 清除缓存数据 */
            memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
            uartLoRa.recvLen = 0;
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  获取RTC时间
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getRTC(void)
{
    u16 retry = 0;
    char valid[] = "AT+RTC?";
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 10000, "+RTC:")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+RTC:"));
            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartLoRa.recvData, "+RTC:");
            int validNum = 0, i = 0;
            cmd_split(validStr, " ", splitBuf, &validNum);

            if (validNum < 6)
                return FALSE;

            u8 time_buf[6];
            /* 转换字符串为hex */
            while (validNum--) {
                cmd_get_hexstr2hex(&time_buf[i], 1, (const u8 *)splitBuf[i + 1], 2);
                i++;
            }

            systime.tm_year = 2000 + __LL_RTC_CONVERT_BCD2BIN(time_buf[0]);
            systime.tm_mon = __LL_RTC_CONVERT_BCD2BIN(time_buf[1]) - 1;
            systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(time_buf[2]);
            systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(time_buf[3]);
            systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(time_buf[4]);
            systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(time_buf[5]);
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
            uartLoRa.recvLen = 0;
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(2000);
    }
    return FALSE;
}

/**
 * @brief  设置 CHMASK
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_setCHMASK(int chmask)
{
    u16 retry = 0;
    char valid[20] = {0};
    sprintf(valid, "AT+CHMASK=%04X", chmask);
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD(valid, 2500, "OK")) {
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}

/**
 * @brief  读取 CHMASK
 * @param
 * @retval 指令执行状态 FALSE 执行失败，TRUE 执行成功
 */
bool AT_LoRaWAN_getCHMASK(void)
{
    u16 retry = 0;
    while (retry < 3) {
        if (AT_OK == AT_Exec_SendCMD("AT+CHMASK?", 1500, "OK")) {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartLoRa.recvData, "+CHMASK"));
            return TRUE;
        }
        retry++;
        LoRa_DelayMs(1000);
    }
    return FALSE;
}
