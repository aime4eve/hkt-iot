
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "acc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "lis3dh.h"
#include "lsm6ds3_reg.h"
#include "multi_button.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

static struct button button_1;
u8                   repeat_cnt;
u8                   update_pwr;
time_t               repeat_stamp;
u8                   recvStat;

uint8_t button_read_pin_status(void)
{
    return GPIO_KEY_STA;
}

void BUTTON1_SINGLE_CLICK_Handler(void)
{
    device_t.press_short_event = 1;
    if (device_t.power_on) {
        ledBlinkTime = SEC_DELAY / 2;
    }
    if (repeat_cnt++ == 0)
        repeat_stamp = Timestamp + 10;

    if (Timestamp > repeat_stamp && repeat_cnt) {
        repeat_cnt = 0;
    } else if (device_t.power_on) {
        if (repeat_cnt >= 7) {
            repeat_cnt                 = 0;
            device_t.wait_default_mode = 1;
        }
    }
}

void BUTTON1_DOUBLE_CLICK_Handler(void)
{
    device_t.press_double_event = 1;
}

void BUTTON1_LONG_RRESS_START_Handler(void)
{
    if (device_t.power_on) {
        device_t.power_on = 0;
        LoRaWAN.joinState = 0;
        ledBlinkTime      = 1 * SEC_DELAY;
    } else {
        device_t.power_on = 1;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
        ledBlinkTime      = 3 * SEC_DELAY;
    }
    update_pwr = 1;
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
            device_t.sleep_state    = 0;
            device_t.key_wake_event = 1;
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

void EXTI9_5_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_5)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_5);
        // if (device_t.sleep_state) {
        //     device_t.sleep_state = 0;
        //     device_t.acc_wake_event = 1;
        // }
        device_t.acc_wake_event = 1;
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
    // ** GPS ** //
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin        = GPIO_GPS_PWR_CTRL_PIN;
    LL_GPIO_Init(GPIO_GPS_PWR_CTRL_PORT, &GPIO_InitStruct);
    GPIO_GPS_PWR_CTRL_L;

    GPIO_InitStruct.Pin = GPIO_GPS_TXD_PIN | GPIO_GPS_RXD_PIN;
    LL_GPIO_Init(GPIO_GPS_TXD_PORT, &GPIO_InitStruct);
    GPIO_GPS_TXD_L;
    GPIO_GPS_RXD_L;

    GPIO_InitStruct.Pin = GPIO_GPS_RESET_CTRL_PIN;
    LL_GPIO_Init(GPIO_GPS_RESET_CTRL_PORT, &GPIO_InitStruct);
    GPIO_GPS_RESET_CTRL_L;

    // ** 电压检测 ** //
    GPIO_InitStruct.Pin = GPIO_VOL_CTRL_PIN;
    LL_GPIO_Init(GPIO_VOL_CTRL_PORT, &GPIO_InitStruct);
    GPIO_VOL_CTRL_L;

    // ** LED ** //
    GPIO_InitStruct.Pin = GPIO_LED_PIN;
    LL_GPIO_Init(GPIO_LED_PORT, &GPIO_InitStruct);
    GPIO_LED_L;

    // ** FLASH ** //
    GPIO_InitStruct.Pin = FLASH_WP_PIN;
    LL_GPIO_Init(FLASH_WP_PORT, &GPIO_InitStruct);
    FLASH_WP_H;

    // ** 初始化输入 IO ** //
    // ** 按键 ** //
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Pin  = GPIO_KEY_PIN;
    LL_GPIO_Init(GPIO_KEY_PORT, &GPIO_InitStruct);

    // ** ACC ** //
    GPIO_InitStruct.Pin  = GPIO_LIS3D_INT_PIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIO_LIS3D_INT_PORT, &GPIO_InitStruct);

    // ** 蓝牙连接状态 ** //
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    GPIO_InitStruct.Pin  = BLE_CONNECT_PIN;
    LL_GPIO_Init(BLE_CONNECT_PORT, &GPIO_InitStruct);

    /** 初始化LoRaWAN 模块IO **/
    LL_GPIO_SetOutputPin(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);

    GPIO_InitStruct.Pin        = LoRaWAN_MODE_PIN;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LoRaWAN_MODE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_NRST_PIN;
    LL_GPIO_Init(LoRaWAN_NRST_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_WAKE_PIN;
    LL_GPIO_Init(LoRaWAN_WAKE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = LoRaWAN_BUSY_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LoRaWAN_BUSY_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_STAT_PIN;
    LL_GPIO_Init(LoRaWAN_STAT_PORT, &GPIO_InitStruct);

    // ** 初始化IO中断 ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
    LL_GPIO_SetPinPull(GPIO_KEY_PORT, GPIO_KEY_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIO_KEY_PORT, GPIO_KEY_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_0;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE5);
    LL_GPIO_SetPinPull(GPIO_LIS3D_INT_PORT, GPIO_LIS3D_INT_PIN, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIO_LIS3D_INT_PORT, GPIO_LIS3D_INT_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_5;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING;
    LL_EXTI_Init(&EXTI_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE2);
    LL_GPIO_SetPinPull(BLE_CONNECT_PORT, BLE_CONNECT_PIN, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinMode(BLE_CONNECT_PORT, BLE_CONNECT_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_2;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_RISING_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI0_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_IRQn);

    NVIC_SetPriority(EXTI2_IRQn, 0);
    NVIC_EnableIRQ(EXTI2_IRQn);

    NVIC_SetPriority(EXTI9_5_IRQn, 0);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    button_init(&button_1, button_read_pin_status, 0);
    button_attach(&button_1, SINGLE_CLICK, (BtnCallback)BUTTON1_SINGLE_CLICK_Handler);
    button_attach(&button_1, DOUBLE_CLICK, (BtnCallback)BUTTON1_DOUBLE_CLICK_Handler);
    button_attach(&button_1, LONG_RRESS_START, (BtnCallback)BUTTON1_LONG_RRESS_START_Handler);
    button_start(&button_1);
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

    GPIO_InitStruct.Pin = LL_GPIO_PIN_13 | LL_GPIO_PIN_0 | LL_GPIO_PIN_3;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13 | LL_GPIO_PIN_0 | LL_GPIO_PIN_3);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_1 | LL_GPIO_PIN_3 | LL_GPIO_PIN_5;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1 | LL_GPIO_PIN_3 | LL_GPIO_PIN_5);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    LL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_2);

    LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_GPIOA |
                              LL_AHB2_GRP1_PERIPH_GPIOB |
                              LL_AHB2_GRP1_PERIPH_GPIOC |
                              LL_AHB2_GRP1_PERIPH_GPIOD |
                              LL_AHB2_GRP1_PERIPH_GPIOH);
    IO_Uart_Deinit();
}

