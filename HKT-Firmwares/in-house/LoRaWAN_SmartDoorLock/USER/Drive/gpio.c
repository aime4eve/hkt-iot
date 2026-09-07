
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "MBI5120.h"
#include "SI12T.h"
#include "W25X40CLSNIG.h"
#include "ZS902R.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "multi_button.h"
#include "sound.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

u16 ledGreenBlinkTime;
u16 ledRedBlinkTime;
u16 ledOrangeBlinkTime;
u8  system_rst;
u8  tamperio_state;

struct button func_key;
struct button rst_key;

uint8_t button_read_rst_key_pin(void)
{
    return READ_RST_KEY_STATUS;
}

uint8_t button_read_func_key_pin(void)
{
    return READ_FUNC_KEY_STATUS;
}

void FUNC_KEY_LONG_RRESS_START_Handler(void)
{
    INFO("\nPress func key long handler\n");
}

void RST_KEY_LONG_RRESS_START_Handler(void)
{
    // INFO("\nPress rst key long handler\n");
    system_rst = 1;
}

/**
 * @brief  This function handles External line 0 to 1 interrupt request.
 * @param  None
 * @retval None
 */
void GPIO_IRQHandler(void)
{
    // 功能按键
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(FUNC_KEY_PORT, FUNC_KEY_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(FUNC_KEY_PORT, FUNC_KEY_PIN);

        if (device_t.sleepState) {
            // device_t.sleepState = 0;
            device_t.tamperWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // 复位按键
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(RST_KEY_PORT, RST_KEY_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(RST_KEY_PORT, RST_KEY_PIN);

        if (device_t.sleepState) {
            // device_t.sleepState = 0;
            device_t.rstKeyWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // TCH中断
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(TCH_IRQ_PORT, TCH_IRQ_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(TCH_IRQ_PORT, TCH_IRQ_PIN);
        GPIO_EXTI_Close(TCH_IRQ_PORT, TCH_IRQ_PIN);
        if (device_t.sleepState) {
            // device_t.sleepState = 0;
            device_t.touchWakeEvent = 1;
        } else {
            tx_event_flags_set(&event_group, BIT_TOUCH, TX_OR);
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // CARD中断
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(CARD_IRQ_PORT, CARD_IRQ_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(CARD_IRQ_PORT, CARD_IRQ_PIN);
        // tx_event_flags_set(&event_group, BIT_CARD, TX_OR);
        if (device_t.sleepState) {
            // device_t.sleepState = 0;
            device_t.cardWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // FINGER中断
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(FINGER_DETECT_PORT, FINGER_DETECT_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(FINGER_DETECT_PORT, FINGER_DETECT_PIN);
        // tx_event_flags_set(&event_group, BIT_FINGER, TX_OR);

        if (device_t.sleepState) {
            // device_t.sleepState = 0;
            device_t.fingerWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

#if !FUNC_LORA_VERSION_LIR
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN);
        if (device_t.sleepState) {
            device_t.loraWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }
#endif
}

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
    u8 static button_key_enable = 0;
    CMU_OPCCR1_EXTICKSEL_Set(CMU_OPCCR1_EXTICKSEL_LSCLK); // EXTI中断采样时钟选择
    CMU_OPCCR1_EXTICKE_Setable(ENABLE);                   // EXTI工作时钟使能
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);                 // IO控制时钟寄存器使能

    // /** 初始化按键 IO **/
    InputtIO(FUNC_KEY_PORT, FUNC_KEY_PIN, IN_PULLUP);
    InputtIO(RST_KEY_PORT, RST_KEY_PIN, IN_PULLUP);

    /** 初始化LED IO **/
    OutputIO(LED_GREEN_PORT, LED_GREEN_PIN, OUT_PUSHPULL);

#if FUNC_LORA_VERSION_LIR
    /** 初始化LoRaWAN 模块IO **/
    OutputIO(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, OUT_PUSHPULL);
    OutputIO(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, OUT_PUSHPULL);
    OutputIO(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN, OUT_PUSHPULL);
    GPIO_SetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    InputtIO(LoRaWAN_BUSY_PORT, LoRaWAN_BUSY_PIN, IN_NORMAL);
    InputtIO(LoRaWAN_STAT_PORT, LoRaWAN_STAT_PIN, IN_NORMAL);
#else
    /** 初始化LoRaWAN 模块IO **/
    OutputIO(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN, OUT_PUSHPULL);
    InputtIO(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, IN_NORMAL);
    OutputIO(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, OUT_PUSHPULL);
    GPIO_SetBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);

#if FUNC_BT_ENABLE
    /** 初始化BLE IO **/
    InputtIO(BLE_STATUS_PORT, BLE_STATUS_PIN, IN_NORMAL);
    OutputIO(BLE_PWR_PORT, BLE_PWR_PIN, OUT_PUSHPULL);
    BLE_PWR_H;
#endif

#endif

    /** 初始化读卡器 IO **/
    InputtIO(CARD_IRQ_PORT, CARD_IRQ_PIN, IN_PULLUP);
    OutputIO(CARD_RST_PORT, CARD_RST_PIN, OUT_PUSHPULL);
#if !FSV9522_CARD_ENABLE
    OutputIO(CARD_IIC_PORT, CARD_IIC_PIN, OUT_PUSHPULL);
    OutputIO(CARD_EA_PORT, CARD_EA_PIN, OUT_PUSHPULL);
    CARD_IIC_L;
    CARD_EA_H;
#endif
    CARD_RST_H;

    /** 初始化触摸IC IO **/
    OutputIO(TCH_IIC_EN_PORT, TCH_IIC_EN_PIN, OUT_PUSHPULL); // IIC EN
    OutputIO(TCH_RST_PORT, TCH_RST_PIN, OUT_PUSHPULL);       // RST
    OutputIO(TCH_SCT_PORT, TCH_SCT_PIN, OUT_PUSHPULL);       // SCT (高电平暂停 0或者悬空工作)
    InputtIO(TCH_IRQ_PORT, TCH_IRQ_PIN, IN_NORMAL);          // IRQ
    TCH_SCT_L;
    TCH_RST_H; // 硬件损耗需要整改 10uA
    TCH_IIC_EN_H;

#if FUNC_FINGERPRINT_ENABLE
    /** 初始化指纹 IO **/
    OutputIO(FINGER_POWER_PORT, FINGER_POWER_PIN, OUT_PUSHPULL);
    InputtIO(FINGER_DETECT_PORT, FINGER_DETECT_PIN, IN_NORMAL);

    GPIO_EXTI_Close(FINGER_DETECT_PORT, FINGER_DETECT_PIN);
    FINGER_POWER_L; // 硬件损耗需要整改    FINGER_POWER_H 40UA  L 20UA
#endif

    Sound_Init();
    MOTOR_Init();
    MBI5120_Init();

    // ** 初始化中断 IO ** //
    GPIO_EXTI_Init(FUNC_KEY_PORT, FUNC_KEY_PIN, EXTI_BOTH, ENABLE);
    GPIO_EXTI_Init(RST_KEY_PORT, RST_KEY_PIN, EXTI_FALLING, ENABLE);
    // GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);
    GPIO_EXTI_Init(CARD_IRQ_PORT, CARD_IRQ_PIN, EXTI_FALLING, ENABLE);
    // GPIO_EXTI_Init(FINGER_DETECT_PORT, FINGER_DETECT_PIN, EXTI_RISING, ENABLE);
#if !FUNC_LORA_VERSION_LIR
    GPIO_EXTI_Init(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, EXTI_RISING, ENABLE);
#endif

    /*NVIC中断配置*/
    NVIC_DisableIRQ(GPIO_IRQn);
    NVIC_SetPriority(GPIO_IRQn, 0); // 中断优先级配置
    NVIC_EnableIRQ(GPIO_IRQn);

    GPIO_PINWKEN_SetableEx(PINWKEN_PG7, ENABLE);
    GPIO_PINWKEN_SetableEx(PINWKEN_PD6, ENABLE);

    if (!button_key_enable) {
        button_key_enable = 0;
        button_init(&rst_key, button_read_rst_key_pin, 0);
        button_attach(&rst_key, LONG_RRESS_START, (BtnCallback)RST_KEY_LONG_RRESS_START_Handler);
        button_start(&rst_key);

        // if (READ_FUNC_KEY_STATUS)
        //     button_init(&func_key, button_read_func_key_pin, 0);
        // else
        //     button_init(&func_key, button_read_func_key_pin, 1);
        // button_attach(&func_key, LONG_RRESS_START, (BtnCallback)FUNC_KEY_LONG_RRESS_START_Handler);
        // button_start(&func_key);
    }
}

void Test_Fout(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能

    // //fout 输出系统时钟64分频
    // GPIO_FOUTSEL_FOUT1SEL_Set(GPIO_FOUTSEL_FOUT1SEL_LSCLK);

    // GPIOx_DFS_Setable(GPIOE, GPIO_Pin_2, ENABLE);
    // AltFunIO( GPIOE, GPIO_Pin_2, ALTFUN_NORMAL );

    // GPIO_FOUTSEL_FOUT0SEL_Set(GPIO_FOUTSEL_FOUT0SEL_RCHFD64);
    GPIO_FOUTSEL_FOUT0SEL_Set(GPIO_FOUTSEL_FOUT0SEL_LSCLK);
    GPIOx_DFS_Setable(GPIOG, GPIO_Pin_6, ENABLE);
    AltFunIO(GPIOG, GPIO_Pin_6, ALTFUN_NORMAL);
}

void MOTOR_Init(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能
    OutputIO(MOTOR_INA_PORT, MOTOR_INA_PIN, OUT_PUSHPULL);
    OutputIO(MOTOR_INB_PORT, MOTOR_INB_PIN, OUT_PUSHPULL);
    MOTOR_STANDBY();
}

void MOTOR_FOREWARD(void)
{
    MOTOR_INA_H;
    MOTOR_INB_L;
}

void MOTOR_REVERSAL(void)
{
    MOTOR_INA_L;
    MOTOR_INB_H;
}

void MOTOR_STANDBY(void)
{
    MOTOR_INA_L;
    MOTOR_INB_L;
}

void MOTOR_BRAKE(void)
{
    MOTOR_INA_H;
    MOTOR_INB_H;
}

void GPIO_OUTPUT_H(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
    OutputIO(GPIOx, GPIO_Pin, 0);
    GPIO_SetBits(GPIOx, GPIO_Pin);
}

void GPIO_OUTPUT_L(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
    OutputIO(GPIOx, GPIO_Pin, 0);
    GPIO_ResetBits(GPIOx, GPIO_Pin);
}

void GPIOH_OUTPUT_L(uint32_t GPIO_Pin)
{
    OutputIO_H(GPIO_Pin);
    GPIOH_DO_Write(GPIO_Pin);
}

void GPIO_Sleep_Config(void)
{
    static u8 isConfig = 0;

    if (isConfig == 0) {
        isConfig = 1;
        GPIO_OUTPUT_L(GPIOA, GPIO_Pin_5);
        GPIO_OUTPUT_L(GPIOA, GPIO_Pin_6);
        GPIO_OUTPUT_L(GPIOA, GPIO_Pin_7);

        GPIO_OUTPUT_L(GPIOA, GPIO_Pin_11);
        GPIO_OUTPUT_L(GPIOA, GPIO_Pin_12);
        GPIO_OUTPUT_L(GPIOA, GPIO_Pin_13);

        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_0);
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_1);

#if !FUNC_FINGERPRINT_ENABLE
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_2);
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_3);
#endif

        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_4);
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_5);
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_6);
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_7);

