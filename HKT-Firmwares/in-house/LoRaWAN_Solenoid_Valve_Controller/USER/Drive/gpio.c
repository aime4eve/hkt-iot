
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "multi_button.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

static struct button button_0;
static struct button button_1;
static struct button button_2;

u8 repeat_cnt;
time_t repeat_stamp;

uint8_t pulse1_read_pin_status(void)
{
    return PULSE1_STA;
}

uint8_t pulse2_read_pin_status(void)
{
    return PULSE2_STA;
}

uint8_t button2_read_pin_status(void)
{
    return VCS_INT_STA;
}

void BUTTON1_SINGLE_CLICK_Handler(void)
{
    device_t.press_short_event = 1;
    if (repeat_cnt++ == 0)
        repeat_stamp = Timestamp + 10;

    if (Timestamp > repeat_stamp && repeat_cnt) {
        repeat_cnt = 0;
    } else if (device_t.power_on) {
        if (repeat_cnt >= 7) {
            repeat_cnt = 0;
            device_t.wait_default_mode = 1;
        }
    }
}

void PULSE1_SINGLE_CLICK_Handler(void)
{
    if (device_t.pulse_delay) // 防止重复计数
        return;
    if (local_schedule_t.current->pulse_count && local_schedule_t.current->is_start == 1) {
        if ((device_t.port_mode & 0x01) == 0) {
            local_schedule_t.pulse_count++;
        }
    }
    if (real_time_task_t.is_start && real_time_task_t.pulse_count) {
        real_time_task_t.pulse_count_trigger++;
    }
    if ((device_t.port_mode & 0x01) == 0)
        device_t.pulse1_count++;
    INFO("\r\npulse1 click\r\n");
}

void PULSE2_SINGLE_CLICK_Handler(void)
{
    if (device_t.pulse_delay)
        return;
    if (local_schedule_t.current->pulse_count && local_schedule_t.current->is_start == 1) {
        if ((device_t.port_mode & 0x02) == 0) {
            local_schedule_t.pulse_count++;
        }
    }
    if (real_time_task_t.is_start && real_time_task_t.pulse_count) {
        real_time_task_t.pulse_count_trigger++;
    }
    if ((device_t.port_mode & 0x02) == 0) {
        device_t.pulse2_count++;
    }
    INFO("\r\npulse2 click\r\n");
}

void BUTTON1_DOUBLE_CLICK_Handler(void)
{
    if (!device_t.wait_default_mode)
        device_t.press_double_event = 1;
    else {
        device_t.wait_default_mode = 0;
    }
}

void BUTTON1_LONG_RRESS_START_Handler(void)
{
    if (!device_t.wait_default_mode)
        device_t.press_long_event = 1;
    else {
        device_t.factoryEvent = 1;
        device_t.wait_default_mode = 0;
    }
}

void BUTTON2_LONG_RRESS_START_Handler(void)
{
    device_t.sense_long_event = 1;
}

void BUTTON2_SINGLE_RRESS_START_Handler(void)
{
    if (LoRaWAN.joinState)
        device_t.sync_state = 1;
}

void BUTTON2_HOLD_Handler(void)
{
    device_t.sense_hold_event = 1;
}

/**
 * @brief  This function handles External line 0 to 1 interrupt request.
 * @param  None
 * @retval None
 */
void EXTI0_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_0)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
        if (device_t.sleep_state) {
            device_t.sleep_state = 0;
            device_t.vcs_wake_event = 1;
        }
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
}

void EXTI2_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_2)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_2);

        // if (device_t.sleep_state) {
        //     device_t.sleep_state = 0;
        //     device_t.ble_wake_event = 1;
        // }

        device_t.ble_wake_event = 1;

        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
}

