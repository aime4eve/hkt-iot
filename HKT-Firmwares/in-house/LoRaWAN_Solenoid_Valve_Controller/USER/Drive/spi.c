
/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

void Spi3Deint(void)
{
    LL_SPI_DeInit(SPI3);
    LL_GPIO_InitTypeDef GPIO_InitStruct = {LL_GPIO_PIN_ALL, LL_GPIO_MODE_ANALOG,
                                           LL_GPIO_SPEED_FREQ_HIGH, LL_GPIO_OUTPUT_PUSHPULL,
                                           LL_GPIO_PULL_NO, LL_GPIO_AF_0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12);
}

void Spi3Init(void)
{
    LL_SPI_InitTypeDef SPI_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI3);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    // LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    /**SPI3 GPIO Configuration
    PA15 (JTDI)   ------> SPI3_NSS
    PC10   ------> SPI3_SCK
    PC11   ------> SPI3_MISO
    PC12   ------> SPI3_MOSI
    */
    // GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
    // GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    // GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    // GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    // GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    // LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    // GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_4); // WP

    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_15);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USER CODE BEGIN SPI3_Init 1 */

    /* USER CODE END SPI3_Init 1 */
    /* SPI3 parameter configuration*/
    // SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
    SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
    SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV8;
    SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly = 7;
    LL_SPI_Init(SPI3, &SPI_InitStruct);
    LL_SPI_SetStandard(SPI3, LL_SPI_PROTOCOL_MOTOROLA);
    // LL_SPI_EnableNSSPulseMgt(SPI3);
    LL_SPI_DisableNSSPulseMgt(SPI3);

    LL_SPI_Enable(SPI3);
}

uint32_t Spi3WriteAndRead(uint32_t data)
{
    uint32_t rdata;
    while (!LL_SPI_IsActiveFlag_TXE(SPI3))
        ;
    LL_SPI_TransmitData8(SPI3, (uint8_t)data);
    while (LL_SPI_IsActiveFlag_BSY(SPI3))
        ;
    while (!LL_SPI_IsActiveFlag_RXNE(SPI3))
        ;
    rdata = LL_SPI_ReceiveData8(SPI3);
    return rdata;
}

void Spi3Write(uint8_t *data, uint32_t length)
{
    while (length--) {
        Spi3WriteAndRead(*data);
        data++;
    }
}

void Spi3Read(uint8_t *data, uint32_t length)
{
    while (length--) {
        *data = Spi3WriteAndRead(0x00);
        data++;
    }
}

void Spi3WriteReadNByte(uint8_t *wdata, u16 wlength, uint8_t *rdata, u16 rlength)
{
    int i;
    int Txtimeout = 200;
    int Bsytimeout = 200;
    int Rxtimeout = 200;

    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_15); // 选中
    FLASH_WP_H;                                    // 写保护关闭

    if (wlength) {

        // LL_SPI_SetTransferDirection(SPI3, LL_SPI_HALF_DUPLEX_TX);
        for (i = 0; i < wlength; i++) {
            do {
                if (LL_SPI_IsActiveFlag_TXE(SPI3))
                    break;
                Delay_Us(1);
            } while (Txtimeout--);

            LL_SPI_TransmitData8(SPI3, wdata[i]);

            do {
                if (!LL_SPI_IsActiveFlag_BSY(SPI3))
                    break;
                Delay_Us(1);
            } while (Bsytimeout--);

            do {
                if (LL_SPI_IsActiveFlag_RXNE(SPI3))
                    break;
                Delay_Us(1);
            } while (Rxtimeout--);

            LL_SPI_ReceiveData8(SPI3);
        }
        if (!Txtimeout) {
            INFO("\n Write Tx Timeout SPI3->SR %04x %s\n", SPI3->SR, __func__);
        }
        if (!Bsytimeout) {
            INFO("\n Write Bsy Timeout SPI3->SR %04x %s\n", SPI3->SR, __func__);
        }
        if (!Rxtimeout) {
            INFO("\n Write Recv Timeout SPI3->SR %04x %s\n", SPI3->SR, __func__);
        }
    }

    Txtimeout = 200;
    Bsytimeout = 200;
    Rxtimeout = 200;
    if (rlength) {
        for (i = 0; i < rlength; i++) {
            // LL_SPI_SetTransferDirection(SPI3, LL_SPI_HALF_DUPLEX_RX);
            do {
                if (LL_SPI_IsActiveFlag_TXE(SPI3))
                    break;
                Delay_Us(1);
            } while (Txtimeout--);

            LL_SPI_TransmitData8(SPI3, 0);

            do {
                if (!LL_SPI_IsActiveFlag_BSY(SPI3))
                    break;
                Delay_Us(1);
            } while (Bsytimeout--);

            do {
                if (LL_SPI_IsActiveFlag_RXNE(SPI3))
                    break;
                Delay_Us(1);
            } while (Rxtimeout--);

            rdata[i] = LL_SPI_ReceiveData8(SPI3);
        }
        if (!Txtimeout) {
            INFO("\n Write Tx Timeout SPI3->SR %04x %s\n", SPI3->SR, __func__);
        }
        if (!Bsytimeout) {
            INFO("\n Write Bsy Timeout SPI3->SR %04x %s\n", SPI3->SR, __func__);
        }
        if (!Rxtimeout) {
            INFO("\n Write Recv Timeout SPI3->SR %04x %s\n", SPI3->SR, __func__);
        }
    }

    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_15);
    FLASH_WP_L; // 写保护开
}
