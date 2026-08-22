

#include "EPD.h"
#include "GUI_Paint.h"
#include "adc.h"
#include "fonts.h"
#include "gpio.h"
#include "image.h"
#include "spi.h"
#include "systick.h"
#include "trng.h"
#include "uart.h"

#include "LoRaWAN_ATCMD.h"
#include "QR_Encode.h"
#include "communicate.h"
#include "control_center.h"
#include <LoRaWAN_APPLY.h>

#if EPD_E042A162

u8 ImageBuff[Source_Pixel / 8 * Gate_Pixel];
u8 epd_status;

/**
 * @brief  电子纸GPIO初始化
 * @param
 * @retval
 */
void EPD_PortCfg(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能

    /** 初始化 IO **/
    OutputIO(EPD_RST_PORT, EPD_RST_PIN, OUT_PUSHPULL);
    OutputIO(EPD_CSB_PORT, EPD_CSB_PIN, OUT_PUSHPULL);
    OutputIO(EPD_SDA_PORT, EPD_SDA_PIN, OUT_PUSHPULL);
    OutputIO(EPD_SCL_PORT, EPD_SCL_PIN, OUT_PUSHPULL);
    OutputIO(EPD_DC_PORT, EPD_DC_PIN, OUT_PUSHPULL);
    InputtIO(EPD_BUSY_PORT, EPD_BUSY_PIN, IN_NORMAL);
    EPD_PWR_CTRL_L;
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xx   电子纸驱动操作函数    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

// 读忙 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void READBUSY(void) // 检查硬件BUSY线是否忙？注意FPC接口要连接良好，否则会一直在这里死循环。
{
    unsigned char busy_cnt = 0;
    while (1) {
        if (!READ_EPD_BUSY_STATUS)
            break; // 硬件BUSY线高电平=不忙时跳出循环。注意高电平时不一定是1，可能是其它值，看具体I/O口的位。
        else {
            if (++busy_cnt >= 20) {
                busy_cnt = 0;
                device_t.epdShowDuCnt = 0; // 出现超时下一次固定全刷
                DEBUG_TRACE(WARN_TAG, "%s Timeout!!!", __func__);
                break;
            }
        }
        tx_thread_sleep(100);
    }
    // tx_thread_sleep(500); // 显示完成后等待1S
}

bool READBUSY_CALLBACK(void)
{
    unsigned char busy_cnt = 0;
    while (1) {
        if (!READ_EPD_BUSY_STATUS)
            break; // 硬件BUSY线高电平=不忙时跳出循环。注意高电平时不一定是1，可能是其它值，看具体I/O口的位。
        else {
            if (++busy_cnt >= 20) {
                busy_cnt = 0;
                device_t.epdShowDuCnt = 0; // 出现超时下一次固定全刷
                DEBUG_TRACE(WARN_TAG, "%s Timeout!!!", __func__);
                return false;
            }
        }
        tx_thread_sleep(100);
    }
    return true;
}

// 写命令 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void SPI4W_WRITECOM(unsigned char INIT_COM)
{
    unsigned char TEMPCOM;
    unsigned char scnt;

    TEMPCOM = INIT_COM;
    EPD_SCL_L;
    EPD_DC_L;
    EPD_CSB_H;
    EPD_CSB_L;
    asm("nop");
    for (scnt = 0; scnt < 8; scnt++) {
        if (TEMPCOM & 0x80)
            EPD_SDA_H;
        else
            EPD_SDA_L;
        EPD_SCL_H;
        asm("nop");
        asm("nop");
        EPD_SCL_L;
        TEMPCOM = TEMPCOM << 1;
    }
    asm("nop");
    // EPD_CSB_H;
    // EPD_SDA_L;
    asm("nop");
}

// 写数据 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void SPI4W_WRITEDATA(unsigned char INIT_DATA)
{
    unsigned char TEMPCOM;
    unsigned char scnt;

    TEMPCOM = INIT_DATA;
    EPD_SCL_L;
    EPD_DC_H;
    EPD_CSB_H;
    EPD_CSB_L;
    asm("nop");
    for (scnt = 0; scnt < 8; scnt++) {
        if (TEMPCOM & 0x80)
            EPD_SDA_H;
        else
            EPD_SDA_L;
        EPD_SCL_H;
        asm("nop");
        asm("nop");
        EPD_SCL_L;
        TEMPCOM = TEMPCOM << 1;
    }
    asm("nop");
    // EPD_CSB_H;
    // EPD_SDA_L;
    // EPD_DC_L;
    asm("nop");
}

// 复位 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void EPD_RESET()
{
    EPD_DC_L;
    EPD_RST_L;
    tx_thread_sleep(5);
    EPD_RST_H;
    tx_thread_sleep(10);
    READBUSY();
}

// 入深度睡眠 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void ENTER_DEEP_SLEEP() // 注意：进入深度睡眠模式后，不接受命令，只能用硬件RESET才能重新操作，软件RESET也不行。然后需要重新初始化配置。
{
    SPI4W_WRITECOM(0x10);
    SPI4W_WRITEDATA(0x01);
    EPD_CSB_H;
}

void EPD_Rfresh_MCU()
{
    SPI4W_WRITECOM(0x22);
    SPI4W_WRITEDATA(0xF7); // Load LUT from MCU(0x32), Display update
    SPI4W_WRITECOM(0x20);
    READBUSY(); // 判断屏是否完成更新图像？
}

void EPD_Rfresh_OTP()
{
    SPI4W_WRITECOM(0x22);
    SPI4W_WRITEDATA(0xFF); // Load LUT from OTP
    // SPI4W_WRITEDATA(0xF7); // Load LUT from MCU(0x32), Display update
    SPI4W_WRITECOM(0x20);
    READBUSY(); // 判断屏是否完成更新图像？
}

// 电子纸驱动初始化 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void EPD_Init(void)
{
    EPD_PortCfg();
    EPD_RESET(); // reset
    SPI4W_WRITECOM(0x12); // 写复位
    READBUSY();
    SPI4W_WRITECOM(0x01); // Set Driver Output Control for Gate setting = Gate Channel - 1
    SPI4W_WRITEDATA(0x2B); // For example, for 300 gate channel => 300 - 1 = 299 (0x12B)
    SPI4W_WRITEDATA(0x01);
    SPI4W_WRITEDATA(0x00);

    SPI4W_WRITECOM(0x11); // Set Data Entry Mode setting
    SPI4W_WRITEDATA(0x01); // Y decrement, X increment, address counter in X-direction

    SPI4W_WRITECOM(0x44); // Set RAM X address
    SPI4W_WRITEDATA(0x00); // start position = 0
    SPI4W_WRITEDATA(0x31); //  end position = Source Channel/8 - 1 => 49(0x31)

    SPI4W_WRITECOM(0x45); // Set RAM Y address
    SPI4W_WRITEDATA(0x2B); // start position = 300 - 1 = 299 (0x12B) and
    SPI4W_WRITEDATA(0x01);
    SPI4W_WRITEDATA(0x00); //  end position = 0
    SPI4W_WRITEDATA(0x00);

    SPI4W_WRITECOM(0x3C); // board
    SPI4W_WRITEDATA(0x01); // White: 0x01

    SPI4W_WRITECOM(0x21);
    SPI4W_WRITEDATA(0x00);

    SPI4W_WRITECOM(0x18); // 选择使用内部温度传感器
    SPI4W_WRITEDATA(0x80);

    SPI4W_WRITECOM(0x4E); // Set RAM X address counter = 0
    SPI4W_WRITEDATA(0x00);
    SPI4W_WRITECOM(0x4F); // Set Driver Output Control for Gate setting = Gate Channel - 1
    SPI4W_WRITEDATA(0x2B); // For example, for 300 gate channel => 300 - 1 = 299 (0x12B)
    SPI4W_WRITEDATA(0x01);
}

// 清除显示
void EPD_Clear()
{
    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;

    device_t.epdOldImageShow = 0;
    EPD_Init();

    // SPI4W_WRITECOM(0x4E);
    // SPI4W_WRITEDATA(0x00);
    // SPI4W_WRITECOM(0x4F);
    // SPI4W_WRITEDATA(0x2B);
    // SPI4W_WRITEDATA(0x01);

    SPI4W_WRITECOM(0x24); // Write B/W image data into to Register 0x24 RAM
    for (Ypoint = 0; Ypoint < Height; Ypoint++) {
        for (Xpoint = 0; Xpoint < Width; Xpoint++) {
            if (device_t.epdOverturnShow) {
                SPI4W_WRITEDATA(WHITE);
            } else {
                SPI4W_WRITEDATA(WHITE);
            }
        }
    }

    // SPI4W_WRITECOM(0x4E);
    // SPI4W_WRITEDATA(0x00);
    // SPI4W_WRITECOM(0x4F);
    // SPI4W_WRITEDATA(0x2B);
    // SPI4W_WRITEDATA(0x01);

    SPI4W_WRITECOM(0x26); // Write B/W image data into to Register 0x24 RAM
    for (Ypoint = 0; Ypoint < Height; Ypoint++) {
        for (Xpoint = 0; Xpoint < Width; Xpoint++) {
            if (device_t.epdOverturnShow) {
                SPI4W_WRITEDATA(WHITE);
            } else {
                SPI4W_WRITEDATA(WHITE);
            }
        }
    }

    EPD_Rfresh_MCU();
}

// 全刷显示
void EPD_Display(const unsigned char *image_buffer)
{
    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;
    u32 Addr = 0;

    device_t.epdOldImageShow = 0;
    if (READBUSY_CALLBACK() == true) {

        // SPI4W_WRITECOM(0x4E);
        // SPI4W_WRITEDATA(0x00);
        // SPI4W_WRITECOM(0x4F);
        // SPI4W_WRITEDATA(0x2B);
        // SPI4W_WRITEDATA(0x01);

        SPI4W_WRITECOM(0x24);
        for (Ypoint = 0; Ypoint < Height; Ypoint++) {
            for (Xpoint = 0; Xpoint < Width; Xpoint++) { // 8 pixel =  1 byte
                Addr = Xpoint + Ypoint * Width;
                SPI4W_WRITEDATA(image_buffer[Addr]);
            }
        }

        // SPI4W_WRITECOM(0x4E);
        // SPI4W_WRITEDATA(0x00);
        // SPI4W_WRITECOM(0x4F);
        // SPI4W_WRITEDATA(0x2B);
        // SPI4W_WRITEDATA(0x01);

        SPI4W_WRITECOM(0x26);
        for (Ypoint = 0; Ypoint < Height; Ypoint++) {
            for (Xpoint = 0; Xpoint < Width; Xpoint++) { // 8 pixel =  1 byte
                Addr = Xpoint + Ypoint * Width;
                SPI4W_WRITEDATA(image_buffer[Addr]);
            }
        }
        EPD_Rfresh_MCU();

    } else {
        device_t.epdInit = 1;
        device_t.epdShowDuCnt = 0;
        tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
    }
}

// 局刷显示
void EPD_Display_Du(const unsigned char *image_buffer)
{
    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;
    u32 Addr = 0;

    // SPI4W_WRITECOM(0x4E);
    // SPI4W_WRITEDATA(0x00);
    // SPI4W_WRITECOM(0x4F);
    // SPI4W_WRITEDATA(0x2B);
    // SPI4W_WRITEDATA(0x01);

    SPI4W_WRITECOM(0x24); // Write B/W image data into to Register 0x24 RAM
    if (READBUSY_CALLBACK() == true) {
        for (Ypoint = 0; Ypoint < Height; Ypoint++) {
            for (Xpoint = 0; Xpoint < Width; Xpoint++) { // 8 pixel =  1 byte
                Addr = Xpoint + Ypoint * Width;
                SPI4W_WRITEDATA(image_buffer[Addr]);
            }
        }
        EPD_Rfresh_OTP();
    } else {
        device_t.epdInit = 1;
        device_t.epdShowDuCnt = 0;
        tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
    }
}

// 显示设备DEVEUI
void EPD_ShowDevEUI(void)
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;
    memset(ImageBuff, 0, sizeof(ImageBuff));
    char qr_msg[] = "HKT AIR SENSOR \r\nDevEUI:123456781234578";

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
    }

    device_t.epdShowDuCnt = 0;

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    Paint_SelectImage(IMAGE_BW);
    char epd_deveui[] = "DevEUI:1234567812345789";
    snprintf(epd_deveui, sizeof(epd_deveui), "DevEUI:%02X%02X%02X%02X%02X%02X%02X%02X", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2],
             LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);

    snprintf(qr_msg, sizeof(qr_msg), "HKT AIR SENSOR\r\n%s", epd_deveui);
    EncodeData(qr_msg);
    int y_start = 50, x_start = 138;
    for (int y = 0; y < 33; y++) {
        for (int x = 0; x < 33; x++) {
            if (m_byModuleData[y][x])
                Paint_DrawPoint(x_start + x * 5, y_start + y * 5, foreground_color, DOT_PIXEL_5X5, DOT_FILL_AROUND);
            else
                Paint_DrawPoint(x_start + x * 5, y_start + y * 5, background_color, DOT_PIXEL_5X5, DOT_FILL_AROUND);
        }
    }

    Paint_DrawString_EN(2, 200, epd_deveui, &Font24, foreground_color, background_color);
    EPD_Display(ImageBuff);
}