#if FSV9522_CARD_ENABLE
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_10);
        GPIO_OUTPUT_L(GPIOB, GPIO_Pin_11);
#endif

#if !FUNC_FINGERPRINT_ENABLE
        GPIO_OUTPUT_L(GPIOE, GPIO_Pin_2);
#endif

        GPIO_OUTPUT_L(GPIOE, GPIO_Pin_7);
        GPIO_OUTPUT_L(GPIOE, GPIO_Pin_9);

        GPIO_OUTPUT_L(GPIOF, GPIO_Pin_0);

#if !FUNC_LORA_VERSION_LIR
        GPIO_OUTPUT_L(GPIOC, GPIO_Pin_3);
        GPIO_OUTPUT_L(GPIOC, GPIO_Pin_5);
#endif
        GPIO_OUTPUT_L(GPIOC, GPIO_Pin_9);

#if FUNC_LORA_VERSION_LIR
        GPIO_OUTPUT_L(GPIOC, GPIO_Pin_10);
        GPIO_OUTPUT_L(GPIOC, GPIO_Pin_11);
#else
        GPIO_OUTPUT_L(GPIOC, GPIO_Pin_0);
        GPIO_OUTPUT_L(GPIOC, GPIO_Pin_1);
#endif

        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_0);
        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_1);
        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_2);
        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_3);
        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_4);
        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_5);

        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_9);
        GPIO_OUTPUT_L(GPIOD, GPIO_Pin_10);

        GPIO_OUTPUT_L(GPIOF, GPIO_Pin_1);
        GPIO_OUTPUT_L(GPIOF, GPIO_Pin_2);

        GPIO_OUTPUT_L(GPIOF, GPIO_Pin_5);
        GPIO_OUTPUT_L(GPIOF, GPIO_Pin_6);

        GPIOH_OUTPUT_L(GPIO_Pin_0);
        GPIOH_OUTPUT_L(GPIO_Pin_1);
        GPIOH_OUTPUT_L(GPIO_Pin_2);
        GPIOH_OUTPUT_L(GPIO_Pin_3);

        GPIO_OUTPUT_L(GPIOF, GPIO_Pin_11);

        GPIO_OUTPUT_L(GPIOG, GPIO_Pin_2);
        GPIO_OUTPUT_L(GPIOG, GPIO_Pin_3);
        GPIO_OUTPUT_L(GPIOG, GPIO_Pin_6);

        GPIO_OUTPUT_L(GPIOG, GPIO_Pin_4);
        GPIO_OUTPUT_L(GPIOG, GPIO_Pin_5);
    }

    MBI5120_OE_L;
    MBI5120_CLK_L;
    MBI5120_LE_L;
    MBI5120_SDI_L;
    MBI5120_PWR_L;
    LED_GREEN_H;
    FINGER_POWER_L;

