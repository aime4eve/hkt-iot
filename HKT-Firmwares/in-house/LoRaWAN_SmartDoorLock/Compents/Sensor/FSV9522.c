#include "fsv9522.h"
#include "SI12T.h"
#include "W25X40CLSNIG.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "mbi5120.h"
#include "sound.h"
#include "spi.h"
#include "systick.h"
#include "uart.h"
#include "zs902r.h"

#if (FSV9522_CARD_ENABLE)

TX_EVENT_FLAGS_GROUP fsv9522_event_group;

void fsv9522_WriteRawRC(u8 Address, u8 value)
{
    u8 ucAddr;
    // CARD_NSS_L;
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_LOW);
    ucAddr = ((Address << 1) & 0x7E);
    Spi1WriteAndRead(ucAddr);
    Spi1WriteAndRead(value);
    // Delay_Us(100);
    // CARD_NSS_H;
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_HIGH);
    // Delay_Us(100);
}

u8 fsv9522_ReadRawRC(u8 Address)
{
    u8 ucAddr, ucResult = 0;
    // CARD_NSS_L;
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_LOW);
    ucAddr = ((Address << 1) & 0x7E) | 0x80;
    Spi1WriteAndRead(ucAddr);
    ucResult = Spi1WriteAndRead(0);
    // CARD_NSS_H;
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_HIGH);
    // Delay_Us(40);
    return ucResult;
}

/***********************************************************************************************
 *1907寄存器指定位置位
 ***********************************************************************************************/
void fsv9522_SetBitMask(u8 reg, u8 mask)
{
    u8 tmp = fsv9522_ReadRawRC(reg);
    fsv9522_WriteRawRC(reg, tmp | mask);
}

/***********************************************************************************************
 *1907寄存器指定位复位
 ***********************************************************************************************/
void fsv9522_ClearBitMask(u8 reg, u8 mask)
{
    u8 tmp = fsv9522_ReadRawRC(reg);
    fsv9522_WriteRawRC(reg, tmp & ~mask);
}

/***********************************************************************************************
 *写LPCD寄存器，输入寄存器地址和要写的值
 ***********************************************************************************************/
void write_Lpcd_reg(u8 reg, u8 value)
{
    fsv9522_WriteRawRC(0x37, 0xa0);
    fsv9522_WriteRawRC(0x0f, reg);
    fsv9522_WriteRawRC(0x37, 0xa1);
    fsv9522_WriteRawRC(0x0f, value);
}
/***********************************************************************************************
 *读取LPCD寄存器，输入寄存器地址,返回读取的值
 ***********************************************************************************************/
u8 read_Lpcd_reg(u8 reg)
{
    u8 value;
    fsv9522_WriteRawRC(0x37, 0xa0);
    fsv9522_WriteRawRC(0x0f, reg);
    fsv9522_WriteRawRC(0x37, 0xa1);
    value = fsv9522_ReadRawRC(0x0f);
    return value;
}

/***********************************************************************************************
 *读取LPCD I通道寄存器值
 ***********************************************************************************************/
u8 Read_I_Channel(void)
{
    u8 i;
    fsv9522_WriteRawRC(0x37, 0xa0);
    fsv9522_WriteRawRC(0x0f, 0x05);
    fsv9522_WriteRawRC(0x37, 0xa1);
    i = fsv9522_ReadRawRC(0x0f); // LPCD_RI
    return i;
}
/***********************************************************************************************
 *读取LPCD Q通道寄存器值
 ***********************************************************************************************/
u8 Read_Q_Channel(void)
{
    u8 q;
    fsv9522_WriteRawRC(0x37, 0xa0);
    fsv9522_WriteRawRC(0x0f, 0x06);
    fsv9522_WriteRawRC(0x37, 0xa1);
    q = fsv9522_ReadRawRC(0x0f); // LPCD_RQ
    return q;
}

/***********************************************************************************************
 *设置LPCD I/Q通道唤醒阈值
 ***********************************************************************************************/
void set_I_Q_threshold(u8 I_value, u8 Q_value)
{
    u8 I_MIN, I_MAX;
    u8 Q_MIN, Q_MAX;
    if ((I_value >= LPCD_Sensitive_Value) && (Q_value >= LPCD_Sensitive_Value)) {
        I_MIN = I_value - LPCD_Sensitive_Value;
        I_MAX = I_value + LPCD_Sensitive_Value;
        Q_MIN = Q_value - LPCD_Sensitive_Value;
        Q_MAX = Q_value + LPCD_Sensitive_Value;
    } else {
        I_MIN = 0x9c;
        I_MAX = 0x9c;
        Q_MIN = 0x7c;
        Q_MAX = 0x7c;
    }

    write_Lpcd_reg(0x01, I_MAX);
    write_Lpcd_reg(0x02, I_MIN);
    write_Lpcd_reg(0x03, Q_MAX);
    write_Lpcd_reg(0x04, Q_MIN);
}

void UpdateCrc_B(u8 bCh, u16 *pLpwCrc)
{
    bCh = (bCh ^ (u8)((*pLpwCrc) & 0x00FFU));
    bCh = (bCh ^ (bCh << 4U));
    *pLpwCrc = (*pLpwCrc >> 8U) ^ ((uint16_t)bCh << 8U) ^ ((u16)bCh << 3U) ^ ((u16)bCh >> 4U);
}

void ComputeCrc_B(u8 *pData, u32 dwLength, u8 *pCrc)
{
    u8 bChBlock = 0;
    u16 wCrc = 0xFFFF;
    do {
        bChBlock = *pData++;
        UpdateCrc_B(bChBlock, &wCrc);
    } while (0u != (--dwLength));
    wCrc = ~wCrc;
    pCrc[0] = (u8)(wCrc & 0xFFU);
    pCrc[1] = (u8)((wCrc >> 8U) & 0xFFU);
}

void fsv9522_FieldOn(void)
{
    u8 n;
    n = fsv9522_ReadRawRC(TxControlReg);
    if (!(n & 0x03))
        fsv9522_WriteRawRC(TxControlReg, n | 0x03);
}

void fsv9522_FieldOff(void)
{
    fsv9522_ClearBitMask(TxControlReg, 0x03);
}

void fsv9522_sleep(void)
{
    fsv9522_WriteRawRC(CommandReg, 0x30);
}

/**
 * @brief  硬复位读卡器
 * @param
 * @retval
 **/
void fsv9522_rst(void)
{
    u8 temp;
    CARD_RST_H;
    tx_thread_sleep(1);
    CARD_RST_L;
    tx_thread_sleep(1);
    CARD_RST_H;
    tx_thread_sleep(5);                                 // delay 500us 请客户确认Delay_Us函数延时准确。
    fsv9522_WriteRawRC(CommandReg, fsv9522_RESETPHASE); // 软复位
    tx_thread_sleep(5);
    temp = fsv9522_ReadRawRC(VersionReg);
#ifdef NFC_DEBUG_PRINTF
    DEBUG_TRACE(LOG_TAG, "FSV9522 Version: %X\n", temp);
#endif
}

/***********************************************************************************************
 *禁止LPCD，若唤醒后读卡没有复位读卡芯片，需要读卡前先禁止LPCD功能
 ***********************************************************************************************/
void fsv9522_LPCD_Stop(void)
{
    fsv9522_WriteRawRC(0x37, 0xa0);
    fsv9522_WriteRawRC(0x0f, 0x00);
    fsv9522_WriteRawRC(0x37, 0xa1);
    fsv9522_WriteRawRC(0x0f, 0x00);
}

/***********************************************************************************************
 *清除读卡芯片LPCD中断标志位
 ***********************************************************************************************/
void Clear_lpcd_irq(void)
{
    fsv9522_WriteRawRC(DivIrqReg, 0x20); // 清中断标记
    // tx_thread_sleep(10);
    Delay_Ms(10);
}

