
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "string.h"
#include <stdlib.h>

#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "qmc5883l.h"
#include "rkb1145d.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include "upgrade_crc.h"

/* Compile-time layout pin: the sizeof(struct)-2 CRC idiom in
 * FLASH_Write/Read_LoRaWAN_Cfg requires crc16 to end the struct with no
 * tail padding (4 double words = 32 bytes). */
typedef char lorawan_cfg_size_check[(sizeof(struct LoRaWANLinkCfg) == 32) ? 1 : -1];

/**
 * @brief  FLASH 字节写入
 * @param   Address：待写入FLASH首地址
 * @param   Data：待写入源数据
 * @param   Len：待写入数据长度
 * @retval  写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_NDoubleWord(u32 Address, uint64_t *Data, int Len)
{
    HAL_StatusTypeDef status = HAL_OK;
    u32 writeADDR = Address;
    for (int i = 0; i < Len; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, writeADDR, Data[i]);
        if (status != HAL_OK) {
            DEBUG_TRACE(FLASH_TAG, "Write failed at address: 0x%08X, data: 0x%016llX", writeADDR, Data[i]);
            return ERROR;
        }
        // 增加延时以确保数据稳定
        HAL_Delay(1);
        if (*(__IO uint64_t *)writeADDR != Data[i]) {
            DEBUG_TRACE(FLASH_TAG, "Verification failed at address: 0x%08X, expected: 0x%016llX, actual: 0x%016llX", writeADDR, Data[i],
                        *(__IO uint64_t *)writeADDR);
            return ERROR;
        }
        writeADDR = writeADDR + 8;
    }
    return SUCCESS;
}

/**
 * @brief  FLASH 字节读取
 * @param   Address：待读取FLASH首地址
 * @param   Data：待读取源数据
 * @param   Len：待读取数据长度
 * @retval
 */
void FLASH_Read_NDoubleWord(u32 Address, uint64_t *Data, int Len)
{
    u32 readADDR = Address;
    for (int i = 0; i < Len; i++) {
        Data[i] = *(__IO uint64_t *)readADDR;
        readADDR = readADDR + 8;
    }
}

/**
 * @brief  写入工作模式
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Park_Mode(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_PARK_MODE_ADDR;
    uint64_t buffer = device_t.park_mode;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, &buffer, 1);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取工作模式
 * @param
 * @retval
 */
void FLASH_Read_Park_Mode(void)
{
    uint64_t buffer;
    u32 Address = FLASH_PARK_MODE_ADDR;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    device_t.park_mode = buffer;
    if (device_t.park_mode > 2) // 数据出错，超出范围(0/1/2)
        device_t.park_mode = 0;
    DEBUG_TRACE(FLASH_TAG, "Read Park Mode : %d", device_t.park_mode);
}

/**
 * @brief  写入开机状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Park_Power(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_PARK_POWER_ADDR;
    uint64_t buffer = device_t.power_on;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, &buffer, 1);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取开机状态
 * @param
 * @retval
 */
void FLASH_Read_Park_Power(void)
{
    uint64_t buffer;
    u32 Address = FLASH_PARK_POWER_ADDR;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    if (buffer > 1) // 数据出错，超出范围
        device_t.power_on = 0;
    else
        device_t.power_on = buffer;
    DEBUG_TRACE(FLASH_TAG, "Read Park Power: %d", device_t.power_on);
}

/**
 * @brief  写入LoRaWAN链路配置（0xA0协议持久化，KERMIT CRC随结构体末字段）
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_LoRaWAN_Cfg(void)
{
    ErrorStatus status;
    u32 Address = FLASH_LORAWAN_CFG_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    uint64_t buffer[4] = {0}; /* 32B struct = 4 double words exactly */

    lorawanCfg.crc16 = upg_crc16((const u8 *)&lorawanCfg, sizeof(lorawanCfg) - 2U);
    memcpy(buffer, &lorawanCfg, sizeof(lorawanCfg));

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, buffer, 4);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取LoRaWAN链路配置，页面空白/校验失败/字段非法时回落编译期默认值（不回写）
 * @param
 * @retval
 */
