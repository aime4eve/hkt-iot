
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "CM1106SL.h"
#include "IM8601PA.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "multi_button.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

u16 ledGreenBlinkTime;
u16 ledRedBlinkTime;
u16 ledOrangeBlinkTime;

static struct button func_key;
static struct button rst_key;

extern TX_EVENT_FLAGS_GROUP event_group;

uint8_t button_read_func_key_pin(void) { return READ_FUNC_KEY_STATUS; }

uint8_t button_read_rst_key_pin(void) { return READ_RST_KEY_STATUS; }

void FUNC_KEY_SINGLE_CLICK_Handler(void)
{
    INFO("\npress func key short handler\n");
#if FUNC_THREE_IN_ONE
    device_t.epdShowMode = 0;
    device_t.syncEpdShowMode = 1;
    device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
    device_t.epdShowDuCnt = 0;
    tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
#elif EPD_SHOW_MODE_FIXED 

#if FUNC_PM2_5
    device_t.epdShowMode = 2;
#elif FUNC_TVOC
    device_t.epdShowMode = 1;
#else
    device_t.epdShowMode = 0;
#endif
    tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);

#else
    switch (device_t.epdShowMode) {
    case 0:
#if (FUNC_AMBIENT_LIGHT && FUNC_TVOC && FUNC_PRESSURE)
        device_t.epdShowMode = 1;
#else
        device_t.epdShowMode = 0;
#endif
        break;
    case 1:
#if FUNC_PM2_5
        device_t.epdShowMode = 2;
#else
        device_t.epdShowMode = 0;
#endif
        break;
#if FUNC_PM2_5
    case 2:
        device_t.epdShowMode = 0;
        break;
#endif
    default:
        device_t.epdShowMode = 0;
        break;
    }
    device_t.syncEpdShowMode = 1;

    device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
    device_t.epdShowDuCnt = 0;
    tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);

#endif
}

void FUNC_KEY_DOUBLE_CLICK_Handler(void)
{
    INFO("\npress func key double handler\n");

    if (device_t.ledShowMode == 0)
        device_t.ledShowMode = 1;
#if FUNC_PM2_5
    else if (device_t.ledShowMode == 1)
        device_t.ledShowMode = 2;
    else if (device_t.ledShowMode == 2)
        device_t.ledShowMode = 0;
#else
    else if (device_t.ledShowMode == 1)
        device_t.ledShowMode = 0;
#endif
    device_t.syncLedShowMode = 1;
    device_t.ledShowModeWriteDelay = 5 * SEC_DELAY;
}

void FUNC_KEY_LONG_RRESS_START_Handler(void)
{
    INFO("\npress func key long handler\n");
    if (device_t.epdShowMode != 0xFF) {
        device_t.epdShowMode = 0xFF;
    } else {
        if (device_t.epdOverturnShow)
            device_t.epdOverturnShow = 0;
        else
            device_t.epdOverturnShow = 1;
        device_t.syncEpdOverturnMode = 1;
        device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
    }

    device_t.epdShowDuCnt = 0;
    tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
}

void RST_KEY_SINGLE_CLICK_Handler(void)
{
    INFO("\npress rst key short handler\n");
    if (device_t.epdShowTempWay)
        device_t.epdShowTempWay = 0;
    else
        device_t.epdShowTempWay = 1;

    device_t.syncTempShowWay = 1;
    device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
    if (device_t.epdShowMode != 0xFF) {
        device_t.epdShowDuCnt = 0;
        tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
    }
}

void RST_KEY_DOUBLE_CLICK_Handler(void) { INFO("\npress rst key double handler\n"); }

