
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "systick.h"
#include "uart.h"

/**
 * @brief   This function handles Event Wake Up interrupt.
 * @retval  None
 */
void EVWUP_IRQHandler(void)
{
    if (EXTI_GetWakeupFlagStatus(EXTI_CHANNEL_1)) {
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_1);
        device_t.aue_event = 1;
    }
    if (EXTI_GetWakeupFlagStatus(EXTI_CHANNEL_6)) {
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_6);
        device_t.dat2_event = 1;
    }
    if (EXTI_GetWakeupFlagStatus(EXTI_CHANNEL_7)) {
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_7);
        device_t.dat1_event = 1;
    }
    if (EXTI_GetWakeupFlagStatus(EXTI_CHANNEL_0)) {
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_0);
        device_t.tamper_event = 1;
    }
    EXTI_WakeupEventIntConfig(DISABLE);
}

void EXTI0_1_IRQHandler(void)
{
    if (EXTI_GetEdgeFlag(EXTI_CHANNEL_1)) {
        EXTI_ClearEdgeFlag(EXTI_CHANNEL_1);
        device_t.aue_event = 1;
    }

    if (EXTI_GetEdgeFlag(EXTI_CHANNEL_0)) {
        EXTI_ClearEdgeFlag(EXTI_CHANNEL_0);
        device_t.tamper_event = 1;
    }
}

void EXTI4_15_IRQHandler(void)
{
    if (EXTI_GetEdgeFlag(EXTI_CHANNEL_6)) {
        EXTI_ClearEdgeFlag(EXTI_CHANNEL_6);
        device_t.dat1_event = 1;
    }
    if (EXTI_GetEdgeFlag(EXTI_CHANNEL_7)) {
        EXTI_ClearEdgeFlag(EXTI_CHANNEL_7);
        device_t.dat2_event = 1;
    }
}

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
    /* enable the clock */
    CKCU_PeripClockConfig_TypeDef CKCUClock = {0};
    EXTI_InitTypeDef EXTI_InitStruct = {0};

    CKCUClock.Bit.AFIO = 1;
    CKCUClock.Bit.PA = 1;
    CKCUClock.Bit.PB = 1;
#if FUNC_TAMPER_ENABLE
    CKCUClock.Bit.PC = 1;
#endif
    CKCUClock.Bit.EXTI = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    // AD PWR
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_6, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(GPIO_AD_PWR_PORT, GPIO_AD_PWR_PIN, GPIO_PR_DISABLE);
    GPIO_WriteOutBits(GPIO_AD_PWR_PORT, GPIO_AD_PWR_PIN, RESET);
    GPIO_DirectionConfig(GPIO_AD_PWR_PORT, GPIO_AD_PWR_PIN, GPIO_DIR_OUT);

    // DAT1
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_7, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(GPIO_DAT1_PORT, GPIO_DAT1_PIN, GPIO_PR_DISABLE);
    GPIO_DirectionConfig(GPIO_DAT1_PORT, GPIO_DAT1_PIN, GPIO_DIR_IN);
    GPIO_InputConfig(GPIO_DAT1_PORT, GPIO_DAT1_PIN, ENABLE);

    // DAT2
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_6, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(GPIO_DAT2_PORT, GPIO_DAT2_PIN, GPIO_PR_DISABLE);
    GPIO_DirectionConfig(GPIO_DAT2_PORT, GPIO_DAT2_PIN, GPIO_DIR_IN);
    GPIO_InputConfig(GPIO_DAT2_PORT, GPIO_DAT2_PIN, ENABLE);

    // AUE
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_1, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(GPIO_AUE_PORT, GPIO_AUE_PIN, GPIO_PR_DISABLE);
    GPIO_DirectionConfig(GPIO_AUE_PORT, GPIO_AUE_PIN, GPIO_DIR_IN);
    GPIO_InputConfig(GPIO_AUE_PORT, GPIO_AUE_PIN, ENABLE);

