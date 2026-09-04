
#ifndef __IM8601PA_H__
#define __IM8601PA_H__

#include "config.h"

#define PIR_SERIN_PORT GPIOB
#define PIR_SERIN_PIN GPIO_Pin_7
#define PIR_SERIN_H GPIO_SetBits(PIR_SERIN_PORT, PIR_SERIN_PIN)
#define PIR_SERIN_L GPIO_ResetBits(PIR_SERIN_PORT, PIR_SERIN_PIN)
// #define PIR_SERIN_H (PIR_SERIN_PORT->DSET = PIR_SERIN_PIN)
// #define PIR_SERIN_L (PIR_SERIN_PORT->DRST = PIR_SERIN_PIN)

#define PIR_SERIN_OUT OutputIO(PIR_SERIN_PORT, PIR_SERIN_PIN, OUT_PUSHPULL)
#define PIR_SERIN_IN InputtIO(PIR_SERIN_PORT, PIR_SERIN_PIN, IN_NORMAL)

#define PIR_DOCI_PORT GPIOB
#define PIR_DOCI_PIN GPIO_Pin_8
#define PIR_DOCI_H GPIO_SetBits(PIR_DOCI_PORT, PIR_DOCI_PIN)
#define PIR_DOCI_L GPIO_ResetBits(PIR_DOCI_PORT, PIR_DOCI_PIN)
// #define PIR_DOCI_H (PIR_DOCI_PORT->DSET = PIR_DOCI_PIN)
// #define PIR_DOCI_L (PIR_DOCI_PORT->DRST = PIR_DOCI_PIN)

#define READ_PIR_DOCI_STATUS GPIO_ReadInputDataBit(PIR_DOCI_PORT, PIR_DOCI_PIN)
// #define READ_PIR_DOCI_STATUS (PIR_DOCI_PORT->DIN & PIR_DOCI_PIN)

#define PIR_DOCI_OUT OutputIO(PIR_DOCI_PORT, PIR_DOCI_PIN, OUT_PUSHPULL)
// #define PIR_DOCI_OUT PIR_DOCI_PORT->FCR = 0x14000;PIR_DOCI_PORT->INEN = 0x01;

#define PIR_DOCI_IN InputtIO(PIR_DOCI_PORT, PIR_DOCI_PIN, IN_NORMAL)
// #define PIR_DOCI_IN PIR_DOCI_PORT->FCR = 0x4000;PIR_DOCI_PORT->INEN = 0x101;

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    // define pir read data structure here
    struct pir_register
    {
        u8 sensitivity;     // 灵敏度阈值
        u8 blind_time;      // 盲区时间
        u8 pulse_cnt;       // 人体运动触发脉冲计数
        u8 window_time;     // 脉冲计数窗口时间
        u8 motion_en;       // 人体运动检测使能
        u8 int_source;      // 中断源选择
        u8 adc_source;      // adc信号源选择
        u8 exldo_en;        // 2.2v稳压器输出使能
        u8 self_test;       // 自检测试
        u8 hpf_cut_frq;     // hpf高通滤波器截至频率设置
        u8 input_clamp;     // 输入钳位
        u8 motion_mode;     // 人体运动检测脉冲计数模式
        u8 out_range;       // PIR数据溢出位 0读取的PIR数值超量程 1读取的数值正常
        u16 signal_voltage; // 信号电压值    6.5uV/count
    };

    extern struct pir_register pir_param;

    void PIR_Init(void);
    void readPIRAllData(void);
    u32 readPIRData_AllBits(void);
    void PIR_ReadEnd(void);
    void thread_pir(ULONG thread_input);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
