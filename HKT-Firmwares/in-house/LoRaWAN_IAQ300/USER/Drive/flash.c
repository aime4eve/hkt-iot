
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "systick.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

bool FlashPageErase(uint32_t address)
{
#define FLS_PAGE_ERASE_KEY0 0x96969696
#define FLS_PAGE_ERASE_KEY1 0xEAEAEAEA
#define FLS_PAGE_ERASE_DATA 0x1234ABCD

    uint16_t i;
    CMU_PERCLK_SetableEx(FLASHCLK, ENABLE);
    CMU_OPCCR2_NVMCKE_Setable(ENABLE);

    FLS_EPCR_ERTYPE_Set(FLS_EPCR_ERTYPE_PAGE);

    FLS_EPCR_EREQ_Set(FLS_EPCR_EREQ_Msk);
    FLS_KEY_Write(FLS_PAGE_ERASE_KEY0);
    FLS_KEY_Write(FLS_PAGE_ERASE_KEY1);
    *(uint32_t *)address = FLS_PAGE_ERASE_DATA;

    while (SET != FLS_ISR_ERD_Chk()) {
        __NOP();
    }
    FLS_ISR_ERD_Clr();

    FLS_KEY_Write(0x00000000);

    CMU_OPCCR2_NVMCKE_Setable(DISABLE);
    CMU_PERCLK_SetableEx(FLASHCLK, DISABLE);
    for (i = 0; i < 128; i++) {
        if (*(uint32_t *)(address + i * 4) != 0xFFFFFFFF) {
            return false;
        }
    }
    return true;

#undef FLS_PAGE_ERASE_KEY0
#undef FLS_PAGE_ERASE_KEY1
#undef FLS_PAGE_ERASE_DATA
}

/**
 * @brief  FLASH 字节写入
 * @param   Address：待写入FLASH首地址
 * @param   Data：待写入源数据
 * @param   Len：待写入数据长度
 * @retval  写入状态 1：成功 0：失败
 */
void FLASH_Write_NWord(uint32_t Address, uint32_t *Data, uint32_t Len)
{
#define FLS_PROG_KEY0 0xA5A5A5A5
#define FLS_PROG_KEY1 0xF1F1F1F1

    CMU_PERCLK_SetableEx(FLASHCLK, ENABLE);
    CMU_OPCCR2_NVMCKE_Setable(ENABLE);

    FLS_ISR_KEYERR_Clr();
    FLS_EPCR_PREQ_Set(FLS_EPCR_PREQ_Msk);
    FLS_KEY_Write(FLS_PROG_KEY0); // 写入flash编程key0
    FLS_KEY_Write(FLS_PROG_KEY1); // 写入flash编程key1

    while (Len--) {
        FLS_EPCR_PREQ_Set(FLS_EPCR_PREQ_Msk);
        *(uint32_t *)Address = *Data;
        Data += 1;
        Address += 4;
        while (FLS_ISR_PRD_Chk() == RESET)
            ;              // 等待写操作完成
        FLS_ISR_PRD_Clr(); // 清除写完成中断标志
    }

    FLS_KEY_Write(0x00000000);

    CMU_OPCCR2_NVMCKE_Setable(DISABLE);
    CMU_PERCLK_SetableEx(FLASHCLK, DISABLE);

#undef FLS_PROG_KEY0
#undef FLS_PROG_KEY1
}

/**
 * @brief  FLASH 字节读取
 * @param   Address：待读取FLASH首地址
 * @param   Data：待读取源数据
 * @param   Len：待读取数据长度
 * @retval
 */
void FLASH_Read_NWord(u32 Address, u32 *Data, int Len)
{
    u32 readADDR = Address;
    for (int i = 0; i < Len; i++) {
        Data[i] = *(__IO u32 *)readADDR;
        readADDR = readADDR + 4;
    }
}

/**
 * @brief  写入 DevEUI AppEUI AppKEY参数到FLASH
 * @param
 * @retval  写入状态 1：成功 0：失败
 */