void FLASH_Read_LoRaWAN_Cfg(void)
{
    struct LoRaWANLinkCfg tmp;
    uint64_t buffer[4] = {0};

    FLASH_Read_NDoubleWord(FLASH_LORAWAN_CFG_ADDR, buffer, 4);
    memcpy(&tmp, buffer, sizeof(tmp));

    if (tmp.magic != LORAWAN_CFG_MAGIC || upg_crc16((const u8 *)&tmp, sizeof(tmp) - 2U) != tmp.crc16 || !LoRaWAN_CfgValidate(&tmp)) {
        LoRaWAN_CfgSetDefaults();
        DEBUG_TRACE(FLASH_TAG, "LoRaWAN Cfg: use defaults band[%d] rx2[%d,%d]", lorawanCfg.band, lorawanCfg.rx2_dr, lorawanCfg.rx2_freq);
    } else {
        memcpy(&lorawanCfg, &tmp, sizeof(lorawanCfg));
        DEBUG_TRACE(FLASH_TAG, "LoRaWAN Cfg: band[%d] dr[%d] power[%d] port[%d] class[%d] %s", lorawanCfg.band, lorawanCfg.dr,
                    lorawanCfg.power, lorawanCfg.port, lorawanCfg.class, lorawanCfg.activation ? "ABP" : "OTAA");
    }
}

/**
 * @brief  写入上报周期（分钟）
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
/* ---- 电量计数环形槽（与上报周期同页，2026-11-14 用户裁决） ----
 * 槽 8B（一个 L4 双字）：byte0=magic(0xB1) byte1=seq(u8 环绕) byte2..5=capacity(u32 LE)
 * byte6..7=sum(u16，前 6 字节累加和)。读=扫全槽取 seq 环绕最新且校验通过者；写=找空槽
 * （0xFF 双字）直接编程，槽满才擦页（页重灌：周期+槽0）。读写异常一律回退默认 100%。 */
#define BATTERY_SLOT_MAGIC 0xB1

static uint64_t battery_slot_pack(u8 seq, u32 cap)
{
    u16 sum = BATTERY_SLOT_MAGIC + seq + (cap & 0xFF) + ((cap >> 8) & 0xFF) + ((cap >> 16) & 0xFF) + ((cap >> 24) & 0xFF);
    return (uint64_t)BATTERY_SLOT_MAGIC | ((uint64_t)seq << 8) | ((uint64_t)cap << 16) | ((uint64_t)sum << 48);
}

/* 解包并校验；OK 返回 1 */
static u8 battery_slot_unpack(uint64_t dw, u8 *seq, u32 *cap)
{
    u8 s = (dw >> 8) & 0xFF;
    u32 c = (u32)(dw >> 16);
    u16 sum = (dw >> 48) & 0xFFFF;
    if ((dw & 0xFF) != BATTERY_SLOT_MAGIC)
        return 0;
    if (battery_slot_pack(s, c) != dw)
        return 0; /* sum 复算不符（含掉电写坏） */
    *seq = s;
    *cap = c;
    return 1;
}