/**********************************************************************************************************
*函数名称：void fsv9522_LPCD_Enter(u16 cycle,u16 duration )
*功能描述：
           设置并进入LPCD模式，有操作顺序要求，不要改动操作顺序。
*输入参数：
           cycle：检测周期，16位，默认0x22ff，380ms，误差约10%；
           duration：检测时长，16位，默认0x001f，2.5us；可选0x003f=5us,0x007f=10us；
*返回参数：
           无
*其它输出：
           IRQ中断，1)可选CMOS输出或开漏输出，开漏输出需外接上拉电阻；2)可选高电平中断或低电平中断；

**********************************************************************************************************/
void fsv9522_LPCD_Enter(u16 cycle, u16 duration)
{
    u8 i, q, n;
    n = fsv9522_ReadRawRC(0x0c);
    fsv9522_WriteRawRC(0x0c, n | 0x10);
    //  fsv9522_WriteRawRC(CWGsPReg,0x01);
    /*set IRQ int level*/
#if (IRQ_LEVEL_SET == IRQ_HIGH_LEVEL)
    fsv9522_WriteRawRC(ComIEnReg, 0x00); // ComIEnReg的bit7 置0，高电平中断
#else
    fsv9522_WriteRawRC(ComIEnReg, 0x80); // ComIEnReg的bit7 置1，低电平中断
#endif

    /*set IRQ Output Mode*/
#if (IRQ_OUT_MODE == IRQ_OUT_PP)
    fsv9522_WriteRawRC(DivlEnReg, 0xE0); // bit7=1设置为CMOS输出，bit5=1 LPCD中断使能
#else
    fsv9522_WriteRawRC(DivlEnReg, 0x60); // bit7=0设置为OPEN DRAIN输出，bit5=1 LPCD中断使能
#endif

    fsv9522_WriteRawRC(DivIrqReg, 0x7F); // 清中断标记

    // 设置检测时长
    write_Lpcd_reg(0x08, (u8)(duration >> 8)); // 时长高位寄存器
    write_Lpcd_reg(0x09, (u8)duration);        // 时长低位寄存器

    // 设置检测周期
    write_Lpcd_reg(0x0a, 0x00); // 检测周期高位寄存器
    write_Lpcd_reg(0x0b, 0xff); // 检测周期低位寄存器

    // 准备启动操作1
    write_Lpcd_reg(0x01, 0xf0); // LPCD操作寄存器
    write_Lpcd_reg(0x02, 0x00);
    write_Lpcd_reg(0x03, 0xf0); // LPCD操作寄存器
    write_Lpcd_reg(0x04, 0x00);

    // 使能LPCD，与启动要分开
    write_Lpcd_reg(0x00, 0x50); // enable LPCD

    // 启动LPCD
    write_Lpcd_reg(0x00, 0xd0); // start LPCD

    // 读取控制寄存器，不是必须的，为了观察
    n = read_Lpcd_reg(0x00);

#ifdef NFC_DEBUG_PRINTF
    INFO("  lpcd_00reg = %02x \n", n);
#endif
    tx_thread_sleep(5);

    // 启动载波并等待一个校准周期
    fsv9522_WriteRawRC(TxControlReg, 0x83);
    tx_thread_sleep(20);

    // 读取I/Q值
    i = Read_I_Channel();
    q = Read_Q_Channel();

#ifdef NFC_DEBUG_PRINTF
    INFO(" lpcd_: I:%02X ,Q:%02X\n", i, q);
#endif
    // 设置唤醒阈值
    set_I_Q_threshold(i, q);

    // 设定检测时长；
    write_Lpcd_reg(0x08, (u8)(duration >> 8)); // 时长高位寄存器
    write_Lpcd_reg(0x09, (u8)duration);        // 时长低位寄存器

    // 设定最终检测周期
    write_Lpcd_reg(0x0a, (u8)(cycle >> 8));
    write_Lpcd_reg(0x0b, (u8)(cycle));
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdComTransceive(struct TranSciveBuffer *pi)
*功能描述： 命令发送函数，各种卡通用。
*输入参数： pi：命令和数据指针；

*返回参数： MI_OK：成功；
           other：失败；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdComTransceive(struct TranSciveBuffer *pi)
{
    s8 status = MI_ERR;
    u8 irqEn, waitFor, n, lastBits;
    u16 i;

    switch (pi->MfCommand) {
    case fsv9522_AUTHENT:
        irqEn = 0x12;
        waitFor = 0x10;
        break;
    case fsv9522_TRANSCEIVE:
        irqEn = 0x77;
        waitFor = 0x30;
        break;
    default:
        break;
    }

    fsv9522_WriteRawRC(CommandReg, fsv9522_IDLE); // 01h
    fsv9522_SetBitMask(FIFOLevelReg, 0x80);       // 0ah,    //FlushFIFO
    fsv9522_WriteRawRC(ComIrqReg, 0x7F);          // 04h

    for (i = 0; i < pi->MfLength; i++)
        fsv9522_WriteRawRC(FIFODataReg, pi->MfData[i]); // 09h

    fsv9522_WriteRawRC(CommandReg, pi->MfCommand); // 01h

    if (pi->MfCommand == fsv9522_TRANSCEIVE)
        fsv9522_SetBitMask(BitFramingReg, 0x80); // StartSend

    //
    for (i = 500; i > 0; i--) {
        n = fsv9522_ReadRawRC(ComIrqReg); // 04h
        if ((n & irqEn & 0x01) || (n & waitFor))
            break; // timerirq idleirq rxirq
    }
    fsv9522_ClearBitMask(BitFramingReg, 0x80); // 0dh        //StartSend

    if (n & waitFor) // IdleIRq
    {
        if (fsv9522_ReadRawRC(ErrorReg) & 0x1B) // 06h
            status = MI_ERR;
        else {
            status = MI_OK;
            if (pi->MfCommand == fsv9522_TRANSCEIVE) {
                n = fsv9522_ReadRawRC(FIFOLevelReg);             // 0ah
                lastBits = fsv9522_ReadRawRC(ControlReg) & 0x07; // 0ch
                if (lastBits)
                    pi->MfLength = (n - 1) * 8 + lastBits;
                else
                    pi->MfLength = n * 8;
                if (n == 0)
                    n = 1;
                if (n > MAXRLEN)
                    n = MAXRLEN;
                for (i = 0; i < n; i++)
                    pi->MfData[i] = fsv9522_ReadRawRC(FIFODataReg); // 09h
            }
        }
    } else if (n & irqEn & 0x01) // TimerIRq
    {
        status = MI_NOTAGERR;
    } else if (!(fsv9522_ReadRawRC(ErrorReg) & 0x1B)) {
        // temp = fsv9522_ReadRawRC(ErrorReg);
        status = MI_ACCESSTIMEOUT;
    }

    fsv9522_SetBitMask(ControlReg, 0x80);         // stop timer now
    fsv9522_WriteRawRC(CommandReg, fsv9522_IDLE); // 01h
    return status;
}

// type-a
s8 fsv9522_JewelTransceive(struct TranSciveBuffer *pi)
{
    s8 status = MI_ERR;
    u8 waitFor, n, lastBits;
    u16 i;
    u8 temp, irqEn;

    irqEn = 0x77;
    waitFor = 0x30;
    fsv9522_WriteRawRC(CommandReg, fsv9522_IDLE);
    fsv9522_SetBitMask(FIFOLevelReg, 0x80);
    fsv9522_WriteRawRC(ComIrqReg, 0x7F);
    fsv9522_WriteRawRC(DivIrqReg, 0x7F);
    fsv9522_WriteRawRC(ManualRCVReg, 0x10);
    for (i = 0; i < pi->MfLength; i++)
        fsv9522_WriteRawRC(FIFODataReg, pi->MfData[i]);

    fsv9522_WriteRawRC(ComIEnReg, irqEn | 0x80);
    fsv9522_WriteRawRC(CommandReg, pi->MfCommand);
    fsv9522_SetBitMask(BitFramingReg, 0x80);
    do {
        temp = fsv9522_ReadRawRC(ComIrqReg); // wait for TxIRQ
    } while ((temp & 0x40) == 0);

    if (pi->MfCommand == fsv9522_TRANSMIT) {
        if (fsv9522_ReadRawRC(ErrorReg) == 0)
            status = MI_OK;
        else
            status = -1;
    }

    if (pi->MfCommand == fsv9522_TRANSCEIVE) {
        fsv9522_WriteRawRC(ManualRCVReg, 0x00);

        for (i = 1000; i > 0; i--) {
            n = fsv9522_ReadRawRC(ComIrqReg); // 04h
            if ((n & irqEn & 0x01) || (n & waitFor))
                break; // timerirq idleirq rxirq
        }

        if (n & waitFor) // IdleIRq
        {
            temp = fsv9522_ReadRawRC(ErrorReg);
            if (temp & 0x1B) // 06h
            {
                status = MI_ERR;
            } else {
                status = MI_OK;
                if (pi->MfCommand == fsv9522_TRANSCEIVE) {
                    n = fsv9522_ReadRawRC(FIFOLevelReg);             // 0ah
                    lastBits = fsv9522_ReadRawRC(ControlReg) & 0x07; // 0ch
                    if (lastBits)
                        pi->MfLength = (n - 1) * 8 + lastBits;
                    else
                        pi->MfLength = n * 8;
                    if (n == 0)
                        n = 1;
                    if (n > MAXRLEN)
                        n = MAXRLEN;
                    for (i = 0; i < n; i++)
                        pi->MfData[i] = fsv9522_ReadRawRC(FIFODataReg); // 09h
                }
            }
        } else if (n & irqEn & 0x01) // TimerIRq
        {
            status = MI_NOTAGERR;
        } else if (!(fsv9522_ReadRawRC(ErrorReg) & 0x1B)) {
            temp = fsv9522_ReadRawRC(ErrorReg);
            status = MI_ACCESSTIMEOUT;
        }
    }
    return status;
}

s8 fsv9522_PcdConfigISOType(u8 type)
{
    if (type == 'A') // ISO14443_A
    {
        fsv9522_WriteRawRC(ControlReg, 0x10);
        fsv9522_WriteRawRC(TxAutoReg, 0x40);
        fsv9522_WriteRawRC(TxModeReg, 0x00);
        fsv9522_WriteRawRC(RxModeReg, 0x00); //   仅检测A卡的设置

        fsv9522_WriteRawRC(ModWidthReg, 0x26); // 24h  若AB卡检测都有，则必须设置

        fsv9522_WriteRawRC(BitFramingReg, 0);    // 0dh,
        fsv9522_WriteRawRC(ModeReg, 0x3D);       // 11h,
        fsv9522_WriteRawRC(TxControlReg, 0x84);  // 14h,
        fsv9522_WriteRawRC(RxSelReg, 0x88);      // 17h,
        fsv9522_WriteRawRC(DemodReg, 0x8D);      // 19h,     //AddIQ
        fsv9522_WriteRawRC(TypeBReg, 0);         // 1eh,
        fsv9522_WriteRawRC(TModeReg, 0x8D);      // 2ah,
        fsv9522_WriteRawRC(TPrescalerReg, 0x3e); // 2bh
        fsv9522_WriteRawRC(TReloadRegL, 30);     // 2dh
        fsv9522_WriteRawRC(TReloadRegH, 0);      // 2ch
        fsv9522_WriteRawRC(AutoTestReg, 0);      // 36h

#if (TYPEA_REG_CONFIG == REG_SET_1)
        fsv9522_WriteRawRC(RxThresholdReg, 0x33); // 20201117
        fsv9522_WriteRawRC(RFCfgReg, 0x44);       // 26h
        fsv9522_WriteRawRC(GsNOnReg, 0xF8);       // 27h
        fsv9522_WriteRawRC(CWGsPReg, 0x3F);       // 28h

#elif (TYPEA_REG_CONFIG == REG_SET_2)
        fsv9522_WriteRawRC(RxThresholdReg, 0x33); // 20201117
        fsv9522_WriteRawRC(RFCfgReg, 0x44);       // 26h
        fsv9522_WriteRawRC(GsNOnReg, 0x88);       // 27h
        fsv9522_WriteRawRC(CWGsPReg, 0x20);       // 28h

#elif (TYPEA_REG_CONFIG == REG_SET_3)
        fsv9522_WriteRawRC(RxThresholdReg, 0x42); // 20201117
        fsv9522_WriteRawRC(RFCfgReg, 0x44);       // 26h
        fsv9522_WriteRawRC(GsNOnReg, 0xF8);       // 27h
        fsv9522_WriteRawRC(CWGsPReg, 0x3F);       // 28h

#else
        fsv9522_WriteRawRC(RxThresholdReg, 0x84); // 20201117   42
        fsv9522_WriteRawRC(RFCfgReg, 0x78);       // 26h     44
        fsv9522_WriteRawRC(GsNOnReg, 0xF8);       // 27h
        fsv9522_WriteRawRC(CWGsPReg, 0x3F);       // 28h
#endif

        fsv9522_FieldOff();
        tx_thread_sleep(1);
        fsv9522_FieldOn(); // 防止某些卡复位较慢,提前打开射频场
        tx_thread_sleep(10);
        if (device_t.isTouchInit == 0) {
            device_t.isTouchInit = 1;
            si14t_init();
            tx_event_flags_set(&event_group, BIT_TOUCH, TX_OR);
        }
    } else if (type == 'K') // CARD SIMULATION
    {
        fsv9522_WriteRawRC(TxModeReg, 0x80);      // 12h,
        fsv9522_WriteRawRC(RxModeReg, 0x80);      // 13h,
        fsv9522_WriteRawRC(RxThresholdReg, 0x55); // 18h,
        fsv9522_WriteRawRC(DemodReg, 0x61);       // 19h,
        fsv9522_WriteRawRC(MifareReg, 0x62);      // 1ch,
        fsv9522_WriteRawRC(GsNOffReg, 0xf2);      /// 23h
        fsv9522_WriteRawRC(TxBitPhaseReg, 0x87);  // 25h
        fsv9522_WriteRawRC(RFCfgReg, 0x59);       // 26h
        fsv9522_WriteRawRC(GsNOnReg, 0x6F);       // 27h
        fsv9522_WriteRawRC(CWGsPReg, 0x3f);       // 28h
        fsv9522_WriteRawRC(ModGsPReg, 0x3A);      // 29h
        fsv9522_WriteRawRC(TxSelReg, 0x17);       // 16h
        fsv9522_WriteRawRC(FIFOLevelReg, 0x80);
    } else if (type == 'L') {
        fsv9522_WriteRawRC(TxModeReg, 0x92);      // 12h
        fsv9522_WriteRawRC(RxModeReg, 0x92);      // 13h
        fsv9522_WriteRawRC(TxSelReg, 0x17);       // 16h
        fsv9522_WriteRawRC(RxThresholdReg, 0x55); // 18h,
        fsv9522_WriteRawRC(DemodReg, 0x61);       // 19h
        fsv9522_WriteRawRC(RFU1A, 0x00);          // 1Bh
        fsv9522_WriteRawRC(ManualRCVReg, 0x08);
        fsv9522_WriteRawRC(RFU1B, 0x80);     // 1Bh
        fsv9522_WriteRawRC(GsNOffReg, 0xf2); // 23h
        fsv9522_WriteRawRC(RFCfgReg, 0x59);  // 26h
        fsv9522_WriteRawRC(GsNOnReg, 0x6f);  // 27h
        fsv9522_WriteRawRC(CWGsPReg, 0x3f);  // 28h
        fsv9522_WriteRawRC(ModGsPReg, 0x3a); // 29h
        fsv9522_WriteRawRC(FIFOLevelReg, 0x80);
    } else
        return MI_ERR;

    return MI_OK;
}

void fsv9522_Simulate_Card(void)
{
    u8 i;
    const u8 CardConfig[] = {0x04, 0x00, 0xa1, 0xa2, 0xa3, 0x11, 0x01, 0xfe,
                             0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xc0, 0xc1,
                             0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0x23, 0x45,
                             0xfa};
    fsv9522_PcdConfigISOType('K');

    for (i = 0; i < 25; i++)
        fsv9522_WriteRawRC(FIFODataReg, CardConfig[i]);

    fsv9522_WriteRawRC(CommandReg, fsv9522_MEM);
    tx_thread_sleep(250);
    fsv9522_WriteRawRC(CommandReg, fsv9522_AUTOCOLL);
}

void fsv9522_Simulate_CardF(void)
{
    u8 i;
    const u8 CardConfig[] = {0x10, 0xfe, 0x01, 0x12, 0x01, 0x24, 0x69, 0x0b, 0x50, 0x37,
                             0x2E, 0x3d, 0x24, 0x69, 0x0b, 0x50, 0x37, 0x2E,
                             0x3d, 0x24, 0x69, 0x0b, 0x50, 0x37, 0x0b};
    fsv9522_PcdConfigISOType('L');
    tx_thread_sleep(10);
    for (i = 0; i < 25; i++)
        fsv9522_WriteRawRC(FIFODataReg, CardConfig[i]);

    tx_thread_sleep(100);
    fsv9522_WriteRawRC(CommandReg, fsv9522_MEM);
    fsv9522_WriteRawRC(CommandReg, fsv9522_AUTOCOLL);
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdRequestA(u8   req_code,u8 *pTagType)
*功能描述：寻卡命令。
*输入参数： level：防撞级别；
           pSnr：id；
           pSize：长度；
*返回参数： MI_OK：选择成功；
           其它：选择失败；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdRequestA(u8 req_code, u8 *pTagType)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_ClearBitMask(TxModeReg, 0x80);   // 12h,    //TxCRCEn
    fsv9522_ClearBitMask(RxModeReg, 0x80);   // 13h,
    fsv9522_WriteRawRC(TReloadRegL, 0x59);   // 2dh,
    fsv9522_WriteRawRC(TReloadRegH, 0x0A);   // 2ch,
    fsv9522_ClearBitMask(Status2Reg, 0x08);  // 08h
    fsv9522_WriteRawRC(BitFramingReg, 0x07); // 0dh
    fsv9522_SetBitMask(TxControlReg, 0x03);  // 14h     0x03

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 1;
    MfComData.MfData[0] = req_code;
    // GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, DISABLE);
    status = fsv9522_PcdComTransceive(pi);
    // GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);
    if (status == MI_OK) {
        if (MfComData.MfLength == 0x10) {
            *pTagType = MfComData.MfData[0];
            *(pTagType + 1) = MfComData.MfData[1];
        } else
            status = MI_VALERR;
    }
    return status;
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdSelect(u8 level,u8 *pSnr,u8 *pSize)
*功能描述：
           选卡函数。
*输入参数：
           level：防撞级别；
           pSnr：id；
           pSize：长度；
*返回参数：
           MI_OK：选择成功；
           其它：选择失败；
*其它输出：
           无
**********************************************************************************************************/
s8 fsv9522_PcdSelect(u8 level, u8 *pSnr, u8 *pSize)
{
    s8 status;
    u8 i, snr_check = 0;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_WriteRawRC(TxModeReg, 0x80); // 12h        TxCRCEn
    fsv9522_WriteRawRC(RxModeReg, 0x80); // 13h

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 7;
    MfComData.MfData[0] = level;
    MfComData.MfData[1] = 0x70;
    for (i = 0; i < 4; i++) {
        snr_check ^= *(pSnr + i);
        MfComData.MfData[i + 2] = *(pSnr + i);
    }
    MfComData.MfData[6] = snr_check;
    status = fsv9522_PcdComTransceive(pi);

    if (status == MI_OK) {
        if (MfComData.MfLength != 0x8)
            status = MI_BITCOUNTERR;
        else
            *pSize = MfComData.MfData[0];
    }

    return status;
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdAuthState(u8 auth_mode,u8 block,u8 *pKey,u8 *pSnr)
*功能描述： 密钥认证函数。
*输入参数： auth_mode：认证模式；
           block：要认证的扇区号；
           pKey：密钥指令；
           pSnr：
*返回参数： MI_OK：写成功；
           其它：认证错误；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdAuthState(u8 auth_mode, u8 block, u8 *pKey, u8 *pSnr)
{
    s8 status;
    u8 reg;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    MfComData.MfCommand = fsv9522_AUTHENT;
    MfComData.MfLength = 12;
    MfComData.MfData[0] = auth_mode;
    MfComData.MfData[1] = block;
    memcpy(&MfComData.MfData[2], pKey, 6);
    memcpy(&MfComData.MfData[8], pSnr, 4);
    status = fsv9522_PcdComTransceive(pi);
    if (status == MI_OK) {
        reg = fsv9522_ReadRawRC(Status2Reg);
        if (!(reg & 0x08))
            status = MI_AUTHERR;
    }
    return status;
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdRead(u8   addr,u8 *pReaddata)
*功能描述： 读指定扇区。
*输入参数： addr：扇区号；
           pWritedata：数据指针；
*返回参数： MI_OK：写成功；
           其它：认证错误；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdRead(u8 addr, u8 *pReaddata)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 2;
    MfComData.MfData[0] = PICC_READ;
    MfComData.MfData[1] = addr;
    status = fsv9522_PcdComTransceive(pi);
    if (status == MI_OK) {
        if (MfComData.MfLength != 0x80)
            status = MI_BITCOUNTERR;
        else
            memcpy(pReaddata, &MfComData.MfData[0], 16);
    }
    return status;
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdWrite(u8   addr,u8 *pWritedata)
*功能描述： 写指定扇区。
*输入参数： addr：扇区号；
           pWritedata：数据指针；
*返回参数： MI_OK：写成功；
           MI_NOTAUTHERR：认证错误；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdWrite(u8 addr, u8 *pWritedata)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 2;
    MfComData.MfData[0] = PICC_WRITE;
    MfComData.MfData[1] = addr;
    status = fsv9522_PcdComTransceive(pi);
    if (status != MI_NOTAGERR) {
        if (MfComData.MfLength != 4)
            status = MI_BITCOUNTERR;
        else {
            MfComData.MfData[0] &= 0x0F;
            switch (MfComData.MfData[0]) {
            case 0x00:
                status = MI_NOTAUTHERR;
                break;
            case 0x0A:
                status = MI_OK;
                break;
            default:
                status = MI_CODEERR;
                break;
            }
        }
    }
    if (status == MI_OK) {
        MfComData.MfCommand = fsv9522_TRANSCEIVE;
        MfComData.MfLength = 16;
        memcpy(&MfComData.MfData[0], pWritedata, 16);
        status = fsv9522_PcdComTransceive(pi);
        if (status != MI_NOTAGERR) {
            MfComData.MfData[0] &= 0x0F;
            switch (MfComData.MfData[0]) {
            case 0x00:
                status = MI_WRITEERR;
                break;
            case 0x0A:
                status = MI_OK;
                break;
            default:
                status = MI_CODEERR;
                break;
            }
        }
    }
    return status;
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdHaltA(void)
*功能描述： 发送暂停命令给卡。
*输入参数： addr：扇区号；
           pWritedata：数据指针；
*返回参数： MI_OK：操作成功；
           其它：操作失败；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdHaltA(void)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 2;
    MfComData.MfData[0] = PICC_HALT;
    MfComData.MfData[1] = 0;
    status = fsv9522_PcdComTransceive(pi);
    if (status) {
        if (status == MI_NOTAGERR || status == MI_ACCESSTIMEOUT)
            status = MI_OK;
    }
    fsv9522_WriteRawRC(CommandReg, fsv9522_IDLE);
    return status;
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdAnticoll(u8 level,u8 *pSnr)
*功能描述： 防撞处理。
*输入参数： level：防撞级别；
           pSnr：UID指针；
*返回参数： MI_OK：操作成功；
           其它：操作失败；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdAnticoll(u8 level, u8 *pSnr)
{
    s8 status;
    u8 i;
    u8 ucBits, ucBytes;
    u8 snr_check = 0;
    u8 ucCollPosition = 0;
    u8 ucTemp;
    u8 ucSNR[5] = {0, 0, 0, 0, 0};
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_WriteRawRC(BitFramingReg, 0x00);
    fsv9522_ClearBitMask(CollReg, 0x80);
    fsv9522_ClearBitMask(TxModeReg, 0x80);
    fsv9522_ClearBitMask(RxModeReg, 0x80);
    do {
        ucBits = (ucCollPosition) % 8;
        if (ucBits != 0) {
            ucBytes = ucCollPosition / 8 + 1;
            fsv9522_WriteRawRC(BitFramingReg, (ucBits << 4) + ucBits);
        } else
            ucBytes = ucCollPosition / 8;

        MfComData.MfCommand = fsv9522_TRANSCEIVE;
        MfComData.MfData[0] = level;
        MfComData.MfData[1] = 0x20 + ((ucCollPosition / 8) << 4) + (ucBits & 0x0F);
        for (i = 0; i < ucBytes; i++)
            MfComData.MfData[i + 2] = ucSNR[i];
        MfComData.MfLength = ucBytes + 2;

        GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, DISABLE);
        status = fsv9522_PcdComTransceive(pi);
        GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);

        ucTemp = ucSNR[(ucCollPosition / 8)];
        if (status == MI_COLLERR) {
            for (i = 0; i < 5 - (ucCollPosition / 8); i++)
                ucSNR[i + (ucCollPosition / 8)] = MfComData.MfData[i + 1];
            ucSNR[(ucCollPosition / 8)] |= ucTemp;
            ucCollPosition = MfComData.MfData[0];
        } else if (status == MI_OK) {
            for (i = 0; i < (MfComData.MfLength / 8); i++)
                ucSNR[4 - i] = MfComData.MfData[MfComData.MfLength / 8 - i - 1];
            ucSNR[(ucCollPosition / 8)] |= ucTemp;
        }
    } while (status == MI_COLLERR);

    if (status == MI_OK) {
        for (i = 0; i < 4; i++) {
            *(pSnr + i) = ucSNR[i];
            snr_check ^= ucSNR[i];
        }
        if (snr_check != ucSNR[i])
            status = MI_COM_ERR;
    }

    fsv9522_SetBitMask(CollReg, 0x80);
    return status;
}

void printStatus(const char *message, s8 status, u8 *pbuf, u8 len)
{
#ifdef NFC_DEBUG_PRINTF
    u8 i;
    INFO("%s: %d_", message, status);
    if (status == MI_OK || status == MI_FRAMINGERR) {
        for (i = 0; i < len; i++)
            INFO(" %02X", *(pbuf + i));
    }
    INFO("\n");
#endif
}

s8 fsv9522_PcdJewelCommand(u8 *pdata, u8 len, u8 *resp, u8 *replen)
{
    s8 status;
    struct TranSciveBuffer ComData, *pi = &ComData;
    u8 i;
    u8 crc[2];
    for (i = 0; i < len - 1; i++) {
        if (i == 0) {
            fsv9522_WriteRawRC(BitFramingReg, 0x07);
        } else {
            fsv9522_WriteRawRC(BitFramingReg, 0x00);
        }
        ComData.MfCommand = fsv9522_TRANSMIT;
        ComData.MfLength = 1;
        ComData.MfData[0] = *(pdata + i);
        GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);
        ;
        status = fsv9522_JewelTransceive(pi);
        GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, DISABLE);

        if (status != MI_OK)
            return status;
    }
    ComData.MfCommand = fsv9522_TRANSCEIVE;
    ComData.MfLength = 1;
    ComData.MfData[0] = *(pdata + len - 1);
    status = fsv9522_JewelTransceive(pi);
    GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);
    ;
    if (status == MI_OK) {
        *replen = ComData.MfLength / 8;
        if (*replen != 0) {
            memcpy(resp, &ComData.MfData[0], *replen);
            ComputeCrc_B(resp, *replen - 2, crc);
            if (crc[0] != resp[*replen - 2] || crc[1] != resp[*replen - 1])
                status = MI_CRCERR;
        }
    }
    return status;
}