void EPD_ShowFirmUpdate(void)
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;
    memset(ImageBuff, 0, sizeof(ImageBuff));

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
    }

    device_t.epdShowDuCnt = 0;

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    Paint_SelectImage(IMAGE_BW);
    Paint_DrawString_EN(30, 150, "Firmware Updating...", &Font24, foreground_color, background_color);
    EPD_Display(ImageBuff);
}

void EPD_ShowLoading(void)
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;
    memset(ImageBuff, 0, sizeof(ImageBuff));

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
    }

    device_t.epdShowDuCnt = 0;

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    Paint_SelectImage(IMAGE_BW);
    Paint_DrawString_EN(120, 150, "Load ......", &Font24, foreground_color, background_color);
    EPD_Display(ImageBuff);
}

// 更新电子纸显示
void EPD_ShowUpdate()
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;

    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;
    u32 Addr = 0;

    memset(ImageBuff, 0, sizeof(ImageBuff));

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;
    u8 flip_color = 1;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
        flip_color = 0;
    }

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    // Select Image
    Paint_SelectImage(IMAGE_BW);

    Paint_DrawLine(0, 36, 400, 36, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
    Paint_DrawLine(282, 36, 282, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

#if (FUNC_SHOW_TIME)
    char time[] = "2022/05/26 17:06";
    snprintf(time, sizeof(time), "%04d/%02d/%02d %02d:%02d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min);
    Paint_DrawString_EN(15, 16, (const char *)time, &Font_12_18, foreground_color, background_color);
#elif (FUNC_SHOW_LOGO)
    char time[] = "HKT";
    Paint_DrawString_EN(20, 14, (const char *)time, &Font20, foreground_color, background_color);
#endif

    // Paint_DrawString_EN(210, 0, "HKT", &Font24, background_color, foreground_color);
    if (device_t.powerIn) {
        if (batteryLevel < 10)
            Paint_DrawBitMap_Paste(Image_Battery_0, 314, 15, 24, 15, flip_color); // 电量信息
        else if (batteryLevel < 30)
            Paint_DrawBitMap_Paste(Image_Battery_20, 314, 15, 24, 15, flip_color); // 电量信息
        else if (batteryLevel < 60)
            Paint_DrawBitMap_Paste(Image_Battery_50, 314, 15, 24, 15, flip_color); // 电量信息
        else if (batteryLevel < 90)
            Paint_DrawBitMap_Paste(Image_Battery_80, 314, 15, 24, 15, flip_color); // 电量信息
        else
            Paint_DrawBitMap_Paste(Image_Battery_100, 314, 15, 24, 15, flip_color); // 电量信息
    }

    if (LoRaWAN.joinState == 1)
        Paint_DrawBitMap_Paste(Image_Signal, 356, 15, 24, 16, flip_color); // 入网状态
    else
        Paint_DrawBitMap_Paste(Image_Unsignal, 356, 15, 32, 16, flip_color);

    // CO2 边框坐标
    u16 co2_x_start = 60;
    u16 co2_x_end = co2_x_start + (36 + 1) * 5;
    u16 co2_y_start = 264;
    u16 co2_y_end = 144;

    // CO2 历史记录显示坐标
    u16 co2_x = co2_x_start + 5;
    u16 co2_y = co2_x_start + 5;
    u16 co2_high = 0;
    int x = 0;

    u16 atmos_pm_2_5;
    u16 atmos_pm_10_0;

    // 电子纸显示模式
    switch (device_t.epdShowMode) {
    case 0: // 温湿度 空气质量等级 CO2
        Paint_DrawBitMap_Paste(Image_Temp, 292, 173, 16, 23, flip_color); // 温度图标
        Paint_DrawBitMap_Paste(Image_Humi, 292, 240, 16, 23, flip_color); // 湿度图标
        /*温湿度数值显示*/
        if (device_t.epdShowTempWay == 1)
            EPD_Custom_Number24_Show_TempHumi(315, 195, 32000 + device_t.sensor_t.temperature * 1.8, 1000, DOT_PIXEL_2X2);
        else
            EPD_Custom_Number24_Show_TempHumi(315, 195, device_t.sensor_t.temperature, 1000, DOT_PIXEL_2X2);
        EPD_Custom_Number24_Show_TempHumi(315, 262, device_t.sensor_t.humidity, 1000, DOT_PIXEL_2X2);
        /*温湿度图标显示*/
        if (device_t.epdShowTempWay == 1)
            Paint_DrawBitMap_Paste(Image_FD, 375, 180, 16, 17, flip_color);
        else
            Paint_DrawBitMap_Paste(Image_CE, 375, 180, 16, 17, flip_color);
        Paint_DrawBitMap_Paste(Image_PCT, 375, 250, 16, 17, flip_color);

        Paint_DrawLine(282, 168, 400, 168, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(282, 234, 400, 234, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        // 空气质量等级
        switch (device_t.sensor_t.air_level) {
        case 0: // 空气质量好
            Paint_DrawBitMap_Paste(Image_Good, 322, 76, 40, 37, flip_color); // 空气质量等级（笑脸）
            Paint_DrawBitMap_Paste(Image_excellent, 295, 122, 104, 18, flip_color);
            break;
        case 1: // 空气质量一般
            Paint_DrawBitMap_Paste(Image_Normal, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_drdinary, 300, 122, 96, 18, flip_color);
            break;
        case 2: // 空气质量差
        default:
            Paint_DrawBitMap_Paste(Image_Low, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_lousy, 313, 122, 60, 18, flip_color);
            break;
        }

        // Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 10, flip_color); // CO2图标
        Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 18, flip_color); // CO2图标
        /*告警图标显示*/
        if (device_t.sensor_t.co2 >= 1000 && device_t.sensor_t.co2 < 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        } else if (device_t.sensor_t.co2 >= 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm3, 70, 55, 24, 23, flip_color);
        }
#if 0 // 测试使用
        Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        Paint_DrawBitMap_Paste(Image_Alarm2, 94, 55, 24, 23, flip_color);
        Paint_DrawBitMap_Paste(Image_Alarm3, 118, 55, 24, 23, flip_color);
#endif

        /*CO2数值显示*/ // 36条历史记录
        EPD_Custom_Number24_Show_Other(120, 90, device_t.sensor_t.co2);
        // Paint_DrawBitMap_Paste(Image_PPM, 210, 90, 40, 12, flip_color);
        Paint_DrawBitMap_Paste(Image_PPM, 210, 100, 24, 12, flip_color);
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_start, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_end, co2_y_start, co2_x_end, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_end, co2_y_start, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 水平分割线

        Paint_DrawNum(co2_x_start - (11 * 5 - 5), co2_y_end, 1600, &Font_12_18, foreground_color, background_color);
        Paint_DrawNum(co2_x_start - (11 * 4 - 5), co2_y_start - 16, 400, &Font_12_18, foreground_color, background_color);

        for (x = 0; x < 36; x++) {
            if (device_t.co2_history_buf[x] < 400) {
                co2_high = 1;
            } else if (device_t.co2_history_buf[x] < 1600) {
                co2_high = (device_t.co2_history_buf[x] - 400) / 10;
            } else // MAX
            {
                co2_high = 120;
            }
            co2_y = co2_y_start - co2_high;
            if (device_t.co2_history_buf[x] != 0)
                Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); // 垂直分割线
#if 0 // 测试使用
            if (device_t.co2_history_buf[x] == 0)
                co2_high = 120;
            co2_y = co2_y_start - co2_high;
            Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); //垂直分割线
#endif
            co2_x += 5;
        }
        break;
    case 1: // 温湿度 空气质量等级 CO2  TVOC 光照 大气压
        Paint_DrawBitMap_Paste(Image_Temp, 292, 173, 16, 23, flip_color); // 温度图标
        Paint_DrawBitMap_Paste(Image_Humi, 292, 240, 16, 23, flip_color); // 湿度图标
        /*温湿度数值显示*/
        if (device_t.epdShowTempWay == 1)
            EPD_Custom_Number24_Show_TempHumi(315, 195, 32000 + device_t.sensor_t.temperature * 1.8, 1000, DOT_PIXEL_2X2);
        else
            EPD_Custom_Number24_Show_TempHumi(315, 195, device_t.sensor_t.temperature, 1000, DOT_PIXEL_2X2);
        EPD_Custom_Number24_Show_TempHumi(315, 262, device_t.sensor_t.humidity, 1000, DOT_PIXEL_2X2);
        /*温湿度图标显示*/
        if (device_t.epdShowTempWay == 1)
            Paint_DrawBitMap_Paste(Image_FD, 375, 180, 16, 17, flip_color);
        else
            Paint_DrawBitMap_Paste(Image_CE, 375, 180, 16, 17, flip_color);
        Paint_DrawBitMap_Paste(Image_PCT, 375, 250, 16, 17, flip_color);

        Paint_DrawLine(282, 168, 400, 168, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(0, 234, 400, 234, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(94, 234, 94, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(188, 234, 188, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        // 空气质量等级
        switch (device_t.sensor_t.air_level) {
        case 0: // 空气质量好
            Paint_DrawBitMap_Paste(Image_Good, 322, 76, 40, 37, flip_color); // 空气质量等级（笑脸）
            Paint_DrawBitMap_Paste(Image_excellent, 295, 122, 104, 18, flip_color);
            break;
        case 1: // 空气质量一般
            Paint_DrawBitMap_Paste(Image_Normal, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_drdinary, 300, 122, 96, 18, flip_color);
            break;
        case 2: // 空气质量差
        default:
            Paint_DrawBitMap_Paste(Image_Low, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_lousy, 313, 122, 60, 18, flip_color);
            break;
        }

        co2_y_start = 204;
        co2_y_end = 124;
        // Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 30, 10, flip_color); // CO2图标
        Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 18, flip_color); // CO2图标
        /*告警图标显示*/
        if (device_t.sensor_t.co2 >= 1000 && device_t.sensor_t.co2 < 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        } else if (device_t.sensor_t.co2 >= 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm3, 70, 55, 24, 23, flip_color);
        }

        /*CO2数值显示*/ // 36条历史记录
        EPD_Custom_Number24_Show_Other(120, 80, device_t.sensor_t.co2);
        // Paint_DrawBitMap_Paste(Image_PPM, 210, 80, 40, 12, flip_color);
        Paint_DrawBitMap_Paste(Image_PPM, 210, 90, 24, 12, flip_color);
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_start, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_end, co2_y_start, co2_x_end, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_end, co2_y_start, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 水平分割线

        Paint_DrawNum(co2_x_start - (11 * 5 - 5), co2_y_end, 1600, &Font_12_18, foreground_color, background_color);
        Paint_DrawNum(co2_x_start - (11 * 4 - 5), co2_y_start - 16, 400, &Font_12_18, foreground_color, background_color);
        for (x = 0; x < 36; x++) {
            if (device_t.co2_history_buf[x] < 400) {
                co2_high = 1;
            } else if (device_t.co2_history_buf[x] < 1600) {
                co2_high = (device_t.co2_history_buf[x] - 400) * 2 / 3 / 10;
                if (co2_high < 1)
                    co2_high = 1;
            } else // MAX
            {
                co2_high = 80;
            }
            co2_y = co2_y_start - co2_high;
            if (device_t.co2_history_buf[x] != 0)
                Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); // 垂直分割线
#if 0 // 测试使用
            if (device_t.co2_history_buf[x] == 0)
                co2_high = 80;
            co2_y = co2_y_start - co2_high;
            Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); //垂直分割线
#endif
            co2_x += 5;
        }

        Paint_DrawBitMap_Paste(Image_TVOC, 25, 250, 40, 10, flip_color); // TVOC
        /*TVOC等级显示*/
        switch (device_t.sensor_t.tvoc_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 29, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 29, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 29, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 29, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 29, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 29, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Light, 130, 245, 24, 22, flip_color); // 光照
        /*环境光等级显示*/
        switch (device_t.sensor_t.light_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 127, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 127, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 127, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 127, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 127, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 127, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Press, 190, 245, 24, 18, flip_color); // 大气压
        Paint_DrawBitMap_Paste(Image_MBAR, 252, 286, 24, 10, flip_color); // 大气压图标
        /*大气压数值显示*/
        EPD_Custom_Number24_Show_Other(210, 262, device_t.sensor_t.pressure);
        break;
#if FUNC_PM2_5
    case 2: // 温湿度 空气质量等级 CO2  TVOC 光照 大气压 PM2.5 PM10 甲醛/臭氧 二选一
        Paint_DrawBitMap_Paste(Image_Temp, 292, 173, 16, 23, flip_color); // 温度图标
        Paint_DrawBitMap_Paste(Image_Humi, 292, 239, 16, 23, flip_color); // 湿度图标
        /*温湿度数值显示*/
        if (device_t.epdShowTempWay == 1)
            EPD_Custom_Number24_Show_TempHumi(315, 195, 32000 + device_t.sensor_t.temperature * 1.8, 1000, DOT_PIXEL_2X2);
        else
            EPD_Custom_Number24_Show_TempHumi(315, 195, device_t.sensor_t.temperature, 1000, DOT_PIXEL_2X2);
        EPD_Custom_Number24_Show_TempHumi(315, 262, device_t.sensor_t.humidity, 1000, DOT_PIXEL_2X2);
        /*温湿度图标显示*/
        if (device_t.epdShowTempWay == 1)
            Paint_DrawBitMap_Paste(Image_FD, 375, 180, 16, 17, flip_color);
        else
            Paint_DrawBitMap_Paste(Image_CE, 375, 180, 16, 17, flip_color);
        Paint_DrawBitMap_Paste(Image_PCT, 375, 250, 16, 17, flip_color);

        Paint_DrawLine(282, 102, 400, 102, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(282, 168, 400, 168, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(0, 234, 400, 234, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(94, 234, 94, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(188, 234, 188, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        Paint_DrawBitMap_Paste(Image_PM2_5, 292, 41, 40, 10, flip_color); // PM2.5
        Paint_DrawBitMap_Paste(Image_PM10, 356, 41, 40, 10, flip_color); // PM10
        /*PM2.5 & PM10数值显示*/
        atmos_pm_2_5 = device_t.sensor_t.atmos_pm_2_5;
        atmos_pm_10_0 = device_t.sensor_t.atmos_pm_10_0;
        if (atmos_pm_2_5 > 999)
            atmos_pm_2_5 = 999;
        if (atmos_pm_10_0 > 999)
            atmos_pm_10_0 = 999;

        // atmos_pm_2_5 = 299;
        // atmos_pm_10_0 = 299;
        EPD_Custom_Number24_Show_PM(285, 61, atmos_pm_2_5);
        EPD_Custom_Number24_Show_PM(345, 61, atmos_pm_10_0);
        Paint_DrawBitMap_Paste(Image_UG_M3, 357, 85, 40, 10, flip_color); // 图标

#if FUNC_HCHO
        /*甲醛数值显示*/
        EPD_Custom_Number24_Show_HCHO(315, 127, device_t.sensor_t.hcho_ppm);
        Paint_DrawBitMap_Paste(Image_HCHO, 292, 107, 48, 10, flip_color); // 甲醛
        Paint_DrawBitMap_Paste(Image_MG_M3, 357, 150, 40, 10, flip_color); // 图标
#endif
#if FUNC_O3
        // Paint_DrawBitMap_Paste(Image_O3, 292, 107, 24, 14, flip_color); //臭氧
        // /*O3等级显示*/
        // switch (device_t.sensor_t.o3_level)
        // {
        // case 0:
        //     Paint_DrawBitMap_Paste(Image_Level_0, 315, 127, 32, 13, flip_color);
        //     break;
        // case 1:
        //     Paint_DrawBitMap_Paste(Image_Level_1, 315, 127, 32, 13, flip_color);
        //     break;
        // case 2:
        //     Paint_DrawBitMap_Paste(Image_Level_2, 315, 127, 32, 13, flip_color);
        //     break;
        // case 3:
        //     Paint_DrawBitMap_Paste(Image_Level_3, 315, 127, 32, 13, flip_color);
        //     break;
        // case 4:
        //     Paint_DrawBitMap_Paste(Image_Level_4, 315, 127, 32, 13, flip_color);
        //     break;
        // case 5:
        //     Paint_DrawBitMap_Paste(Image_Level_5, 315, 127, 32, 13, flip_color);
        //     break;
        // default:
        //     break;
        // }
        EPD_Custom_Number24_Show_Other(315, 127, device_t.sensor_t.o3_aqi);
        Paint_DrawBitMap_Paste(Image_O3, 292, 107, 24, 14, flip_color); // 臭氧
        Paint_DrawBitMap_Paste(Image_PPB, 370, 152, 24, 10, flip_color); // 图标
#endif

        co2_y_start = 204;
        co2_y_end = 124;
        // Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 30, 10, flip_color); // CO2图标
        Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 18, flip_color); // CO2图标
        /*告警图标显示*/
        if (device_t.sensor_t.co2 >= 1000 && device_t.sensor_t.co2 < 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        } else if (device_t.sensor_t.co2 >= 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm3, 70, 55, 24, 23, flip_color);
        }

        /*CO2数值显示*/ // 36条历史记录
        EPD_Custom_Number24_Show_Other(120, 80, device_t.sensor_t.co2);
        // Paint_DrawBitMap_Paste(Image_PPM, 210, 80, 40, 12, flip_color);
        Paint_DrawBitMap_Paste(Image_PPM, 210, 90, 24, 12, flip_color);
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_start, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_end, co2_y_start, co2_x_end, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_end, co2_y_start, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 水平分割线

        Paint_DrawNum(co2_x_start - (11 * 5 - 5), co2_y_end, 1600, &Font_12_18, foreground_color, background_color);
        Paint_DrawNum(co2_x_start - (11 * 4 - 5), co2_y_start - 16, 400, &Font_12_18, foreground_color, background_color);
        for (x = 0; x < 36; x++) {
            if (device_t.co2_history_buf[x] < 400) {
                co2_high = 1;
            } else if (device_t.co2_history_buf[x] < 1600) {
                co2_high = (device_t.co2_history_buf[x] - 400) * 2 / 3 / 10;
            } else // MAX
            {
                co2_high = 80;
            }
            co2_y = co2_y_start - co2_high;
            if (device_t.co2_history_buf[x] != 0)
                Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); // 垂直分割线
#if 0 // 测试使用
            if (device_t.co2_history_buf[x] == 0)
                co2_high = 80;
            co2_y = co2_y_start - co2_high;
            Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); //垂直分割线
#endif
            co2_x += 5;
        }

        Paint_DrawBitMap_Paste(Image_TVOC, 25, 250, 40, 10, flip_color); // TVOC
        /*TVOC等级显示*/
        switch (device_t.sensor_t.tvoc_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 29, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 29, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 29, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 29, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 29, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 29, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Light, 130, 245, 24, 22, flip_color); // 光照
        /*环境光等级显示*/
        switch (device_t.sensor_t.light_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 127, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 127, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 127, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 127, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 127, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 127, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Press, 190, 245, 24, 18, flip_color); // 大气压
        Paint_DrawBitMap_Paste(Image_MBAR, 252, 286, 24, 10, flip_color); // 大气压图标
        /*大气压数值显示*/
        EPD_Custom_Number24_Show_Other(210, 262, device_t.sensor_t.pressure);
#endif
    default:
        break;
    }

    DEBUG_TRACE(LOG_TAG, "EPD Will Update... %d", device_t.epdShowDuCnt);
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    if (device_t.epdShowDuCnt >= 1 && device_t.epdShowDuCnt <= 6) {
        EPD_Display_Du(ImageBuff);
    } else {
        device_t.epdShowDuCnt = 0;
        EPD_Display(ImageBuff);
    }
    device_t.epdShowDuCnt++;
}

void EPD_ShowClean()
{
    EPD_Init();
    EPD_Clear();
    ENTER_DEEP_SLEEP();
    device_t.epdShowDuCnt = 0;
}

void dis_img_fast(unsigned char num)
{
    unsigned int row, col;
    unsigned int pcnt;

    /***************************************************bw image****************************************************/
    SPI4W_WRITECOM(0x4E); // Set RAM X address counter = 0
    SPI4W_WRITEDATA(0x00);
    SPI4W_WRITECOM(0x4F); // Set Driver Output Control for Gate setting = Gate Channel - 1
    SPI4W_WRITEDATA(0x2B); // For example, for 300 gate channel => 300 - 1 = 299 (0x12B)
    SPI4W_WRITEDATA(0x01);
    SPI4W_WRITECOM(0x24); // Write B/W image data into to Register 0x24 RAM

    pcnt = 0; // 复位或保存提示字节序号
    for (col = 0; col < 300; col++) // 总共172列		// send 128x252bits ram 2D13
    {
        for (row = 0; row < 50; row++) // 总共72行，每个像素2bit,即 72/4 字节
        {
            switch (num) {

                // case PIC_A:
                // 	SPI4W_WRITEDATA(Image[pcnt]);
                // 	break;

            case PIC_WHITE:
                SPI4W_WRITEDATA(0xff);
                break;

            case PIC_BLACK:
                SPI4W_WRITEDATA(0x00);
                break;
            default:
                break;
            }
            pcnt++;
        }
    }

    SPI4W_WRITECOM(0x18);
    SPI4W_WRITEDATA(0x80);

    SPI4W_WRITECOM(0x22);
    SPI4W_WRITEDATA(0xFF); // Load LUT from MCU(0x32), Display update
    SPI4W_WRITECOM(0x20);
    READBUSY();
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xx   图片显示函数    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void dis_img(unsigned char num)
{
    unsigned int row, col;
    unsigned int pcnt;
    /***************************************************bw image****************************************************/
    SPI4W_WRITECOM(0x4E); // Set RAM X address counter = 0
    SPI4W_WRITEDATA(0x00);
    SPI4W_WRITECOM(0x4F); // Set Driver Output Control for Gate setting = Gate Channel - 1
    SPI4W_WRITEDATA(0x2B); // For example, for 300 gate channel => 300 - 1 = 299 (0x12B)
    SPI4W_WRITEDATA(0x01);
    SPI4W_WRITECOM(0x24); // Write B/W image data into to Register 0x24 RAM

    pcnt = 0; // 复位或保存提示字节序号
    for (col = 0; col < 300; col++) // 总共172列		// send 128x252bits ram 2D13
    {
        for (row = 0; row < 50; row++) // 总共72行，每个像素2bit,即 72/4 字节
        {
            switch (num) {

            case PIC_WHITE:
                SPI4W_WRITEDATA(0xff);
                break;

            case PIC_BLACK:
                SPI4W_WRITEDATA(0x00);
                break;
            default:
                break;
            }
            pcnt++;
        }
    }

    SPI4W_WRITECOM(0x4E); // Set RAM X address counter = 0
    SPI4W_WRITEDATA(0x00);
    SPI4W_WRITECOM(0x4F); // Set Driver Output Control for Gate setting = Gate Channel - 1
    SPI4W_WRITEDATA(0x2B); // For example, for 300 gate channel => 300 - 1 = 299 (0x12B)
    SPI4W_WRITEDATA(0x01);
    SPI4W_WRITECOM(0x26); // Write B/W image data into to Register 0x24 RAM

    pcnt = 0; // 复位或保存提示字节序号
    for (col = 0; col < 300; col++) // 总共172列		// send 128x252bits ram 2D13
    {
        for (row = 0; row < 50; row++) // 总共72行，每个像素2bit,即 72/4 字节
        {
            switch (num) {

            case PIC_WHITE:
                SPI4W_WRITEDATA(0xff);
                break;

            case PIC_BLACK:
                SPI4W_WRITEDATA(0x00);
                break;
            default:
                break;
            }
            pcnt++;
        }
    }

    SPI4W_WRITECOM(0x18);
    SPI4W_WRITEDATA(0x80);

    SPI4W_WRITECOM(0x22);
    SPI4W_WRITEDATA(0xF7); // Load LUT from MCU(0x32), Display update
    SPI4W_WRITECOM(0x20);
    READBUSY();
}

typedef struct {
    u8 len;
    u8 width;
    u8 height;
    const u8 *data_buf;
} number_conf_index;

// zmod4xxx_conf.

// 16 *24
number_conf_index number24_index[10] = {
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_0[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_1[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_2[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_3[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_4[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_5[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_6[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_7[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_8[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_9[0]},
};

void EPD_Custom_Number24_Show_TempHumi(u16 x, u16 y, u32 data, u16 enlargement, u8 dot_size)
{
    u16 x_start = x, y_start = y;
    u8 number_h, number_l;
    u8 flip_color = 1;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        foreground_color = WHITE;
        flip_color = 0;
    }

    number_h = data / enlargement;
    number_l = data / (enlargement / 10) % 10;

    u8 index = number_h / 10;

    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;
    index = number_h % 10;
    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;

    if (dot_size > 0) {
        x_start += dot_size;
        Paint_DrawPoint(x_start, y_start + number24_index[index].height - 3, foreground_color, (DOT_PIXEL)dot_size, DOT_STYLE_DFT);
        x_start += dot_size;
        index = number_l;
        Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height,
                               flip_color);
    }
}

void EPD_Custom_Number24_Show_HCHO(u16 x, u16 y, u16 data)
{
    u16 x_start = x, y_start = y;

    u8 hcho_ppm_l = (data + 5) / 10 % 100; // 四舍五入
    u8 index = 0;
    u8 flip_color = 1;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        foreground_color = WHITE;
        flip_color = 0;
    }

    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;

    x_start += DOT_PIXEL_2X2;
    Paint_DrawPoint(x_start, y_start + number24_index[index].height - 3, foreground_color, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    x_start += DOT_PIXEL_2X2;

    index = hcho_ppm_l / 10;
    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;

    index = hcho_ppm_l % 10;
    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;
}

void EPD_Custom_Number24_Show_Other(u16 x, u16 y, u16 data)
{
    u16 x_start = x, y_start = y;
    u8 i = 0, index = 0;

    u8 number[5] = {0};
    u8 number_len = 1;
    u8 flip_color = 1;

    if (device_t.epdOverturnShow) {
        flip_color = 0;
    }
    if (data > 0) {
        if (data <= 9) {
            number[0] = data % 10;
            number_len = 1;
        }
        if (data <= 99) {
            number[0] = data / 10;
            number[1] = data % 10;
            number_len = 2;
        } else if (data <= 999) {
            number[0] = data / 100;
            number[1] = data / 10 % 10;
            number[2] = data % 10;
            number_len = 3;
        } else if (data <= 9999) {
            number[0] = data / 1000;
            number[1] = data / 100 % 10;
            number[2] = data / 10 % 10;
            number[3] = data % 10;
            number_len = 4;
        } else if (data > 9999) {
            number[0] = 9;
            number[1] = 9;
            number[2] = 9;
            number[3] = 9;
            number_len = 4;
        }
    }

    while (number_len--) {
        index = number[i++];
        Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height,
                               flip_color);
        x_start += number24_index[index].width;
    }
}

void EPD_Custom_Number24_Show_PM(u16 x, u16 y, u16 data)
{
    u16 x_start = x, y_start = y;
    u8 i = 0, index = 0;

    u8 number[3] = {0};
    u8 number_len = 1;
    u8 flip_color = 1;

    if (device_t.epdOverturnShow) {
        flip_color = 0;
    }
    if (data >= 0) {
        if (data <= 9) {
            number[0] = data % 10;
            number_len = 1;
            x_start += 20;
        } else if (data <= 99) {
            number[0] = data / 10;
            number[1] = data % 10;
            number_len = 2;

            x_start += 10;
        } else if (data <= 999) {
            number[0] = data / 100;
            number[1] = data / 10 % 10;
            number[2] = data % 10;
            number_len = 3;
        }
    }

    while (number_len--) {
        index = number[i++];
        Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height,
                               flip_color);
        x_start += number24_index[index].width;
    }
}

/**
 * @brief  屏幕显示管理任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_display(ULONG thread_input)
{
    extern TX_EVENT_FLAGS_GROUP event_group;
    ULONG actual_events;
    UINT status;

    u8 epd_power_enable = 1;

    EPD_Init();
    EPD_ShowLoading();
    status = tx_event_flags_get(&event_group, /* 事件标志控制块 */
                                BIT_DISPLAY, /* 等待标志 */
                                TX_OR_CLEAR, /* 等待任意bit满足即可 */
                                &actual_events, /* 获取实际值 */
                                TX_WAIT_FOREVER); /* 永久等待 */
    // while (1) {
    //     EPD_ShowUpdate();
    //     if (device_t.epdShowMode)
    //         device_t.epdShowMode = 0;
    //     else
    //         device_t.epdShowMode = 1;
    //     tx_thread_sleep(3000);
    // }

    while (1) {
        /* 室内温度限制 0-50摄氏度 */
        if ((device_t.sensor_t.temperature >= 50000 || device_t.sensor_t.temperature <= 0) && epd_power_enable) {
            epd_power_enable = 0;
            EPD_ShowClean();
            EPD_PWR_CTRL_H;
            device_t.epdShowDuCnt = 0;
        } else {
            if (!epd_power_enable) {
                EPD_Init();
            }
            epd_power_enable = 1;
            if (device_t.epdShowMode == 0xFF) {
                EPD_ShowDevEUI();
            } else
                EPD_ShowUpdate();
        }

        // if (device_t.epdShowMode == 0xFF) {
        //     EPD_ShowDevEUI();
        // } else
        //     EPD_ShowUpdate();

        tx_thread_sleep(1000); // 刷新完后最少等待3秒再进行刷新
        status = tx_event_flags_get(&event_group, /* 事件标志控制块 */
                                    BIT_DISPLAY, /* 等待标志 */
                                    TX_OR_CLEAR, /* 等待任意bit满足即可 */
                                    &actual_events, /* 获取实际值 */
                                    TX_WAIT_FOREVER); /* 永久等待 */
        if (device_t.epdInit) {
            device_t.epdInit = 0;
            device_t.epdShowDuCnt = 0;
            EPD_Init();
            if (device_t.epdShowDuCnt >= 1 && device_t.epdShowDuCnt <= 4) {
                device_t.epdOldImageShow = 1;
            }
        }
    }
}

#else

u8 ImageBuff[Source_Pixel / 8 * Gate_Pixel];

u8 epd_status;

/**
 * @brief  电子纸GPIO初始化
 * @param
 * @retval
 */
void EPD_PortCfg(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能

    /** 初始化 IO **/
    OutputIO(EPD_RST_PORT, EPD_RST_PIN, OUT_PUSHPULL);
    OutputIO(EPD_CSB_PORT, EPD_CSB_PIN, OUT_PUSHPULL);
    OutputIO(EPD_SDA_PORT, EPD_SDA_PIN, OUT_PUSHPULL);
    OutputIO(EPD_SCL_PORT, EPD_SCL_PIN, OUT_PUSHPULL);
    OutputIO(EPD_DC_PORT, EPD_DC_PIN, OUT_PUSHPULL);
    InputtIO(EPD_BUSY_PORT, EPD_BUSY_PIN, IN_NORMAL);
    EPD_PWR_CTRL_L;
}

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xx   电子纸驱动操作函数    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

// 读忙 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void READBUSY(void) // 检查硬件BUSY线是否忙？注意FPC接口要连接良好，否则会一直在这里死循环。
{
    unsigned char busy_cnt = 0;
    while (1) {
        if (READ_EPD_BUSY_STATUS)
            break; // 硬件BUSY线高电平=不忙时跳出循环。注意高电平时不一定是1，可能是其它值，看具体I/O口的位。
        else {
            if (++busy_cnt >= 20) {
                busy_cnt = 0;
                device_t.epdShowDuCnt = 0; // 出现超时下一次固定全刷
                DEBUG_TRACE(WARN_TAG, "%s Timeout!!!", __func__);
                break;
            }
        }
        tx_thread_sleep(100);
    }
    // tx_thread_sleep(500); // 显示完成后等待1S
}

bool READBUSY_CALLBACK(void)
{
    unsigned char busy_cnt = 0;
    while (1) {
        if (READ_EPD_BUSY_STATUS)
            break; // 硬件BUSY线高电平=不忙时跳出循环。注意高电平时不一定是1，可能是其它值，看具体I/O口的位。
        else {
            if (++busy_cnt >= 20) {
                busy_cnt = 0;
                device_t.epdShowDuCnt = 0; // 出现超时下一次固定全刷
                DEBUG_TRACE(WARN_TAG, "%s Timeout!!!", __func__);
                return false;
            }
        }
        tx_thread_sleep(100);
    }
    return true;
}

// 写命令 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void SPI4W_WRITECOM(unsigned char INIT_COM)
{
    unsigned char TEMPCOM;
    unsigned char scnt;

    TEMPCOM = INIT_COM;
    EPD_SCL_L;
    EPD_DC_L;
    EPD_CSB_H;
    EPD_CSB_L;
    asm("nop");
    for (scnt = 0; scnt < 8; scnt++) {
        if (TEMPCOM & 0x80)
            EPD_SDA_H;
        else
            EPD_SDA_L;
        EPD_SCL_H;
        asm("nop");
        asm("nop");
        EPD_SCL_L;
        TEMPCOM = TEMPCOM << 1;
    }
    asm("nop");
    EPD_CSB_H;
    EPD_SDA_L;
    asm("nop");
}

// 写数据 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void SPI4W_WRITEDATA(unsigned char INIT_DATA)
{
    unsigned char TEMPCOM;
    unsigned char scnt;

    TEMPCOM = INIT_DATA;
    EPD_SCL_L;
    EPD_DC_H;
    EPD_CSB_H;
    EPD_CSB_L;
    asm("nop");
    for (scnt = 0; scnt < 8; scnt++) {
        if (TEMPCOM & 0x80)
            EPD_SDA_H;
        else
            EPD_SDA_L;
        EPD_SCL_H;
        asm("nop");
        asm("nop");
        EPD_SCL_L;
        TEMPCOM = TEMPCOM << 1;
    }
    asm("nop");
    EPD_CSB_H;
    EPD_SDA_L;
    EPD_DC_L;
    asm("nop");
}

// // 写命令 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// void SPI4W_WRITECOM(unsigned char INIT_COM)
// {
//     unsigned char TEMPCOM;
//     unsigned char scnt;

//     TEMPCOM = INIT_COM;

//     SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_LOW);
//     EPD_DC_L;
//     TicksDelayUs(1);
//     SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_HIGH);
//     Spi4Write(&TEMPCOM, 1);
// }

// // 写数据 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// void SPI4W_WRITEDATA(unsigned char INIT_DATA)
// {
//     unsigned char TEMPCOM;
//     unsigned char scnt;

//     TEMPCOM = INIT_DATA;

//     SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_LOW);
//     EPD_DC_H;
//     TicksDelayUs(1);
//     SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_HIGH);
//     Spi4Write(&TEMPCOM, 1);
//     EPD_DC_L;
// }

// 复位 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void EPD_RESET()
{
    EPD_DC_L;
    EPD_RST_L;
    tx_thread_sleep(5);
    EPD_RST_H;
    tx_thread_sleep(10);
    READBUSY();
}

// 入深度睡眠 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void ENTER_DEEP_SLEEP() // 注意：进入深度睡眠模式后，不接受命令，只能用硬件RESET才能重新操作，软件RESET也不行。然后需要重新初始化配置。
{
    SPI4W_WRITECOM(0x07);
    SPI4W_WRITEDATA(0xA5);
}

// 写波形数据表 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void lut_5S()
{
    unsigned int count;
    SPI4W_WRITECOM(0x20);
    for (count = 0; count < 56; count++) {
        SPI4W_WRITEDATA(lut_R20_5S[count]);
    }
    SPI4W_WRITECOM(0x21);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R21_5S[count]);
    }
    SPI4W_WRITECOM(0x22);
    for (count = 0; count < 56; count++) {
        SPI4W_WRITEDATA(lut_R22_5S[count]);
    }
    SPI4W_WRITECOM(0x23);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R23_5S[count]);
    }
    SPI4W_WRITECOM(0x24);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R24_5S[count]);
    }
}

