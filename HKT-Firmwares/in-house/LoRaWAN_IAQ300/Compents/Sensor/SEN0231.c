#include "SEN0231.h"
#include "uart.h"
#include "control_center.h"

/*
国家要求甲醛的安全范围是 0.08mg/m3-0.10mg/m3以下
当空气中甲醛浓度在0.06mg/m3-0.07mg/m3，儿童会有气促的表现。
当甲醛浓度达到0.1mg/m³，会有咽部不适的感觉，当浓度达到0.5mg/m³，就会刺激眼睛，引起流泪。
当达到0.6mg/m³，引起嗓子疼痛、恶心、呕吐、肺水肿等严重的情况。当达到30mg/m³，可能导致人的死亡。
*/

// HCHO 传感器校验和计算
u8 HCHO_CRC_Calculate(u8 *data, u16 count)
{
    u8 sum = 0;
    for (int i = 0; i < count - 1; i++)
    {
        sum += data[i];
    }
    sum = (~sum) + 1;
    return sum;
}

// HCHO 传感器初始化
void SEN0231_Init(void)
{
    Uartx_Init(UART5, 9600);
}

// HCHO 传感器串口数据处理
// void HCHO_Cmd_Process(u8 *cmd, u16 cmd_size)
//{
//    float ppb, ppm;
//    u8 sumNum = HCHO_CRC_Calculate(cmd, cmd_size);
//    if ((cmd[0] == 0xFF) && (cmd[1] == 0x17) && (cmd[2] == 0x04) && (cmd[cmd_size - 1] == sumNum)) //校验数据同步头
//    {
//        ppb = (unsigned int)cmd[4] << 8 | cmd[5]; // bit 4: ppm high 8-bit; bit 5: ppm low 8-bit
//        ppm = ppb / 1000.0;                       // 1ppb = 1000ppm]
//        device_t.sensor_t.hcho_ppm = ppb;
//        DEBUG_TRACE(LOG_TAG, "HCHO Read Data: %.2fppb, %.2fppm", ppb, ppm);
//    }
//    else
//        DEBUG_TRACE(ERROR_TAG, "HCHO Cmd Check CRC ERRO: %d", sumNum);
//}
