#include "TB_03F.h"
#include "uart.h"
#include "control_center.h"
#include <LoRaWAN_ATCMD.h>
#include <LoRaWAN_APPLY.h>
#include "systick.h"
#include "cJSON.h"
#include "gpio.h"

const char BLETXUUID[] = "484B54424C455F424C45545855554944"; // HKTBLE_BLETXUUID
const char BLERXUUID[] = "484B54424C455F424C45525855554944"; // HKTBLE_BLERXUUID
u8 BLEMAC[6];

/**
 * @brief  AT指令执行
 * @param  None
 * @retval None
 */
static AT_Rsp_Typedef AT_Exec_SendCMD(const char *cmd, u16 timeout, const char *keyword)
{
    u16 retry = 0;
    u16 time_slice = 100;

    /* 清除缓存数据 */
    memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
    uartInfo.recvLen = 0;

    /* 发送指令 */
    Ble_SendCMD(cmd);
    Ble_SendCMD("\r\n");
    char *value = (char *)uartInfo.recvData;
    while (strstr((const char *)value, (const char *)keyword) == NULL)
    {
        if (strstr((const char *)value, "ERROR"))
        {
            /* 指令执行错误 */
            DEBUG_TRACE(ERROR_TAG, "\"%s\" at cmd exec error", cmd);
            return AT_ERROR;
        }
        else if (strstr((const char *)value, "BAD PARM"))
        {
            /* 指令执行参数错误 */
            DEBUG_TRACE(ERROR_TAG, "\"%s\" at cmd exec bad parm", cmd);
            return AT_ERROR;
        }

        if (time_slice * retry > timeout)
        {
            /* 指令超时 */
            DEBUG_TRACE(WARN_TAG, "\"%s\" at cmd exec timeout", cmd);
            return AT_TIMEOUT;
        }
        mDelay(time_slice);
        retry++;
    }
    DEBUG_TRACE(LOG_TAG, "\"%s\" at cmd exec success", cmd);
    return AT_OK;
}

