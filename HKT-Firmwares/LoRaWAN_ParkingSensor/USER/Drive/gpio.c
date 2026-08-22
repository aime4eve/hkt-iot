
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "control_center.h"
#include "systick.h"
#include "uart.h"

/**
 * @brief  设备中断事件处理
 * @param
 * @retval
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
        // if (device_t.sleep_state)
        {
            device_t.sleep_state = 0;
            device_t.ble_wake_event = 1;
        }
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_13)) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
        if (device_t.sleep_state) {
            device_t.sleep_state = 0;
            device_t.mag_wake_event = 1;
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
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);


    // // ** FLASH ** //
    // GPIO_InitStruct.Pin = FLASH_HOLD_PIN;
    // GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    // GPIO_InitStruct.Speed = LL_GPIO_SPEED_HIGH;
    // GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    // LL_GPIO_Init(FLASH_HOLD_PORT, &GPIO_InitStruct);
    // FLASH_HOLD_L;

    // ** 蓝牙连接状态 ** //
    GPIO_InitStruct.Pin = BLE_CONNECT_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(BLE_CONNECT_PORT, &GPIO_InitStruct);

    // // ** FLASH ** //
    // GPIO_InitStruct.Pin = FLASH_HOLD_PIN;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    // LL_GPIO_Init(FLASH_HOLD_PIN, &GPIO_InitStruct);

    // ** 雷达模块 ** //
    GPIO_InitStruct.Pin = PWR_CTRL_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(PWR_CTRL_PORT, &GPIO_InitStruct);
    PWR_CTRL_H;

    // ** LED ** //
    GPIO_InitStruct.Pin = LED_RED_PIN | LED_BLUE_PIN;
    LL_GPIO_Init(LED_RED_PORT, &GPIO_InitStruct);

    // ** FLASH ** //
    GPIO_InitStruct.Pin = FLASH_WP_PIN;
    LL_GPIO_Init(FLASH_WP_PORT, &GPIO_InitStruct);

    /** 初始化LoRaWAN 模块IO **/
    GPIO_InitStruct.Pin = LoRaWAN_MODE_PIN;
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

    // ** 光敏电阻 ** //
    GPIO_InitStruct.Pin = LIGHT_PWR_PIN;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LIGHT_PWR_PORT, &GPIO_InitStruct);
    LIGHT_PWR_L;

    // ** 初始化磁力计中断 IO ** //
    //    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13);
    //    LL_GPIO_SetPinPull(QMC_INT_PORT, QMC_INT_PIN, LL_GPIO_PULL_NO);
    //    LL_GPIO_SetPinMode(QMC_INT_PORT, QMC_INT_PIN, LL_GPIO_MODE_INPUT);

    //    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_13;
    //    EXTI_InitStruct.LineCommand = ENABLE;
    //    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    //    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    //    LL_EXTI_Init(&EXTI_InitStruct);

    // // ** 初始化磁敏开关中断 IO ** //
    // LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
    // LL_GPIO_SetPinPull(VCS_INT_PORT, VCS_INT_PIN, LL_GPIO_PULL_NO);
    // LL_GPIO_SetPinMode(VCS_INT_PORT, VCS_INT_PIN, LL_GPIO_MODE_INPUT);

    // EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
    // EXTI_InitStruct.LineCommand = ENABLE;
    // EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    // EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    // LL_EXTI_Init(&EXTI_InitStruct);

    // ** 蓝牙连接状态中断 IO ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE2);
    LL_GPIO_SetPinPull(BLE_CONNECT_PORT, BLE_CONNECT_PIN, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinMode(BLE_CONNECT_PORT, BLE_CONNECT_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_2;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /* EXTI interrupt init*/
    // NVIC_SetPriority(EXTI0_IRQn, 0);
    // NVIC_EnableIRQ(EXTI0_IRQn);

    NVIC_SetPriority(EXTI2_IRQn, 0);
    NVIC_EnableIRQ(EXTI2_IRQn);

    // NVIC_SetPriority(EXTI15_10_IRQn, 0);
    // NVIC_EnableIRQ(EXTI15_10_IRQn);
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

    LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOA |
                              LL_AHB2_GRP1_PERIPH_GPIOB |
                              LL_AHB2_GRP1_PERIPH_GPIOC |
                              LL_AHB2_GRP1_PERIPH_GPIOD |
                              LL_AHB2_GRP1_PERIPH_GPIOH);
}