// 全刷
void lut_GC()
{
    unsigned int count;
    SPI4W_WRITECOM(0x20);
    for (count = 0; count < 56; count++) {
        SPI4W_WRITEDATA(lut_R20_GC[count]);
    }
    SPI4W_WRITECOM(0x21);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R21_GC[count]);
    }
    SPI4W_WRITECOM(0x22);
    for (count = 0; count < 56; count++) {
        SPI4W_WRITEDATA(lut_R22_GC[count]);
    }
    SPI4W_WRITECOM(0x23);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R23_GC[count]);
    }
    SPI4W_WRITECOM(0x24);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R24_GC[count]);
    }
}

// 局刷
void lut_DU()
{
    unsigned int count;
    SPI4W_WRITECOM(0x20);
    for (count = 0; count < 56; count++) {
        SPI4W_WRITEDATA(lut_R20_DU[count]);
    }
    SPI4W_WRITECOM(0x21);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R21_DU[count]);
    }
    SPI4W_WRITECOM(0x22);
    for (count = 0; count < 56; count++) {
        SPI4W_WRITEDATA(lut_R22_DU[count]);
    }
    SPI4W_WRITECOM(0x23);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R23_DU[count]);
    }
    SPI4W_WRITECOM(0x24);
    for (count = 0; count < 42; count++) {
        SPI4W_WRITEDATA(lut_R24_DU[count]);
    }
}