/*******************************************************************************/
/****************************      AT CMD      ********************************/
/*******************************************************************************/
/**
 * @brief  设备是否响应
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_checkAlive(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT", 5000, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "Device Is Alive");
            return true;
        }
        retry++;
        mDelay(1000);
    }
    DEBUG_TRACE(ERROR_TAG, "Device Is Dead");
    return false;
}

/**
 * @brief  复位模组参数为缺省状态
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_restFactory(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+RESTORE", 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  复位重启模组
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_restart(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+RST", 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  配置模组进入睡眠模式
 * @param  mode
 *          0：进入浅睡眠并且下次电不自动进入浅睡眠状态
            1：进入浅睡眠并且下次电自动进入浅睡眠状态
            2：进入深度睡眠模式
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_setSleep(u8 mode)
{
    u16 retry = 0;
    char value[] = "AT+SLEEP=0";
    sprintf(value, "AT+SLEEP=%d", mode);
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD(value, 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  获取模组版本信息
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_getVer(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+GMR", 1500, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "%s", uartInfo.recvData);
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  获取模组蓝牙MAC地址
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_getMac(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+BLEMAC?", 1500, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "%s", strstr((const char *)uartInfo.recvData, "+BLEMAC"));

            /* 获取有效数据 */
            memset(&splitBuf, 0, sizeof(splitBuf));
            char *validStr = strstr((const char *)uartInfo.recvData, "+BLEMAC");
            int validNum = 0, i = 0;
            cmd_split(validStr, ":", splitBuf, &validNum);
            /* 转换字符串为hex */
            for (i = 0; i < 6; i++)
                cmd_get_hexstr2hex(&BLEMAC[i], 1, (const u8 *)splitBuf[1] + 2 * i, 2);

            DEBUG_TRACE(LOG_TAG, "Ble Mac:%02x%02x%02x%02x%02x%02x", BLEMAC[0], BLEMAC[1], BLEMAC[2], BLEMAC[3], BLEMAC[4], BLEMAC[5]);
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  设置 蓝牙名称
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_setName(void)
{
    u16 retry = 0;
    char value[] = "AT+BLENAME=HKT_CST_0392";
    sprintf(value, "AT+BLENAME=HKT_CST_%02X%02X", BLEMAC[4], BLEMAC[5]);
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD(value, 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  获取 蓝牙名称
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_getName(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+BLENAME?", 1500, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "%s", uartInfo.recvData);
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  设置 蓝牙 TX UUID
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_setTxUUID(void)
{
    u16 retry = 0;
    char value[] = "AT+BLETXUUID=16962447C62361BAD94B4D1E43535349";
    sprintf(value, "AT+BLETXUUID=%s", BLETXUUID);
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD(value, 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  设置 蓝牙 RX UUID
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_setRxUUID(void)
{
    u16 retry = 0;
    char value[] = "AT+BLERXUUID=B39B7234BEECD4A8F443418843535349";
    sprintf(value, "AT+BLERXUUID=%s", BLERXUUID);
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD(value, 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  获取蓝牙 TX UUID
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_getTxUUID(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+BLETXUUID?", 1500, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "%s", uartInfo.recvData);
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  获取蓝牙 RX UUID
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_getRxUUID(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+BLERXUUID?", 1500, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "%s", uartInfo.recvData);
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  设置 蓝牙广播使能
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_setAdvEn(u16 isAble)
{
    u16 retry = 0;
    char value[] = "AT+BLEADVEN=1";
    sprintf(value, "AT+BLEADVEN=%d", isAble);
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD(value, 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  设置 蓝牙广播间隔
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_setAdvIntv(u16 interval)
{
    u16 retry = 0;
    char value[] = "AT+BLEADVINTV=1000";
    sprintf(value, "AT+BLEADVINTV=%d", interval);
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD(value, 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  设置 蓝牙MTU
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_setMTU(u16 mtu)
{
    u16 retry = 0;
    char value[] = "AT+BLEMTU=200";
    if (mtu > 250 || mtu < 23)
        return false;
    sprintf(value, "AT+BLEMTU=%d", mtu);
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD(value, 1500, "OK"))
        {
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  获取蓝牙 MTU
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_getMTU(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+BLEMTU?", 1500, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "%s", uartInfo.recvData);
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}

/**
 * @brief  获取 蓝牙连接状态
 * @param
 * @retval 指令执行状态 false 执行失败，true 执行成功
 */
bool AT_Ble_getBleState(void)
{
    u16 retry = 0;
    while (retry < 3)
    {
        if (AT_OK == AT_Exec_SendCMD("AT+BLESTATE?", 1500, "OK"))
        {
            DEBUG_TRACE(LOG_TAG, "%s", uartInfo.recvData);
            u8 *state = strstr("BLESTATE:", uartInfo.recvData) + 9;
            if (state[0] == 0x31)
            {
                if (!device_t.bleConnectState)
                {
                    device_t.bleConnectState = 1;
                    // 发送设备信息给手机 DevEui 时间戳
                    char DevEui[16];
                    snprintf(DevEui, sizeof(DevEui), "%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);

                    cJSON *JsonBuffer = NULL;
                    JsonBuffer = cJSON_CreateObject();
                    cJSON_AddStringToObject(JsonBuffer, "DevEui", DevEui);
                    char ver[] = "v09.11";
                    snprintf(ver, sizeof(ver), "v%2d.%2d", HARDWARE_VER, SOFTWARE_VER);
                    cJSON_AddStringToObject(JsonBuffer, "versionCode", ver);
                    cJSON_AddNumberToObject(JsonBuffer, "timestamp", Timestamp);
                    char *s = cJSON_PrintUnformatted(JsonBuffer);
                    Ble_SendCMD(s);

                    if (s)
                        cJSON_free((void *)s);
                    if (JsonBuffer)
                        cJSON_Delete(JsonBuffer);
                }
            }
            return true;
        }
        retry++;
        mDelay(1000);
    }
    return false;
}


/**
 * @brief  蓝牙模组复位
 * @param
 * @retval
 */
void ble_rst(void)
{
    GPIO_BLE_RST_L;
    mDelay(50);
    GPIO_BLE_RST_H;
    mDelay(50);
}

/**
 * @brief  蓝牙模组初始化
 * @param
 * @retval
 */
void ble_init(void)
{   
    ble_rst();
    while (AT_Ble_checkAlive() == false)
        ;
    AT_Ble_getVer();
    AT_Ble_restFactory();
    AT_Ble_getMac();
    AT_Ble_setName();
    AT_Ble_setTxUUID();
    AT_Ble_setRxUUID();
    AT_Ble_setAdvEn(1);
    AT_Ble_setAdvIntv(1500);
    AT_Ble_setMTU(247);
    AT_Ble_restart();

    AT_Ble_getName();
    AT_Ble_getTxUUID();
    AT_Ble_getRxUUID();
    AT_Ble_setSleep(0);
    memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
}