void RST_KEY_LONG_RRESS_START_Handler(void)
{
    INFO("\npress rst key long handler\n");
    FLASH_Reset_Param();
    device_t.syncDeviceState = 1;
    dataReportTimestamp = Timestamp + device_t.reportInterval * 60;
    sensorConvertTimestamp = Timestamp;
    device_t.epdShowDuCnt = 0;
    tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
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
            device_t.extiWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // 复位按键
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(RST_KEY_PORT, RST_KEY_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(RST_KEY_PORT, RST_KEY_PIN);

        if (device_t.sleepState) {
            // device_t.sleepState = 0;
            device_t.extiWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // PIR
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(PIR_DOCI_PORT, PIR_DOCI_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(PIR_DOCI_PORT, PIR_DOCI_PIN);
        // tx_event_flags_set(&event_group, BIT_PIR, TX_OR);
        if (device_t.sleepState) {
            // device_t.sleepState = 0;
        }
        device_t.pirEvent = 1;
        if (device_t.sleepDelay < 3)
            device_t.sleepDelay = 3;
    }

    // USB电源输入
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(DC_PWR_IN_PORT, DC_PWR_IN_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(DC_PWR_IN_PORT, DC_PWR_IN_PIN);

        if (device_t.sleepState) {
            // device_t.sleepState = 0;
            device_t.extiWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // TVOC 中断
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(TVOC_INT_PORT, TVOC_INT_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(TVOC_INT_PORT, TVOC_INT_PIN);
        if (device_t.sleepState) {
            // device_t.sleepState = 0;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }

    // O3 中断
    if (SET == GPIO_EXTI_EXTIISR_ChkEx(O3_INT_PORT, O3_INT_PIN)) {
        GPIO_EXTI_EXTIISR_ClrEx(O3_INT_PORT, O3_INT_PIN);
        if (device_t.sleepState) {
            // device_t.sleepState = 0;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }
}

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
    static u8 button_enable = 0;
    CMU_OPCCR1_EXTICKSEL_Set(CMU_OPCCR1_EXTICKSEL_LSCLK); // EXTI中断采样时钟选择
    CMU_OPCCR1_EXTICKE_Setable(ENABLE);                   // EXTI工作时钟使能
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);                 // IO控制时钟寄存器使能

    /** 初始化按键 IO **/
    InputtIO(FUNC_KEY_PORT, FUNC_KEY_PIN, IN_PULLUP);
    InputtIO(RST_KEY_PORT, RST_KEY_PIN, IN_PULLUP);

    /** 初始化LED IO **/
    OutputIO(LED_GREEN_PORT, LED_GREEN_PIN, OUT_PUSHPULL);
    OutputIO(LED_ORANGE_PORT, LED_ORANGE_PIN, OUT_PUSHPULL);
    OutputIO(LED_RED_PORT, LED_RED_PIN, OUT_PUSHPULL);

    /** 初始化LoRaWAN 模块IO **/
    OutputIO(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, OUT_PUSHPULL);
    OutputIO(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, OUT_PUSHPULL);
    OutputIO(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN, OUT_PUSHPULL);

    InputtIO(LoRaWAN_BUSY_PORT, LoRaWAN_BUSY_PIN, IN_NORMAL);
    InputtIO(LoRaWAN_STAT_PORT, LoRaWAN_STAT_PIN, IN_NORMAL);

    /** 初始化其它 IO **/
    InputtIO(DC_PWR_IN_PORT, DC_PWR_IN_PIN, IN_PULLUP);
    OutputIO(CO2_PWR_CTRL_PORT, CO2_PWR_CTRL_PIN, OUT_PUSHPULL);
    OutputIO(EPD_PWR_CTRL_PORT, EPD_PWR_CTRL_PIN, OUT_PUSHPULL);
#if FUNC_TVOC
    OutputIO(TVOC_RST_PORT, TVOC_RST_PIN, OUT_PUSHPULL);
    // InputtIO(TVOC_INT_PORT, TVOC_INT_PIN, IN_PULLUP);
    TVOC_RST_H;
#endif
    // InputtIO(O3_INT_PORT, O3_INT_PIN, IN_PULLUP);

    OutputIO(BEEP_CTRL_PORT, BEEP_CTRL_PIN, OUT_PUSHPULL);
    BEEP_CTRL_L;

    // ** 初始化中断 IO ** //
    // GPIO_EXTI_Init(FUNC_KEY_PORT, FUNC_KEY_PIN, EXTI_FALLING, ENABLE);
    // GPIO_EXTI_Init(RST_KEY_PORT, RST_KEY_PIN, EXTI_FALLING, ENABLE);
    // GPIO_EXTI_Init(TVOC_INT_PORT, TVOC_INT_PIN, EXTI_FALLING, ENABLE);
    // GPIO_EXTI_Init(O3_INT_PORT, O3_INT_PIN, EXTI_FALLING, ENABLE);
    GPIO_EXTI_Init(DC_PWR_IN_PORT, DC_PWR_IN_PIN, EXTI_BOTH, ENABLE);
    // TicksDelayUs(100);

    /*NVIC中断配置*/
    NVIC_DisableIRQ(GPIO_IRQn);
    NVIC_SetPriority(GPIO_IRQn, 0); // 中断优先级配置
    NVIC_EnableIRQ(GPIO_IRQn);

    GPIO_PINWKEN_SetableEx(PINWKEN_PG7, ENABLE);
    GPIO_PINWKEN_SetableEx(PINWKEN_PB0, ENABLE);

    if (button_enable)
        return;
    button_enable = 1;
    button_init(&func_key, button_read_func_key_pin, 0);
    button_init(&rst_key, button_read_rst_key_pin, 0);

    button_attach(&func_key, SINGLE_CLICK, (BtnCallback)FUNC_KEY_SINGLE_CLICK_Handler);
    button_attach(&func_key, DOUBLE_CLICK, (BtnCallback)FUNC_KEY_DOUBLE_CLICK_Handler);
    button_attach(&func_key, LONG_RRESS_START, (BtnCallback)FUNC_KEY_LONG_RRESS_START_Handler);

    button_attach(&rst_key, SINGLE_CLICK, (BtnCallback)RST_KEY_SINGLE_CLICK_Handler);
    button_attach(&rst_key, DOUBLE_CLICK, (BtnCallback)RST_KEY_DOUBLE_CLICK_Handler);
    button_attach(&rst_key, LONG_RRESS_START, (BtnCallback)RST_KEY_LONG_RRESS_START_Handler);

    button_start(&func_key);
    button_start(&rst_key);
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
    CMU_OPCCR1_EXTICKSEL_Set(CMU_OPCCR1_EXTICKSEL_LSCLK); // EXTI中断采样时钟选择
    CMU_OPCCR1_EXTICKE_Setable(ENABLE);                   // EXTI工作时钟使能
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);                 // IO控制时钟寄存器使能

    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_5);
    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_6);
    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_7);

    // #if FUNC_HCHO
    //     CloseIO(GPIOA, GPIO_Pin_8);
    //     CloseIO(GPIOA, GPIO_Pin_9);
    // #else
    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_8);
    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_9);
    // #endif

    // GPIO_OUTPUT_L(GPIOA, GPIO_Pin_10);
    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_11);
    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_12);
    GPIO_OUTPUT_L(GPIOA, GPIO_Pin_13);

    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_1);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_2);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_3);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_4);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_5);
    GPIO_SetBits(GPIOB, GPIO_Pin_4);
    GPIO_SetBits(GPIOB, GPIO_Pin_5);

    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_9);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_10);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_11);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_12);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_13);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_14);
    GPIO_OUTPUT_L(GPIOB, GPIO_Pin_15);
    // 电子纸部分
    // EPD_PWR_CTRL_H;
    // GPIO_OUTPUT_L(GPIOE, GPIO_Pin_2);
    // GPIO_OUTPUT_L(GPIOE, GPIO_Pin_3);
    // GPIO_OUTPUT_L(GPIOE, GPIO_Pin_4);
    // GPIO_OUTPUT_L(GPIOE, GPIO_Pin_5);
    GPIO_OUTPUT_L(GPIOE, GPIO_Pin_6);
    // GPIO_OUTPUT_L(GPIOE, GPIO_Pin_7);
    // GPIO_OUTPUT_L(GPIOE, GPIO_Pin_8);

    GPIO_OUTPUT_L(GPIOE, GPIO_Pin_9);

    GPIO_OUTPUT_L(GPIOF, GPIO_Pin_0);

    GPIO_OUTPUT_L(GPIOC, GPIO_Pin_10);
    GPIO_OUTPUT_L(GPIOC, GPIO_Pin_11);
    GPIO_OUTPUT_L(GPIOC, GPIO_Pin_13);