/* 擦电量/周期共用页并重灌：页首周期 + 槽0=当前电量计数（seq 从 bestSeq+1 续） */
static ErrorStatus battery_page_rewrite(u8 nextSeq)
{
    ErrorStatus status;
    uint64_t buffer = device_t.report_interval;
    uint64_t slot = battery_slot_pack(nextSeq, batteryInitialCapacity);
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (FLASH_REPORT_INTERVAL_ADDR - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(FLASH_REPORT_INTERVAL_ADDR, &buffer, 1);
    FLASH_Write_NDoubleWord(FLASH_BATTERY_SLOT_ADDR, &slot, 1);
    HAL_FLASH_Lock();
    batterySavedCapacity = batteryInitialCapacity;
    return status;
}

/**
 * @brief  落盘电量计数（环形槽推进；判脏由调用方负责）
 * @param
 * @retval 写入状态
 */
ErrorStatus FLASH_Write_Battery(void)
{
    u32 addr = 0, slotAddr, cap;
    uint64_t dw;
    u8 seq, bestSeq = 0, haveValid = 0;
    int i;

    for (i = 0; i < FLASH_BATTERY_SLOT_NUM; i++) {
        slotAddr = FLASH_BATTERY_SLOT_ADDR + (u32)i * 8;
        FLASH_Read_NDoubleWord(slotAddr, &dw, 1);
        if (dw == 0xFFFFFFFFFFFFFFFFULL) { /* 空槽 */
            if (addr == 0)
                addr = slotAddr;
            continue;
        }
        if (battery_slot_unpack(dw, &seq, &cap) && (!haveValid || (u8)(seq - bestSeq) < 0x80)) {
            bestSeq = seq;
            haveValid = 1;
        } else if (addr == 0) { /* 坏槽视作空位复用 */
            addr = slotAddr;
        }
    }
    if (addr == 0) /* 槽满：擦页重灌（周期+槽0，seq 续号） */
        return battery_page_rewrite(bestSeq + 1);

    dw = battery_slot_pack(bestSeq + 1, batteryInitialCapacity);
    HAL_FLASH_Unlock();
    FLASH_Write_NDoubleWord(addr, &dw, 1);
    HAL_FLASH_Lock();
    batterySavedCapacity = batteryInitialCapacity;
    return SUCCESS;
}

/**
 * @brief  上电恢复电量计数；槽区全无效（全新设备/读取异常）回退默认 100%（计数 0）
 * @param
 * @retval
 */
void FLASH_Read_Battery(void)
{
    uint64_t dw;
    u8 seq, bestSeq = 0;
    u32 cap, bestCap = 0;
    u8 haveValid = 0;
    int i;

    for (i = 0; i < FLASH_BATTERY_SLOT_NUM; i++) {
        FLASH_Read_NDoubleWord(FLASH_BATTERY_SLOT_ADDR + (u32)i * 8, &dw, 1);
        if (dw == 0xFFFFFFFFFFFFFFFFULL)
            continue;
        if (battery_slot_unpack(dw, &seq, &cap) && (!haveValid || (u8)(seq - bestSeq) < 0x80)) {
            bestSeq = seq;
            bestCap = cap;
            haveValid = 1;
        }
    }
    if (haveValid) {
        batteryInitialCapacity = bestCap;
        batterySavedCapacity = bestCap;
        DEBUG_TRACE(FLASH_TAG, "Battery Restored: used %u, seq %d", bestCap, bestSeq);
    } else {
        batteryInitialCapacity = 0;                 /* 默认 100% */
        batterySavedCapacity = 0xFFFFFFFF;          /* 本次上电尚未落盘 */
        DEBUG_TRACE(FLASH_TAG, "Battery Slots Invalid, Default 100%%");
    }
}

ErrorStatus FLASH_Write_Report_Interval(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_REPORT_INTERVAL_ADDR;
    uint64_t buffer = device_t.report_interval;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, &buffer, 1);
    HAL_FLASH_Lock();
    /* 共用页被擦，电量槽需重灌当前值，否则周期写入会抹掉电量历史 */
    FLASH_Write_Battery();
    return status;
}

/**
 * @brief  读取上报周期（分钟）
 * @param
 * @retval
 */
void FLASH_Read_Report_Interval(void)
{
    uint64_t buffer;
    u32 Address = FLASH_REPORT_INTERVAL_ADDR;

    FLASH_Read_NDoubleWord(Address, &buffer, 1);
    if ((buffer >= 1 && buffer <= 1440) || buffer == 0) {
        device_t.report_interval = buffer;
    } else { // 数据出错，超出范围
        device_t.report_interval = 1440; // 默认间隔1440分钟
    }
    DEBUG_TRACE(FLASH_TAG, "Read Flash Device Report Interval : %dmin", device_t.report_interval);
}

/**
 * @brief  写入地磁参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Mag_Param(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_MAG_PARAM_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    HAL_FLASH_Unlock();
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    uint64_t buffer[10] = {0};
    buffer[0] = qmc5883l_t.offset.x;
    buffer[1] = qmc5883l_t.offset.y;
    buffer[2] = qmc5883l_t.offset.z;
    buffer[3] = qmc5883l_t.threshold.x;
    buffer[4] = qmc5883l_t.threshold.y;
    buffer[5] = qmc5883l_t.threshold.z;
    buffer[6] = qmc5883l_t.is_calibration;
    buffer[7] = qmc5883l_t.state;
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

ErrorStatus FLASH_Write_Mag_Param_Back(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_MAG_PARAM_BACK_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    HAL_FLASH_Unlock();
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    uint64_t buffer[10] = {0};
    buffer[0] = qmc5883l_t.offset.x;
    buffer[1] = qmc5883l_t.offset.y;
    buffer[2] = qmc5883l_t.offset.z;
    buffer[3] = qmc5883l_t.threshold.x;
    buffer[4] = qmc5883l_t.threshold.y;
    buffer[5] = qmc5883l_t.threshold.z;
    buffer[6] = qmc5883l_t.is_calibration;
    buffer[7] = qmc5883l_t.state;
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取地磁参数
 * @param
 * @retval
 */