s8 fsv9522_PcdRidA(u8 *pUid)
{
    s8 status;
    u8 cmd_rid[9] = {0x78, 0, 0, 0, 0, 0, 0};
    u8 resp[8];
    u8 len;

    ComputeCrc_B(cmd_rid, 7, &cmd_rid[7]);
    status = fsv9522_PcdJewelCommand(cmd_rid, sizeof(cmd_rid), resp, &len);
    printStatus("RID", status, resp, len);
    if (status == MI_OK) {
        memcpy(pUid, &resp[2], 4);
    }
    return status;
}

s8 fsv9522_PcdReadA(u8 *pUid)
{
    s8 status;
    u8 cmdbuf[9] = {1, 8, 0};
    u8 resp[8];
    u8 len;
    memcpy(&cmdbuf[3], pUid, 4);
    ComputeCrc_B(cmdbuf, 7, &cmdbuf[7]);
    status = fsv9522_PcdJewelCommand(cmdbuf, sizeof(cmdbuf), resp, &len);
    printStatus("Read", status, resp, len);
    return status;
}

s8 fsv9522_PcdJewel(void)
{
    s8 status;
    u8 uid[4];
    status = fsv9522_PcdRidA(uid);
    if (status != MI_OK)
        return status;
    status = fsv9522_PcdReadA(uid);
    return status;
}

