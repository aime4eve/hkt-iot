#include "MBI5120.h"
#include "SI12T.h"
#include "control_center.h"
#include "gpio.h"
#include "systick.h"

union u_mbi5120 u_mbi5120_t;

/**
 * @brief  LED驱动芯片初始化
 * @param
 * @retval
 */
void MBI5120_Init(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能

    OutputIO(MBI5120_SDI_PORT, MBI5120_SDI_PIN, OUT_PUSHPULL);
    OutputIO(MBI5120_CLK_PORT, MBI5120_CLK_PIN, OUT_PUSHPULL);
    OutputIO(MBI5120_LE_PORT, MBI5120_LE_PIN, OUT_PUSHPULL);
    OutputIO(MBI5120_OE_PORT, MBI5120_OE_PIN, OUT_PUSHPULL);
    OutputIO(MBI5120_PWR_PORT, MBI5120_PWR_PIN, OUT_PUSHPULL);

    MBI5120_OE_H; // 关闭LED显示
    MBI5120_PWR_H;
    // tx_thread_sleep(100);
}

/**
 * @brief  显示配置
 * @param  display LED显示参数
 * @retval
 */
void MBI5120_Display(u16 display)
{
    if (device_t.isLedShow) {
        device_t.isLedShow = 0;
    }

    if (display == 0) {
        s_touch.clear_signal = 1;
    } else {
        s_touch.clear_signal = 0;
    }
    u16 dat = display;

    // __disable_irq();
    MBI5120_SDI_L;
    MBI5120_CLK_L;
    MBI5120_LE_L; // 锁定当前输出
    TicksDelayUs(500);
    for (int i = 0; i < 16; i++) {
        if (dat & 0x8000) {
            MBI5120_SDI_H;
        } else {
            MBI5120_SDI_L;
        }
        dat <<= 1;
        TicksDelayUs(500);
        MBI5120_CLK_H;
        TicksDelayUs(500);
        MBI5120_CLK_L;
    }
    MBI5120_LE_H; // 解锁，将数据更新到输出
    TicksDelayUs(500);
    MBI5120_LE_L; // 锁定，防止意外改变
    MBI5120_OE_L; // 一直低电平
    MBI5120_CLK_L;
    // __enable_irq();
}

u16 getLedShowNum(u16 inputNum)
{
    union u_mbi5120 t_mbi5120;
    t_mbi5120.dat = 0xFFFF;
#if EU_TOUCH_LED
    switch (inputNum) {
    case 0x32:
        t_mbi5120.s_mbi5120_t.led_0 = 0;
        break;
    case 0x23:
        t_mbi5120.s_mbi5120_t.led_1 = 0;
        break;
    case 0x30:
        t_mbi5120.s_mbi5120_t.led_2 = 0;
        break;
    case 0x2A:
        t_mbi5120.s_mbi5120_t.led_3 = 0;
        break;
    case 0x39:
        t_mbi5120.s_mbi5120_t.led_4 = 0;
        break;
    case 0x38:
        t_mbi5120.s_mbi5120_t.led_5 = 0;
        break;
    case 0x37:
        t_mbi5120.s_mbi5120_t.led_6 = 0;
        break;
    case 0x36:
        t_mbi5120.s_mbi5120_t.led_7 = 0;
        break;
    case 0x35:
        t_mbi5120.s_mbi5120_t.led_8 = 0;
        break;
    case 0x34:
        t_mbi5120.s_mbi5120_t.led_9 = 0;
        break;
    case 0x33:
        t_mbi5120.s_mbi5120_t.led_x = 0;
        break;
    case 0x31:
        t_mbi5120.s_mbi5120_t.led_j = 0;
        break;
    default:
        break;
    }
#else
    switch (inputNum) {
    case 0x30:
        t_mbi5120.s_mbi5120_t.led_0 = 0;
        break;
    case 0x31:
        t_mbi5120.s_mbi5120_t.led_1 = 0;
        break;
    case 0x32:
        t_mbi5120.s_mbi5120_t.led_2 = 0;
        break;
    case 0x33:
        t_mbi5120.s_mbi5120_t.led_3 = 0;
        break;
    case 0x34:
        t_mbi5120.s_mbi5120_t.led_4 = 0;
        break;
    case 0x35:
        t_mbi5120.s_mbi5120_t.led_5 = 0;
        break;
    case 0x36:
        t_mbi5120.s_mbi5120_t.led_6 = 0;
        break;
    case 0x37:
        t_mbi5120.s_mbi5120_t.led_7 = 0;
        break;
    case 0x38:
        t_mbi5120.s_mbi5120_t.led_8 = 0;
        break;
    case 0x39:
        t_mbi5120.s_mbi5120_t.led_9 = 0;
        break;
    case 0x2A:
        t_mbi5120.s_mbi5120_t.led_x = 0;
        break;
    case 0x23:
        t_mbi5120.s_mbi5120_t.led_j = 0;
        break;
    default:
        break;
    }

#endif
    return t_mbi5120.dat;
}

u16 Mbi_ShowNum(u16 inputNum)
{
    union u_mbi5120 t_mbi5120;
    t_mbi5120.dat = 0;

    switch (inputNum) {
    case 0:
        t_mbi5120.s_mbi5120_t.led_0 = 1;
        break;
    case 1:
        t_mbi5120.s_mbi5120_t.led_1 = 1;
        break;
    case 2:
        t_mbi5120.s_mbi5120_t.led_2 = 1;
        break;
    case 3:
        t_mbi5120.s_mbi5120_t.led_3 = 1;
        break;
    case 4:
        t_mbi5120.s_mbi5120_t.led_4 = 1;
        break;
    case 5:
        t_mbi5120.s_mbi5120_t.led_5 = 1;
        break;
    case 6:
        t_mbi5120.s_mbi5120_t.led_6 = 1;
        break;
    case 7:
        t_mbi5120.s_mbi5120_t.led_7 = 1;
        break;
    case 8:
        t_mbi5120.s_mbi5120_t.led_8 = 1;
        break;
    case 9:
        t_mbi5120.s_mbi5120_t.led_9 = 1;
        break;
    case 10:
        t_mbi5120.s_mbi5120_t.led_x = 1;
        break;
    case 11:
        t_mbi5120.s_mbi5120_t.led_j = 1;
        break;
    default:
        break;
    }
    return t_mbi5120.dat;
}