void FLASH_Write_DevEUI_AppEUI_AppKEY(void)
{
    u32 Address = FLASH_DEVEUI_APPEUI_APPKEY_ADDR;

    int t, i = 0;
    u32 temp;
    u32 buffer[8] = {0};

    u8 *dfu = LoRaWAN.DevEUI;
    for (t = 0; t < sizeof(LoRaWAN.DevEUI); t += 4) {
        temp = (uint16_t)dfu[0] << 24;
        temp += (uint16_t)dfu[1] << 16;
        temp += (uint16_t)dfu[2] << 8;
        temp += (uint16_t)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppEUI;
    for (t = 0; t < sizeof(LoRaWAN.AppEUI); t += 4) {
        temp = (uint16_t)dfu[0] << 24;
        temp += (uint16_t)dfu[1] << 16;
        temp += (uint16_t)dfu[2] << 8;
        temp += (uint16_t)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppKEY); t += 4) {
        temp = (uint16_t)dfu[0] << 24;
        temp += (uint16_t)dfu[1] << 16;
        temp += (uint16_t)dfu[2] << 8;
        temp += (uint16_t)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
}

/**
 * @brief  读取 DevEUI AppEUI AppKEY参数
 * @param
 * @retval
 */
void FLASH_Read_DevEUI_AppEUI_AppKEY(void)
{
    u32 buffer[8] = {0};
    u32 Address = FLASH_DEVEUI_APPEUI_APPKEY_ADDR;

    FLASH_Read_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));

    u32 *src = buffer;
    int t, i = 0;

    u8 *dfu = LoRaWAN.DevEUI;
    for (t = 0; t < sizeof(LoRaWAN.DevEUI); t += 4) {
        dfu[t] = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.AppEUI;
    for (t = 0; t < sizeof(LoRaWAN.AppEUI); t += 4) {
        dfu[t] = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.AppKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppKEY); t += 4) {
        dfu[t] = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    /* 参数未写入 */
    if ((LoRaWAN.AppEUI[0] == 0 || LoRaWAN.AppEUI[0] == 0xFF) && (LoRaWAN.AppKEY[0] == 0 || LoRaWAN.AppKEY[0] == 0xFF)) {
        memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
        memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));
    }
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN DevEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5],
    //                LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN APPEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5],
    //                LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5],
    //                LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11],
    //                LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
}

/**
 * @brief  写入 DevADDR AppSKEY NwkSKEY参数到FLASH
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_DevADDR_AppSKEY_NwkSKEY(void)
{
    u32 Address = FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR;

    int t, i = 0;
    u32 temp;
    u32 buffer[9] = {0};

    u8 *dfu = LoRaWAN.DevADDR;
    for (t = 0; t < sizeof(LoRaWAN.DevADDR); t += 4) {
        temp = (uint16_t)dfu[0] << 24;
        temp += (uint16_t)dfu[1] << 16;
        temp += (uint16_t)dfu[2] << 8;
        temp += (uint16_t)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.AppSKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppSKEY); t += 4) {
        temp = (uint16_t)dfu[0] << 24;
        temp += (uint16_t)dfu[1] << 16;
        temp += (uint16_t)dfu[2] << 8;
        temp += (uint16_t)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    dfu = LoRaWAN.NwkSKEY;
    for (t = 0; t < sizeof(LoRaWAN.NwkSKEY); t += 4) {
        temp = (uint16_t)dfu[0] << 24;
        temp += (uint16_t)dfu[1] << 16;
        temp += (uint16_t)dfu[2] << 8;
        temp += (uint16_t)dfu[3];
        dfu += 4; // 偏移2个字节
        buffer[i++] = temp;
    }

    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
}

/**
 * @brief  读取 DevADDR AppSKEY NwkSKEY参数
 * @param
 * @retval
 */
void FLASH_Read_DevADDR_AppSKEY_NwkSKEY(void)
{
    u32 buffer[9] = {0};
    u32 Address = FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR;

    FLASH_Read_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));

    u32 *src = buffer;
    int t, i = 0;

    u8 *dfu = LoRaWAN.DevADDR;
    for (t = 0; t < sizeof(LoRaWAN.DevADDR); t += 4) {
        dfu[t] = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.AppSKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppSKEY); t += 4) {
        dfu[t] = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.NwkSKEY;
    for (t = 0; t < sizeof(LoRaWAN.NwkSKEY); t += 4) {
        dfu[t] = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    /* 参数未写入 */
    if ((LoRaWAN.DevADDR[0] == 0 || LoRaWAN.DevADDR[0] == 0xFF) && (LoRaWAN.AppSKEY[0] == 0 || LoRaWAN.AppSKEY[0] == 0xFF)) {
        memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
        memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
        memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));
    }

    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN DevADDR: %02x %02x %02x %02x",
    //                LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN AppSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5],
    //                LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7], LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11],
    //                LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN NwkSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5],
    //                LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7], LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11],
    //                LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
}

/**
 * @brief  写入LoRaWAN端口与入网类型
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_LoRaWAN_Port_Class(void)
{
    u32 Address = FLASH_LoRaWAN_PORT_CLASS_ADDR;
    u32 buffer[2];
    buffer[0] = LoRaWAN.port;
    buffer[1] = LoRaWAN.class;

    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
}

/**
 * @brief  读取LoRaWAN端口与入网类型
 * @param
 * @retval
 */