#if FUNC_THREE_IN_ONE
    GPIO_OUTPUT_L(GPIOD, GPIO_Pin_0);
    GPIO_OUTPUT_L(GPIOD, GPIO_Pin_1);
    GPIO_OUTPUT_L(GPIOD, GPIO_Pin_4);
#endif
    GPIO_OUTPUT_L(GPIOD, GPIO_Pin_5);

    GPIO_OUTPUT_L(GPIOD, GPIO_Pin_6);
    GPIO_OUTPUT_L(GPIOD, GPIO_Pin_7);
    GPIO_OUTPUT_L(GPIOD, GPIO_Pin_8);
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
    GPIO_OUTPUT_L(GPIOF, GPIO_Pin_12);
    GPIO_OUTPUT_L(GPIOF, GPIO_Pin_13);
    GPIO_OUTPUT_L(GPIOF, GPIO_Pin_14);
    GPIO_OUTPUT_L(GPIOF, GPIO_Pin_15);

    GPIO_OUTPUT_L(GPIOG, GPIO_Pin_2);
    GPIO_OUTPUT_L(GPIOG, GPIO_Pin_3);
    GPIO_OUTPUT_L(GPIOG, GPIO_Pin_6);

    GPIO_OUTPUT_L(GPIOG, GPIO_Pin_4);
    GPIO_OUTPUT_L(GPIOG, GPIO_Pin_5);

    // // LoRaWAN通讯串口
    // CloseIO(GPIOC, GPIO_Pin_0); // PC0 LPUART RX
    // CloseIO(GPIOC, GPIO_Pin_1); // PC1 LPUART TX

    // 打印串口
    CloseIO(GPIOF, GPIO_Pin_3); // PF3 UART0 RX
    CloseIO(GPIOF, GPIO_Pin_4); // PF4 UART0 TX
    // AltFunIO(GPIOF, GPIO_Pin_3, ALTFUN_PULLUP);
    // AltFunIO(GPIOF, GPIO_Pin_4, ALTFUN_PULLUP);

    AltFunIO(GPIOG, GPIO_Pin_8, ALTFUN_PULLUP); // SWCLK上拉使能
    AltFunIO(GPIOG, GPIO_Pin_9, ALTFUN_PULLUP); // SWIO上拉使能

    // CloseIO(GPIOG, GPIO_Pin_8); // SWCLK
    // CloseIO(GPIOG, GPIO_Pin_9); // SWIO

    // GPIO_PINWKEN_SetableEx(PINWKEN_PB0, ENABLE);
    // GPIO_PINWKEN_SetableEx(PINWKEN_PD6, ENABLE);
    // GPIO_PINWKEN_SetableEx(PINWKEN_PE2, ENABLE);
    // GPIO_PINWKEN_SetableEx(PINWKEN_PG7, ENABLE);

    // CloseIO(GPIOA, GPIO_Pin_14); // PA14;//SCL
    // CloseIO(GPIOA, GPIO_Pin_15); // PA15;//SDA
    // GPIO_OUTPUT_L(GPIOA, GPIO_Pin_14); // PA14;//SCL
    // GPIO_OUTPUT_L(GPIOA, GPIO_Pin_15); // PA15;//SDA
    CO2_PWR_CTRL_L;
    // LED_GREEN_H;
    // LED_ORANGE_H;
    // LED_RED_H;
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

