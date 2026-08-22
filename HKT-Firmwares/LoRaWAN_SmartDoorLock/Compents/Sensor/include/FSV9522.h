
#ifndef __FSV9522_H__
#define __FSV9522_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// 命令字
/////////////////////////////////////////////////////////////////////
#define fsv9522_IDLE 0x00       // 取消当前命令
#define fsv9522_MEM 0x01        // CONFIG命令
#define fsv9522_AUTHENT 0x0E    // 验证密钥
#define fsv9522_RECEIVE 0x08    // 接收数据
#define fsv9522_TRANSMIT 0x04   // 发送数据
#define fsv9522_TRANSCEIVE 0x0C // 发送并接收数据
#define fsv9522_RESETPHASE 0x0F // 复位
#define fsv9522_CALCCRC 0x03    // CRC计算
#define fsv9522_AUTOCOLL 0x0D   // MIFARE anticollision, Card Operation mode only

// 寄存器定义
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define RFU00 0x00
#define CommandReg 0x01
#define ComIEnReg 0x02
#define DivlEnReg 0x03
#define ComIrqReg 0x04
#define DivIrqReg 0x05
#define ErrorReg 0x06
#define Status1Reg 0x07
#define Status2Reg 0x08
#define FIFODataReg 0x09
#define FIFOLevelReg 0x0A
#define WaterLevelReg 0x0B
#define ControlReg 0x0C
#define BitFramingReg 0x0D
#define CollReg 0x0E
#define RFU0F 0x0F
// PAGE 1
#define RFU10 0x10
#define ModeReg 0x11
#define TxModeReg 0x12
#define RxModeReg 0x13
#define TxControlReg 0x14
#define TxAskReg 0x15  // rc523
#define TxAutoReg 0x15 // pn512
#define TxSelReg 0x16
#define RxSelReg 0x17
#define RxThresholdReg 0x18
#define DemodReg 0x19
#define RFU1A 0x1A
#define RFU1B 0x1B
#define MifareReg 0x1C
#define ManualRCVReg 0x1D
#define TypeBReg 0x1E
#define SerialSpeedReg 0x1F
// PAGE 2
#define RFU20 0x20
#define CRCResultRegM 0x21
#define CRCResultRegL 0x22
#define GsNOffReg 0x23
#define ModWidthReg 0x24
#define TxBitPhaseReg 0x25
#define RFCfgReg 0x26
#define GsNOnReg 0x27
#define CWGsPReg 0x28
#define ModGsPReg 0x29
#define TModeReg 0x2A
#define TPrescalerReg 0x2B
#define TReloadRegH 0x2C
#define TReloadRegL 0x2D
#define TCounterValueRegH 0x2E
#define TCounterValueRegL 0x2F
// PAGE 3
#define RFU30 0x30
#define TestSel1Reg 0x31
#define TestSel2Reg 0x32
#define TestPinEnReg 0x33
#define TestPinValueReg 0x34
#define TestBusReg 0x35
#define AutoTestReg 0x36
#define VersionReg 0x37
#define AnalogTestReg 0x38
#define TestDAC1Reg 0x39
#define TestDAC2Reg 0x3A
#define TestADCReg 0x3B
#define RFU3C 0x3C
#define RFU3D 0x3D
#define RFU3E 0x3E
#define RFU3F 0x3F

// M1,ISO14443A
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL 0x26    // 寻天线区内未进入休眠状态
#define PICC_REQALL 0x52    // 寻天线区内全部卡
#define PICC_ANTICOLL1 0x93 // 防冲撞
#define PICC_ANTICOLL2 0x95 // 防冲撞
#define PICC_ANTICOLL3 0x97
#define PICC_AUTHENT1A 0x60 // 验证A密钥
#define PICC_AUTHENT1B 0x61 // 验证B密钥
#define PICC_READ 0x30      // 读块
#define PICC_WRITE 0xA0     // 写块
#define PICC_DECREMENT 0xC0 // 扣款
#define PICC_INCREMENT 0xC1 // 充值
#define PICC_RESTORE 0xC2   // 调块数据到缓冲区
#define PICC_TRANSFER 0xB0  // 保存缓冲区中数据
#define PICC_HALT 0x50      // 休眠

/////////////////////////////////////////////////////////////////////ISO14443B
#define PICC_ANTI 0x05
#define PICC_ATTRIB 0x1D

/////////////////////////////////////////////////////////////////////
#define MI_OK 0
#define MI_CHK_OK 0