#if FUNC_BT_ENABLE
    // 蓝牙串口
    AnalogIO(GPIOA, GPIO_Pin_8); // PA8 UART5 RX
    AnalogIO(GPIOA, GPIO_Pin_9); // PA9 UART5 TX
    BLE_PWR_L;
    // GPIO_OUTPUT_L(GPIOA, GPIO_Pin_8);
    // GPIO_OUTPUT_L(GPIOA, GPIO_Pin_9);
#endif

    // LoRaWAN通讯串口
#if FUNC_LORA_VERSION_LIR
    AnalogIO(GPIOC, GPIO_Pin_10);
    AnalogIO(GPIOC, GPIO_Pin_11);
#else
    AnalogIO(GPIOC, GPIO_Pin_0);
    AnalogIO(GPIOC, GPIO_Pin_1);
#endif

    // 打印串口
    // CloseIO(GPIOF, GPIO_Pin_3); // PF3 UART0 RX
    // CloseIO(GPIOF, GPIO_Pin_4); // PF4 UART0 TX
    AnalogIO(GPIOF, GPIO_Pin_3); // PA8 UART5 RX
    AnalogIO(GPIOF, GPIO_Pin_4); // PA9 UART5 TX

    // 需要关闭 指纹模块漏电
    // GPIO_OUTPUT_L(GPIOB, GPIO_Pin_2);
    // GPIO_OUTPUT_L(GPIOB, GPIO_Pin_3);
    AnalogIO(GPIOB, GPIO_Pin_2);
    AnalogIO(GPIOB, GPIO_Pin_3);

    AltFunIO(GPIOG, GPIO_Pin_8, ALTFUN_PULLUP); // SWCLK上拉使能
    AltFunIO(GPIOG, GPIO_Pin_9, ALTFUN_PULLUP); // SWDIO上拉使能

    // CloseIO(GPIOG, GPIO_Pin_8); // SWCLK
    // CloseIO(GPIOG, GPIO_Pin_9); // SWDIO
}

