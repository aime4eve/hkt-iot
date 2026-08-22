/**
 ******************************************************************************
 * @file     Project/STM8L10x_StdPeriph_Templates/main.c
 * @author   MCD Application Team
 * @version V1.2.1
 * @date    30-September-2014
 * @brief    This file contains the firmware main function.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm8l10x_conf.h"

/** @addtogroup STM8L10x_StdPeriph_Templates
 * @{
 */

uint32_t LsiFreq;
void Delay(uint16_t nCount);

#ifdef _COSMIC_
#define ASM _asm
#endif
#ifdef _IAR_
#define ASM asm
#endif
/* This delay should be added just after reset to have access to SWIM pin
 and to be able to reprogram the device after power on (otherwise the
 device will be locked) */
#define STARTUP_SWIM_DELAY_5S     \
    {                             \
        ASM(" PUSHW X \n"         \
            " PUSH A \n"          \
            " LDW X, #0xFFFF \n"  \
            "loop1: LD A, #50 \n" \
                                  \
            "loop2: DEC A \n"     \
            " JRNE loop2 \n"      \
                                  \
            " DECW X \n"          \
            " JRNE loop1 \n"      \
                                  \
            " POP A \n"           \
            " POPW X ");          \
    }

/* not connected pins as output low state (the best EMC immunity)
(PA1, PA3, PA5, PB0, PB1, PB2, PB4, PC5, PC6, PD1, PD2, PD3, PD4, PD5,
 PD6, PD7)*/
#define CONFIG_UNUSED_PINS_STM8L001                                       \
    {                                                                     \
        GPIOA->DDR |= GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_5;               \
        GPIOB->DDR |= GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_4;  \
        GPIOC->DDR |= GPIO_Pin_5 | GPIO_Pin_6;                            \
        GPIOD->DDR |= GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | \
                      GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;               \
    }

#define AWU_MAX_INTERNVAL_COEFFICIENT ((uint32_t)3932160)
#define AWU_APR_MAX_VALUE ((uint8_t)64)
#define AWU_TBR_MAX_VALUE ((uint8_t)0x0f)
#define AWU_APR_MIN_VALUE ((uint8_t)2)
#define AWU_TBR_MIN_VALUE ((uint8_t)0x01)
#define AWU_HIGH_RESOLUTION_THRESHOLD ((uint32_t)6889)

void delay_us(int n)
{
    for (; n > 0; n--)
    {
        asm("nop"); // 在STM8里面，16M晶振，_nop_() 延时了 333ns
        asm("nop");
        asm("nop");
        asm("nop");
    }
}

//---- 毫秒级延时程序-----------------------
void delay_ms(unsigned int time)
{
    unsigned int i;
    while (time--)
        for (i = 900; i > 0; i--)
            delay_us(1);
}

/**
  * @brief  Update APR register with the measured LSI frequency.
            Accuracy is much better than AWU_LSICalibrationConfig().
  * @param  LSIFreqHz -- the LSI frequency, in Hertz.
            internval -- AWU wake up interval, in milliseconds
  * @note   AWU must be disabled to avoid unwanted interrupts.
  * @retval None
  */
ErrorStatus AWU_ConfigLSI(uint32_t LSIFreqHz, uint32_t internval)
{
    uint32_t tmp = 0, z = 0;
    uint8_t y = 0, x = 0;
    uint8_t flag = 0;

    /* Check parameter */
    assert_param(IS_LSI_FREQUENCY(LSIFreqHz));

    z = LSIFreqHz * internval;

    if (internval > AWU_HIGH_RESOLUTION_THRESHOLD)
    {
        tmp = z / 10240000;
        if (tmp >= AWU_APR_MIN_VALUE && tmp <= AWU_APR_MAX_VALUE)
        {
            AWU->TBR |= 0x0e;
            AWU->APR = (tmp)-2;
            return SUCCESS;
        }

        tmp = z / 61440000;
        if (tmp >= AWU_APR_MIN_VALUE && tmp <= AWU_APR_MAX_VALUE)
        {
            AWU->TBR |= 0x0f;
            AWU->APR = (tmp)-2;
            return SUCCESS;
        }
    }

    /* 2^x*y = LSIFreqHz * internval */
    for (y = 64; y > 1; y = y >> 1)
    {
        tmp = z / ((uint32_t)y * 1000);
        if (tmp >= 1 && tmp <= 4096) /*value is between 2^0 and 2^12*/
        {
            flag = 1;
            break;
        }
    }

    /*计算TBR，再根据TBR推导出APR*/
    if (flag != 0)
    {
        for (x = 0; x < 13; x++)
        {
            if ((tmp >> x) == 0)
            {
                break;
            }
        }
        tmp = (uint32_t)1 << x;
        y = z / ((uint32_t)tmp * 1000);
        if (y < 2)
        {
            return ERROR;
        }
        AWU->TBR = x + 1;
        AWU->APR = y - 2;
        return SUCCESS;
    }
    else
    {
        return ERROR;
    }
}

/**
 * @brief Configurate the Clock & enable the clock of TIM2 and TIM3.
 * @param[in] None
 * @retval  None
 */
void CLK_Config(void)
{
    /* Clock Master = 16 Mhz */
    CLK_MasterPrescalerConfig(CLK_MasterPrescaler_HSIDiv1);
    /* Enable TIM2 clock */
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);

    CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_I2C, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_SPI, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_USART, DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_AWU, ENABLE);

    AWU->CSR |= AWU_CSR_MSR;
    LsiFreq = TIM2_ComputeLsiClockFreq(CLK_GetClockFreq());
    /* Disable the LSI measurement: LSI clock disconnected from timer Input Capture 1 */
    AWU->CSR &= (uint8_t)(~AWU_CSR_MSR);

    // AWU_LSICalibrationConfig(LsiFreq);
    // AWU_Init(AWU_Timebase_2ms);

    AWU_ConfigLSI(LsiFreq, 2);

    AWU_Cmd(ENABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, DISABLE);
}

void main(void)
{
    CLK_Config();
    // delay_ms(2000);
    /* Initialize I/Os in Output Mode */

    CONFIG_UNUSED_PINS_STM8L001;
    GPIO_Init(GPIOB, GPIO_Pin_6, GPIO_Mode_Out_PP_Low_Fast);
    GPIO_SetBits(GPIOB, GPIO_Pin_6);
    delay_ms(1000);
    GPIO_ResetBits(GPIOB, GPIO_Pin_6);

    GPIO_Init(GPIOA, GPIO_Pin_0, GPIO_Mode_Out_PP_Low_Fast);
    GPIO_ResetBits(GPIOA, GPIO_Pin_0);
#if 1
    GPIO_Init(GPIOB, GPIO_Pin_3, GPIO_Mode_Out_PP_Low_Fast);
    GPIO_ResetBits(GPIOB, GPIO_Pin_3);
#endif
    while (1)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_0);
        delay_us(10);
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);
#if 1
        GPIO_SetBits(GPIOB, GPIO_Pin_3);
        delay_us(10);
        GPIO_ResetBits(GPIOB, GPIO_Pin_3);
#endif
        halt(); /* Program halted */
    }
}

/* Private functions ---------------------------------------------------------*/
/**
 * @brief Generate a delay
 * @param[in] nCount : value of the delay
 * @retval  None
 */
void Delay(uint16_t nCount)
{
    /* Decrement nCount value */
    while (nCount != 0)
    {
        nCount--;
    }
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval : None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/