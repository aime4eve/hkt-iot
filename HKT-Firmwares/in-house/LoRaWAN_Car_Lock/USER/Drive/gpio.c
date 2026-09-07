
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"

/**
 * @brief  This function handles External line 0 to 1 interrupt request.
 * @param  None
 * @retval None
 */
void EXTI0_1_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_0)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
        if (device_t.sleepState) {
            device_t.extiWakeEvent = 1;
        }
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }
}

void EXTI4_15_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);
        if (device_t.sleepState) {
            device_t.loraWakeEvent = 1;
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
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    LL_GPIO_DeInit(GPIOA);
    LL_GPIO_DeInit(GPIOB);
    LL_GPIO_DeInit(GPIOC);

    // ** 初始化LED IO ** //
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pin = GPIO_LED_PIN;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_LED1_PIN;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_LED2_PIN;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_LED_H;
    GPIO_LED1_L;
    GPIO_LED2_L;

    /** 初始化LoRaWAN 模块IO **/
    GPIO_InitStruct.Pin = LoRaWAN_NRST_PIN;
    LL_GPIO_Init(LoRaWAN_NRST_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_WAKE_PIN;
    LL_GPIO_Init(LoRaWAN_WAKE_PORT, &GPIO_InitStruct);

    LL_GPIO_SetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
    LL_GPIO_SetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);

    GPIO_InitStruct.Pin = LoRaWAN_IRQ_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(LoRaWAN_IRQ_PORT, &GPIO_InitStruct);

    // ** 初始化KEY IO ** //
    GPIO_InitStruct.Pin = GPIO_KEY_PIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIO_KEY_PORT, &GPIO_InitStruct);

    // ** 初始化中断 IO ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE12);
    LL_GPIO_SetPinPull(LoRaWAN_IRQ_PORT, LoRaWAN_IRQ_PIN, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinMode(LoRaWAN_IRQ_PORT, LoRaWAN_IRQ_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_12;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
    LL_EXTI_Init(&EXTI_InitStruct);

    // ** 初始化按键中断 IO ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
    LL_GPIO_SetPinPull(GPIO_KEY_PORT, GPIO_KEY_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIO_KEY_PORT, GPIO_KEY_PIN, LL_GPIO_MODE_INPUT);

    EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
    EXTI_InitStruct.LineCommand = ENABLE;
    EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
    LL_EXTI_Init(&EXTI_InitStruct);

    /* EXTI interrupt init*/
    NVIC_SetPriority(EXTI0_1_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_1_IRQn);

    NVIC_SetPriority(EXTI4_15_IRQn, 1);
    NVIC_EnableIRQ(EXTI4_15_IRQn);
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