/**
 * @brief  用于LED闪烁
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_led(ULONG thread_input)
{
    (void)thread_input;
    ULONG actual_events;
    tamperio_state = READ_FUNC_KEY_STATUS;

    UINT            status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                                BIT_LED,          /* 等待标志 */
                                                TX_AND_CLEAR,     /* 等待任意bit满足即可 */
                                                &actual_events,   /* 获取实际值 */
                                                TX_WAIT_FOREVER); /* 永久等待 */
    union u_mbi5120 t_mbi5120;
    MBI5120_Display(0xFFFF);

    while (1) {
        if (device_t.sleepState == 1) {
            LED_GREEN_H;
            tx_thread_sleep(200);
        } else {
            LED_GREEN_TOGGLEBITS;
            tx_thread_sleep(200);

            if (device_t.lockTimeout) {
                while (1) {
                    t_mbi5120.dat = Mbi_ShowNum(device_t.lockTimeout / (10 * SEC_DELAY)); // 十位
                    t_mbi5120.dat |= Mbi_ShowNum(device_t.lockTimeout / SEC_DELAY % 10);  // 个位
                    MBI5120_Display(t_mbi5120.dat);
                    tx_thread_sleep(200);
                    if (!device_t.lockTimeout) {
                        MBI5120_Display(0);
                        break;
                    }
                }
            }
        }
    }
}