void EXTI4_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_4)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_4);
        if (device_t.sleep_state) {
            device_t.sleep_state = 0;
            device_t.pulse1_wake_event = 1;
            if (local_schedule_t.current->pulse_count && local_schedule_t.current->is_start == 1) {
                local_schedule_t.pulse_count++;
            }

            if (real_time_task_t.is_start && real_time_task_t.pulse_count) {
                real_time_task_t.pulse_count_trigger++;
            }

            if ((device_t.port_mode & 0x01) == 0)
                device_t.pulse1_count++;
            device_t.pulse_delay = 500;
        }
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_5)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_5);
        if (device_t.sleep_state) {
            device_t.sleep_state = 0;
            device_t.pulse2_wake_event = 1;
            if (local_schedule_t.current->pulse_count && local_schedule_t.current->is_start == 1) {
                local_schedule_t.pulse_count++;
            }
            if (real_time_task_t.is_start && real_time_task_t.pulse_count) {
                real_time_task_t.pulse_count_trigger++;
            }
            if ((device_t.port_mode & 0x02) == 0) {
                device_t.pulse2_count++;
            }
            device_t.pulse_delay = 500;
        }
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }

    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_6)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);
        // if (device_t.sleep_state) {
        //     device_t.sleep_state = 0;
        //     device_t.insert1_wake_event = 1;
        // }

        device_t.insert1_wake_event = 1;
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }

    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_7)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_7);
        // if (device_t.sleep_state) {
        //     device_t.sleep_state = 0;
        //     device_t.insert2_wake_event = 1;
        // }

        device_t.insert2_wake_event = 1;

        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_11)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_11);

        if (device_t.sleep_state) {
            device_t.lora_wake_event = 1;
        }

        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
}

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
    port_sleep_init();

    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    // ** 初始化输出 IO ** //
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin = GPIO_BOOST_PWR_PIN;
    LL_GPIO_Init(GPIO_BOOST_PWR_PORT, &GPIO_InitStruct);

    // ** FLASH ** //
    GPIO_InitStruct.Pin = FLASH_WP_PIN;
    LL_GPIO_Init(FLASH_WP_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_RED_LED_PIN;
    LL_GPIO_Init(GPIO_RED_LED_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_BLUE_LED_PIN;
    LL_GPIO_Init(GPIO_BLUE_LED_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_CON1_EN_PIN;
    LL_GPIO_Init(GPIO_CON1_EN_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_CON1_A_PIN;
    LL_GPIO_Init(GPIO_CON1_A_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_CON1_B_PIN;
    LL_GPIO_Init(GPIO_CON1_B_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_CON2_EN_PIN;
    LL_GPIO_Init(GPIO_CON2_EN_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_CON2_A_PIN;
    LL_GPIO_Init(GPIO_CON2_A_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_CON2_B_PIN;
    LL_GPIO_Init(GPIO_CON2_B_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_5V_PWR_PIN;
    LL_GPIO_Init(GPIO_5V_PWR_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_9V_PWR_PIN;
    LL_GPIO_Init(GPIO_9V_PWR_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_12V_PWR_PIN;
    LL_GPIO_Init(GPIO_12V_PWR_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_BEEP_PIN;
    LL_GPIO_Init(GPIO_BEEP_PORT, &GPIO_InitStruct);

    GPIO_BOOST_PWR_L;
    GPIO_BOOST_PWR_L;
    GPIO_RED_LED_L;
    GPIO_BLUE_LED_L;
    GPIO_CON1_EN_L;
    GPIO_CON1_A_L;
    GPIO_CON1_B_L;
    GPIO_CON2_EN_L;
    GPIO_CON2_A_L;
    GPIO_CON2_B_L;
    GPIO_5V_PWR_L;
    GPIO_9V_PWR_L;
    GPIO_12V_PWR_L;
    GPIO_BEEP_L;

    // ** 初始化输入 IO ** //
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin = VCS_INT_PIN;
    LL_GPIO_Init(VCS_INT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    GPIO_InitStruct.Pin = BLE_CONNECT_PIN;
    LL_GPIO_Init(BLE_CONNECT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin = GPIO_INSERT1_INT_PIN;
    LL_GPIO_Init(GPIO_INSERT1_INT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_INSERT2_INT_PIN;
    LL_GPIO_Init(GPIO_INSERT2_INT_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PULSE1_PIN;
    LL_GPIO_Init(GPIO_PULSE1_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PULSE2_PIN;
    LL_GPIO_Init(GPIO_PULSE2_PORT, &GPIO_InitStruct);

#if FUNC_LORA_VERSION_LIR
    /** 初始化LoRaWAN 模块IO **/
    LL_GPIO_SetOutputPin(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);

    GPIO_InitStruct.Pin = LoRaWAN_MODE_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LoRaWAN_MODE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_NRST_PIN;
    LL_GPIO_Init(LoRaWAN_NRST_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_WAKE_PIN;
    LL_GPIO_Init(LoRaWAN_WAKE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_BUSY_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LoRaWAN_BUSY_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_STAT_PIN;
    LL_GPIO_Init(LoRaWAN_STAT_PORT, &GPIO_InitStruct);
#else

    /** 初始化LoRaWAN 模块IO **/
    LL_GPIO_SetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);

    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin = LoRaWAN_NRST_PIN;
    LL_GPIO_Init(LoRaWAN_NRST_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_WAKE_PIN;
    LL_GPIO_Init(LoRaWAN_WAKE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_IRQ_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LoRaWAN_IRQ_PORT, &GPIO_InitStruct);

    // ** 初始化IO中断 ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE11);
    LL_GPIO_SetPinPull(LoRaWAN_IRQ_PORT, LoRaWAN_IRQ_PIN, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(LoRaWAN_IRQ_PORT, LoRaWAN_IRQ_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_11;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    NVIC_SetPriority(EXTI15_10_IRQn, 0);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

#endif

    // ** 初始化IO中断 ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
    LL_GPIO_SetPinPull(VCS_INT_PORT, VCS_INT_PIN, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(VCS_INT_PORT, VCS_INT_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE2);
    LL_GPIO_SetPinPull(BLE_CONNECT_PORT, BLE_CONNECT_PIN, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinMode(BLE_CONNECT_PORT, BLE_CONNECT_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_2;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE6);
    LL_GPIO_SetPinPull(GPIO_INSERT1_INT_PORT, GPIO_INSERT1_INT_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIO_INSERT1_INT_PORT, GPIO_INSERT1_INT_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_6;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE7);
    LL_GPIO_SetPinPull(GPIO_INSERT2_INT_PORT, GPIO_INSERT2_INT_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIO_INSERT2_INT_PORT, GPIO_INSERT2_INT_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_7;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE4);
    LL_GPIO_SetPinPull(GPIO_PULSE1_PORT, GPIO_PULSE1_PIN, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinMode(GPIO_PULSE1_PORT, GPIO_PULSE1_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_4;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE5);
    LL_GPIO_SetPinPull(GPIO_PULSE2_PORT, GPIO_PULSE2_PIN, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinMode(GPIO_PULSE2_PORT, GPIO_PULSE2_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_5;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI0_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_IRQn);

    NVIC_SetPriority(EXTI2_IRQn, 0);
    NVIC_EnableIRQ(EXTI2_IRQn);

    NVIC_SetPriority(EXTI4_IRQn, 0);
    NVIC_EnableIRQ(EXTI4_IRQn);

    NVIC_SetPriority(EXTI9_5_IRQn, 0);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    // NVIC_SetPriority(EXTI15_10_IRQn, 0);
    // NVIC_EnableIRQ(EXTI15_10_IRQn);

    button_init(&button_0, pulse1_read_pin_status, 1);
    button_attach(&button_0, SINGLE_CLICK, (BtnCallback)PULSE1_SINGLE_CLICK_Handler);
    button_start(&button_0);

    button_init(&button_1, pulse2_read_pin_status, 1);
    button_attach(&button_1, SINGLE_CLICK, (BtnCallback)PULSE2_SINGLE_CLICK_Handler);
    button_start(&button_1);

    // button_init(&button_2, button2_read_pin_status, 0);
    // button_attach(&button_2, SINGLE_CLICK, (BtnCallback)BUTTON2_SINGLE_RRESS_START_Handler);
    // // button_attach(&button_2, LONG_PRESS_HOLD, (BtnCallback)BUTTON2_HOLD_Handler);
    // button_start(&button_2);
}

void port_sleep_init(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {LL_GPIO_PIN_ALL, LL_GPIO_MODE_ANALOG,
                                           LL_GPIO_SPEED_FREQ_HIGH, LL_GPIO_OUTPUT_PUSHPULL,
                                           LL_GPIO_PULL_NO, LL_GPIO_AF_0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA |
                             LL_AHB2_GRP1_PERIPH_GPIOB |
                             LL_AHB2_GRP1_PERIPH_GPIOC |
                             LL_AHB2_GRP1_PERIPH_GPIOD |
                             LL_AHB2_GRP1_PERIPH_GPIOH);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
    LL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOH, LL_GPIO_PIN_0 | LL_GPIO_PIN_1);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_13 | LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13 | LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_7;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_7);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_11 | LL_GPIO_PIN_12);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_2);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_5 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9);

#if (!FUNC_LORA_VERSION_LIR)
    GPIO_InitStruct.Pin = LL_GPIO_PIN_7 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_7 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9);
#endif


    LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOA |
                              LL_AHB2_GRP1_PERIPH_GPIOB |
                              LL_AHB2_GRP1_PERIPH_GPIOC |
                              LL_AHB2_GRP1_PERIPH_GPIOD |
                              LL_AHB2_GRP1_PERIPH_GPIOH);
}

void Beep(int time)
{
    while (time--) {
        GPIO_BEEP_H;
        Delay_Us(200);
        GPIO_BEEP_L;
        Delay_Us(200);
    }
}

void GpioEventProcess(void)
{
    static u32 over_time;
    static u8 value1_state_time, value2_state_time;
    static u8 value1_state, value2_state;

    if (!device_t.wait_default_mode) {
        if (device_t.press_short_event) {
            device_t.press_short_event = 0;
            INFO("\nButton Press Short Handler\n");
            ledRedBlinkTime = SEC_DELAY;
        }

        // 双击主动同步数据
        if (device_t.press_double_event) {
            device_t.press_double_event = 0;
            INFO("\nButton Press Double Handler\n");
            if (LoRaWAN.joinState)
                device_t.sync_state = 1;
        }

        if (device_t.press_long_event) {
            device_t.press_long_event = 0;
            INFO("\nButton Press Long Handler\n");
            if (device_t.power_on) {
                device_t.power_on = 0;
                BEEP_RUN(3000, 50); // 关机长鸣
            } else {
                device_t.power_on = 1;
                BEEP_RUN(1000, 50); // 开机短鸣
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
            }
        }

        if (device_t.sense_long_event) {
            device_t.sense_long_event = 0;
            INFO("\nSense Long Handler\n");
            if (device_t.power_on) {
                device_t.power_on = 0;
                BEEP_RUN(3000, 50); // 关机长鸣
            } else {
                device_t.power_on = 1;
                BEEP_RUN(1000, 50); // 开机短鸣
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
            }
        }

        if (device_t.sense_long_event) {
            device_t.sense_long_event = 0;
            INFO("\nSense Long Handler\n");
            if (device_t.power_on) {
                device_t.power_on = 0;
                BEEP_RUN(3000, 50); // 关机长鸣
            } else {
                device_t.power_on = 1;
                BEEP_RUN(1000, 50); // 开机短鸣
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
            }
        }

        if (device_t.sense_hold_event) {
            device_t.sense_hold_event = 0;
            INFO("\nButton Press Hold Handler\n");
        }
    }

    if (device_t.factoryEvent) {
        device_t.factoryEvent = 0;
        // INFO("\nButton Repeat 7 Tick, Device Default\n");
        INFO("\nDevice Default\n");
        FLASH_Reset_Param();
        ledRedBlinkTime = 3 * SEC_DELAY;
        BEEP_RUN(200, 50);
        mDelay(500);
        BEEP_RUN(200, 50);
    }

    if (device_t.port_mode) {
        switch (device_t.port_mode) {
        case 1: {
            if (device_t.value1_state != PULSE1_STA) {
                mDelay(500);
                if (device_t.value1_state != PULSE1_STA) {
                    device_t.value1_state = PULSE1_STA;
                    device_t.sync_state = 1;
                }
            }
        } break;
        case 2: {
            if (device_t.value2_state != PULSE2_STA) {
                mDelay(500);
                if (device_t.value2_state != PULSE2_STA) {
                    device_t.value2_state = PULSE2_STA;
                    device_t.sync_state = 1;
                }
            }
        } break;
        case 3: {
            if (device_t.value1_state != PULSE1_STA) {
                mDelay(500);
                if (device_t.value1_state != PULSE1_STA) {
                    device_t.value1_state = PULSE1_STA;
                    device_t.sync_state = 1;
                }
            }
            if (device_t.value2_state != PULSE2_STA) {
                mDelay(500);
                if (device_t.value2_state != PULSE2_STA) {
                    device_t.value2_state = PULSE2_STA;
                    device_t.sync_state = 1;
                }
            }
        } break;
        case 0x81: {
            if (value1_state != PULSE1_STA || value1_state != device_t.value1_state) {
                device_t.sleep_delay = 3;
            }
            if (value1_state != PULSE1_STA) {
                mDelay(20);
                if (value1_state != PULSE1_STA) {
                    value1_state = PULSE1_STA;
                }
            }
            if (over_time < get_syspant_ms()) {
                over_time = get_syspant_ms() + 1000;
                if (value1_state != device_t.value1_state) {
                    if (++value1_state_time >= device_t.stable_time) {
                        device_t.value1_state = value1_state;
                        device_t.sync_state = 1;
                        value1_state_time = 0;
                    }
                }
            }
        } break;
        case 0x82:
            if (value2_state != PULSE2_STA || value2_state != device_t.value2_state) {
                device_t.sleep_delay = 3;
            }
            if (value2_state != PULSE2_STA) {
                mDelay(20);
                if (value2_state != PULSE2_STA) {
                    value2_state = PULSE2_STA;
                }
            }
            if (over_time < get_syspant_ms()) {
                over_time = get_syspant_ms() + 1000;
                if (value2_state != device_t.value2_state) {
                    if (++value2_state_time >= device_t.stable_time) {
                        device_t.value2_state = value2_state;
                        device_t.sync_state = 1;
                        value2_state_time = 0;
                    }
                }
            }
            break;
        case 0x83: {
            if (value1_state != PULSE1_STA || value1_state != device_t.value1_state || value2_state != PULSE2_STA || value2_state != device_t.value2_state) {
                device_t.sleep_delay = 3;
            }
            if (value1_state != PULSE1_STA) {
                mDelay(20);
                if (value1_state != PULSE1_STA) {
                    value1_state = PULSE1_STA;
                }
            }
            if (value2_state != PULSE2_STA) {
                mDelay(20);
                if (value2_state != PULSE2_STA) {
                    value2_state = PULSE2_STA;
                }
            }

            if (over_time < get_syspant_ms()) {
                over_time = get_syspant_ms() + 1000;
                if (value1_state != device_t.value1_state) {
                    if (++value1_state_time >= device_t.stable_time) {
                        device_t.value1_state = value1_state;
                        device_t.sync_state = 1;
                        value1_state_time = 0;
                    }
                }
                if (value2_state != device_t.value2_state) {
                    if (++value2_state_time >= device_t.stable_time) {
                        device_t.value2_state = value2_state;
                        device_t.sync_state = 1;
                        value2_state_time = 0;
                    }
                }
            }
        } break;
        default:
            break;
        }
    }
}

// 电磁阀控制
void back_1_control(void)
{
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_H;
        break;
    case 1:
        GPIO_9V_PWR_H;
        break;
    case 2:
        GPIO_5V_PWR_H;
        break;
    default:
        break;
    }

    mDelay(100);
    GPIO_BOOST_PWR_H;
    mDelay(3000);

    GPIO_CON1_EN_L;
    GPIO_CON1_A_L;
    GPIO_CON1_B_H;
    mDelay(1000);
    // GPIO_CON1_EN_H;
    GPIO_CON1_A_L;
    GPIO_CON1_B_L;
    GPIO_BOOST_PWR_L;
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_L;
        break;
    case 1:
        GPIO_9V_PWR_L;
        break;
    case 2:
        GPIO_5V_PWR_L;
        break;
    default:
        break;
    }

    if ((device_t.port_mode & 0x01) != 1) {
        device_t.value1_state = 0;
    }
}

void back_2_control(void)
{
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_H;
        break;
    case 1:
        GPIO_9V_PWR_H;
        break;
    case 2:
        GPIO_5V_PWR_H;
        break;
    default:
        break;
    }

    mDelay(100);
    GPIO_BOOST_PWR_H;
    mDelay(3000);

    GPIO_CON2_EN_L;
    GPIO_CON2_A_L;
    GPIO_CON2_B_H;
    mDelay(1000);
    // GPIO_CON2_EN_H;
    GPIO_CON2_A_L;
    GPIO_CON2_B_L;
    GPIO_BOOST_PWR_L;
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_L;
        break;
    case 1:
        GPIO_9V_PWR_L;
        break;
    case 2:
        GPIO_5V_PWR_L;
        break;
    default:
        break;
    }

    if ((device_t.port_mode & 0x02) != 1) {
        device_t.value2_state = 0;
    }
}

void forward_1_control(void)
{
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_H;
        break;
    case 1:
        GPIO_9V_PWR_H;
        break;
    case 2:
        GPIO_5V_PWR_H;
        break;
    default:
        break;
    }

    mDelay(100);
    GPIO_BOOST_PWR_H;
    mDelay(3000);

    GPIO_CON1_EN_L;
    GPIO_CON1_A_H;
    GPIO_CON1_B_L;
    mDelay(1000);
    // GPIO_CON1_EN_H;
    GPIO_CON1_A_L;
    GPIO_CON1_B_L;
    GPIO_BOOST_PWR_L;
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_L;
        break;
    case 1:
        GPIO_9V_PWR_L;
        break;
    case 2:
        GPIO_5V_PWR_L;
        break;
    default:
        break;
    }

    if ((device_t.port_mode & 0x01) != 1) {
        device_t.value1_state = 1;
    }
}

void forward_2_control(void)
{
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_H;
        break;
    case 1:
        GPIO_9V_PWR_H;
        break;
    case 2:
        GPIO_5V_PWR_H;
        break;
    default:
        break;
    }
    mDelay(100);
    GPIO_BOOST_PWR_H;
    mDelay(3000);

    GPIO_CON2_EN_L;
    GPIO_CON2_A_H;
    GPIO_CON2_B_L;
    mDelay(1000);
    // GPIO_CON2_EN_H;
    GPIO_CON2_A_L;
    GPIO_CON2_B_L;
    GPIO_BOOST_PWR_L;
    switch (device_t.vol_control_sw) {
    case 0:
        GPIO_12V_PWR_L;
        break;
    case 1:
        GPIO_9V_PWR_L;
        break;
    case 2:
        GPIO_5V_PWR_L;
        break;
    default:
        break;
    }

    if ((device_t.port_mode & 0x02) != 1) {
        device_t.value2_state = 1;
    }
}