#if FUNC_TAMPER_ENABLE
    // TAMPER
    AFIO_GPxConfig(GPIO_PC, AFIO_PIN_0, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN, GPIO_PR_DISABLE);
    GPIO_DirectionConfig(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN, GPIO_DIR_IN);
    GPIO_InputConfig(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN, ENABLE);
#endif

    delay_1ms(1000); // 等待1秒状态稳定

    /* Select Port as EXTI Trigger Source                                                                     */
    AFIO_EXTISourceConfig(AFIO_EXTI_CH_7, AFIO_ESS_PA);
    EXTI_InitStruct.EXTI_Channel = EXTI_CHANNEL_7;
    EXTI_InitStruct.EXTI_Debounce = EXTI_DEBOUNCE_ENABLE;
    EXTI_InitStruct.EXTI_DebounceCnt = 3;
    EXTI_InitStruct.EXTI_IntType = EXTI_BOTH_EDGE;
    EXTI_Init(&EXTI_InitStruct);
    EXTI_IntConfig(EXTI_CHANNEL_7, ENABLE);

    /* Select Port as EXTI Trigger Source                                                                     */
    AFIO_EXTISourceConfig(AFIO_EXTI_CH_6, AFIO_ESS_PA);
    EXTI_InitStruct.EXTI_Channel = EXTI_CHANNEL_6;
    EXTI_InitStruct.EXTI_Debounce = EXTI_DEBOUNCE_ENABLE;
    EXTI_InitStruct.EXTI_DebounceCnt = 3;
    EXTI_InitStruct.EXTI_IntType = EXTI_POSITIVE_EDGE;
    EXTI_Init(&EXTI_InitStruct);
    /* Enable EXTI & NVIC line Interrupt                                                                      */
    EXTI_IntConfig(EXTI_CHANNEL_6, ENABLE);

    /* Select Port as EXTI Trigger Source                                                                     */
    AFIO_EXTISourceConfig(AFIO_EXTI_CH_1, AFIO_ESS_PA);
    EXTI_InitStruct.EXTI_Channel = EXTI_CHANNEL_1;
    EXTI_InitStruct.EXTI_Debounce = EXTI_DEBOUNCE_ENABLE;
    EXTI_InitStruct.EXTI_DebounceCnt = 3;
    EXTI_InitStruct.EXTI_IntType = EXTI_BOTH_EDGE;
    EXTI_Init(&EXTI_InitStruct);
    /* Enable EXTI & NVIC line Interrupt                                                                      */
    EXTI_IntConfig(EXTI_CHANNEL_1, ENABLE);

#if FUNC_TAMPER_ENABLE
    /* Select Port as EXTI Trigger Source                                                                     */
    AFIO_EXTISourceConfig(AFIO_EXTI_CH_0, AFIO_ESS_PC);
    EXTI_InitStruct.EXTI_Channel = EXTI_CHANNEL_0;
    EXTI_InitStruct.EXTI_Debounce = EXTI_DEBOUNCE_ENABLE;
    EXTI_InitStruct.EXTI_DebounceCnt = 3;
    EXTI_InitStruct.EXTI_IntType = EXTI_BOTH_EDGE;
    EXTI_Init(&EXTI_InitStruct);
    /* Enable EXTI & NVIC line Interrupt                                                                      */
    EXTI_IntConfig(EXTI_CHANNEL_0, ENABLE);
#endif

    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_EnableIRQ(EXTI4_15_IRQn);

    AFIO_EXTISourceConfig(AFIO_EXTI_CH_7, AFIO_ESS_PA);
    AFIO_EXTISourceConfig(AFIO_EXTI_CH_6, AFIO_ESS_PA);
    AFIO_EXTISourceConfig(AFIO_EXTI_CH_1, AFIO_ESS_PA);