void EPD_Rfresh()
{
    SPI4W_WRITECOM(0x17); // 自动更新指令
    SPI4W_WRITEDATA(0xA5); // PON DRF POF 屏自动更新
    READBUSY(); // 判断屏是否完成更新图像？
}

// 电子纸驱动初始化 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
void EPD_Init(void)
{
    EPD_PortCfg();
    EPD_RESET(); // reset
    SPI4W_WRITECOM(0x00); // PANEL SETTING
    SPI4W_WRITEDATA(
        0x37); // [5]REG=1(LUT from register); [4]BWR=1 黑白运行；[3]UD=0 Gn-1->G0；[2]SHL=1 S0->Sn-1；[1]SHD_N=1 DC-DC打开；RST_N=1 no reset
    SPI4W_WRITEDATA(0x01); //

    SPI4W_WRITECOM(0x01); // Power setting
    SPI4W_WRITEDATA(0x03); // [2]=0:VDHR disable  [1]VDS_EN: Source power selection =1: Internal DC/DC function for generating VDH/VDL     [0]VDG_EN:
                           // Gate power selection =1: Internal DC/DC function for generating VGH/VGL  设置使用内部升压
    SPI4W_WRITEDATA(0x10); // VGH=20V, VGL= -20V
    SPI4W_WRITEDATA(0x3f); // +15V
    SPI4W_WRITEDATA(0x3f); // -15V
    SPI4W_WRITEDATA(0x03); //   3V

    SPI4W_WRITECOM(0x06); // Booster Soft Start设定
    SPI4W_WRITEDATA(0xE7); //
    SPI4W_WRITEDATA(0xE7); //
    SPI4W_WRITEDATA(0x3D);

    SPI4W_WRITECOM(0X60); // TCON SETTING  (TCON)
    SPI4W_WRITEDATA(0x22); // 0010=12 (Default)

    SPI4W_WRITECOM(0x82); // vcom设定
    SPI4W_WRITEDATA(0x0E); // -1.5V

    SPI4W_WRITECOM(0x30); // 帧频设定
    SPI4W_WRITEDATA(0x09); //   50HZ
    // SPI4W_WRITEDATA(0x0F); // 80HZ
    // SPI4W_WRITEDATA(0x0B); // 3A 100HZ   29 150Hz 39 200HZ	31 171HZ

    SPI4W_WRITECOM(0X50); // VCOM AND DATA INTERVAL SETTING
                          //    SPI4W_WRITEDATA(0x09);     // border=KK D7=BDZ=1(Border output Hi-Z enabled) =0(Border output Hi-Z disabled)
                          //    D5:4=BDV[1:0]   D3=N2OCP=1(Copy NEW data to OLD data enabled)  D1:0=01(Default)  注意：这个值关系到波形表
                          //    SPI4W_WRITEDATA(0x29);     // border=KW D7=BDZ=1(Border output Hi-Z enabled) =0(Border output Hi-Z disabled)
                          //    D5:4=BDV[1:0]   D3=N2OCP=1(Copy NEW data to OLD data enabled)  D1:0=01(Default)  注意：这个值关系到波形表
                          //    SPI4W_WRITEDATA(0x39);     // border=LUTBD D7=BDZ=1(Border output Hi-Z enabled) =0(Border output Hi-Z disabled)
                          //    D5:4=BDV[1:0]   D3=N2OCP=1(Copy NEW data to OLD data enabled)  D1:0=01(Default)  注意：这个值关系到波形表
    SPI4W_WRITEDATA(0x17); // CDI[3:0]:VCOM and data interval  =0111：10 (Default)

    SPI4W_WRITECOM(0xE3); //
    SPI4W_WRITEDATA(0x88); //

    SPI4W_WRITECOM(0x61); // Resolution setting
    SPI4W_WRITEDATA(Source_Pixel_high); // Horizontal Display Resolution=800  High 8bit
    SPI4W_WRITEDATA(Source_Pixel_low); // Horizontal Display Resolution=800  Low  8bit  D0~D2为0
    SPI4W_WRITEDATA(Gate_Pixel_high); //   Vertical Display Resolution=480  High 8bit
    SPI4W_WRITEDATA(Gate_Pixel_low); //   Vertical Display Resolution=480  Low  8bit

    lut_GC(); //  3段式居中 有效驱动时间15Frames

    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;

    SPI4W_WRITECOM(DATA_START_TRANSMISSION_1);
    for (Ypoint = 0; Ypoint < Height; Ypoint++) {
        for (Xpoint = 0; Xpoint < Width; Xpoint++) { // 8 pixel =  1 byte
            if (device_t.epdOverturnShow) {
                SPI4W_WRITEDATA(BLACK);
            } else {
                SPI4W_WRITEDATA(WHITE);
            }
        }
    }
}