void FLASH_Read_LoRaWAN_Port_Class(void)
{
    u32 buffer[2];
    u32 Address = FLASH_LoRaWAN_PORT_CLASS_ADDR;

    FLASH_Read_NWord(Address, buffer, 2);
    LoRaWAN.port = buffer[0];
    LoRaWAN.class = buffer[1];
    if (LoRaWAN.port > 254 || LoRaWAN.port < 1) // 数据出错，超出范围
    {
        LoRaWAN.port = LoRaWAN_DEFAULT_PORT;
    }
    if (LoRaWAN.class != LoRaWAN_DEFAULT_CLASS) // 数据出错，超出范围
        LoRaWAN.class = LoRaWAN_DEFAULT_CLASS;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device LoRaWAN Port: %d, Class: %s", LoRaWAN.port, LoRaWAN.class ? "Class C" : "Class A");
}

/**
 * @brief  写入上报周期（秒）
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Report_Interval(void)
{
    u32 buffer[8];
    u32 Address = FLASH_REPORT_INTERVAL_ADDR;
    buffer[0] = device_t.reportInterval;
    buffer[1] = device_t.sensorInterval;
    buffer[2] = device_t.sensorIntervalTask;
    buffer[3] = device_t.start_hour;
    buffer[4] = device_t.start_min;
    buffer[5] = device_t.end_hour;
    buffer[6] = device_t.end_min;
    buffer[7] = device_t.pirInterval;

    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
}


/**
 * @brief  读取上报周期（秒）
 * @param
 * @retval
 */
void FLASH_Read_Report_Interval(void)
{
    u32 buffer[8];
    u32 Address = FLASH_REPORT_INTERVAL_ADDR;

    FLASH_Read_NWord(Address, buffer, 8);
    device_t.reportInterval = buffer[0];
    device_t.sensorInterval = buffer[1];

    device_t.sensorIntervalTask = buffer[2];

    device_t.start_hour = buffer[3];
    device_t.start_min = buffer[4];

    device_t.end_hour = buffer[5];
    device_t.end_min = buffer[6];

    device_t.pirInterval = buffer[7];

    if ((device_t.reportInterval > 1440 || device_t.reportInterval < 1) && device_t.reportInterval != 0) // 数据出错，超出范围
        device_t.reportInterval = 1440;

    if (device_t.sensorInterval > 1440 || device_t.sensorInterval < 1) { // 数据出错，超出范围
        device_t.sensorInterval = 60;
    }

    if (device_t.sensorIntervalTask > 1440 || device_t.sensorIntervalTask < 1) { // 数据出错，超出范围
        device_t.sensorIntervalTask = 60;
    }

    if (device_t.pirInterval > 60 || device_t.pirInterval < 1) { // 数据出错，超出范围
        device_t.pirInterval = 3;
    }

    if (device_t.start_hour > 24 || device_t.start_min > 60 || device_t.end_hour > 24 || device_t.end_min > 60) {
        device_t.start_hour = 0;
        device_t.start_min = 0;
        device_t.end_hour = 0;
        device_t.end_min = 0;
    }

    DEBUG_TRACE(FLASH_TAG,
                "Read Flash Device Report Interval: %dmin, Sensor Report Interval: %dmin, Pir Interval: %dmin, Task: %dmin [%02d:%02d-%02d:%02d]",
                device_t.reportInterval, device_t.sensorInterval, device_t.pirInterval, device_t.sensorIntervalTask, device_t.start_hour,
                device_t.start_min, device_t.end_hour, device_t.end_min);
}

/**
 * @brief  写入电子纸显示模式
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_EPD_Show_Mode(void)
{
    u32 Address = FLASH_EPD_SHOW_MODE_ADDR;
    u32 buffer[3];

    buffer[0] = device_t.epdShowMode;
    buffer[1] = device_t.epdOverturnShow;
    buffer[2] = device_t.epdShowTempWay;

    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
}

/**
 * @brief  读取电子纸显示模式
 * @param
 * @retval
 */
void FLASH_Read_EPD_Show_Mode(void)
{
    u32 buffer[3];
    u32 Address = FLASH_EPD_SHOW_MODE_ADDR;

    FLASH_Read_NWord(Address, buffer, 3);
    device_t.epdShowMode = buffer[0];
    device_t.epdOverturnShow = buffer[1];
    device_t.epdShowTempWay = buffer[2];
#if !FUNC_PM2_5
    if (device_t.epdShowMode > 1) // 数据出错，超出范围
        device_t.epdShowMode = 0;
#else
    if (device_t.epdShowMode > 2) // 数据出错，超出范围
        device_t.epdShowMode = 0;
#endif
    if (device_t.epdOverturnShow > 1)
        device_t.epdOverturnShow = 0;

    if (device_t.epdShowTempWay > 1)
        device_t.epdShowTempWay = 0;

    DEBUG_TRACE(FLASH_TAG, "Read Flash Device EPD Show Mode: %d, Overturn: %d, Temperature: %d", device_t.epdShowMode, device_t.epdOverturnShow,
                device_t.epdShowTempWay);
}