#if FUNC_TAMPER_ENABLE
    AFIO_EXTISourceConfig(AFIO_EXTI_CH_0, AFIO_ESS_PC);
#endif

    NVIC_SetPriority(EVWUP_IRQn, 0xF);
    NVIC_EnableIRQ(EVWUP_IRQn);
    EXTI_WakeupEventIntConfig(ENABLE);

    // LoRaWAN GPIO初始化
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_4, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, GPIO_PR_DISABLE);
    GPIO_WriteOutBits(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, RESET);
    GPIO_DirectionConfig(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN, GPIO_DIR_OUT);

    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_14, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, GPIO_PR_DISABLE);
    GPIO_WriteOutBits(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, RESET);
    GPIO_DirectionConfig(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN, GPIO_DIR_OUT);

    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_15, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN, GPIO_PR_DISABLE);
    GPIO_WriteOutBits(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN, RESET);
    GPIO_DirectionConfig(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN, GPIO_DIR_OUT);

    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_3, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(LoRaWAN_BUSY_PORT, LoRaWAN_BUSY_PIN, GPIO_PR_DISABLE);
    GPIO_DirectionConfig(LoRaWAN_BUSY_PORT, LoRaWAN_BUSY_PIN, GPIO_DIR_IN);
    GPIO_InputConfig(LoRaWAN_BUSY_PORT, LoRaWAN_BUSY_PIN, ENABLE);
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_2, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(LoRaWAN_STAT_PORT, LoRaWAN_STAT_PIN, GPIO_PR_DISABLE);
    GPIO_DirectionConfig(LoRaWAN_STAT_PORT, LoRaWAN_STAT_PIN, GPIO_DIR_IN);
    GPIO_InputConfig(LoRaWAN_STAT_PORT, LoRaWAN_STAT_PIN, ENABLE);
}

/**
 * @brief  GPIO 周期事件，用于检测按键、烟雾报警、高温报警、故障状态
 * @param
 * @retval
 */
void Gpio_Event(void)
{
    // 报警触发事件
    if (device_t.dat1_event || device_t.aue_event) {
        device_t.dat1_event = 0;
        device_t.aue_event = 0;
        if (READ_DAT1_STATE && !READ_AUE_STATE) {
            if (!device_t.alarm_state) {
                device_t.alarm_state = 1;
                device_t.sync_state = 1;
                DEBUG_TRACE(WARN_TAG, "Smoke Alarm!!!!");
            }
        } else if (!READ_DAT1_STATE && READ_AUE_STATE) {
            if (device_t.alarm_state) {
                device_t.alarm_state = 0;
                device_t.sync_state = 1;
                DEBUG_TRACE(WARN_TAG, "Smoke DisAlarm!!!!");
            }
        }
    }

    // 防拆报警
    if (device_t.tamper_event) {
        device_t.tamper_event = 0;
        if (READ_TAMPER_STATE) {
            if (device_t.tamper_state) {
                device_t.tamper_state = 0;
                device_t.sync_state = 1;
                DEBUG_TRACE(WARN_TAG, "Smoke DisTamper!!!!");
            }
        } else {
            if (!device_t.tamper_state) {
                device_t.tamper_state = 1;
                device_t.sync_state = 1;
                DEBUG_TRACE(WARN_TAG, "Smoke Tamper!!!!");
            }
        }
    }

    // 电量信息
    if (device_t.dat2_event) {
        device_t.dat2_event = 0;
        if (READ_DAT2_STATE && device_t.battery_state) {
            device_t.battery_state = 0;
            device_t.sync_state = 1;
            DEBUG_TRACE(WARN_TAG, "Smoke Battery Low!!!!");
        } else if (!READ_DAT2_STATE && !device_t.battery_state) {
            device_t.battery_state = 1;
            device_t.sync_state = 1;
            DEBUG_TRACE(WARN_TAG, "Smoke Battery Normal!!!!");
        }
    }
}