// 清除显示
void EPD_Clear()
{
    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;

    device_t.epdOldImageShow = 0;
    SPI4W_WRITECOM(DATA_START_TRANSMISSION_2);
    for (Ypoint = 0; Ypoint < Height; Ypoint++) {
        for (Xpoint = 0; Xpoint < Width; Xpoint++) {
            if (device_t.epdOverturnShow) {
                SPI4W_WRITEDATA(WHITE);
            } else {
                SPI4W_WRITEDATA(WHITE);
            }
        }
    }
    EPD_Rfresh();
}

// 填充显示
void EPD_Display(const unsigned char *image_buffer)
{
    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;
    u32 Addr = 0;

    device_t.epdOldImageShow = 0;
    if (READBUSY_CALLBACK() == true) {
        SPI4W_WRITECOM(DATA_START_TRANSMISSION_2);
        for (Ypoint = 0; Ypoint < Height; Ypoint++) {
            for (Xpoint = 0; Xpoint < Width; Xpoint++) { // 8 pixel =  1 byte
                Addr = Xpoint + Ypoint * Width;
                SPI4W_WRITEDATA(image_buffer[Addr]);
            }
        }
        EPD_Rfresh();
    } else {
        device_t.epdInit = 1;
        device_t.epdShowDuCnt = 0;
        tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
    }
}

