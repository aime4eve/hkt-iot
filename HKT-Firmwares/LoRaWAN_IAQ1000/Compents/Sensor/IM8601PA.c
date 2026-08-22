
#include "im8601pa.h"
#include "control_center.h"
#include "uart.h"
#include "systick.h"
#include "gpio.h"
#include "communicate.h"

/****************************/

struct pir_register pir_param;

void pir_delay(uint8 counter)
{
    uint8 i;

    for (i = 0; i < counter; i++)
        asm("nop");
}

/**
 * @brief	发送数据给PIR传感器
 * @param   num： 待发送数据字节长度
 * @param   data： 待发送数据
 * @retval
 */
void sendPIRDataBits(u8 num, u8 data)
{
    for (u8 i = 0; i < num; i++)
    {
        PIR_SERIN_L;
        TicksDelayUs(1);
        PIR_SERIN_H;
        TicksDelayUs(1);
        if (!(data & 0x80)) // 仅当输出0的时候调整输出
            PIR_SERIN_L;
        TicksDelayUs(70);
        data = (data << 1);
    }
}

/**
 * @brief	读取PIR传感器的数据
 * @param
 * @retval
 */
u32 readPIRData_AllBits()
{
    u16 data1 = 0;
    u32 data2 = 0;
    u8 i = 0;

    __disable_irq(); // 关闭全局中断使能

    PIR_DOCI_OUT;
    PIR_DOCI_L;
    TicksDelayUs(5);
    PIR_DOCI_H;
    TicksDelayUs(115);

    for (i = 0; i < 15; i++)
    {
        PIR_DOCI_L;
        PIR_DOCI_OUT;
        pir_delay(1);
        PIR_DOCI_H;
        pir_delay(1);
        PIR_DOCI_IN;
        pir_delay(1);
        data1 <<= 1;
        if (READ_PIR_DOCI_STATUS)
            data1++;
    }
    for (i = 0; i < 25; i++)
    {
        PIR_DOCI_L;
        PIR_DOCI_OUT;
        pir_delay(1);
        PIR_DOCI_H;
        pir_delay(1);
        PIR_DOCI_IN;
        pir_delay(1);
        data2 <<= 1;
        if (READ_PIR_DOCI_STATUS)
            data2++;
    }

    PIR_ReadEnd();

    if (data1 & 0x4000)
    {
        pir_param.out_range = 1;
        data1 &= 0x3FFF;
    }
    else
    {
        pir_param.out_range = 0;
    }

    pir_param.signal_voltage = data1;
    // pir_param.signal_voltage = (data1 & 0x3FFF) * 6.5;

    DEBUG_TRACE(LOG_TAG, "%s Success, 0x%04X, 0x%08X, Out Range: %d, Signal Voltage: %duV", __func__, data1, data2, pir_param.out_range, pir_param.signal_voltage);

    return data2;
}

/**
 * @brief	强制结束PIR中断读数
 * @param
 * @retval
 */
void PIR_ReadEnd(void)
{
    __disable_irq();
    PIR_DOCI_L;
    PIR_DOCI_OUT;
    pir_delay(50);  // delay tL
    PIR_DOCI_IN;    // set to interrupt mode
    __enable_irq(); // 打开全局中断使能
}

/**
 * @brief	初始化PIR参数配置
 * @param
 * @retval
 */
