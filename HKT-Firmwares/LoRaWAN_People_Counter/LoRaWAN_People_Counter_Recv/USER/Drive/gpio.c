
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "control_center.h"

/**
 * @brief  This function handles External line 0 to 1 interrupt request.
 * @param  None
 * @retval None
 */
void EXTI0_1_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_0)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0); // 触发干簧管
        if (!factoryMode) {
            device_t.mode = ALIGNMENT_MODE;
            device_t.mode_timeout = SEC_DELAY * 60; // 维持一分钟的对准模式
        }
    }
}

void EXTI4_15_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_6)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);
        if (device_t.counter_a_sense_count < 1000)
            device_t.counter_a_sense_count++;
    }
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_7)) {
        /* Clear the EXTI line 1 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_7);
        if (device_t.counter_b_sense_count < 1000)
            device_t.counter_b_sense_count++;
    }
}

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_GPIO_DeInit(GPIOA);
    LL_GPIO_DeInit(GPIOB);
    LL_GPIO_DeInit(GPIOC);

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    /** 初始化LED IO **/
    LL_GPIO_ResetOutputPin(GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN);
    LL_GPIO_ResetOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN);
    LL_GPIO_ResetOutputPin(GPIO_LED_ORANGE_PORT, GPIO_LED_ORANGE_PIN);
    LL_GPIO_ResetOutputPin(GPIO_LED_RED_PORT, GPIO_LED_RED_PIN);

    GPIO_InitStruct.Pin = GPIO_LED_GREEN_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIO_LED_GREEN_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_LED_BLUE_PIN;
    LL_GPIO_Init(GPIO_LED_BLUE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_LED_ORANGE_PIN;
    LL_GPIO_Init(GPIO_LED_ORANGE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_LED_RED_PIN;
    LL_GPIO_Init(GPIO_LED_RED_PORT, &GPIO_InitStruct);

    /** 初始化LoRaWAN 模块IO **/
    LL_GPIO_SetOutputPin(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);

    GPIO_InitStruct.Pin = LoRaWAN_MODE_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
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

    // ** 初始化干簧管中断 IO ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
    LL_GPIO_SetPinPull(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
    LL_EXTI_Init(&EXTI_InitStruct);

    // ** 初始化红外接收中断 IO ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE6);
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE7);

    LL_GPIO_SetPinPull(GPIO_INFRARED_A_PORT, GPIO_INFRARED_A_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinPull(GPIO_INFRARED_B_PORT, GPIO_INFRARED_B_PIN, LL_GPIO_PULL_UP);

    LL_GPIO_SetPinMode(GPIO_INFRARED_A_PORT, GPIO_INFRARED_A_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinMode(GPIO_INFRARED_B_PORT, GPIO_INFRARED_B_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_6;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
    LL_EXTI_Init(&EXTI_InitStruct);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_7;
    LL_EXTI_Init(&EXTI_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI4_15_IRQn, 0);
    // NVIC_EnableIRQ(EXTI4_15_IRQn);
    NVIC_DisableIRQ(EXTI4_15_IRQn);
    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI0_1_IRQn, 0);
    NVIC_EnableIRQ(EXTI0_1_IRQn);
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_PinState bitstatus;

    /* Check the parameters */
    assert_param(IS_GPIO_PIN_AVAILABLE(GPIOx, GPIO_Pin));

    if ((GPIOx->IDR & GPIO_Pin) != (uint32_t)GPIO_PIN_RESET) {
        bitstatus = GPIO_PIN_SET;
    } else {
        bitstatus = GPIO_PIN_RESET;
    }
    return bitstatus;
}

/**
 * @brief  获取门磁触发状态
 * @param
 * @retval 门磁触发状态
 */
int getDoorSensorState(void)
{
    if (LL_GPIO_ReadInputPin(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN) != GPIO_PIN_STATUS_RESET)
        return GPIO_PIN_STATUS_SET;
    else
        return GPIO_PIN_STATUS_RESET;
}

/**
 * @brief  获取计数器A状态
 * @param
 * @retval 计数器A状态
 */
int getPeopleCounterAState(void)
{
    if (LL_GPIO_ReadInputPin(GPIO_INFRARED_A_PORT, GPIO_INFRARED_A_PIN) != GPIO_PIN_STATUS_RESET)
        return GPIO_PIN_STATUS_SET;
    else
        return GPIO_PIN_STATUS_RESET;
}

/**
 * @brief  获取计数器B状态
 * @param
 * @retval 计数器A状态
 */
int getPeopleCounterBState(void)
{
    if (LL_GPIO_ReadInputPin(GPIO_INFRARED_B_PORT, GPIO_INFRARED_B_PIN) != GPIO_PIN_STATUS_RESET)
        return GPIO_PIN_STATUS_SET;
    else
        return GPIO_PIN_STATUS_RESET;
}

// 读取输入IO状态
u32 LL_GPIO_ReadInputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    return (READ_BIT(GPIOx->IDR, PinMask));
}

// 读取输出IO状态
u32 LL_GPIO_ReadOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
    return (READ_BIT(GPIOx->ODR, PinMask));
}

// 设置IO状态
void LL_GPIO_WriteOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t PinValue)
{
    if (PinValue != 0) {
        WRITE_REG(GPIOx->BSRR, PinMask);
    } else {
        WRITE_REG(GPIOx->BRR, PinMask);
    }
}
