
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "W25X40CLSNIG.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "sound.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

u32 temp_flashbuf[(FLASH_ONE_PAGE_SIZE * 4) / 4];

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
        *(volatile uint32_t *)Address = *Data++;
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
        Data[i]  = *(__IO u32 *)readADDR;
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
    u32 Address   = FLASH_DEVEUI_APPEUI_APPKEY_ADDR;

    FLASH_Read_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));

    u32 *src = buffer;
    int  t, i = 0;

    u8 *dfu = LoRaWAN.DevEUI;
    for (t = 0; t < sizeof(LoRaWAN.DevEUI); t += 4) {
        dfu[t]     = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.AppEUI;
    for (t = 0; t < sizeof(LoRaWAN.AppEUI); t += 4) {
        dfu[t]     = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.AppKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppKEY); t += 4) {
        dfu[t]     = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    /* 参数未写入 */
    if ((LoRaWAN.AppEUI[0] == 0 || LoRaWAN.AppEUI[0] == 0xFF) && (LoRaWAN.AppKEY[0] == 0 || LoRaWAN.AppKEY[0] == 0xFF)) {
        memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
        memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));
        memset(LoRaWAN.DevEUI, 0, sizeof(LoRaWAN.DevEUI));
    }
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN DevEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN APPEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
    //                LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
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
    u32 Address   = FLASH_DEVADDR_APPSKEY_NWkSKEY_ADDR;

    FLASH_Read_NWord(Address, buffer, sizeof(buffer) / sizeof(u32));

    u32 *src = buffer;
    int  t, i = 0;

    u8 *dfu = LoRaWAN.DevADDR;
    for (t = 0; t < sizeof(LoRaWAN.DevADDR); t += 4) {
        dfu[t]     = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.AppSKEY;
    for (t = 0; t < sizeof(LoRaWAN.AppSKEY); t += 4) {
        dfu[t]     = (src[i] >> 24) & 0xFF;
        dfu[t + 1] = (src[i] >> 16) & 0xFF;
        dfu[t + 2] = (src[i] >> 8) & 0xFF;
        dfu[t + 3] = src[i++] & 0xFF;
        dfu += 4; // 偏移2个字节
    }

    dfu = LoRaWAN.NwkSKEY;
    for (t = 0; t < sizeof(LoRaWAN.NwkSKEY); t += 4) {
        dfu[t]     = (src[i] >> 24) & 0xFF;
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
    //                LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7],
    //                LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
    //    DEBUG_TRACE(FLASH_TAG, "Read Flash LoRaWAN NwkSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
    //                LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7],
    //                LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
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
    LoRaWAN.port  = buffer[0];
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
 * @brief  写入门锁参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Device_Param(void)
{
    u32 Address = FLASH_DEVICE_PARAM_ADDR;
    u32 buffer[10];
    buffer[0] = device_t.reportInterval;
    buffer[2] = sound_t.volume_number; // 音量等级
    buffer[3] = device_t.auto_lock_time;
    buffer[4] = device_t.volume;
    buffer[5] = device_t.isBand;
    u8 timezone;
    if (device_t.timezone == 3.5) {
        timezone = 25;
    } else if (device_t.timezone == 5.5) {
        timezone = 26;
    } else if (device_t.timezone < 13 && device_t.timezone >= 0) {
        timezone = device_t.timezone;
    } else if (device_t.timezone < 24) {
        timezone = -(device_t.timezone - 12);
    }
    buffer[6] = timezone;
    buffer[7] = device_t.network;
    FlashPageErase(Address);
    FLASH_Write_NWord(Address, buffer, 10);
}

/**
 * @brief  读取门锁参数
 * @param
 * @retval
 */
void FLASH_Read_Device_Param(void)
{
    u32 buffer[10] = {0};
    u32 Address    = FLASH_DEVICE_PARAM_ADDR;
    u8  timezone   = 0;

    FLASH_Read_NWord(Address, buffer, 10);
    device_t.reportInterval = buffer[0];
    sound_t.volume_number   = buffer[2];
    device_t.auto_lock_time = buffer[3];
    device_t.volume         = buffer[4];
    device_t.isBand         = buffer[5];
    timezone                = buffer[6];
    device_t.network        = buffer[7];

    if ((device_t.reportInterval > 1440 || device_t.reportInterval < 10) && device_t.reportInterval != 0) // 数据出错，超出范围
        device_t.reportInterval = 1440;
    if (sound_t.volume_number > VOICE_VOLUME8 || sound_t.volume_number < VOICE_VOLUME1) // 数据出错，超出范围
        sound_t.volume_number = VOICE_VOLUME8;
    if (device_t.auto_lock_time < 3 || device_t.auto_lock_time > 30) {
        device_t.auto_lock_time = 5;
    }
    if (device_t.volume > 100) {
        device_t.volume = 100;
    }
    if (device_t.isBand > 1) {
        device_t.isBand = 0;
    }

    if (timezone > 26) {       // 为负数
        device_t.timezone = 8; // 默认东8区
    } else if (timezone == 25) {
        device_t.timezone = 3.5;
    } else if (timezone == 26) {
        device_t.timezone = 5.5;
    } else if (timezone < 13) {
        device_t.timezone = timezone;
    } else {
        device_t.timezone = -(timezone - 12);
    }

    if (device_t.network > 1) {
#if FUNC_DEFAULT_NETWORK_ON
        device_t.network = 1;
#else
        device_t.network = 0;
#endif
    }
    device_t.network = 1;

    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval[%d]min, Volume[%d], Auto Lock Time[%d], Volume[%d], Band[%d], Time Zone[%.1f], Network[%d]",
                device_t.reportInterval, sound_t.volume_number, device_t.auto_lock_time, device_t.volume, device_t.isBand, device_t.timezone, device_t.network);
}

/**
 * @brief  写入更新状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Update_Flag(void)
{
    u32 Address = FLASH_UPDATE_FLAG_ADDR;
    u32 buffer  = 0x12345678;
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
 * @brief  写入复位状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Factory_Flag(void)
{
    u32 Address = FLASH_DEVICE_FACTORY_ADDR;
    u32 buffer  = device_t.factoryFlag;
    FlashPageErase(Address);
    FLASH_Write_NWord(Address, &buffer, 1);
}

/**
 * @brief  读取复位状态
 * @param
 * @retval
 */
void FLASH_Read_Factory_Flag(void)
{
    u32 buffer[2];
    u32 Address = FLASH_DEVICE_FACTORY_ADDR;
    FLASH_Read_NWord(Address, buffer, 1);
    if (buffer[0] == 1)
        device_t.factoryFlag = 1;
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Factory Flag: %d", device_t.factoryFlag);
}

#if FUNC_OPERATIONAL_VERSION_ENABLE

void FLASH_Write_Door_Lock_Coded_Test(void)
{
    struct secret_key_timeliness  key_info  = {0};
    struct secret_card_timeliness card_info = {0};
    u8                            srcHex[8] = {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
    u8                            cardid[]  = {0x5A, 0xCD, 0xF7, 0xE3};
    u8                            cardkey[] = {0x5a, 0xcd, 0xf7, 0xe3, 0x83, 0x08, 0x04, 0x00, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69};

    u8 cardid1[]  = {0xB2, 0x26, 0xE4, 0x22};
    u8 cardkey1[] = {0xB2, 0x26, 0xE4, 0x22, 0x52, 0x08, 0x04, 0x00, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69};

    u8 arg = 0; // 新增
    for (int i = 0; i < MAX_SECRET_TIMELINESS_NUM + 1; i++) {
        key_info.user_num = i;
        key_info.isable   = 1;
        key_info.len      = 8;
        if (i == 100) {
            srcHex[0] = 0x38;
            srcHex[1] = 0x38;
            srcHex[2] = 0x38;
            srcHex[3] = 0x38;
            srcHex[4] = 0x38;
            srcHex[5] = 0x38;
            srcHex[6] = 0x38;
            srcHex[7] = 0x38;
        } else if (i > 90) {
            srcHex[7]++;
        } else if (i > 80) {
            srcHex[6]++;
        } else if (i > 70) {
            srcHex[5]++;
        } else if (i > 60) {
            srcHex[4]++;
        } else if (i > 50) {
            srcHex[3]++;
        } else if (i > 40) {
            srcHex[2]++;
        } else if (i > 30) {
            srcHex[1]++;
        } else if (i > 20) {
            srcHex[0]++;
        }

        for (int j = 0; j < 8; j++) {
            if (srcHex[j] >= 0x3A) {
                srcHex[j] = 0x30;
            }
        }

        memcpy(key_info.word, srcHex, key_info.len);
        key_info.start_timestamp = 1640966400; // 2022-01-01 00:00:00
        key_info.end_timestamp   = 1735660800; // 2025-01-01 00:00:00
        key_info.valid_cnt       = 100;
        DEBUG_TRACE(LOG_TAG, "Key Word Arg[%d] Num[%d] state[%d] Len[%d] [%s] [%d-%d] cnt[%d]",
                    arg, key_info.user_num, key_info.isable, key_info.len, key_info.word,
                    key_info.start_timestamp, key_info.end_timestamp, key_info.valid_cnt);

        u8 index = find_coded_key_num(key_info.user_num, arg); // 查找当前用户编号位置
        if (arg == 0) {                                        // 新增修改
            if (index >= MAX_SECRET_TIMELINESS_NUM + 1) {
                DEBUG_TRACE(ERROR_TAG, "Key Password Len More MAX Error");
            } else {
                DEBUG_TRACE(LOG_TAG, "Key Word Add Index[%d]", index);
                add_coded_key_num(index, &key_info);
            }
        }

        if (i > 0) {
            card_info.user_num = i;
            card_info.isable   = 1;
            if (i == 100) {
                memcpy(card_info.id, cardid1, 4);
                memcpy(card_info.id_key, cardkey1, 16);
            } else {
                memcpy(card_info.id, cardid, 4);
                memcpy(card_info.id_key, cardkey, 16);
            }
            card_info.start_timestamp = 1640966400; // 2022-01-01 00:00:00
            card_info.end_timestamp   = 1735660800; // 2025-01-01 00:00:00
            card_info.valid_cnt       = 100;
            DEBUG_TRACE(LOG_TAG, "Card Arg[%d] Num[%d] state[%d] id[%02X%02X%02X%02X] key[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X] [%d-%d] cnt[%d]",
                        arg, card_info.user_num, card_info.isable, card_info.id[0], card_info.id[1], card_info.id[2], card_info.id[3],
                        card_info.id_key[0], card_info.id_key[1], card_info.id_key[2], card_info.id_key[3], card_info.id_key[4], card_info.id_key[5], card_info.id_key[6], card_info.id_key[7],
                        card_info.id_key[8], card_info.id_key[9], card_info.id_key[10], card_info.id_key[11], card_info.id_key[12], card_info.id_key[13], card_info.id_key[14], card_info.id_key[15],
                        card_info.start_timestamp, card_info.end_timestamp, card_info.valid_cnt);

            u8 index = find_coded_card_num(card_info.user_num, arg); // 查找当前用户编号位置
            if (arg == 0) {                                          // 新增修改
                if (index >= MAX_SECRET_TIMELINESS_NUM) {
                    DEBUG_TRACE(ERROR_TAG, "Card Len More MAX Error");
                } else {
                    DEBUG_TRACE(LOG_TAG, "Card Add Index[%d]", index);
                    add_coded_card_num(index, &card_info);
                }
            }
        }
        IWDT_Clr();
    }
}

/**
 * @brief  写入门锁开锁信息
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Door_Lock_Coded(void)
{
    u32 Address = FLASH_PASSWORD_ADDR;
    u32 bufsize = sizeof(coded_lock_t);
    memset(temp_flashbuf, 0, sizeof(temp_flashbuf));
    memcpy(&temp_flashbuf, &coded_lock_t, bufsize);

    u32 num_pages = (bufsize + MCU_PAGE_SIZE - 1) / MCU_PAGE_SIZE;
    for (u32 i = 0; i < num_pages; i++) {
        FlashPageErase(Address + i * MCU_PAGE_SIZE);
    }

    FLASH_Write_NWord(Address, (u32 *)temp_flashbuf, bufsize / sizeof(u32));

    // print_coded_key_info();
}

/**
 * @brief  读门锁开锁信息
 * @param
 * @retval
 */
void FLASH_Read_Door_Lock_Coded(void)
{
    u32 Address = FLASH_PASSWORD_ADDR;
    u32 bufsize = sizeof(coded_lock_t);
    memset(temp_flashbuf, 0, sizeof(temp_flashbuf));
    FLASH_Read_NWord(Address, (u32 *)temp_flashbuf, bufsize / sizeof(u32) + 1);
    memcpy(&coded_lock_t, &temp_flashbuf, bufsize);
    if (coded_lock_t.total_num_pass > MAX_SECRET_TIMELINESS_NUM + 1 || coded_lock_t.total_num_pass == 0) {
        memset(&coded_lock_t.pass, 0, sizeof(coded_lock_t.pass));
        coded_lock_t.total_num_pass = 0;
    }
    if (coded_lock_t.total_num_card > MAX_SECRET_TIMELINESS_NUM || coded_lock_t.total_num_card == 0) {
        memset(&coded_lock_t.card, 0, sizeof(coded_lock_t.card));
        coded_lock_t.total_num_card = 0;
    }

    // if (coded_lock_t.total_num_bt > MAX_SECRET_TIMELINESS_NUM || coded_lock_t.total_num_bt == 0) {
    //     memset(&coded_lock_t.bt_pass, 0, sizeof(coded_lock_t.bt_pass));
    //     coded_lock_t.total_num_bt = 0;
    // }
    // 读flash 默认清除临时密码
    memset(&coded_lock_t.temp_pass, 0, sizeof(coded_lock_t.temp_pass));
    print_coded_key_info();
}

/**
 * @brief  写入门锁常开模式参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
void FLASH_Write_Open_Mode(void)
{
    u32 Address = FLASH_DEVICE_OPEN_MODE_ADDR;
    u32 bufsize = sizeof(coded_open_mode_t);
    memset(temp_flashbuf, 0, sizeof(temp_flashbuf));
    memcpy(&temp_flashbuf, &coded_open_mode_t, bufsize);
    FlashPageErase(Address);
    FLASH_Write_NWord(Address, (u32 *)temp_flashbuf, bufsize / sizeof(u32));
}

/**
 * @brief  读门锁常开模式参数
 * @param
 * @retval
 */
void FLASH_Read_Open_Mode(void)
{
    u32 Address = FLASH_DEVICE_OPEN_MODE_ADDR;
    u32 bufsize = sizeof(struct coded_lock_open_mode);
    memset(temp_flashbuf, 0, bufsize);
    FLASH_Read_NWord(Address, (u32 *)temp_flashbuf, sizeof(struct coded_lock_open_mode) / sizeof(u32));
    memcpy(&coded_open_mode_t, &temp_flashbuf, sizeof(struct coded_lock_open_mode));
    if (coded_open_mode_t.mode > 3) {
        memset(&coded_open_mode_t, 0, sizeof(coded_open_mode_t));
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Open Mode: %d", coded_open_mode_t.mode);
    free(temp_flashbuf);
}
#endif

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.reportInterval = 1440;
    sound_t.volume_number   = VOICE_VOLUME8;
    device_t.auto_lock_time = 5;
    device_t.volume         = 100;
    device_t.isBand         = 0;
    device_t.timezone       = 8;

#if FUNC_OPERATIONAL_VERSION_ENABLE
    memset(&coded_lock_t, 0, sizeof(struct coded_lock_timeliness));
    memset(&coded_open_mode_t, 0, sizeof(struct coded_lock_open_mode));
    FLASH_Write_Door_Lock_Coded();
    FLASH_Write_Open_Mode();
#endif
    FLASH_Write_Device_Param();
    Sound_Play(sound_t.volume_number);

    convertTimestampToSystime();
    RTC_Init();
}

/**
 * @brief  读取系统默认参数
 * @param
 * @retval
 */
void FLASH_Read_Param(void)
{
    FLASH_Read_Device_Param();
    FLASH_Read_DevEUI_AppEUI_AppKEY();    // 读取出厂写入密钥
    FLASH_Read_DevADDR_AppSKEY_NwkSKEY(); // 读取出厂写入密钥
    FLASH_Read_LoRaWAN_Port_Class();
    FLASH_Read_Factory_Flag();
#if FUNC_OPERATIONAL_VERSION_ENABLE
    // FLASH_Write_Door_Lock_Coded_Test();
    FLASH_Read_Door_Lock_Coded();
    FLASH_Read_Open_Mode();
#endif
}
