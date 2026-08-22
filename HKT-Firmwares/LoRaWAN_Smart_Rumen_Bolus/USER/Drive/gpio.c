
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "acc.h"
#include "control_center.h"
#include "multi_button.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

void EXTI4_15_IRQHandler(void)
{
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_7)) {
        /* Clear the EXTI line 0 pending bit */
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_7);
        if (device_t.sleepState) {
            device_t.sleepState = 0;
            device_t.extiWakeEvent = 1;
        }
        device_t.da_int = 1;
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

    LL_GPIO_DeInit(GPIOA);
    LL_GPIO_DeInit(GPIOB);
    LL_GPIO_DeInit(GPIOC);
		
    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

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

    GPIO_InitStruct.Pin = LoRaWAN_NRST_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LoRaWAN_NRST_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_WAKE_PIN;
    LL_GPIO_Init(LoRaWAN_WAKE_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LoRaWAN_IRQ_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(LoRaWAN_IRQ_PORT, &GPIO_InitStruct);
    // LL_GPIO_ResetOutputPin(LoRaWAN_IRQ_PORT, LoRaWAN_IRQ_PIN);

#endif

    // ** 初始化LED IO ** //
    GPIO_InitStruct.Pin = GPIO_LED_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIO_LED_PORT, &GPIO_InitStruct);
    GPIO_LED_H;

    // ** 初始化加速计 IO ** //
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    GPIO_InitStruct.Pin = GPIO_DA_INT_PIN;
    LL_GPIO_Init(GPIO_DA_INT_PORT, &GPIO_InitStruct);

    // ** 初始化加速计中断 IO ** //
    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE7);
    LL_GPIO_SetPinPull(GPIO_DA_INT_PORT, GPIO_DA_INT_PIN, LL_GPIO_PULL_DOWN);
    LL_GPIO_SetPinMode(GPIO_DA_INT_PORT, GPIO_DA_INT_PIN, LL_GPIO_MODE_INPUT);

    // EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_7;
    // EXTI_InitStruct.LineCommand = ENABLE;
    // EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    // EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
    // LL_EXTI_Init(&EXTI_InitStruct);

    // NVIC_SetPriority(EXTI4_15_IRQn, 1);
    // NVIC_EnableIRQ(EXTI4_15_IRQn);
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