/**
 * @brief  写入设备工作模式
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Device_Work_Mode(void)
{
    u32 Address = FLASH_DEVICE_WORK_MODE_ADDR;
    u32 buffer[2];
    buffer[0] = device_t.ledShowMode;
    buffer[1] = device_t.beepEnable;

    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
}

/**
 * @brief  读取设备工作模式
 * @param
 * @retval
 */
void FLASH_Read_Device_Work_Mode(void)
{
    u32 buffer[2];
    u32 Address = FLASH_DEVICE_WORK_MODE_ADDR;

    FLASH_Read_NWord(Address, buffer, 2);
    device_t.ledShowMode = buffer[0];
    device_t.beepEnable = buffer[1];
#if FUNC_PM2_5
    if (buffer[0] > 2) // 数据出错，超出范围
        device_t.ledShowMode = 0;
#else
    if (buffer[0] > 1) // 数据出错，超出范围
        device_t.ledShowMode = 0;
#endif
    if (buffer[1] > 1) // 数据出错，超出范围
        device_t.beepEnable = 0;

    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Led Show Mode: %d, Beep Enable State: %d", device_t.ledShowMode, device_t.beepEnable);
}

/**
 * @brief  写入设备温度偏移值
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Temperature_Offset(void)
{
    u32 Address = FLASH_DEVICE_TEMP_OFFSET_ADDR;
    u32 buffer[2] = {0};
    buffer[0] = device_t.temperature_offset;
    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));
}

/**
 * @brief  读取设备温度偏移值
 * @param
 * @retval
 */
void FLASH_Read_Temperature_Offset(void)
{
    u32 buffer[2] = {0};
    u32 Address = FLASH_DEVICE_TEMP_OFFSET_ADDR;
    FLASH_Read_NWord(Address, buffer, 2);

    int8_t value = (int8_t)buffer[0];
    if (value >= -100 && value <= 100 && buffer[0] != 0xFFFFFFFF) {
        device_t.temperature_offset = value;
    } else {
        device_t.temperature_offset = 0;
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Temperature Offset: %d", device_t.temperature_offset);
}

/**
 * @brief  写入更新状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Update_Flag(void)
{
    u32 Address = FLASH_UPDATE_FLAG_ADDR;
    u32 buffer = 0x12345678;
    FlashPageErase(Address);
    FLASH_Write_NWord(Address, &buffer, 1);
}

/**
 * @brief  读取更新状态
 * @param
 * @retval
 */
void FLASH_Read_Update_Flag(void)
{
    u32 buffer;
    u32 Address = FLASH_UPDATE_FLAG_ADDR;

    FLASH_Read_NWord(Address, &buffer, 1);

    if (buffer != 0x12345678) {
        __disable_irq(); // 关闭全局中断使能

        /* 跳转到应用程序位置 */
        uint32 JumpAddress = *(__IO uint32 *)(FLASH_APP_START_ADDR + 4);
        __set_MSP(*(__IO uint32 *)FLASH_APP_START_ADDR);
        (*(void (*)())JumpAddress)();
    }
}

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.reportInterval = 1440;
    device_t.sensorInterval = 60;
    device_t.sensorIntervalTask = 60;
    device_t.start_hour = 0;
    device_t.start_min = 0;
    device_t.end_hour = 0;
    device_t.end_min = 0;
    device_t.pirInterval = 3;

    device_t.epdShowMode = 0;
    device_t.epdOverturnShow = 0;
    device_t.epdShowTempWay = 0;
    device_t.ledShowMode = 0;
    device_t.beepEnable = 0;
    device_t.temperature_offset = 0;

    FLASH_Write_EPD_Show_Mode();
    FLASH_Write_Device_Work_Mode();
    FLASH_Write_Report_Interval();
    FLASH_Write_Temperature_Offset();
}

/**
 * @brief  读取系统默认参数
 * @param
 * @retval
 */
void FLASH_Read_Param(void)
{
    FLASH_Read_Report_Interval();
    FLASH_Read_DevEUI_AppEUI_AppKEY();    // 读取出厂写入密钥
    FLASH_Read_DevADDR_AppSKEY_NwkSKEY(); // 读取出厂写入密钥
    FLASH_Read_LoRaWAN_Port_Class();
    FLASH_Read_EPD_Show_Mode();
    FLASH_Read_Device_Work_Mode();
    FLASH_Read_Temperature_Offset();
}
