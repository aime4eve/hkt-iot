#include "cb_hcho_v4c.h"
#include "control_center.h"
#include "uart.h"

u8 heater_wait_time = 60; // 预热60秒
/*
国家要求甲醛的安全范围是 0.08mg/m3-0.10mg/m3以下
当空气中甲醛浓度在0.06mg/m3-0.07mg/m3，儿童会有气促的表现。
当甲醛浓度达到0.1mg/m³，会有咽部不适的感觉，当浓度达到0.5mg/m³，就会刺激眼睛，引起流泪。
当达到0.6mg/m³，引起嗓子疼痛、恶心、呕吐、肺水肿等严重的情况。当达到30mg/m³，可能导致人的死亡。
*/

// HCHO 传感器校验和计算 CS= 256-(HEAD+LEN+CMD+DATA)
static u8 HCHO_CRC_Calculate(u8 *data, u16 count)
{
    u16 sum = 0;
    for (int i = 0; i < count - 1; i++) {
        sum += data[i];
    }
    sum = (256 - sum) % 0xFF;
    return (u8)sum;
}

// HCHO 传感器初始化
void HCHO_Init(void)
{
    Uartx_Init(UART5, 9600);
}

// 查询甲醛浓度
void Select_PPM_From_HCHO(void)
{
    if (heater_wait_time)
        return;
    u8 buffer[4] = {0x11, 0x01, 0x01, 0xED};
    HCHO_SendData(buffer, 4);
}

// HCHO 传感器串口数据处理
void HCHO_Cmd_Process(u8 *cmd, u16 cmd_size)
{
    float ppb, ppm;
		int i = 0; 
    u8 sumNum = HCHO_CRC_Calculate(cmd, cmd_size);
    if ((cmd[0] == 0x16) && (cmd[1] == 0x0d) && (cmd[2] == 0x01) && (cmd[cmd_size - 1] == sumNum)) // 校验数据同步头
    {
        ppb = (unsigned int)cmd[3] << 8 | cmd[4]; // bit 4: ppm high 8-bit; bit 5: ppm low 8-bit
        ppm = ppb / 1000.0;                       // 1ppb = 1000ppm

        u8 status = cmd[13];
        u8 zero = cmd[14];
        device_t.sensor_t.hcho_ppm = ppb;

        if (ppb == 0) {
            ppb = device_t.sensor_t.hcho_ppm; // 读取失败使用上一次的值
            DEBUG_TRACE(ERROR_TAG, "HCHO Invalid sample detected, skipping.");
            goto __hcho_histery;
        } else {
        __hcho_histery:
            device_t.sensor_t.hcho_ppm = ppb;
            for (i = 0; i < 36; i++) {
                if (device_t.hcho_history_buf[i] == 0)
                    break;
            }
            if (i < 36) {
                device_t.hcho_history_buf[i] = ppb;
            } else {
                for (i = 0; i < 35; i++) {
                    device_t.hcho_history_buf[i] = device_t.hcho_history_buf[i + 1];
                }
                device_t.hcho_history_buf[35] = ppb;
            }
            DEBUG_TRACE(LOG_TAG, "HCHO Read Data: %.2fppb, %.2fppm, status: %d, zero: %d", ppb, ppm, status, zero);
        }
    } else {
        DEBUG_TRACE(ERROR_TAG, "HCHO Cmd Check CRC ERRO: 0x%02x", sumNum);
    }
}