#define MI_NOTAGERR (-1)
#define MI_CHK_FAILED (-1)
#define MI_CRCERR (-2)
#define MI_CHK_COMPERR (-2)
#define MI_EMPTY (-3)
#define MI_AUTHERR (-4)
#define MI_PARITYERR (-5)
#define MI_CODEERR (-6)
#define MI_SERNRERR (-8)
#define MI_KEYERR (-9)
#define MI_NOTAUTHERR (-10)
#define MI_BITCOUNTERR (-11)
#define MI_BYTECOUNTERR (-12)
#define MI_IDLE (-13)
#define MI_TRANSERR (-14)
#define MI_WRITEERR (-15)
#define MI_INCRERR (-16)
#define MI_DECRERR (-17)
#define MI_READERR (-18)
#define MI_OVFLERR (-19)
#define MI_POLLING (-20)
#define MI_FRAMINGERR (-21)
#define MI_ACCESSERR (-22)
#define MI_UNKNOWN_COMMAND (-23)
#define MI_COLLERR (-24)
#define MI_RESETERR (-25)
#define MI_INITERR (-25)
#define MI_INTERFACEERR (-26)
#define MI_ACCESSTIMEOUT (-27)
#define MI_NOBITWISEANTICOLL (-28)
#define MI_QUIT (-30)
#define MI_RECBUF_OVERFLOW (-50)
#define MI_SENDBYTENR (-51)
#define MI_SENDBUF_OVERFLOW (-53)
#define MI_BAUDRATE_NOT_SUPPORTED (-54)
#define MI_SAME_BAUDRATE_REQUIRED (-55)
#define MI_WRONG_PARAMETER_VALUE (-60)
#define MI_BREAK (-99)
#define MI_NY_IMPLEMENTED (-100)
#define MI_NO_MFRC (-101)
#define MI_MFRC_NOTAUTH (-102)
#define MI_WRONG_DES_MODE (-103)
#define MI_HOST_AUTH_FAILED (-104)
#define MI_WRONG_LOAD_MODE (-106)
#define MI_WRONG_DESKEY (-107)
#define MI_MKLOAD_FAILED (-108)
#define MI_FIFOERR (-109)
#define MI_WRONG_ADDR (-110)
#define MI_DESKEYLOAD_FAILED (-111)
#define MI_WRONG_SEL_CNT (-114)
#define MI_WRONG_TEST_MODE (-117)
#define MI_TEST_FAILED (-118)
#define MI_TOC_ERROR (-119)
#define MI_COMM_ABORT (-120)
#define MI_INVALID_BASE (-121)
#define MI_MFRC_RESET (-122)
#define MI_WRONG_VALUE (-123)
#define MI_VALERR (-124)
#define MI_COM_ERR (-125)
#define MI_ERR (-126)

#define DEF_FIFO_LENGTH 64 // FIFO size=64byte
#define MAXRLEN 18

// set IRQ LEVEL
#define IRQ_HIGH_LEVEL 1             // set high level irq
#define IRQ_LOW_LEVEL 2              // set low level irq
#define IRQ_LEVEL_SET IRQ_LOW_LEVEL // 设为高电平中断

// set irq output mode
#define IRQ_OUT_PP 1            // set IRQ CMOS ouput
#define IRQ_OUT_OD 2            // set IRQ OPEN drain
#define IRQ_OUT_MODE IRQ_OUT_PP // 中断输出设置为CMOS

// sensitive adjust, 0-8
#define SENSITIVE_MIN 0
#define SENSITIVE_SEC 2
#define SENSITIVE_MID 4
#define SENSITIVE_SIX 6
#define SENSITIVE_MAX 8

#define LPCD_Sensitive_Value SENSITIVE_MID //  灵敏度设置

/* set check cycle */
#define CHECK_PERIOD_110MS 0x08ff // 110ms +-12%
#define CHECK_PERIOD_220MS 0x10ff // 220ms +-12%
#define CHECK_PERIOD_330MS 0x1aff // 330ms +-12%
#define CHECK_PERIOD_430MS 0x22ff // 430ms +-12%

/*设置LPCD检测周期*/
#define LPCD_Cycle_Time CHECK_PERIOD_430MS // CHECK_PERIOD_430MS   430ms

/* set LPCD check time */
#define CHECK_TIME_10US 0x007f  // 10us
#define CHECK_TIME_05US 0x003f  // 5us
#define CHECK_TIME_2D5US 0x001f // 2.5us

/*选择检卡时长*/
#define LPCD_Duration_Time CHECK_TIME_10US

/* 读卡寄存器配置，不同的天线适配不同的配置，选择最优的定义 */
#define REG_SET_1 1 // A卡寄存配置1
#define REG_SET_2 2 // A卡寄存配置2
#define REG_SET_3 3 // A卡寄存配置3
#define REG_SET_4 4 // A卡寄存配置4

#define TYPEA_REG_CONFIG REG_SET_4 // 选择寄存器配置4

struct TranSciveBuffer {
    u8 MfCommand;
    u16 MfLength;
    u8 MfData[DEF_FIFO_LENGTH];
};

#if (FSV9522_CARD_ENABLE)

#define FSV9522_BIT_IDLE (1 << 0)
#define FSV9522_BIT_SLEEP (1 << 1)
#define FSV9522_BIT_SLEEP_DONE (1 << 2)

#define FSV9522_BIT_DISPLAY_ALL (FSV9522_BIT_IDLE | FSV9522_BIT_SLEEP | FSV9522_BIT_SLEEP_DONE)

extern TX_EVENT_FLAGS_GROUP fsv9522_event_group;

void thread_card(ULONG thread_input);
void fsv9522_init(void);
void fsv9522_rst(void);
void Clear_lpcd_irq(void);
void fsv9522_LPCD_Stop(void);
void fsv9522_sleep(void);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