void FLASH_Read_Mag_Param(void)
{
    u32 Address = FLASH_MAG_PARAM_ADDR;
    uint64_t buffer[10] = {0};
    FLASH_Read_NDoubleWord(Address, buffer, 10);
    qmc5883l_t.offset.x = buffer[0];
    qmc5883l_t.offset.y = buffer[1];
    qmc5883l_t.offset.z = buffer[2];
    qmc5883l_t.threshold.x = buffer[3];
    qmc5883l_t.threshold.y = buffer[4];
    qmc5883l_t.threshold.z = buffer[5];
    qmc5883l_t.is_calibration = buffer[6];
    qmc5883l_t.state = buffer[7];
    // 数据出错，超出范围
    if (qmc5883l_t.state > MAGNETOMETER_IN_WORK || qmc5883l_t.threshold.x < MAGNETOMETER_THRESHOLD_X / 2 ||
        qmc5883l_t.threshold.y < MAGNETOMETER_THRESHOLD_Y / 2 || qmc5883l_t.threshold.z < MAGNETOMETER_THRESHOLD_Z / 2) {

        // 读取异常时读备份区域数据
        Address = FLASH_MAG_PARAM_BACK_ADDR;
        FLASH_Read_NDoubleWord(Address, buffer, 10);
        qmc5883l_t.offset.x = buffer[0];
        qmc5883l_t.offset.y = buffer[1];
        qmc5883l_t.offset.z = buffer[2];
        qmc5883l_t.threshold.x = buffer[3];
        qmc5883l_t.threshold.y = buffer[4];
        qmc5883l_t.threshold.z = buffer[5];
        qmc5883l_t.is_calibration = buffer[6];
        qmc5883l_t.state = buffer[7];
        if (qmc5883l_t.state > MAGNETOMETER_IN_WORK || qmc5883l_t.threshold.x < MAGNETOMETER_THRESHOLD_X / 2 ||
            qmc5883l_t.threshold.y < MAGNETOMETER_THRESHOLD_Y / 2 || qmc5883l_t.threshold.z < MAGNETOMETER_THRESHOLD_Z / 2) {

            memset(&qmc5883l_t, 0, sizeof(qmc5883l_t));
            qmc5883l_t.threshold.x = MAGNETOMETER_THRESHOLD_X;
            qmc5883l_t.threshold.y = MAGNETOMETER_THRESHOLD_Y;
            qmc5883l_t.threshold.z = MAGNETOMETER_THRESHOLD_Z;
            qmc5883l_t.is_calibration = 0;
            qmc5883l_t.state = MAGNETOMETER_IN_IDLE;
        } else {
            // 读取备份区域数据成功，写入正常区域
            FLASH_Write_Mag_Param();
        }
    }
    qmc5883l_t.is_changed = 0;
    DEBUG_TRACE(FLASH_TAG, "Read Mag Param ");
    INFO("[%d, %d, %d] Offset XYZ, [%d, %d, %d] Threshold, Calibration[%d]", qmc5883l_t.offset.x, qmc5883l_t.offset.y, qmc5883l_t.offset.z,
         qmc5883l_t.threshold.x, qmc5883l_t.threshold.y, qmc5883l_t.threshold.z, qmc5883l_t.is_calibration);
}

/**
 * @brief  写入雷达参数
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Rkb_Param(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_RKB_PARAM_ADDR;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct;
    HAL_FLASH_Unlock();
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    uint64_t buffer[10] = {0};
    for (int i = 0; i < 10; i++) {
        buffer[i] = rkb1145d_t.base_power[i];
    }
    status = FLASH_Write_NDoubleWord(Address, buffer, 10);
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  读取雷达参数
 * @param
 * @retval
 */