// 填充显示
void EPD_Display_Du(const unsigned char *image_buffer)
{
    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;
    u32 Addr = 0;

    if (READBUSY_CALLBACK() == true) {
        SPI4W_WRITECOM(DATA_START_TRANSMISSION_2);
        for (Ypoint = 0; Ypoint < Height; Ypoint++) {
            for (Xpoint = 0; Xpoint < Width; Xpoint++) { // 8 pixel =  1 byte
                Addr = Xpoint + Ypoint * Width;
                SPI4W_WRITEDATA(image_buffer[Addr]);
            }
        }
        EPD_Rfresh();
    } else {
        device_t.epdInit = 1;
        device_t.epdShowDuCnt = 0;
        tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
    }
}

// 显示设备DEVEUI
void EPD_ShowDevEUI(void)
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;
    memset(ImageBuff, 0, sizeof(ImageBuff));
    char qr_msg[] = "HKT AIR SENSOR \r\nDevEUI:123456781234578";

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
    }

    device_t.epdShowDuCnt = 0;

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    Paint_SelectImage(IMAGE_BW);
    char epd_deveui[] = "DevEUI:1234567812345789";
    snprintf(epd_deveui, sizeof(epd_deveui), "DevEUI:%02X%02X%02X%02X%02X%02X%02X%02X", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2],
             LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);

    snprintf(qr_msg, sizeof(qr_msg), "HKT AIR SENSOR\r\n%s", epd_deveui);
    EncodeData(qr_msg);
    int y_start = 50, x_start = 138;
    for (int y = 0; y < 33; y++) {
        for (int x = 0; x < 33; x++) {
            if (m_byModuleData[y][x])
                Paint_DrawPoint(x_start + x * 5, y_start + y * 5, foreground_color, DOT_PIXEL_5X5, DOT_FILL_AROUND);
            else
                Paint_DrawPoint(x_start + x * 5, y_start + y * 5, background_color, DOT_PIXEL_5X5, DOT_FILL_AROUND);
        }
    }

    Paint_DrawString_EN(2, 200, epd_deveui, &Font24, foreground_color, background_color);
    EPD_Display(ImageBuff);
}

void EPD_ShowFirmUpdate(void)
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;
    memset(ImageBuff, 0, sizeof(ImageBuff));

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
    }

    device_t.epdShowDuCnt = 0;

    lut_GC();

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    Paint_SelectImage(IMAGE_BW);
    Paint_DrawString_EN(30, 150, "Firmware Updating...", &Font24, foreground_color, background_color);
    EPD_Display(ImageBuff);
}

void EPD_ShowLoading(void)
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;
    memset(ImageBuff, 0, sizeof(ImageBuff));

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
    }

    device_t.epdShowDuCnt = 0;

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    Paint_SelectImage(IMAGE_BW);
    Paint_DrawString_EN(120, 150, "Load ......", &Font24, foreground_color, background_color);
    EPD_Display(ImageBuff);
}