void GpioEventProcess(void)
{
    if (!device_t.wait_default_mode) {
        if (device_t.press_short_event) {
            device_t.press_short_event = 0;
            INFO("\nButton Press Short Handler\n");
            ledBlinkTime = SEC_DELAY;
        }

        // 双击主动同步数据
        if (device_t.press_double_event) {
            device_t.press_double_event = 0;
            INFO("\nButton Press Double Handler\n");
            device_t.sync_state = LoRaWAN.joinState ? 1 : 0;
        }
    } else {
        device_t.wait_default_mode = 0;
        FLASH_Reset_Param();
    }

    if (device_t.acc_wake_event) {
        device_t.acc_wake_event = 0;
        // lis3dh_int_data_source_t data_src = {0};
        // lis3dh_int_event_source_t event_src = {0};
        // // lis3dh_int_click_source_t click_src = {0};
        // // get the source of the interrupt and reset *INTx* signals
        // lis3dh_get_int_data_source(acc_sensor, &data_src);
        // lis3dh_get_int_event_source(acc_sensor, &event_src, lis3dh_int_event1_gen);
        INFO("\nAcc Int Event\n");
    }

    if (update_pwr) {
        update_pwr = 0;
        FLASH_Write_Device_Param();
    }
}

void IO_Uart_Init(void)
{
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin        = GPIO_GPS_TXD_PIN;
    LL_GPIO_Init(GPIO_GPS_TXD_PORT, &GPIO_InitStruct);
    GPIO_GPS_TXD_L;

    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Pin  = GPIO_GPS_RXD_PIN;
    LL_GPIO_Init(GPIO_GPS_RXD_PORT, &GPIO_InitStruct);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE7);
    LL_GPIO_SetPinPull(GPIO_GPS_RXD_PORT, GPIO_GPS_RXD_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIO_GPS_RXD_PORT, GPIO_GPS_RXD_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31   = LL_EXTI_LINE_7;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode        = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger     = LL_EXTI_TRIGGER_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    NVIC_SetPriority(EXTI9_5_IRQn, 0);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    recvStat = COM_STOP_BIT;
}

void IO_Uart_Deinit(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin        = GPIO_GPS_TXD_PIN | GPIO_GPS_RXD_PIN;
    LL_GPIO_Init(GPIO_GPS_TXD_PORT, &GPIO_InitStruct);
    GPIO_GPS_TXD_L;
    GPIO_GPS_RXD_L;
    LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_6); // 关闭下降沿中断
}