void FLASH_Read_Rkb_Param(void)
{
    u32 Address = FLASH_RKB_PARAM_ADDR;
    uint64_t buffer[10] = {0};

    FLASH_Read_NDoubleWord(Address, buffer, 10);
    for (int i = 0; i < 10; i++) {
        rkb1145d_t.base_power[i] = buffer[i];
    }
    if (rkb1145d_t.base_power[0] >= 0xFFFF || rkb1145d_t.base_power[1] >= 0xFFFF) // 数据出错，超出范围
        memset(&rkb1145d_t, 0, sizeof(rkb1145d_t));

    u8 max_num;
    u16 max_power = 0;
    DEBUG_TRACE(FLASH_TAG, "Read RD Base Power[");
    for (u8 i = 0; i < 10; i++) {
        INFO(" %d", rkb1145d_t.base_power[i]);
        if (max_power < rkb1145d_t.base_power[i]) {
            max_power = rkb1145d_t.base_power[i];
            max_num = i;
        }
    }
    u16 max_power_distance = (max_num + 1) * 10;
    INFO("], Max Power[%d] Distance[%d]", max_power, max_power_distance);
}

/**
 * @brief  写入更新状态
 * @param
 * @retval 写入状态 1：成功 0：失败
 */
ErrorStatus FLASH_Write_Update_Flag(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_USER_END_ADDR - FLASH_ONE_PAGE_SIZE;
    uint64_t buffer = 0x12345678;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    status = FLASH_Write_NDoubleWord(Address, &buffer, 1);
    HAL_FLASH_Lock();

    return status;
}

/**
 * @brief  清除更新状态
 * @param
 * @retval 清除状态 1：成功 0：失败
 */
ErrorStatus FLASH_Clear_Update_Flag(void)
{
    ErrorStatus status = SUCCESS;
    u32 Address = FLASH_USER_END_ADDR - FLASH_ONE_PAGE_SIZE;
    u32 PAGEError;
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks = FLASH_BANK_1;
    EraseInitStruct.Page = (Address - 0x08000000) / (2 * 1024);
    EraseInitStruct.NbPages = 1;
    HAL_FLASH_Unlock();
    while (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
        ;
    HAL_FLASH_Lock();
    return status;
}

/**
 * @brief  重设系统默认参数
 * @param
 * @retval
 */
void FLASH_Reset_Param(void)
{
    device_t.report_interval = 1440;
    memset(&rkb1145d_t, 0, sizeof(rkb1145d_t));
    memset(&qmc5883l_t, 0, sizeof(qmc5883l_t));

    /* 参数写入FLASH */
    FLASH_Write_Report_Interval();
    FLASH_Write_Mag_Param();
    FLASH_Write_Mag_Param_Back();
    FLASH_Write_Rkb_Param();

    qmc5883l_t.threshold.x = MAGNETOMETER_THRESHOLD_X;
    qmc5883l_t.threshold.y = MAGNETOMETER_THRESHOLD_Y;
    qmc5883l_t.threshold.z = MAGNETOMETER_THRESHOLD_Z;
}

/**
 * @brief  读取系统默认参数
 * @param
 * @retval
 */
void FLASH_Read_Param(void)
{
    INFO("\n************************************************************\n");
    INFO("************************************************************\n");
    INFO("*************** HKT : Parking Sensor ***************\n");
    INFO("******** HARDWARE_VER: 0x%02X ,SOFTWARE_VER: 0x%02X ********\n", HARDWARE_VER, SOFTWARE_VER);

    FLASH_Read_Report_Interval();
    FLASH_Read_Park_Mode();
    FLASH_Read_Park_Power();
    FLASH_Read_Mag_Param();
    FLASH_Read_Rkb_Param();
    FLASH_Read_LoRaWAN_Cfg();
    FLASH_Clear_Update_Flag();

    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));

    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));

    /* 电量计数从环形槽恢复（槽区全无效/读取异常回退默认 100%），再按恢复值重算百分比 */
    FLASH_Read_Battery();
    ADC_BatterySglConvert();
    if (device_t.power_on) {
        LPTIM1_Init();
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
    }
    factoryMode = 0;
    device_t.ble_connected = BLE_CONNECT_STA;

    device_t.ble_wake_event = 0;
    DEBUG_TRACE(LOG_TAG, "BLE Connect State %d", device_t.ble_connected);
}