// 更新电子纸显示
void EPD_ShowUpdate()
{
    UBYTE *IMAGE_BW;
    IMAGE_BW = ImageBuff;

    u16 Xpoint, Ypoint, Height, Width;
    Height = Gate_Pixel;
    Width = Source_Pixel / 8;
    u32 Addr = 0;

    if (device_t.epdOldImageShow) {
        if (READBUSY_CALLBACK() == true) {
            SPI4W_WRITECOM(DATA_START_TRANSMISSION_1);
            for (Ypoint = 0; Ypoint < Height; Ypoint++) {
                for (Xpoint = 0; Xpoint < Width; Xpoint++) { // 8 pixel =  1 byte
                    Addr = Xpoint + Ypoint * Width;
                    SPI4W_WRITEDATA(ImageBuff[Addr]);
                }
            }
            device_t.epdOldImageShow = 0;
        } else {
            device_t.epdInit = 1;
            device_t.epdShowDuCnt = 0;
            tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
        }
    }

    memset(ImageBuff, 0, sizeof(ImageBuff));

    u8 background_color = WHITE;
    u8 foreground_color = BLACK;
    u8 flip_color = 1;

    if (device_t.epdOverturnShow) {
        background_color = BLACK;
        foreground_color = WHITE;
        flip_color = 0;
    }

    Paint_NewImage(IMAGE_BW, EPD_WIDTH, EPD_HEIGHT, ROTATE_180, background_color);
    Paint_Clear(background_color);
    // Select Image
    Paint_SelectImage(IMAGE_BW);

    Paint_DrawLine(0, 36, 400, 36, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
    Paint_DrawLine(282, 36, 282, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

#if (FUNC_SHOW_TIME)
    char time[] = "2022/05/26 17:06";
    snprintf(time, sizeof(time), "%04d/%02d/%02d %02d:%02d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min);
    Paint_DrawString_EN(15, 16, (const char *)time, &Font_12_18, foreground_color, background_color);
#elif (FUNC_SHOW_LOGO)
    char time[] = "HKT";
    Paint_DrawString_EN(20, 14, (const char *)time, &Font20, foreground_color, background_color);
#endif

    // Paint_DrawString_EN(210, 0, "HKT", &Font24, background_color, foreground_color);
    if (device_t.powerIn) {
        if (batteryLevel < 10)
            Paint_DrawBitMap_Paste(Image_Battery_0, 314, 15, 24, 15, flip_color); // 电量信息
        else if (batteryLevel < 30)
            Paint_DrawBitMap_Paste(Image_Battery_20, 314, 15, 24, 15, flip_color); // 电量信息
        else if (batteryLevel < 60)
            Paint_DrawBitMap_Paste(Image_Battery_50, 314, 15, 24, 15, flip_color); // 电量信息
        else if (batteryLevel < 90)
            Paint_DrawBitMap_Paste(Image_Battery_80, 314, 15, 24, 15, flip_color); // 电量信息
        else
            Paint_DrawBitMap_Paste(Image_Battery_100, 314, 15, 24, 15, flip_color); // 电量信息
    }

    if (LoRaWAN.joinState == 1)
        Paint_DrawBitMap_Paste(Image_Signal, 356, 15, 24, 16, flip_color); // 入网状态
    else
        Paint_DrawBitMap_Paste(Image_Unsignal, 356, 15, 32, 16, flip_color);

    // CO2 边框坐标
    u16 co2_x_start = 60;
    u16 co2_x_end = co2_x_start + (36 + 1) * 5;
    u16 co2_y_start = 264;
    u16 co2_y_end = 144;

    // CO2 历史记录显示坐标
    u16 co2_x = co2_x_start + 5;
    u16 co2_y = co2_x_start + 5;
    u16 co2_high = 0;
    int x = 0;

    u16 atmos_pm_2_5;
    u16 atmos_pm_10_0;

    // 电子纸显示模式
    switch (device_t.epdShowMode) {
    case 0: // 温湿度 空气质量等级 CO2
        Paint_DrawBitMap_Paste(Image_Temp, 292, 173, 16, 23, flip_color); // 温度图标
        Paint_DrawBitMap_Paste(Image_Humi, 292, 240, 16, 23, flip_color); // 湿度图标
        /*温湿度数值显示*/
        if (device_t.epdShowTempWay == 1)
            EPD_Custom_Number24_Show_TempHumi(315, 195, 32000 + device_t.sensor_t.temperature * 1.8, 1000, DOT_PIXEL_2X2);
        else
            EPD_Custom_Number24_Show_TempHumi(315, 195, device_t.sensor_t.temperature, 1000, DOT_PIXEL_2X2);
        EPD_Custom_Number24_Show_TempHumi(315, 262, device_t.sensor_t.humidity, 1000, DOT_PIXEL_2X2);
        /*温湿度图标显示*/
        if (device_t.epdShowTempWay == 1)
            Paint_DrawBitMap_Paste(Image_FD, 375, 180, 16, 17, flip_color);
        else
            Paint_DrawBitMap_Paste(Image_CE, 375, 180, 16, 17, flip_color);
        Paint_DrawBitMap_Paste(Image_PCT, 375, 250, 16, 17, flip_color);

        Paint_DrawLine(282, 168, 400, 168, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(282, 234, 400, 234, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        // 空气质量等级
        switch (device_t.sensor_t.air_level) {
        case 0: // 空气质量好
            Paint_DrawBitMap_Paste(Image_Good, 322, 76, 40, 37, flip_color); // 空气质量等级（笑脸）
            Paint_DrawBitMap_Paste(Image_excellent, 295, 122, 104, 18, flip_color);
            break;
        case 1: // 空气质量一般
            Paint_DrawBitMap_Paste(Image_Normal, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_drdinary, 300, 122, 96, 18, flip_color);
            break;
        case 2: // 空气质量差
        default:
            Paint_DrawBitMap_Paste(Image_Low, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_lousy, 313, 122, 60, 18, flip_color);
            break;
        }

        // Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 10, flip_color); // CO2图标
        Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 18, flip_color); // CO2图标
        /*告警图标显示*/
        if (device_t.sensor_t.co2 >= 1000 && device_t.sensor_t.co2 < 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        } else if (device_t.sensor_t.co2 >= 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm3, 70, 55, 24, 23, flip_color);
        }
#if 0 // 测试使用
        Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        Paint_DrawBitMap_Paste(Image_Alarm2, 94, 55, 24, 23, flip_color);
        Paint_DrawBitMap_Paste(Image_Alarm3, 118, 55, 24, 23, flip_color);
#endif

        /*CO2数值显示*/ // 36条历史记录
        EPD_Custom_Number24_Show_Other(120, 90, device_t.sensor_t.co2);
        // Paint_DrawBitMap_Paste(Image_PPM, 210, 90, 40, 12, flip_color);
        Paint_DrawBitMap_Paste(Image_PPM, 210, 100, 24, 12, flip_color);
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_start, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_end, co2_y_start, co2_x_end, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_end, co2_y_start, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 水平分割线

        Paint_DrawNum(co2_x_start - (11 * 5 - 5), co2_y_end, 1600, &Font_12_18, foreground_color, background_color);
        Paint_DrawNum(co2_x_start - (11 * 4 - 5), co2_y_start - 16, 400, &Font_12_18, foreground_color, background_color);

        for (x = 0; x < 36; x++) {
            if (device_t.co2_history_buf[x] < 400) {
                co2_high = 1;
            } else if (device_t.co2_history_buf[x] < 1600) {
                co2_high = (device_t.co2_history_buf[x] - 400) / 10;
            } else // MAX
            {
                co2_high = 120;
            }
            co2_y = co2_y_start - co2_high;
            if (device_t.co2_history_buf[x] != 0)
                Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); // 垂直分割线
#if 0 // 测试使用
            if (device_t.co2_history_buf[x] == 0)
                co2_high = 120;
            co2_y = co2_y_start - co2_high;
            Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); //垂直分割线
#endif
            co2_x += 5;
        }
        break;
    case 1: // 温湿度 空气质量等级 CO2  TVOC 光照 大气压
        Paint_DrawBitMap_Paste(Image_Temp, 292, 173, 16, 23, flip_color); // 温度图标
        Paint_DrawBitMap_Paste(Image_Humi, 292, 240, 16, 23, flip_color); // 湿度图标
        /*温湿度数值显示*/
        if (device_t.epdShowTempWay == 1)
            EPD_Custom_Number24_Show_TempHumi(315, 195, 32000 + device_t.sensor_t.temperature * 1.8, 1000, DOT_PIXEL_2X2);
        else
            EPD_Custom_Number24_Show_TempHumi(315, 195, device_t.sensor_t.temperature, 1000, DOT_PIXEL_2X2);
        EPD_Custom_Number24_Show_TempHumi(315, 262, device_t.sensor_t.humidity, 1000, DOT_PIXEL_2X2);
        /*温湿度图标显示*/
        if (device_t.epdShowTempWay == 1)
            Paint_DrawBitMap_Paste(Image_FD, 375, 180, 16, 17, flip_color);
        else
            Paint_DrawBitMap_Paste(Image_CE, 375, 180, 16, 17, flip_color);
        Paint_DrawBitMap_Paste(Image_PCT, 375, 250, 16, 17, flip_color);

        Paint_DrawLine(282, 168, 400, 168, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(0, 234, 400, 234, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(94, 234, 94, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(188, 234, 188, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        // 空气质量等级
        switch (device_t.sensor_t.air_level) {
        case 0: // 空气质量好
            Paint_DrawBitMap_Paste(Image_Good, 322, 76, 40, 37, flip_color); // 空气质量等级（笑脸）
            Paint_DrawBitMap_Paste(Image_excellent, 295, 122, 104, 18, flip_color);
            break;
        case 1: // 空气质量一般
            Paint_DrawBitMap_Paste(Image_Normal, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_drdinary, 300, 122, 96, 18, flip_color);
            break;
        case 2: // 空气质量差
        default:
            Paint_DrawBitMap_Paste(Image_Low, 322, 76, 40, 37, flip_color);
            Paint_DrawBitMap_Paste(Image_lousy, 313, 122, 60, 18, flip_color);
            break;
        }

        co2_y_start = 204;
        co2_y_end = 124;
        // Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 30, 10, flip_color); // CO2图标
        Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 18, flip_color); // CO2图标
        /*告警图标显示*/
        if (device_t.sensor_t.co2 >= 1000 && device_t.sensor_t.co2 < 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        } else if (device_t.sensor_t.co2 >= 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm3, 70, 55, 24, 23, flip_color);
        }

        /*CO2数值显示*/ // 36条历史记录
        EPD_Custom_Number24_Show_Other(120, 80, device_t.sensor_t.co2);
        // Paint_DrawBitMap_Paste(Image_PPM, 210, 80, 40, 12, flip_color);
        Paint_DrawBitMap_Paste(Image_PPM, 210, 90, 24, 12, flip_color);
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_start, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_end, co2_y_start, co2_x_end, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_end, co2_y_start, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 水平分割线

        Paint_DrawNum(co2_x_start - (11 * 5 - 5), co2_y_end, 1600, &Font_12_18, foreground_color, background_color);
        Paint_DrawNum(co2_x_start - (11 * 4 - 5), co2_y_start - 16, 400, &Font_12_18, foreground_color, background_color);
        for (x = 0; x < 36; x++) {
            if (device_t.co2_history_buf[x] < 400) {
                co2_high = 1;
            } else if (device_t.co2_history_buf[x] < 1600) {
                co2_high = (device_t.co2_history_buf[x] - 400) * 2 / 3 / 10;
                if (co2_high < 1)
                    co2_high = 1;
            } else // MAX
            {
                co2_high = 80;
            }
            co2_y = co2_y_start - co2_high;
            if (device_t.co2_history_buf[x] != 0)
                Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); // 垂直分割线
#if 0 // 测试使用
            if (device_t.co2_history_buf[x] == 0)
                co2_high = 80;
            co2_y = co2_y_start - co2_high;
            Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); //垂直分割线
#endif
            co2_x += 5;
        }

        Paint_DrawBitMap_Paste(Image_TVOC, 25, 250, 40, 10, flip_color); // TVOC
        /*TVOC等级显示*/
        switch (device_t.sensor_t.tvoc_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 29, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 29, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 29, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 29, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 29, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 29, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Light, 130, 245, 24, 22, flip_color); // 光照
        /*环境光等级显示*/
        switch (device_t.sensor_t.light_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 127, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 127, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 127, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 127, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 127, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 127, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Press, 190, 245, 24, 18, flip_color); // 大气压
        Paint_DrawBitMap_Paste(Image_MBAR, 252, 286, 24, 10, flip_color); // 大气压图标
        /*大气压数值显示*/
        EPD_Custom_Number24_Show_Other(210, 262, device_t.sensor_t.pressure);
        break;
#if FUNC_PM2_5
    case 2: // 温湿度 空气质量等级 CO2  TVOC 光照 大气压 PM2.5 PM10 甲醛/臭氧 二选一
        Paint_DrawBitMap_Paste(Image_Temp, 292, 173, 16, 23, flip_color); // 温度图标
        Paint_DrawBitMap_Paste(Image_Humi, 292, 239, 16, 23, flip_color); // 湿度图标
        /*温湿度数值显示*/
        if (device_t.epdShowTempWay == 1)
            EPD_Custom_Number24_Show_TempHumi(315, 195, 32000 + device_t.sensor_t.temperature * 1.8, 1000, DOT_PIXEL_2X2);
        else
            EPD_Custom_Number24_Show_TempHumi(315, 195, device_t.sensor_t.temperature, 1000, DOT_PIXEL_2X2);
        EPD_Custom_Number24_Show_TempHumi(315, 262, device_t.sensor_t.humidity, 1000, DOT_PIXEL_2X2);
        /*温湿度图标显示*/
        if (device_t.epdShowTempWay == 1)
            Paint_DrawBitMap_Paste(Image_FD, 375, 180, 16, 17, flip_color);
        else
            Paint_DrawBitMap_Paste(Image_CE, 375, 180, 16, 17, flip_color);
        Paint_DrawBitMap_Paste(Image_PCT, 375, 250, 16, 17, flip_color);

        Paint_DrawLine(282, 102, 400, 102, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(282, 168, 400, 168, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(0, 234, 400, 234, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(94, 234, 94, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(188, 234, 188, 300, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

        Paint_DrawBitMap_Paste(Image_PM2_5, 292, 41, 40, 10, flip_color); // PM2.5
        Paint_DrawBitMap_Paste(Image_PM10, 356, 41, 40, 10, flip_color); // PM10
        /*PM2.5 & PM10数值显示*/
        atmos_pm_2_5 = device_t.sensor_t.atmos_pm_2_5;
        atmos_pm_10_0 = device_t.sensor_t.atmos_pm_10_0;
        if (atmos_pm_2_5 > 999)
            atmos_pm_2_5 = 999;
        if (atmos_pm_10_0 > 999)
            atmos_pm_10_0 = 999;

        // atmos_pm_2_5 = 299;
        // atmos_pm_10_0 = 299;
        EPD_Custom_Number24_Show_PM(285, 61, atmos_pm_2_5);
        EPD_Custom_Number24_Show_PM(345, 61, atmos_pm_10_0);
        Paint_DrawBitMap_Paste(Image_UG_M3, 357, 85, 40, 10, flip_color); // 图标

#if FUNC_HCHO
        /*甲醛数值显示*/
        EPD_Custom_Number24_Show_HCHO(315, 127, device_t.sensor_t.hcho_ppm);
        Paint_DrawBitMap_Paste(Image_HCHO, 292, 107, 48, 10, flip_color); // 甲醛
        Paint_DrawBitMap_Paste(Image_MG_M3, 357, 150, 40, 10, flip_color); // 图标
#endif
#if FUNC_O3
        // Paint_DrawBitMap_Paste(Image_O3, 292, 107, 24, 14, flip_color); //臭氧
        // /*O3等级显示*/
        // switch (device_t.sensor_t.o3_level)
        // {
        // case 0:
        //     Paint_DrawBitMap_Paste(Image_Level_0, 315, 127, 32, 13, flip_color);
        //     break;
        // case 1:
        //     Paint_DrawBitMap_Paste(Image_Level_1, 315, 127, 32, 13, flip_color);
        //     break;
        // case 2:
        //     Paint_DrawBitMap_Paste(Image_Level_2, 315, 127, 32, 13, flip_color);
        //     break;
        // case 3:
        //     Paint_DrawBitMap_Paste(Image_Level_3, 315, 127, 32, 13, flip_color);
        //     break;
        // case 4:
        //     Paint_DrawBitMap_Paste(Image_Level_4, 315, 127, 32, 13, flip_color);
        //     break;
        // case 5:
        //     Paint_DrawBitMap_Paste(Image_Level_5, 315, 127, 32, 13, flip_color);
        //     break;
        // default:
        //     break;
        // }
        EPD_Custom_Number24_Show_Other(315, 127, device_t.sensor_t.o3_aqi);
        Paint_DrawBitMap_Paste(Image_O3, 292, 107, 24, 14, flip_color); // 臭氧
        Paint_DrawBitMap_Paste(Image_PPB, 370, 152, 24, 10, flip_color); // 图标
#endif

        co2_y_start = 204;
        co2_y_end = 124;
        // Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 30, 10, flip_color); // CO2图标
        Paint_DrawBitMap_Paste(Image_CO2, 20, 60, 32, 18, flip_color); // CO2图标
        /*告警图标显示*/
        if (device_t.sensor_t.co2 >= 1000 && device_t.sensor_t.co2 < 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm1, 70, 55, 24, 23, flip_color);
        } else if (device_t.sensor_t.co2 >= 1500) {
            Paint_DrawBitMap_Paste(Image_Alarm3, 70, 55, 24, 23, flip_color);
        }

        /*CO2数值显示*/ // 36条历史记录
        EPD_Custom_Number24_Show_Other(120, 80, device_t.sensor_t.co2);
        // Paint_DrawBitMap_Paste(Image_PPM, 210, 80, 40, 12, flip_color);
        Paint_DrawBitMap_Paste(Image_PPM, 210, 90, 24, 12, flip_color);
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_start, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_end, co2_y_start, co2_x_end, co2_y_end, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 垂直分割线
        Paint_DrawLine(co2_x_start, co2_y_start, co2_x_end, co2_y_start, foreground_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID); // 水平分割线

        Paint_DrawNum(co2_x_start - (11 * 5 - 5), co2_y_end, 1600, &Font_12_18, foreground_color, background_color);
        Paint_DrawNum(co2_x_start - (11 * 4 - 5), co2_y_start - 16, 400, &Font_12_18, foreground_color, background_color);
        for (x = 0; x < 36; x++) {
            if (device_t.co2_history_buf[x] < 400) {
                co2_high = 1;
            } else if (device_t.co2_history_buf[x] < 1600) {
                co2_high = (device_t.co2_history_buf[x] - 400) * 2 / 3 / 10;
            } else // MAX
            {
                co2_high = 80;
            }
            co2_y = co2_y_start - co2_high;
            if (device_t.co2_history_buf[x] != 0)
                Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); // 垂直分割线
#if 0 // 测试使用
            if (device_t.co2_history_buf[x] == 0)
                co2_high = 80;
            co2_y = co2_y_start - co2_high;
            Paint_DrawLine(co2_x, co2_y, co2_x, co2_y_start - 1, foreground_color, DOT_PIXEL_2X2, LINE_STYLE_SOLID); //垂直分割线
#endif
            co2_x += 5;
        }

        Paint_DrawBitMap_Paste(Image_TVOC, 25, 250, 40, 10, flip_color); // TVOC
        /*TVOC等级显示*/
        switch (device_t.sensor_t.tvoc_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 29, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 29, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 29, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 29, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 29, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 29, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Light, 130, 245, 24, 22, flip_color); // 光照
        /*环境光等级显示*/
        switch (device_t.sensor_t.light_level) {
        case 0:
            Paint_DrawBitMap_Paste(Image_Level_0, 127, 275, 32, 13, flip_color);
            break;
        case 1:
            Paint_DrawBitMap_Paste(Image_Level_1, 127, 275, 32, 13, flip_color);
            break;
        case 2:
            Paint_DrawBitMap_Paste(Image_Level_2, 127, 275, 32, 13, flip_color);
            break;
        case 3:
            Paint_DrawBitMap_Paste(Image_Level_3, 127, 275, 32, 13, flip_color);
            break;
        case 4:
            Paint_DrawBitMap_Paste(Image_Level_4, 127, 275, 32, 13, flip_color);
            break;
        case 5:
            Paint_DrawBitMap_Paste(Image_Level_5, 127, 275, 32, 13, flip_color);
            break;
        default:
            break;
        }

        Paint_DrawBitMap_Paste(Image_Press, 190, 245, 24, 18, flip_color); // 大气压
        Paint_DrawBitMap_Paste(Image_MBAR, 252, 286, 24, 10, flip_color); // 大气压图标
        /*大气压数值显示*/
        EPD_Custom_Number24_Show_Other(210, 262, device_t.sensor_t.pressure);
#endif
    default:
        break;
    }

    DEBUG_TRACE(LOG_TAG, "EPD Will Update... %d", device_t.epdShowDuCnt);
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
#if FUNC_PM2_5
    if (device_t.epdShowDuCnt >= 1 && device_t.epdShowDuCnt <= 4) {
        EPD_Display_Du(ImageBuff);
    } else {
        device_t.epdShowDuCnt = 0;
        lut_GC(); //  3段式居中 有效驱动时间15Frames
        EPD_Display(ImageBuff);
        lut_DU();
    }
    device_t.epdShowDuCnt++;
#else
    EPD_Display(ImageBuff);
#endif
}

void EPD_ShowClean()
{
    EPD_Init();
    lut_GC();
    EPD_Clear();
    ENTER_DEEP_SLEEP();
    device_t.epdShowDuCnt = 0;
}

typedef struct {
    u8 len;
    u8 width;
    u8 height;
    const u8 *data_buf;
} number_conf_index;

// zmod4xxx_conf.

// 16 *24
number_conf_index number24_index[10] = {
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_0[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_1[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_2[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_3[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_4[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_5[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_6[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_7[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_8[0]},
    {.len = 48, .width = 16, .height = 24, .data_buf = &Image_Number24_9[0]},
};

void EPD_Custom_Number24_Show_TempHumi(u16 x, u16 y, u32 data, u16 enlargement, u8 dot_size)
{
    u16 x_start = x, y_start = y;
    u8 number_h, number_l;
    u8 flip_color = 1;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        foreground_color = WHITE;
        flip_color = 0;
    }

    number_h = data / enlargement;
    number_l = data / (enlargement / 10) % 10;

    u8 index = number_h / 10;

    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;
    index = number_h % 10;
    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;

    if (dot_size > 0) {
        x_start += dot_size;
        Paint_DrawPoint(x_start, y_start + number24_index[index].height - 3, foreground_color, (DOT_PIXEL)dot_size, DOT_STYLE_DFT);
        x_start += dot_size;
        index = number_l;
        Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height,
                               flip_color);
    }
}

void EPD_Custom_Number24_Show_HCHO(u16 x, u16 y, u16 data)
{
    u16 x_start = x, y_start = y;

    u8 hcho_ppm_l = (data + 5) / 10 % 100; // 四舍五入
    u8 index = 0;
    u8 flip_color = 1;
    u8 foreground_color = BLACK;

    if (device_t.epdOverturnShow) {
        foreground_color = WHITE;
        flip_color = 0;
    }

    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;

    x_start += DOT_PIXEL_2X2;
    Paint_DrawPoint(x_start, y_start + number24_index[index].height - 3, foreground_color, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    x_start += DOT_PIXEL_2X2;

    index = hcho_ppm_l / 10;
    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;

    index = hcho_ppm_l % 10;
    Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height, flip_color);
    x_start += number24_index[index].width;
}

void EPD_Custom_Number24_Show_Other(u16 x, u16 y, u16 data)
{
    u16 x_start = x, y_start = y;
    u8 i = 0, index = 0;

    u8 number[5] = {0};
    u8 number_len = 1;
    u8 flip_color = 1;

    if (device_t.epdOverturnShow) {
        flip_color = 0;
    }
    if (data > 0) {
        if (data <= 9) {
            number[0] = data % 10;
            number_len = 1;
        }
        if (data <= 99) {
            number[0] = data / 10;
            number[1] = data % 10;
            number_len = 2;
        } else if (data <= 999) {
            number[0] = data / 100;
            number[1] = data / 10 % 10;
            number[2] = data % 10;
            number_len = 3;
        } else if (data <= 9999) {
            number[0] = data / 1000;
            number[1] = data / 100 % 10;
            number[2] = data / 10 % 10;
            number[3] = data % 10;
            number_len = 4;
        } else if (data > 9999) {
            number[0] = 9;
            number[1] = 9;
            number[2] = 9;
            number[3] = 9;
            number_len = 4;
        }
    }

    while (number_len--) {
        index = number[i++];
        Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height,
                               flip_color);
        x_start += number24_index[index].width;
    }
}

void EPD_Custom_Number24_Show_PM(u16 x, u16 y, u16 data)
{
    u16 x_start = x, y_start = y;
    u8 i = 0, index = 0;

    u8 number[3] = {0};
    u8 number_len = 1;
    u8 flip_color = 1;

    if (device_t.epdOverturnShow) {
        flip_color = 0;
    }
    if (data >= 0) {
        if (data <= 9) {
            number[0] = data % 10;
            number_len = 1;
            x_start += 20;
        } else if (data <= 99) {
            number[0] = data / 10;
            number[1] = data % 10;
            number_len = 2;

            x_start += 10;
        } else if (data <= 999) {
            number[0] = data / 100;
            number[1] = data / 10 % 10;
            number[2] = data % 10;
            number_len = 3;
        }
    }

    while (number_len--) {
        index = number[i++];
        Paint_DrawBitMap_Paste(number24_index[index].data_buf, x_start, y_start, number24_index[index].width, number24_index[index].height,
                               flip_color);
        x_start += number24_index[index].width;
    }
}

/**
 * @brief  屏幕显示管理任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_display(ULONG thread_input)
{
    extern TX_EVENT_FLAGS_GROUP event_group;
    ULONG actual_events;
    UINT status;

    u8 epd_power_enable = 1;
    EPD_Init();
    lut_GC();
    EPD_ShowLoading();
    // tx_thread_sleep(8000);
    // tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
    status = tx_event_flags_get(&event_group, /* 事件标志控制块 */
                                BIT_DISPLAY, /* 等待标志 */
                                TX_OR_CLEAR, /* 等待任意bit满足即可 */
                                &actual_events, /* 获取实际值 */
                                TX_WAIT_FOREVER); /* 永久等待 */
    while (1) {
        /* 室内温度限制 0-50摄氏度 */
        if ((device_t.sensor_t.temperature >= 50000 || device_t.sensor_t.temperature <= 0)) {
            if(epd_power_enable){
                epd_power_enable = 0;
                EPD_ShowClean();
                EPD_PWR_CTRL_H;
            }
            device_t.epdShowDuCnt = 0;
        } else {
            if (device_t.epdInit || !epd_power_enable) {
                device_t.epdInit = 0;
                device_t.epdShowDuCnt = 0;
                EPD_Init();
                if (device_t.epdShowDuCnt >= 1 && device_t.epdShowDuCnt <= 4) {
                    device_t.epdOldImageShow = 1;
                }
            }
            
            epd_power_enable = 1;
            if (device_t.epdShowMode == 0xFF) {
                EPD_ShowDevEUI();
            } else
                EPD_ShowUpdate();
        }
        tx_thread_sleep(1000); // 刷新完后最少等待3秒再进行刷新
        status = tx_event_flags_get(&event_group, /* 事件标志控制块 */
                                    BIT_DISPLAY, /* 等待标志 */
                                    TX_OR_CLEAR, /* 等待任意bit满足即可 */
                                    &actual_events, /* 获取实际值 */
                                    TX_WAIT_FOREVER); /* 永久等待 */
#if FUNC_PM2_5
            if (device_t.epdShowDuCnt >= 1 && device_t.epdShowDuCnt <= 4) {
                lut_DU();
                device_t.epdOldImageShow = 1;
            } else
                lut_GC();
#endif
    }
}

#endif