/**********************************************************************************************************
*函数名称：s8 fsv9522_PcdActivateA(u8 *patqa,u8 *puid,u8 *plen,u8 *psak)
*功能描述： 读取A卡并进行防撞处理。
*输入参数： patqa，puid，plen，psak：需要返回数据的指针；
*返回参数： patqa：；
           puid：ID；
           plen：ID长度；
           psak：类型；
*其它输出： 无
**********************************************************************************************************/
s8 fsv9522_PcdActivateA(u8 *patqa, u8 *puid, u8 *plen, u8 *psak)
{
    s8 status;

    fsv9522_PcdConfigISOType('A');
    tx_thread_sleep(10);
    status = fsv9522_PcdRequestA(PICC_REQIDL, patqa);

#ifdef NFC_DEBUG_PRINTF
    INFO("ATQA: %d_", status);
    if (status == MI_OK)
        INFO(" %02X %02X", patqa[1], patqa[0]);
    INFO("\n");
#endif

    if (status != MI_OK)
        return status;

    if (*(patqa + 1) == 0x0C && *patqa == 0) {
        *plen = 6;
        status = fsv9522_PcdJewel();
        if (status != MI_OK)
            return status;
    } else {
        *plen = 4;
        status = fsv9522_PcdAnticoll(PICC_ANTICOLL1, puid);
        printStatus("UID1", status, puid, 4);
        if (status != MI_OK)
            return status;

        status = fsv9522_PcdSelect(PICC_ANTICOLL1, puid, psak);
        printStatus("SEL1", status, psak, 1);
        if (status != MI_OK)
            return status;

        if (*patqa & 0xC0) // level 2
        {
            *plen = 8;
            status = fsv9522_PcdAnticoll(PICC_ANTICOLL2, puid + 4);
            printStatus("UID2", status, puid + 4, 4);
            if (status != MI_OK)
                return status;

            status = fsv9522_PcdSelect(PICC_ANTICOLL2, puid + 4, psak);
            printStatus("SEL2", status, psak, 1);
            if (status != MI_OK)
                return status;
        }

        if (*patqa & 0x80) // level 3
        {
            *plen = 12;
            status = fsv9522_PcdAnticoll(PICC_ANTICOLL3, puid + 8);
            printStatus("UID3", status, puid + 8, 4);
            if (status != MI_OK)
                return status;

            status = fsv9522_PcdSelect(PICC_ANTICOLL3, puid + 8, psak);
            printStatus("SEL3", status, psak, 1);
            if (status != MI_OK)
                return status;
        }
    }
    return status;
}

void fsv9522_SetBaudrate(u8 txrate, u8 rrate)
{
    u8 tempt, tempr;

    tempt = fsv9522_ReadRawRC(TxModeReg);
    tempr = fsv9522_ReadRawRC(RxModeReg);

    switch (txrate) {
    case 0:                                          // 106k
        fsv9522_WriteRawRC(TxModeReg, tempt & 0x8f); // 2fh,

        fsv9522_WriteRawRC(ModWidthReg, 0x26);
        break;
    case 1:                                          // 212k
        fsv9522_WriteRawRC(TxModeReg, tempt & 0x9f); // 2fh,
        fsv9522_WriteRawRC(TxModeReg, tempt | 0x10);

        fsv9522_WriteRawRC(ModWidthReg, 0x10);
        break;
    case 2:                                          // 424k
        fsv9522_WriteRawRC(TxModeReg, tempt & 0xaf); // 2fh,
        fsv9522_WriteRawRC(TxModeReg, tempt | 0x20);

        fsv9522_WriteRawRC(ModWidthReg, 0x07);
        break;
    case 3:                                          // 848k
        fsv9522_WriteRawRC(TxModeReg, tempt & 0xbf); // 2fh,
        fsv9522_WriteRawRC(TxModeReg, tempt | 0x30);

        fsv9522_WriteRawRC(ModWidthReg, 0x02);
        break;
    default:                                         // 106k
        fsv9522_WriteRawRC(TxModeReg, tempt & 0x8f); // 2fh,
        fsv9522_WriteRawRC(ModWidthReg, 0x26);
        break;
    }
    switch (rrate) {
    case 0:                                          // 106k
        fsv9522_WriteRawRC(RxModeReg, tempr & 0x8f); // 35h,
        break;
    case 1:                                          // 212k
        fsv9522_WriteRawRC(RxModeReg, tempr & 0x9f); // 2fh,
        fsv9522_WriteRawRC(RxModeReg, tempr | 0x10);
        break;
    case 2:                                          // 424k
        fsv9522_WriteRawRC(RxModeReg, tempr & 0xaf); // 2fh,
        fsv9522_WriteRawRC(RxModeReg, tempr | 0x20);
        break;
    case 3:                                          // 848k
        fsv9522_WriteRawRC(RxModeReg, tempr & 0xbf); // 2fh,
        fsv9522_WriteRawRC(RxModeReg, tempr | 0x30);
        break;
    default:                                         // 106k
        fsv9522_WriteRawRC(RxModeReg, tempr & 0x8f); // 35h,
        break;
    }
}

u8 fsv9522_RatsA(u8 *pbuf, u8 *plen)
{
    s8 status;
    struct TranSciveBuffer ComData, *pi = &ComData;

    ComData.MfCommand = fsv9522_TRANSCEIVE;
    ComData.MfLength = 2;
    ComData.MfData[0] = 0xE0;
    ComData.MfData[1] = 0x51; // default=0x51    Fsdi,CID

    status = fsv9522_PcdComTransceive(pi);
    if (status == MI_OK) {
        *plen = ComData.MfLength / 8;
        memcpy(pbuf, &ComData.MfData[0], *plen);
    }
    return status;
}

u8 fsv9522_PpsA(u8 param, u8 *pPpss)
{
    s8 status;
    struct TranSciveBuffer ComData, *pi = &ComData;

    ComData.MfCommand = fsv9522_TRANSCEIVE;
    ComData.MfLength = 3;
    ComData.MfData[0] = 0xD1;  // 0xD0 | CID
    ComData.MfData[1] = 0x11;  // default=0x51    bFsdi,bCid
    ComData.MfData[2] = param; // 0x0A(424,424);    0x0F(848,848)

    status = fsv9522_PcdComTransceive(pi);
    if (status == MI_OK) {
        if (ComData.MfLength != 8)
            status = MI_BITCOUNTERR;
        else
            *pPpss = ComData.MfData[0]; //
    }
    return status;
}

s8 fsv9522_PcdMfulRead(u8 addr, u8 *pReaddata)
{
    s8 status;
    struct TranSciveBuffer ComData, *pi = &ComData;

    fsv9522_SetBitMask(TxModeReg, 0x80); // 12h,    //TxCRCEn
    fsv9522_SetBitMask(RxModeReg, 0x80); // 13h,

    ComData.MfCommand = fsv9522_TRANSCEIVE;
    ComData.MfLength = 2;
    ComData.MfData[0] = PICC_READ;
    ComData.MfData[1] = addr;
    status = fsv9522_PcdComTransceive(pi);
    if (status == MI_OK) {
        if (ComData.MfLength != 0x80)
            status = MI_BITCOUNTERR;
        else
            memcpy(pReaddata, &ComData.MfData[0], 16);
    }

    return status;
}

