
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
#if FUNC_BOARD_VERSION_CL
	
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

//    LL_GPIO_DeInit(GPIOA);
//    LL_GPIO_DeInit(GPIOB);
//    LL_GPIO_DeInit(GPIOC);

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

		
#else

	/* enable the clock */
	rcu_periph_clock_enable(RCU_GPIOB);

	/** 初始化LED IO **/
	gpio_init(GPIO_LED1_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_LED1_PIN);
	gpio_init(GPIO_LED2_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_LED2_PIN);

	gpio_bit_write(GPIO_LED1_PORT, GPIO_LED1_PIN, RESET);
	gpio_bit_write(GPIO_LED2_PORT, GPIO_LED2_PIN, RESET);

#if FUNC_LORA_VERSION_LIR
	/** 初始化LoRaWAN 模块IO **/
	gpio_init(LoRaWAN_MODE_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LoRaWAN_MODE_PIN);
	gpio_init(LoRaWAN_NRST_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LoRaWAN_NRST_PIN);
	gpio_init(LoRaWAN_WAKE_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LoRaWAN_WAKE_PIN);

	gpio_bit_write(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, RESET);
	gpio_bit_write(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, RESET);
	gpio_bit_write(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN, RESET);

	gpio_init(LoRaWAN_BUSY_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, LoRaWAN_BUSY_PIN);
	gpio_init(LoRaWAN_STAT_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, LoRaWAN_STAT_PIN);
#else
    /** 初始化LoRaWAN 模块IO **/
	gpio_init(LoRaWAN_WAKE_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LoRaWAN_WAKE_PIN);
	gpio_init(LoRaWAN_NRST_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LoRaWAN_NRST_PIN);
	gpio_bit_write(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, SET);
	gpio_init(LoRaWAN_IRQ_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, LoRaWAN_IRQ_PIN);

#endif

#endif
}