void PIR_Init(void)
{
    CMU_OPCCR1_EXTICKSEL_Set(CMU_OPCCR1_EXTICKSEL_LSCLK); // EXTI中断采样时钟选择
    CMU_OPCCR1_EXTICKE_Setable(ENABLE);                   // EXTI工作时钟使能
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);                 // IO控制时钟寄存器使能
    PIR_SERIN_OUT;
    PIR_DOCI_OUT;
    PIR_SERIN_L;
    PIR_DOCI_L;
    // PIR_DOCI_IN;

    pir_param.sensitivity = 0x30; // 灵敏度调节
    // pir_param.sensitivity = 0xAA; //灵敏度调节  阈值 0xAA * 6.5uV
    pir_param.blind_time = 3;  // 盲区时间8s  x*0.5 + 0.5  0.5~8s
    pir_param.pulse_cnt = 1;   // 2
    pir_param.window_time = 0; // 2s
    pir_param.motion_en = 1;   // 使能运动检测
    pir_param.int_source = 0;
    pir_param.adc_source = 0;
    pir_param.exldo_en = 0;
    pir_param.self_test = 0;
    pir_param.hpf_cut_frq = 0;
    pir_param.input_clamp = 0;
    pir_param.motion_mode = 0;

    tx_thread_sleep(20);

    __disable_irq(); // 关闭全局中断使能
    // start to send 25bit data
    // 8bit sensitivity
    sendPIRDataBits(8, pir_param.sensitivity);

    // 4bit blind time
    sendPIRDataBits(4, pir_param.blind_time << 4);

    // 2bit pluse counter
    sendPIRDataBits(2, pir_param.pulse_cnt << 6);

    // 2bit window time
    sendPIRDataBits(2, pir_param.window_time << 6);

    // 1bit motion enable
    sendPIRDataBits(1, pir_param.motion_en << 7);

    // 1bit interrupt source
    sendPIRDataBits(1, pir_param.int_source << 7);

    // 2bit adc source
    sendPIRDataBits(2, pir_param.adc_source << 6);

    // 1bit external ldo turnoff
    sendPIRDataBits(1, pir_param.exldo_en << 7);

    // 1bit start self test
    sendPIRDataBits(1, pir_param.self_test << 7);

    // 1bit hpf cut-off frequency
    sendPIRDataBits(1, pir_param.hpf_cut_frq << 7);

    // 1bit clamp input
    sendPIRDataBits(1, pir_param.input_clamp << 7);

    // 1bit motion detector mode
    sendPIRDataBits(1, pir_param.motion_mode << 7);
    PIR_SERIN_L;
    __enable_irq();      // 打开全局中断使能
    tx_thread_sleep(10); // 写完后等待5ms以上再读取数据
    PIR_DOCI_IN;
    GPIO_EXTI_Init(PIR_DOCI_PORT, PIR_DOCI_PIN, EXTI_RISING, ENABLE);
    // #if 0
    // while (1)
    // {
    //     tx_thread_sleep(1000);

    u32 value = readPIRData_AllBits();
    struct pir_register pir_temp;

    // decode data
    pir_temp.sensitivity = ((uint8)(value >> 17));
    pir_temp.blind_time = (((uint8)(value >> 13)) & 0x0f);
    pir_temp.pulse_cnt = (((uint8)(value >> 11)) & 0x03);
    pir_temp.window_time = (((uint8)(value >> 9)) & 0x03);
    pir_temp.motion_en = value & 0x100;
    pir_temp.int_source = value & 0x80;
    pir_temp.adc_source = (((uint8)(value >> 5)) & 0x03);
    pir_temp.exldo_en = value & 0x10;
    pir_temp.self_test = value & 0x08;
    pir_temp.hpf_cut_frq = value & 0x04;
    pir_temp.input_clamp = value & 0x02;
    pir_temp.motion_mode = value & 0x01;

    INFO("\n********************************\n");
    INFO("PIR Read Set Param \n");
    INFO("\t sensitivity: 0x%02X\n", pir_temp.sensitivity);
    INFO("\t blind_time: %d\n", pir_temp.blind_time);
    INFO("\t pulse_cnt: %d\n", pir_temp.pulse_cnt);
    INFO("\t window_time: %d\n", pir_temp.window_time);
    INFO("\t motion_en: %d\n", pir_temp.motion_en);
    INFO("\t int_source: %d\n", pir_temp.int_source);
    INFO("\t adc_source: %d\n", pir_temp.adc_source);
    INFO("\t exldo_en: %d\n", pir_temp.exldo_en);
    INFO("\t self_test: %d\n", pir_temp.self_test);
    INFO("\t hpf_cut_frq: %d\n", pir_temp.hpf_cut_frq);
    INFO("\t input_clamp: %d\n", pir_temp.input_clamp);
    INFO("\t motion_mode: %d\n", pir_temp.motion_mode);
    INFO("********************************\n");
    //     }
    // #endif
}