u8 fsv9522_CPU_I_Block(u8 *psbuf, u8 slen, u8 *prbuf, u8 *prlen)
{
    s8 status;
    struct TranSciveBuffer ComData, *pi = &ComData;

    fsv9522_WriteRawRC(TReloadRegL, 0x0B); // 2dh,        //30
    fsv9522_WriteRawRC(TReloadRegH, 0x41);
    ComData.MfCommand = fsv9522_TRANSCEIVE;
    ComData.MfLength = slen + 2;
    ComData.MfData[0] = 0x0A; // PCB
    ComData.MfData[1] = 0x01; // Cid
    memcpy(&ComData.MfData[2], psbuf, slen);
    status = fsv9522_PcdComTransceive(pi);
    if (status == MI_OK) {
        *prlen = ComData.MfLength / 8;
        memcpy(prbuf, &ComData.MfData[0], *prlen);
    }
    return status;
}

/**********************************************************************************************************
*函数名称：void fsv9522_TypeA(u8 *UID_a, u8 *len)
*功能描述： 读取A卡返回UID。
*输入参数： UID_a，len：需要返回数据的指针；
*返回参数： UID_a：ID；
           len：ID长度；
*其它输出： 无
**********************************************************************************************************/
void fsv9522_TypeA(u8 *UID_a, u8 *len, u8 *uid_key)
{
    s8 status;
    u8 atqa[7];
    u8 uid[12], ulen;
    u8 sak, i;
    u8 RD_Data[16];
    static u8 KEY[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u8 ver1 = 0x60;
    u8 random[] = {0x00, 0x84, 0x00, 0x00, 0x08};

    *len = 0;
    memset(UID_a, 0x00, 7);

    fsv9522_rst();
    status = fsv9522_PcdActivateA(atqa, uid, &ulen, &sak);
    if (status == MI_OK) {
        if (ulen == 8) {
            *len = ulen - 1;
            memcpy(UID_a, &uid[1], 7);
        } else {
            *len = ulen;
            memcpy(UID_a, uid, 4);
        }
    } else {
        return;
    }
    if (ulen != 6) {
        if (sak & 0x04) {
#ifdef NFC_DEBUG_PRINTF
            INFO("uid not complete");
#endif
            return;
        }
        if (sak & 0x20) // compliant with ISO-14443-4
        {
            status = fsv9522_RatsA(RD_Data, &ulen);
            printStatus("Rats", status, RD_Data, ulen);
            if (status != MI_OK)
                return;

            if ((RD_Data[1] & 0x10) && (RD_Data[2] & 0x44) == 0x44) // TA(1) 848k supported    //mifare desfire
            {
                status = fsv9522_PpsA(0x0f, &sak); // 106:0x00   212:0x05  424:0x0a  848:0x0f
                printStatus("Pps", status, &sak, 1);
                if (status != MI_OK)
                    return;
                fsv9522_SetBaudrate(3, 3); // 106：0，0  212：1，1   424：2，2   848：3，3
                status = fsv9522_CPU_I_Block(&ver1, 1, uid, &ulen);
                printStatus("ver1", status, uid, ulen);
                if (status != MI_OK)
                    return;
                fsv9522_WriteRawRC(ModWidthReg, 0x26);
                fsv9522_SetBaudrate(0, 0);
            } else {
                status = fsv9522_CPU_I_Block(random, 5, uid, &ulen);
                printStatus("random", status, uid, ulen);
                if (status != MI_OK)
                    return;
            }
        } else if (ulen == 4) // not compliant with ISO-14443-4
        {
            if (sak == 0x08 && atqa[0] == 0x04) {
#ifdef NFC_DEBUG_PRINTF
                INFO("mifare S50 has detected!\n");
#endif
            } else if (sak == 0x18 && atqa[0] == 0x02) {
#ifdef NFC_DEBUG_PRINTF
                INFO("mifare S70 has detected!\n");
#endif
            }
            status = fsv9522_PcdAuthState(0x60, 0x00, KEY, &uid[0]);

#ifdef NFC_DEBUG_PRINTF
            INFO("AUTH: %X\n", status);
#endif

            status = fsv9522_PcdRead(0, RD_Data);
            memcpy(uid_key, RD_Data, 16);

#ifdef NFC_DEBUG_PRINTF
            INFO("READ:%d_", status);
            for (i = 0; i < 16; i++)
                INFO(" %02X", RD_Data[i]);
            INFO("\n");
#endif
            if (status != MI_OK)
                return;
        } else {
            status = fsv9522_PcdMfulRead(3, RD_Data);
            printStatus("MfulRead", status, RD_Data, 16);
            if (status != MI_OK)
                return;
        }
    }

    // // 有卡则一直检测，无卡则退出
    // do {
    //     fsv9522_FieldOff();
    //     tx_thread_sleep(10);
    //     fsv9522_FieldOn();
    //     tx_thread_sleep(10);
    //     status = fsv9522_PcdRequestA(PICC_REQALL, atqa);
    //     if (status != MI_OK) {
    //         break;
    //     }
    // } while (status == MI_OK);
}

// type-b
void fsv9522_PcdConfigISOTypeB(void)
{
    fsv9522_ClearBitMask(Status2Reg, 0x08);  // 08h
    fsv9522_WriteRawRC(WaterLevelReg, 0x10); // 0bh,
    fsv9522_WriteRawRC(ControlReg, 0x10);    // 0ch,
    fsv9522_WriteRawRC(BitFramingReg, 0);    // 0dh,

    fsv9522_WriteRawRC(ModeReg, 0x3F);        // 11h
    fsv9522_WriteRawRC(TxModeReg, 0x03);      // 12h,
    fsv9522_WriteRawRC(RxModeReg, 0x03);      // 13h,
    fsv9522_WriteRawRC(TxControlReg, 0x80);   // 14h
    fsv9522_WriteRawRC(TxAutoReg, 0x00);      // 15h,
    fsv9522_WriteRawRC(RxSelReg, 0x88);       // 17h,
    fsv9522_WriteRawRC(RxThresholdReg, 0x50); // 18h,
    fsv9522_WriteRawRC(DemodReg, 0x4D);       // 19h,
    fsv9522_WriteRawRC(TypeBReg, 0x00);       // 1eh,
    fsv9522_WriteRawRC(GsNOffReg, 0xF2);      // 23h
    fsv9522_WriteRawRC(ModWidthReg, 0x68);    // 24h,
    fsv9522_WriteRawRC(RFCfgReg, 0x59);       // 26h
    fsv9522_WriteRawRC(GsNOnReg, 0xF8);       // 27h
    fsv9522_WriteRawRC(CWGsPReg, 0x3f);       // 28h
    fsv9522_WriteRawRC(ModGsPReg, 0x2A);      // 29h,

    fsv9522_WriteRawRC(TModeReg, 0x8D);      // 2ah,
    fsv9522_WriteRawRC(TPrescalerReg, 0x3e); // 2bh
    fsv9522_WriteRawRC(TReloadRegL, 30);     // 2dh
    fsv9522_WriteRawRC(TReloadRegH, 0);      // 2ch

    fsv9522_WriteRawRC(AutoTestReg, 0); // 36h

    fsv9522_FieldOff();
    tx_thread_sleep(5);
    fsv9522_FieldOn();
}

s8 fsv9522_PcdRequestB(u8 *atqb, u8 *pLen)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;
    fsv9522_WriteRawRC(TxModeReg, 0x83);    // 12h
    fsv9522_WriteRawRC(RxModeReg, 0x83);    // 13h
    fsv9522_WriteRawRC(TReloadRegL, 0x59);  // 2dh,
    fsv9522_WriteRawRC(TReloadRegH, 0x0A);  // 2ch,
    fsv9522_ClearBitMask(Status2Reg, 0x08); // 08h
    fsv9522_SetBitMask(TxControlReg, 0x03); // 14h     0x03

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 3;
    MfComData.MfData[0] = PICC_ANTI;
    MfComData.MfData[1] = 0;
    MfComData.MfData[2] = 0;
    status = fsv9522_PcdComTransceive(pi);

    if (status == MI_OK) {
        if ((MfComData.MfLength == 0x60) || (MfComData.MfLength == 0x10)) {
            *pLen = MfComData.MfLength / 8;
            memcpy(atqb, &MfComData.MfData[0], *pLen);
        } else
            status = MI_VALERR;
    }
    return status;
}

s8 fsv9522_PcdAttribB(u8 *pCid)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_WriteRawRC(TPrescalerReg, 0x80); // 2bh,
    fsv9522_WriteRawRC(TModeReg, 0x80);      // 2ah,
    fsv9522_WriteRawRC(TReloadRegL, 0xC9);   // 2dh,
    fsv9522_WriteRawRC(TReloadRegH, 0xFF);   // 2ch,

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 9;
    MfComData.MfData[0] = PICC_ATTRIB;
    memset(&MfComData.MfData[1], 0x00, 4); // pupi
    MfComData.MfData[5] = 0;
    MfComData.MfData[6] = 0x08;
    MfComData.MfData[7] = 0x01;
    MfComData.MfData[8] = 0x08;
    status = fsv9522_PcdComTransceive(pi);

    if (status == MI_OK) {
        if (MfComData.MfLength != 0x8)
            status = MI_BITCOUNTERR;
        else
            *pCid = MfComData.MfData[0];
    }

    return status;
}

s8 fsv9522_PcdGetUidB(u8 *puid)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;
    fsv9522_WriteRawRC(TxModeReg, 0x83); // 12h
    fsv9522_WriteRawRC(RxModeReg, 0x83); // 13h

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 5;
    MfComData.MfData[0] = 0x00;
    MfComData.MfData[1] = 0x36;
    MfComData.MfData[2] = 0x00;
    MfComData.MfData[3] = 0x00;
    MfComData.MfData[4] = 0x08;
    status = fsv9522_PcdComTransceive(pi);
    if ((status == MI_OK) && (MfComData.MfLength == 0x50))
        memcpy(puid, &MfComData.MfData[0], 10);
    else
        status = MI_ERR;
    return status;
}

