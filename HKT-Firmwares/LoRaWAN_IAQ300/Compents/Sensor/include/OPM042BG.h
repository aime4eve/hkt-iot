
#ifndef __OPM042BG_H__
#define __OPM042BG_H__

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

//定义屏的分辨率
#define Source_Pixel 400
#define Source_Pixel_high Source_Pixel / 256 // Source分辨率的高8位
#define Source_Pixel_low Source_Pixel % 256  // Source分辨率的低8位
#define Gate_Pixel 300
#define Gate_Pixel_high Gate_Pixel / 256             // Gate分辨率的高8位
#define Gate_Pixel_low Gate_Pixel % 256              // Gate分辨率的低8位
#define Picture_length Source_Pixel / 8 * Gate_Pixel // 400*300/8=15000bytes

// ** IO ** //
#define OPM_RST_PORT GPIOE
// #define OPM_RST_PIN GPIO_Pin_8
#define OPM_RST_PIN GPIO_Pin_2
#define OPM_RST_H GPIO_SetBits(OPM_RST_PORT, OPM_RST_PIN)
#define OPM_RST_L GPIO_ResetBits(OPM_RST_PORT, OPM_RST_PIN)

#define OPM_CSB_PORT GPIOE
// #define OPM_CSB_PIN GPIO_Pin_7
#define OPM_CSB_PIN GPIO_Pin_8
#define OPM_CSB_H GPIO_SetBits(OPM_CSB_PORT, OPM_CSB_PIN)
#define OPM_CSB_L GPIO_ResetBits(OPM_CSB_PORT, OPM_CSB_PIN)

#define OPM_SDA_PORT GPIOE
// #define OPM_SDA_PIN GPIO_Pin_6
#define OPM_SDA_PIN GPIO_Pin_5
#define OPM_SDA_H GPIO_SetBits(OPM_SDA_PORT, OPM_SDA_PIN)
#define OPM_SDA_L GPIO_ResetBits(OPM_SDA_PORT, OPM_SDA_PIN)
#define READ_OPM_SDA_STATUS GPIO_ReadInputDataBit(OPM_SDA_PORT, OPM_SDA_PIN)
#define OPM_SDA_OUT OutputIO(OPM_SDA_PORT, OPM_SDA_PIN, OUT_PUSHPULL); // gpio 配置为输出
#define OPM_SDA_IN InputtIO(OPM_SDA_PORT, OPM_SDA_PIN, IN_NORMAL);     // gpio 配置为输入

#define OPM_SCL_PORT GPIOE
// #define OPM_SCL_PIN GPIO_Pin_5
#define OPM_SCL_PIN GPIO_Pin_7
#define OPM_SCL_H GPIO_SetBits(OPM_SCL_PORT, OPM_SCL_PIN)
#define OPM_SCL_L GPIO_ResetBits(OPM_SCL_PORT, OPM_SCL_PIN)

#define OPM_DC_PORT GPIOE
#define OPM_DC_PIN GPIO_Pin_4
#define OPM_DC_H GPIO_SetBits(OPM_DC_PORT, OPM_DC_PIN)
#define OPM_DC_L GPIO_ResetBits(OPM_DC_PORT, OPM_DC_PIN)

#define OPM_BUSY_PORT GPIOE
#define OPM_BUSY_PIN GPIO_Pin_3
#define READ_OPM_BUSY_STATUS GPIO_ReadInputDataBit(OPM_BUSY_PORT, OPM_BUSY_PIN)

#define DELAY_TIME 8 // 图片显示完停留时间(单位:秒) 主频升高8倍，时间是1/8。再加上补偿送数据减少的时间。

// 测试图
#define PIC_WHITE 255                  // 全白
#define PIC_BLACK 254                  // 全黑
#define PIC_Orientation 253            // 方向图
#define PIC_CHESSBOARD 252             // 单像素棋盘格
#define PIC_Source_Line 251            // Source线
#define PIC_Gate_Line 250              // Gate线
#define PIC_LEFT_BLACK_RIGHT_WHITE 249 // 左黑右白
#define PIC_UP_BLACK_DOWN_WHITE 248    // 上黑下白

// Display resolution
#define EPD_WIDTH 400
#define EPD_HEIGHT 300

// EPD4IN2 commands
#define PANEL_SETTING 0x00
#define POWER_SETTING 0x01
#define POWER_OFF 0x02
#define POWER_OFF_SEQUENCE_SETTING 0x03
#define POWER_ON 0x04
#define POWER_ON_MEASURE 0x05
#define BOOSTER_SOFT_START 0x06
#define DEEP_SLEEP 0x07
#define DATA_START_TRANSMISSION_1 0x10
#define DATA_STOP 0x11
#define DISPLAY_REFRESH 0x12
#define DATA_START_TRANSMISSION_2 0x13
#define LUT_FOR_VCOM 0x20
#define LUT_WHITE_TO_WHITE 0x21
#define LUT_BLACK_TO_WHITE 0x22
#define LUT_WHITE_TO_BLACK 0x23
#define LUT_BLACK_TO_BLACK 0x24
#define PLL_CONTROL 0x30
#define TEMPERATURE_SENSOR_COMMAND 0x40
#define TEMPERATURE_SENSOR_SELECTION 0x41
#define TEMPERATURE_SENSOR_WRITE 0x42
#define TEMPERATURE_SENSOR_READ 0x43
#define VCOM_AND_DATA_INTERVAL_SETTING 0x50
#define LOW_POWER_DETECTION 0x51
#define TCON_SETTING 0x60
#define RESOLUTION_SETTING 0x61
#define GSST_SETTING 0x65
#define GET_STATUS 0x71
#define AUTO_MEASUREMENT_VCOM 0x80
#define READ_VCOM_VALUE 0x81
#define VCM_DC_SETTING 0x82
#define PARTIAL_WINDOW 0x90
#define PARTIAL_IN 0x91
#define PARTIAL_OUT 0x92
#define PROGRAM_MODE 0xA0
#define ACTIVE_PROGRAMMING 0xA1
#define READ_OTP 0xA2
#define POWER_SAVING 0xE3

    extern u8 ImageBuff[Source_Pixel / 8 * Gate_Pixel];
    void OPM_PortCfg(void);
    void OPM_Init(void);
    void OPM_TEST(void);
    void EPD_TEST(void);

    void lut_GC();
    void lut_DU();
    void EPD_ShowDevEUI(void);
    void EPD_ShowFirmUpdate(void);
    void EPD_Display(const unsigned char *image_buffer);
    void EPD_Display_Du(const unsigned char *image_buffer);
    void EPD_Clear(void);
    void ENTER_DEEP_SLEEP(void);
    void EPD_ShowUpdate(void);
    void EPD_ShowLoading(void);
    void EPD_ShowClean(void);
    void EPD_Custom_Number24_Show_TempHumi(u16 x, u16 y, u32 data, u16 enlargement, u8 dot_size);
    void EPD_Custom_Number24_Show_Other(u16 x, u16 y, u16 data);
    void EPD_Custom_Number24_Show_PM(u16 x, u16 y, u16 data);
    void EPD_Custom_Number24_Show_HCHO(u16 x, u16 y, u16 data);
    void EPD_DU_TEST(void);

    void thread_display(ULONG thread_input);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