/**
 * @brief	读取PIR数据
 * @param
 * @retval
 */
void readPIRAllData(void)
{
    u32 value = readPIRData_AllBits();

    // 转换间隔强制大于180秒
    if (pirConvertTimestamp <= Timestamp && (pir_param.out_range || pir_param.signal_voltage > 200) && sensorConvertTimestamp > Timestamp)
    {
        device_t.sensor_t.pir_signal = 1;
        sensorConvertTimestamp = Timestamp;
        pirConvertTimestamp = Timestamp + 180;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        DEBUG_TRACE(LOG_TAG, "%s Success, Next Read Timestamp: %d, Now Timestamp: %d", __func__, pirConvertTimestamp, Timestamp);
    }

#if PIR_DEBUG_PRINTF

    struct pir_register pir_temp;
    // decode data
    pir_temp.sensitivity = ((uint8)(value >> 17));
    pir_temp.blind_time = (((uint8)(value >> 13)) & 0x0f);
    pir_temp.pulse_cnt = (((uint8)(value >> 11)) & 0x03);
    pir_temp.window_time = (((uint8)(value >> 9)) & 0x03);
    pir_temp.motion_en = value & 0x100;
    pir_temp.int_source = value & 0x80;
    pir_temp.adc_source = (((uint8)(value >> 5)) & 0x03);
    pir_temp.exldo_en = value & 0x10;
    pir_temp.self_test = value & 0x08;
    pir_temp.hpf_cut_frq = value & 0x04;
    pir_temp.input_clamp = value & 0x02;
    pir_temp.motion_mode = value & 0x01;

    INFO("\n********************************\n");
    INFO("PIR Read Set Param \n");
    INFO("\t sensitivity: 0x%02X\n", pir_temp.sensitivity);
    INFO("\t blind_time: %d\n", pir_temp.blind_time);
    INFO("\t pulse_cnt: %d\n", pir_temp.pulse_cnt);
    INFO("\t window_time: %d\n", pir_temp.window_time);
    INFO("\t motion_en: %d\n", pir_temp.motion_en);
    INFO("\t int_source: %d\n", pir_temp.int_source);
    INFO("\t adc_source: %d\n", pir_temp.adc_source);
    INFO("\t exldo_en: %d\n", pir_temp.exldo_en);
    INFO("\t self_test: %d\n", pir_temp.self_test);
    INFO("\t hpf_cut_frq: %d\n", pir_temp.hpf_cut_frq);
    INFO("\t input_clamp: %d\n", pir_temp.input_clamp);
    INFO("\t motion_mode: %d\n", pir_temp.motion_mode);
    INFO("********************************\n");
#endif
    //   PIR_DOCI_OUT;
    //   PIR_DOCI_L;
    //   mDelay(20);
    //   PIR_DOCI_H;
    //   mDelay(20);
    //   PIR_DOCI_IN;
    //  GPIO_EXTI_Init(PIR_DOCI_PORT, PIR_DOCI_PIN, EXTI_RISING, ENABLE);
}

/**
 * @brief  pir传感器转换任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_pir(ULONG thread_input)
{
    extern TX_EVENT_FLAGS_GROUP event_group;
    ULONG actual_events;
    PIR_Init();
    // tx_event_flags_set(&event_group, BIT_PIR, TX_OR);
    while (1)
    {
        // UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
        //                                  BIT_PIR,          /* 等待标志 */
        //                                  TX_OR_CLEAR,      /* 等待任意bit满足即可 */
        //                                  &actual_events,   /* 获取实际值 */
        //                                  TX_WAIT_FOREVER); /* 永久等待 */
        if (device_t.pirEvent)
        {
            device_t.pirEvent = 0;
            // GPIO_EXTI_Close(PIR_DOCI_PORT, PIR_DOCI_PIN);
            readPIRAllData();
            // tx_thread_sleep(1000);
            // GPIO_EXTI_Init(PIR_DOCI_PORT, PIR_DOCI_PIN, EXTI_RISING, ENABLE);
        }
        tx_thread_sleep(200);
    }
}