void fsv9522_SRT512(void)
{
    s8 status;
    u8 i;
    u8 RD_Data[12], Len;
    u8 chipID;

    fsv9522_PcdConfigISOTypeB();
    tx_thread_sleep(10);

    status = INITIATE_PCALL16(&chipID, &Len, 0);
    if (status == MI_OK) {
#ifdef NFC_DEBUG_PRINTF
        INFO("Chip_ID1: %02X", chipID);
        INFO("\n");
#endif
    }
    if (status != MI_OK)
        return;

    status = SelectChipID(RD_Data, &Len, chipID);
    if (status == MI_OK) {
#ifdef NFC_DEBUG_PRINTF
        INFO("Chip_ID2: %02X", RD_Data[0]);
        INFO("\n");
#endif
    }
    if (status != MI_OK)
        return;

    status = GetUid(RD_Data);
    if (status == MI_OK) {
#ifdef NFC_DEBUG_PRINTF
        for (i = 0; i < 8; i++)
            INFO(" %02X", RD_Data[i]);
        INFO("\n");
#endif
    }

    if (status != MI_OK)
        return;
    status = SRTReadBlock(0, RD_Data);
    if (status == MI_OK) {
#ifdef NFC_DEBUG_PRINTF
        for (i = 0; i < 4; i++)
            INFO(" %02X", RD_Data[i]);
        INFO("\n");
#endif
    }

    if (status != MI_OK)
        return;
    do {
        status = GetUid(RD_Data);
        if (status != MI_OK) {
            break;
        }
    } while (status == MI_OK);
}
s8 Write_ReadBlock(u8 block, u8 *Data)
{
    s8 status;
    u8 i;
    u8 random[] = {0x80, 0x23, 0x03, 0x27};
    struct TranSciveBuffer MfComData, *pi = &MfComData;
    fsv9522_WriteRawRC(TxModeReg, 0x83); // 12h
    fsv9522_WriteRawRC(RxModeReg, 0x83);
    MfComData.MfCommand = fsv9522_TRANSMIT;
    MfComData.MfLength = 6;
    MfComData.MfData[0] = 0x09;
    MfComData.MfData[1] = block;
    for (i = 0; i < 4; i++) {
        MfComData.MfData[2 + i] = random[i];
    }
    status = fsv9522_PcdComTransceive(pi);
    if (status != MI_OK)
        return MI_ERR;
    tx_thread_sleep(5);
    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 2;
    MfComData.MfData[0] = 0x08;
    MfComData.MfData[1] = block;
    status = fsv9522_PcdComTransceive(pi);
    if ((status == MI_OK) && (MfComData.MfLength == 0x20))
        memcpy(Data, &MfComData.MfData[0], 4);
    else
        status = MI_ERR;

    return status;
}
s8 SRTReadBlock(u8 block, u8 *Data)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;
    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 2;
    MfComData.MfData[0] = 0x08;
    MfComData.MfData[1] = block;
    status = fsv9522_PcdComTransceive(pi);
    if ((status == MI_OK) && (MfComData.MfLength == 0x20))
        memcpy(Data, &MfComData.MfData[0], 4);
    else
        status = MI_ERR;

    return status;
}

s8 GetUid(u8 *puid)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_WriteRawRC(TxModeReg, 0x83); // 12h
    fsv9522_WriteRawRC(RxModeReg, 0x83); // 13h

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 1;
    MfComData.MfData[0] = 0x0B;

    status = fsv9522_PcdComTransceive(pi);
    if ((status == MI_OK) && (MfComData.MfLength == 0x40))
        memcpy(puid, &MfComData.MfData[0], 8);
    else
        status = MI_ERR;

    return status;
}

s8 SelectChipID(u8 *atqb, u8 *pLen, u8 id)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_WriteRawRC(TxModeReg, 0x83);    // 12h
    fsv9522_WriteRawRC(RxModeReg, 0x83);    // 13h
    fsv9522_WriteRawRC(TReloadRegL, 0x59);  // 2dh,
    fsv9522_WriteRawRC(TReloadRegH, 0x0A);  // 2ch,
    fsv9522_ClearBitMask(Status2Reg, 0x08); // 08h
    fsv9522_SetBitMask(TxControlReg, 0x03); // 14h

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 2;
    MfComData.MfData[0] = 0x0E;
    MfComData.MfData[1] = id;

    status = fsv9522_PcdComTransceive(pi);

    if (status == MI_OK) {
        if (MfComData.MfLength == 0x08) {
            *pLen = MfComData.MfLength / 8;
            memcpy(atqb, &MfComData.MfData[0], *pLen);
        } else
            status = MI_VALERR;
    }
    return status;
}

s8 INITIATE_PCALL16(u8 *atqb, u8 *pLen, u8 cmd)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_WriteRawRC(TxModeReg, 0x83);    // 12h
    fsv9522_WriteRawRC(RxModeReg, 0x83);    // 13h
    fsv9522_WriteRawRC(TReloadRegL, 0x59);  // 2dh,
    fsv9522_WriteRawRC(TReloadRegH, 0x0A);  // 2ch,
    fsv9522_ClearBitMask(Status2Reg, 0x08); // 08h
    fsv9522_SetBitMask(TxControlReg, 0x03); // 14h

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 2;
    MfComData.MfData[0] = 0x06;
    MfComData.MfData[1] = cmd;

    status = fsv9522_PcdComTransceive(pi);

    if (status == MI_OK) {
        if (MfComData.MfLength == 0x08) {
            *pLen = MfComData.MfLength / 8;
            memcpy(atqb, &MfComData.MfData[0], *pLen);
        } else
            status = MI_VALERR;
    }
    return status;
}

void fsv9522_ChinaID2(u8 *UID_b)
{
    s8 status;
    u8 i;
    u8 RD_Data[12], Len;

    memset(UID_b, 0x00, 8);
    fsv9522_PcdConfigISOTypeB();
    tx_thread_sleep(10);
    status = fsv9522_PcdRequestB(RD_Data, &Len);

#ifdef NFC_DEBUG_PRINTF
    INFO("ATQB: %d_", status);
    if (status == MI_OK) {
        for (i = 0; i < Len; i++)
            INFO(" %02X", RD_Data[i]);
    }
    INFO("\n");
#endif

    if (status != MI_OK)
        return;

    if (Len == 12) {
        status = fsv9522_PcdAttribB(RD_Data);

#ifdef NFC_DEBUG_PRINTF
        INFO("SELECT:%d_", status);
        if (status == MI_OK)
            INFO(" %02X", RD_Data[0]);
        INFO("\n");
#endif

        if (status != MI_OK)
            return;
    }

    status = fsv9522_PcdGetUidB(RD_Data);
    if (status == MI_OK) {
        memcpy(UID_b, RD_Data, 8);
    }

#ifdef NFC_DEBUG_PRINTF
    INFO("UID: %d_", status);
    for (i = 0; i < 8; i++) {
        INFO(" %02X", RD_Data[i]);
    }
    INFO("\n");
#endif

    // 有卡则一直检测，无卡则退出
    while (1) {
        fsv9522_FieldOff();
        tx_thread_sleep(10);
        fsv9522_FieldOn();
        tx_thread_sleep(10);
        status = fsv9522_PcdRequestB(RD_Data, &Len);
        if (status == MI_OK) {
        } else
            break;
    }
}

void ReadmoniF(void)
{
    s8 status;
    u8 i;
    u8 IDm[10], PMm[8];

    fsv9522_PcdConfigISOTypeF();
    tx_thread_sleep(10);
    status = fsv9522_PcdRequestF(IDm, PMm);

#ifdef NFC_DEBUG_PRINTF
    INFO("ATQF: %d_", status);
    if (status == MI_OK) {
        for (i = 0; i < 10; i++)
            INFO(" %02X ", IDm[i]);
        INFO("_");
        for (i = 0; i < 8; i++)
            INFO(" %02X", PMm[i]);
    }
    INFO("\n");
#endif

    if (status != MI_OK)
        return;
}

/***********************************************************************************************
 *配置为FeliCa卡
 ***********************************************************************************************/
void fsv9522_PcdConfigISOTypeF(void)
{
    fsv9522_ClearBitMask(Status2Reg, 0x08);  // 08h        //MFCrypto1On
    fsv9522_WriteRawRC(WaterLevelReg, 0x10); // 0bh,
    fsv9522_WriteRawRC(ControlReg, 0x10);    // 0ch, //Initiator
    fsv9522_WriteRawRC(BitFramingReg, 0);    // 0dh,

    fsv9522_WriteRawRC(ModeReg, 0x00);   // 11h,    *00*
    fsv9522_WriteRawRC(TxModeReg, 0x92); // 12h, 212k
    fsv9522_WriteRawRC(RxModeReg, 0x92); // 13h,  212k
    // fsv9522_WriteRawRC(TxModeReg,0xa2); //12h  424k
    // fsv9522_WriteRawRC(RxModeReg,0xa2);//13h  424k
    fsv9522_WriteRawRC(TxControlReg, 0x80);   // 14h,
    fsv9522_WriteRawRC(TxAutoReg, 0x00);      // 15h,
    fsv9522_WriteRawRC(TxSelReg, 0x10);       // 16h
    fsv9522_WriteRawRC(RxSelReg, 0x83);       // 17h
    fsv9522_WriteRawRC(RxThresholdReg, 0x55); // 18h
    fsv9522_WriteRawRC(DemodReg, 0x6D);       // 19h
    fsv9522_WriteRawRC(TypeBReg, 0);          // 1eh,
    fsv9522_WriteRawRC(GsNOffReg, 0xF2);      // 23h
    fsv9522_WriteRawRC(ModWidthReg, 0x26);    // 24h,
    fsv9522_WriteRawRC(RFCfgReg, 0x59);       // 26h
    fsv9522_WriteRawRC(GsNOnReg, 0xFF);       // 27h
    fsv9522_WriteRawRC(CWGsPReg, 0x3f);       // 28h
    fsv9522_WriteRawRC(ModGsPReg, 0x12);      // 29h
    fsv9522_WriteRawRC(TModeReg, 0x8D);       // 2ah
    fsv9522_WriteRawRC(TPrescalerReg, 0x00);  // 2bh

    fsv9522_FieldOff();
    tx_thread_sleep(5);
    fsv9522_FieldOn();
}

s8 fsv9522_PcdRequestF(u8 *pIDm, u8 *pPMm)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    fsv9522_WriteRawRC(TPrescalerReg, 0x01); // 2bh,
    fsv9522_WriteRawRC(TModeReg, 0x80);      // 2ah,
    fsv9522_WriteRawRC(TReloadRegL, 0xEC);   // 2dh,
    fsv9522_WriteRawRC(TReloadRegH, 0x5C);   // 2ch,

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 6;
    MfComData.MfData[0] = 0x06;
    MfComData.MfData[1] = 0;
    MfComData.MfData[2] = 0xFF;
    MfComData.MfData[3] = 0xFF;
    MfComData.MfData[4] = 0;
    MfComData.MfData[5] = 0;
    GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, DISABLE);
    status = fsv9522_PcdComTransceive(pi);
    GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);
    if (status == MI_OK) {
        if (MfComData.MfLength == 0x90) {
            memcpy(pIDm, &MfComData.MfData[2], 8);
            memcpy(pPMm, &MfComData.MfData[10], 8);
        } else
            status = MI_VALERR;
    }
    return status;
}

s8 fsv9522_PcdReadF(u8 *pIDm, u8 *pDat)
{
    s8 status;
    struct TranSciveBuffer MfComData, *pi = &MfComData;

    MfComData.MfCommand = fsv9522_TRANSCEIVE;
    MfComData.MfLength = 16;
    MfComData.MfData[0] = 0x10; // len
    MfComData.MfData[1] = 0x06;
    memcpy(&MfComData.MfData[2], pIDm, 8);
    MfComData.MfData[10] = 1;    // bNumServices
    MfComData.MfData[11] = 0x0B; // pServiceList..
    MfComData.MfData[12] = 0;
    MfComData.MfData[13] = 1;    // read how much number of block
    MfComData.MfData[14] = 0x80; // pBlockList..
    MfComData.MfData[15] = 0x01;
    status = fsv9522_PcdComTransceive(pi);
    if (status == MI_OK) {
        if (MfComData.MfLength == 0xE8) {
            memcpy(pDat, &MfComData.MfData[13], 16);
        } else
            status = MI_VALERR;
    }
    return status;
}