/**
 * @brief  电机控制
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_motor(ULONG thread_input)
{
    (void)thread_input;
    ULONG actual_events;

    while (1) {
        UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
                                         BIT_MOTOR,        /* 等待标志 */
                                         TX_AND_CLEAR,     /* 等待任意bit满足即可 */
                                         &actual_events,   /* 获取实际值 */
                                         TX_WAIT_FOREVER); /* 永久等待 */

        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY) {
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        }
        device_t.openFailCnt = 0;

        /* 语音 */
        // 开锁提示音
        // 已开锁
        Sound_Insert_Number(0, VOICE_TONE_OPEN_LOCK);
        Sound_Insert_Number(1, VOICE_OPEN_LOCK);

        /* 正转 */
        MOTOR_FOREWARD();
        tx_thread_sleep(150);
        /* 刹车 */
        MOTOR_BRAKE();

        u_mbi5120_t.dat = 0;
#if EU_TOUCH_LED
        u_mbi5120_t.s_mbi5120_t.led_3 = 1;
#else
        u_mbi5120_t.s_mbi5120_t.led_x = 1;
#endif
        MBI5120_Display(u_mbi5120_t.dat);

        device_t.lockDoorTimeout = 5 * SEC_DELAY;

#if FUNC_OPERATIONAL_VERSION_ENABLE
        if (coded_open_mode_t.mode == 1) { // 常开模式
            tx_thread_sleep(1000);
        } else if (coded_open_mode_t.mode == 2) { // 延时一段时间上锁
            device_t.lockDoorTimeout = coded_open_mode_t.lock_delay * SEC_DELAY;
            // 等待上锁
            while (device_t.lockDoorTimeout) {
                tx_thread_sleep(100);
                if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY) {
                    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                }
            }

            /* 语音 */
            // 关锁提示音
            // 已关锁
            Sound_Insert_Number(0, VOICE_TONE_OFF_LOCK);
            Sound_Insert_Number(1, VOICE_OFF_LOCK);
            /* 反转 */
            MOTOR_REVERSAL();
            tx_thread_sleep(150);
            /* 刹车 */
            MOTOR_BRAKE();
            /* 待机 */
            MOTOR_STANDBY();
        } else if (coded_open_mode_t.mode == 3) {
            ;
        } else {
            device_t.lockDoorTimeout = device_t.auto_lock_time * SEC_DELAY;
            // 等待上锁
            while (device_t.lockDoorTimeout) {
                tx_thread_sleep(100);
                if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY) {
                    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                }
            }
            /* 语音 */
            // 关锁提示音
            // 已关锁
            Sound_Insert_Number(0, VOICE_TONE_OFF_LOCK);
            Sound_Insert_Number(1, VOICE_OFF_LOCK);
            /* 反转 */
            MOTOR_REVERSAL();

            tx_thread_sleep(150);
            /* 刹车 */
            MOTOR_BRAKE();
            /* 待机 */
            MOTOR_STANDBY();
        }

        MBI5120_Display(0);
        if (device_t.sleepDelay >= WAIT_SLEEP_TIME_DELAY && LoRaWAN.joinState)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        s_touch.timeout = 0;

        batteryTimestamp = Timestamp;
        if (batteryLevel <= 10) {
            // 电量低
            Sound_Insert_Number(1, VOICE_LOW_BATTERY);
        }
    }
#else

        // 等待上锁
        while (device_t.lockDoorTimeout) {
            tx_thread_sleep(100);
            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY) {
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            }
        }
        /* 语音 */
        // 关锁提示音
        // 已关锁
        Sound_Insert_Number(0, VOICE_TONE_OFF_LOCK);
        Sound_Insert_Number(1, VOICE_OFF_LOCK);
        /* 反转 */
        MOTOR_REVERSAL();
        tx_thread_sleep(150);
        /* 刹车 */
        MOTOR_BRAKE();
        /* 待机 */
        MOTOR_STANDBY();

        MBI5120_Display(0);
        if (device_t.sleepDelay >= WAIT_SLEEP_TIME_DELAY && LoRaWAN.joinState)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        s_touch.timeout = 0;

        batteryTimestamp = Timestamp;
        if (batteryLevel <= 10) {
            // 电量低
            Sound_Insert_Number(1, VOICE_LOW_BATTERY);
        }
    }

#endif
}