/**
 * @brief  获取DC输入状态
 * @param
 * @retval
 */
void readDCPowerInputStatus(void)
{
    if (!READ_DC_PWR_IN_STATUS) {
        tx_thread_sleep(20);
        if (!READ_DC_PWR_IN_STATUS) {
            device_t.powerIn = 0;
            device_t.energyMode = 0;
        }
    } else {
        tx_thread_sleep(20);
        if (READ_DC_PWR_IN_STATUS) {
            device_t.powerIn = 1;
            device_t.energyMode = 1;
        }
    }
}

void Beep(int time)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);
    OutputIO(BEEP_CTRL_PORT, BEEP_CTRL_PIN, OUT_PUSHPULL);
    while (time--) {
        BEEP_CTRL_L;
        TicksDelayUs(400);
        BEEP_CTRL_H;
        TicksDelayUs(400);
    }
}

/**
 * @brief  用于LED闪烁
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_led(ULONG thread_input)
{
    (void)thread_input;
    u16 led_tick = 0, beep_tick = 0;
    while (1) {
        IWDT_Clr();

        led_tick++;
        /* LED控制逻辑 */
        if (device_t.ledShowMode == 1) { // 模式1：常闭
            LED_GREEN_H;
            LED_ORANGE_H;
            LED_RED_H;
        } else if (device_t.ledShowMode == 2) { // 模式2：常亮
            switch (device_t.sensor_t.air_level) {
            case AIR_LEVEL_NORMAL:
                LED_GREEN_L;
                LED_ORANGE_H;
                LED_RED_H;
                break;
            case AIR_LEVEL_MILD:
                LED_ORANGE_L;
                LED_GREEN_H;
                LED_RED_H;
                break;
            case AIR_LEVEL_BAD:
            default:
                LED_RED_L;
                LED_ORANGE_H;
                LED_GREEN_H;
                break;
            }
        } else { // 模式0：10秒周期闪烁
            if (led_tick >= 100)
                led_tick = 0; // 10秒周期复位
            // 在周期末尾亮1秒（最后10个tick）
            if (led_tick >= 99) {
                switch (device_t.sensor_t.air_level) {
                case AIR_LEVEL_NORMAL:
                    LED_GREEN_L;
                    LED_ORANGE_H;
                    LED_RED_H;
                    break;
                case AIR_LEVEL_MILD:
                    LED_ORANGE_L;
                    LED_GREEN_H;
                    LED_RED_H;
                    break;
                case AIR_LEVEL_BAD:
                default:
                    LED_RED_L;
                    LED_ORANGE_H;
                    LED_GREEN_H;
                    break;
                }
            } else {
                LED_GREEN_H;
                LED_ORANGE_H;
                LED_RED_H; // 熄灭
            }
        }

        if (device_t.beepEnable) {
            beep_tick++;
            if (device_t.sensor_t.air_level == AIR_LEVEL_MILD) {
                if (beep_tick == 300) {
                    beep_set(2000, 50);
                } else if (beep_tick == 301) {
                    beep_tick = 0;
                    BEEP_OFF();
                }
            } else if (device_t.sensor_t.air_level >= AIR_LEVEL_BAD) {
                if (beep_tick == 300 || beep_tick == 302) {
                    beep_set(2000, 50);
                } else if (beep_tick == 301 || beep_tick == 303) {
                    BEEP_OFF();
                }
                if (beep_tick == 303) {
                    beep_tick = 0;
                }
            } else {
                beep_tick = 0;
            }
        }
        tx_thread_sleep(100);
    }
}