/***********************************************************************************************
 *检测并读取FeliCa卡函数
 ***********************************************************************************************/
void fsv9522_Felica(void)
{
    s8 status;
    u8 i;
    u8 IDm[8], PMm[8], buf[16];
    tx_thread_sleep(100);
    fsv9522_PcdConfigISOTypeF();
    tx_thread_sleep(100);
    status = fsv9522_PcdRequestF(IDm, PMm);
    tx_thread_sleep(100);

#ifdef NFC_DEBUG_PRINTF
    INFO("ATQF: %d_", status);
    if (status == MI_OK) {
        for (i = 0; i < 8; i++)
            INFO(" %02X ", IDm[i]);
        INFO("_");
        for (i = 0; i < 8; i++)
            INFO(" %02X", PMm[i]);
    } else {
        INFO("Error!!!!\n");
    }
    INFO("\n");
#endif

    if (status != MI_OK)
        return;

    status = fsv9522_PcdReadF(IDm, buf);

#ifdef NFC_DEBUG_PRINTF
    INFO("Read: %d_", status);
    if (status == MI_OK) {
        for (i = 0; i < 16; i++)
            INFO(" %02X", buf[i]);
    }
    INFO("\n");
#endif

    if (status != MI_OK)
        return;

    do {
        fsv9522_FieldOff();
        tx_thread_sleep(10);
        fsv9522_FieldOn();
        tx_thread_sleep(10);
        status = fsv9522_PcdRequestF(IDm, PMm);
        if (status != MI_OK)
            break;
    } while (status == MI_OK);
}

/**
 * @brief  读卡器初始化
 * @param
 * @retval
 **/
void fsv9522_init(void)
{
    fsv9522_rst();
#if (LPCD_ENABLE)
    fsv9522_LPCD_Enter(LPCD_Cycle_Time, LPCD_Duration_Time);
    DEBUG_TRACE(LOG_TAG, "Fsv9522 Enter LPCD Mode");
#else
    fsv9522_sleep();
#endif
}

#if FUNC_OPERATIONAL_VERSION_ENABLE

/**
 * @brief  用于读卡器控制
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_card(ULONG thread_input)
{
    (void)thread_input;

    ULONG actual_events;

    u8 len;
    u8 uid[12];
    char str_uid[24];
    u8 key[20];
    u8 i, err, action, userNum, uid_len;

    u8 buffer[50];
    u8 sendLen;

    ULONG main_bit, fsv9522_bit;
    UINT status = tx_event_flags_create(&fsv9522_event_group, "fsv9522_event_group");
    if (status) {
        DEBUG_TRACE(ERROR_TAG, "fsv9522_event_group Creat Fail !!! %d", status);
    }

    while (1) {
    __back:
        // 等待处于运行状态
        tx_event_flags_get(&main_event_group, MAIN_BIT_RUN, TX_AND, &main_bit, TX_WAIT_FOREVER);
        tx_thread_sleep(5);

        status = tx_event_flags_get(&fsv9522_event_group, FSV9522_BIT_SLEEP, TX_OR_CLEAR, &fsv9522_bit, TX_NO_WAIT);
        if (status == TX_SUCCESS) {
            if (fsv9522_bit & FSV9522_BIT_SLEEP) {
                fsv9522_init();
                tx_event_flags_set(&fsv9522_event_group, FSV9522_BIT_SLEEP_DONE, TX_OR);
                goto __back;
            }
        }

        tx_event_flags_set(&fsv9522_event_group, FSV9522_BIT_IDLE, TX_OR);
        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &main_bit, TX_NO_WAIT);
        if ((main_bit & MAIN_BIT_SLEEP) || (main_bit & MAIN_BIT_DOOR_OPEN)) {
            goto __back;
        }

        tx_thread_sleep(495);

        tx_event_flags_get(&fsv9522_event_group, FSV9522_BIT_IDLE, TX_OR_CLEAR, &fsv9522_bit, TX_NO_WAIT);
        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &main_bit, TX_NO_WAIT);
        if ((main_bit & MAIN_BIT_SLEEP) || (main_bit & MAIN_BIT_DOOR_OPEN)) {
            goto __back;
        }

        action = 0;
        if (device_t.lockDoorTimeout) {
            goto __back;
        }
        tx_thread_sleep(10);
        memset(uid, 0, sizeof(uid));
        memset(key, 0, sizeof(key));
        /*读取A卡并返回ID和长度*/
        fsv9522_TypeA(uid, &uid_len, key);
        if (uid_len > 0) {              // 读卡成功
            if (device_t.lockTimeout) { // 锁定状态
                DEBUG_TRACE(WARN_TAG, "The Lock is Locked, %s", __func__);
                memset(&s_touch, 0, sizeof(s_touch));
                /* 语音 */
                // 失败提示音
                // 系统已锁定
                Sound_Insert_Number(0, VOICE_TONE_FAIL);
                Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                tx_thread_sleep(2000); // 失败后强制延时播放语音
                goto __back;
            }

            snprintf(str_uid, sizeof(str_uid), "%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
            DEBUG_TRACE(LOG_TAG, "Read Card UID: %s", str_uid);
            PrintInfoDumpExt(key, 16, "Key");

            userNum = 0xFF;
            if (coded_lock_t.total_num_card == 0) {
                device_t.openFailCnt++;
                action = 0xC3; // MF 卡报警
                DEBUG_TRACE(WARN_TAG, "Card Uid Not Match!!!");

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
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }

                if (action == 0xC3) { // 写入开锁失败记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockFail = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                        strcat(exc_lock_info_t.word, "123456");
                    } else {
                        strcat(exc_lock_info_t.word, str_uid);
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    // nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
            } else {
                for (i = 0; i < MAX_SECRET_TIMELINESS_NUM; i++) {
                    if (coded_lock_t.card[i].isable) {
                        if (memcmp(uid, coded_lock_t.card[i].id, 4) == 0 && memcmp(key, coded_lock_t.card[i].id_key, 16) == 0 &&
                            Timestamp > coded_lock_t.card[i].start_timestamp && Timestamp < coded_lock_t.card[i].end_timestamp) {
                            if (coded_lock_t.card[i].valid_cnt) { // 有效次数使用完后，直接变为无效状态
                                coded_lock_t.card[i].valid_cnt--;
                                if (!coded_lock_t.card[i].valid_cnt) {
                                    coded_lock_t.card[i].isable = 0;
                                }
                                FLASH_Write_Door_Lock_Coded();
                            }
                            userNum = coded_lock_t.card[i].user_num;
                            action = 0x03;                                                // 卡片开门
                            tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);           // 关闭灯光
                            DEBUG_TRACE(LOG_TAG, "Card Unlock[%d] %s", userNum, str_uid); // 卡片匹配成功
                            break;
                        }
                    }
                }
                if (i >= MAX_SECRET_TIMELINESS_NUM) {
                    action = 0xC3; // MF卡报警
                    device_t.openFailCnt++;
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
                    DEBUG_TRACE(LOG_TAG, "Card Uid Not Match: %s", str_uid);
                }
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
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }

            // 写入开锁失败记录
            if (action == 0xC3) {
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.unlockFail = 1;
                exc_lock_info_t.type = 2;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, str_uid);
                }
                exc_lock_info_t.stamp = Timestamp;
                // nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            } else if (action == 0x03) // 写入开锁成功记录
            {
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.unlockSuccess = 1;
                exc_lock_info_t.type = 2;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, str_uid);
                }
                exc_lock_info_t.stamp = Timestamp;
                // nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }

            /* MF卡开锁解除防拆报警*/
            if (device_t.tamperAlarm && action == 0x03) {
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
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }

                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.tamperRecover = 1;
                exc_lock_info_t.type = 2;
                exc_lock_info_t.number = userNum;
                if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card) {
                    strcat(exc_lock_info_t.word, "123456");
                } else {
                    strcat(exc_lock_info_t.word, str_uid);
                }
                exc_lock_info_t.stamp = Timestamp;
                // nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }
            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            if (action == 0xC3) { // 开锁失败停止读卡一段时间
                // LED闪烁一次
                MBI5120_Display(0);
                tx_thread_sleep(2000);
                MBI5120_Display(0xFFFF);
                tx_thread_sleep(1000);
            }
        }
    }
}

#else

/**
 * @brief  用于读卡器控制
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_card(ULONG thread_input)
{
    (void)thread_input;

    ULONG actual_events;

    u8 uid_a[12], len;
    u8 uid_b[12];
    u8 uid[12];
    u8 key[20];
    char str_uid[20];
    char temp_str_uid[20];
    u8 i, err, action, userNum, uid_len;

    u8 buffer[50];
    u8 sendLen;

    ULONG main_bit, fsv9522_bit;
    UINT status = tx_event_flags_create(&fsv9522_event_group, "fsv9522_event_group");
    if (status) {
        DEBUG_TRACE(ERROR_TAG, "fsv9522_event_group Creat Fail !!! %d", status);
    }

    while (1) {
    __back:
        // 等待处于运行状态
        tx_event_flags_get(&main_event_group, MAIN_BIT_RUN, TX_AND, &main_bit, TX_WAIT_FOREVER);
        tx_thread_sleep(5);

        status = tx_event_flags_get(&fsv9522_event_group, FSV9522_BIT_SLEEP, TX_OR_CLEAR, &fsv9522_bit, TX_NO_WAIT);
        if (status == TX_SUCCESS) {
            if (fsv9522_bit & FSV9522_BIT_SLEEP) {
                fsv9522_init();
                tx_event_flags_set(&fsv9522_event_group, FSV9522_BIT_SLEEP_DONE, TX_OR);
                goto __back;
            }
        }

        tx_event_flags_set(&fsv9522_event_group, FSV9522_BIT_IDLE, TX_OR);
        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &main_bit, TX_NO_WAIT);
        if ((main_bit & MAIN_BIT_SLEEP) || (main_bit & MAIN_BIT_DOOR_OPEN)) {
            goto __back;
        }

        tx_thread_sleep(495);

        tx_event_flags_get(&fsv9522_event_group, FSV9522_BIT_IDLE, TX_OR_CLEAR, &fsv9522_bit, TX_NO_WAIT);
        tx_event_flags_get(&main_event_group, MAIN_BIT_DISPLAY_ALL, TX_OR, &main_bit, TX_NO_WAIT);
        if ((main_bit & MAIN_BIT_SLEEP) || (main_bit & MAIN_BIT_DOOR_OPEN)) {
            goto __back;
        }

        action = 0;
        if (device_t.lockDoorTimeout) {
            goto __back;
        }
        if (device_t.sleepState) {
            goto __back;
        }
        memset(uid_a, 0, sizeof(uid_a));
        memset(uid_b, 0, sizeof(uid_b));
        memset(key, 0, sizeof(key));
        memset(str_uid, 0, sizeof(str_uid));
        /*读取A卡并返回ID和长度*/
        fsv9522_TypeA(uid_a, &uid_len, key);
        // /*读取B卡并返回8字节ID*/
        // fsv9522_ChinaID2(uid_b);
        // fsv9522_Felica();
        memcpy(uid, uid_a, uid_len);
        if (uid_len > 0) {              // 读卡成功
            if (device_t.lockTimeout) { // 锁定状态
                DEBUG_TRACE(WARN_TAG, "The Lock is Locked, %s", __func__);
                memset(&s_touch, 0, sizeof(s_touch));
                /* 语音 */
                // 失败提示音
                // 系统已锁定
                Sound_Insert_Number(0, VOICE_TONE_FAIL);
                Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                tx_thread_sleep(2000); // 失败后强制延时播放语音
                goto __back;
            }

            snprintf(str_uid, sizeof(str_uid), "%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
            DEBUG_TRACE(LOG_TAG, "Read Card UID: %s", str_uid);
            PrintInfoDumpExt(key, 16, "Key");
            if (device_t.lockMode) {
                device_t.operateTimeout = 20 * SEC_DELAY;
                if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_CARD || device_t.lockMode == SYSTEM_MODE_INPUT_USER_CARD) {
                    device_t.lockMode += 7;
                    goto __success; // 刷卡仅校验一次

                    /* 语音 */
                    // 请再次输入
                    //    Sound_Insert_Number(1, VOICE_INPUT_SECOND);
                    //    memcpy(temp_str_uid, str_uid, uid_len * 2);
                    //    DEBUG_TRACE(LOG_TAG, "Exec Success, Card Number[%d], Lock Mode %d", device_t.inputNum, device_t.lockMode);
                } else if (device_t.lockMode == SYSTEM_MODE_INPUT_ROOT_CARD_2 || device_t.lockMode == SYSTEM_MODE_INPUT_USER_CARD_2) {
                    if (!memcmp(temp_str_uid, str_uid, uid_len * 2)) {
                    __success:
                        set_card_binding((u8)device_t.inputNum, str_uid, 1);
                        // 更新缓存
                        strncpy(coded_lock_t.card[(u8)device_t.inputNum].word, str_uid, uid_len * 2);
                        coded_lock_t.total_num_card++;
                        coded_lock_t.card[(u8)device_t.inputNum].isable = 1;
                        DEBUG_TRACE(LOG_TAG, "Exec Success, Save Card[%s], Lock Mode %d", str_uid, device_t.lockMode);

                        // 退出系统设置
                        device_t.lockMode = 0;
                        /* 语音 */
                        // 操作成功
                        Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);

                        struct lock_log exc_lock_info_t;
                        memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                        exc_lock_info_t.u_exec.isLocal = 1;
                        exc_lock_info_t.u_exec.inputAction = 1;
                        exc_lock_info_t.type = 2;
                        exc_lock_info_t.number = userNum;
                        strcat(exc_lock_info_t.word, str_uid);
                        exc_lock_info_t.stamp = Timestamp;
                        nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash

                        tx_thread_sleep(2000); // 等2s再读卡
                    } else {
                        device_t.lockMode -= 8; // 返回输入第一次读卡
                        DEBUG_TRACE(WARN_TAG, "Exec Fail, Please Input Card, Back Lock Mode %d", device_t.lockMode);
                        /* 语音 */
                        // 操作失败
                        // 请放卡
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                        Sound_Insert_Number(1, VOICE_INPUT_CARD);
                    }
                }
                /* 验证管理员密码 */
                else if (device_t.lockMode == SYSTEM_MODE_WAIT_INPUT_PASSWORD) {
                    i = 0xFF;
                    if (!coded_lock_t.total_num_finger && !coded_lock_t.total_num_card && !coded_lock_t.total_num_pass) {
                        device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                        DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Choose Mode, Lock Mode %d", device_t.lockMode);
                        /* 语音 */
                        // 操作成功
                        // 1 密码用户
                        // 2 卡用户
                        // 3 指纹用户
                        // 4 系统设置
                        Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
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
                    } else if (coded_lock_t.total_num_card > 0) {
                        for (i = 0; i < 10; i++) {
                            if (coded_lock_t.card[i].isable) {
                                if (strncmp(str_uid, coded_lock_t.card[i].word, uid_len * 2) == 0) {
                                    device_t.tamperAlarm = 0;
                                    device_t.enterRootNum = i;
                                    device_t.enterRootType = 1;
                                    device_t.lockMode = SYSTEM_MODE_WAIT_CHOOSE_MODE;
                                    DEBUG_TRACE(LOG_TAG, "Exec Success, Wait Choose Mode, Lock Mode %d", device_t.lockMode);
                                    /* 语音 */
                                    // 操作成功
                                    // 1 密码用户
                                    // 2 卡用户
                                    // 3 指纹用户
                                    // 4 系统设置
                                    Sound_Insert_Number(1, VOICE_EXCE_SUCCESS);
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
                                    break;
                                }
                            }
                        }
                        if (i >= 10) {
                            goto __card_fail;
                        }
                    } else {
                    __card_fail:
                        /* 语音 */
                        // 操作失败
                        // 请输入管理员信息
                        Sound_Insert_Number(1, VOICE_EXCE_FAIL);
                        if (++device_t.enterRootFailCnt >= 5) {
                            device_t.enterRootFailCnt = 0;
                            device_t.lockTimeout = 100 * SEC_DELAY;

                            /* 语音 */
                            // 系统已锁定
                            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
#if FUNC_FINGERPRINT_ENABLE
                            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                            // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                        } else
                            Sound_Insert_Number(1, VOICE_PLS_INPUT_ROOT_INFO);
                    }
                    // 写入操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.enterRoot = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = i;
                    if (i < 100 || i == 0xFF)
                        exc_lock_info_t.u_exec.unlockSuccess = 1;
                    else
                        exc_lock_info_t.u_exec.unlockFail = 1;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                    tx_thread_sleep(2000);
                }
            } else {
                userNum = 0xFF;
                if (coded_lock_t.total_num_card == 0) {
                    device_t.openFailCnt++;
                    action = 0xC3; // MF 卡报警
                    DEBUG_TRACE(WARN_TAG, "Card Uid Not Match!!!");

                    /* 语音 */
                    // 失败提示音
                    Sound_Insert_Number(0, VOICE_TONE_FAIL);
                    if (device_t.openFailCnt >= 5) {
                        device_t.lockTimeout = 100 * SEC_DELAY;
#if FUNC_FINGERPRINT_ENABLE
                        tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                        // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                        DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                        /* 语音 */
                        // 系统已锁定
                        Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                    } else {
                        /* 语音 */
                        // 开锁失败
                        Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
#if FUNC_FINGERPRINT_ENABLE
                        tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_BLUE, TX_OR);
                        // finger_led_control(3, 1, 1, 0); // 蓝色常亮
#endif
                    }
                    // memset(buffer, 0, sizeof(buffer));
                    // sendLen = 7;

                    // buffer[0] = 0x30;
                    // buffer[1] = action;  // 操作记录
                    // buffer[2] = userNum; // 用户编号
                    // buffer[3] = (Timestamp >> 24) & 0xFF;
                    // buffer[4] = (Timestamp >> 16) & 0xFF;
                    // buffer[5] = (Timestamp >> 8) & 0xFF;
                    // buffer[6] = Timestamp & 0xFF;

                    // int free_number = lwrb_get_free(&lwrbBuff_t);
                    // if (free_number >= (sendLen + 1))
                    // {
                    //     lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    //     lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    //     lwrb_write_count++;
                    // }
                    // else
                    // {
                    //     DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                    // }

                    //                    if (action == 0xC3) // 写入开锁失败记录
                    //                    {
                    //                        struct lock_log exc_lock_info_t;
                    //                        memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    //                        exc_lock_info_t.u_exec.isLocal = 1;
                    //                        exc_lock_info_t.u_exec.unlockFail = 1;
                    //                        exc_lock_info_t.type = 2;
                    //                        exc_lock_info_t.number = userNum;
                    //                        if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger)
                    //                        {
                    //                            strcat(exc_lock_info_t.word, "123456");
                    //                        }
                    //                        else
                    //                        {
                    //                            strcat(exc_lock_info_t.word, str_uid);
                    //                        }
                    //                        exc_lock_info_t.stamp = Timestamp;
                    //                        nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                    //                    }
                } else {
                    for (i = 0; i < 100; i++) {
                        if (coded_lock_t.card[i].isable) {
                            if (strncmp(str_uid, coded_lock_t.card[i].word, uid_len * 2) == 0) {
                                userNum = i;
                                action = 0x03; // MF卡开门
                                /* 卡验证成功 执行开锁 */
                                tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                                DEBUG_TRACE(LOG_TAG, "Card Unlock[%d]: %s", userNum, str_uid); // UID匹配成功
#if FUNC_FINGERPRINT_ENABLE
                                tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                                // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                                break;
                            }
                        }
                    }
                    if (i >= 100) {
                        action = 0xC3; // MF卡报警
                        device_t.openFailCnt++;
                        /* 语音 */
                        // 失败提示音
                        Sound_Insert_Number(0, VOICE_TONE_FAIL);
                        if (device_t.openFailCnt >= 5) {
                            device_t.lockTimeout = 100 * SEC_DELAY;
#if FUNC_FINGERPRINT_ENABLE
                            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
                            // finger_led_control(3, 0, 0, 0); // 关闭灯光
#endif
                            DEBUG_TRACE(LOG_TAG, "System Enter Lock Status");
                            /* 语音 */
                            // 系统已锁定
                            Sound_Insert_Number(1, VOICE_SYSTEM_LOCK);
                        } else {
                            /* 语音 */
                            // 开锁失败
                            Sound_Insert_Number(1, VOICE_OPEN_LOCK_FAIL);
#if FUNC_FINGERPRINT_ENABLE
                            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_BLUE, TX_OR);
                            // finger_led_control(3, 1, 1, 0); // 蓝色常亮
#endif
                        }
                        DEBUG_TRACE(LOG_TAG, "Card Uid Not Match: %s", str_uid);
                    }
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
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }

                // 写入开锁失败记录
                if (action == 0xC3) {
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockFail = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                        strcat(exc_lock_info_t.word, "123456");
                    } else {
                        strcat(exc_lock_info_t.word, str_uid);
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                } else if (action == 0x03)                    // 写入开锁成功记录
                {
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.unlockSuccess = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                        strcat(exc_lock_info_t.word, "123456");
                    } else {
                        strcat(exc_lock_info_t.word, str_uid);
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }

                /* MF卡开锁解除防拆报警*/
                if (device_t.tamperAlarm && action == 0x03) {
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
                        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                    }

                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 1;
                    exc_lock_info_t.u_exec.tamperRecover = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    if (!coded_lock_t.total_num_pass && !coded_lock_t.total_num_card && !coded_lock_t.total_num_finger) {
                        strcat(exc_lock_info_t.word, "123456");
                    } else {
                        strcat(exc_lock_info_t.word, coded_lock_t.card[i].word);
                    }
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }

                if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                if (action == 0xC3) { // 开锁失败停止读卡一段时间
                    // LED闪烁一次
                    MBI5120_Display(0);
                    tx_thread_sleep(2000);
                    MBI5120_Display(0xFFFF);
                    tx_thread_sleep(1000);
                }
            }
        }
    }
}
#endif

#endif
