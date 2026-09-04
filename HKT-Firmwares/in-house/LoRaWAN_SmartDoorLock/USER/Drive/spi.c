
/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

#define W25X_PageSize 256 // 页缓存

/*命令列表*/
#define W25X_WriteEnable 0x06
#define W25X_WriteDisable 0x04
#define W25X_ReadStatusReg 0x05
#define W25X_WriteStatusReg 0x01
#define W25X_ReadData 0x03
#define W25X_FastReadData 0x0B
#define W25X_FastReadDual 0x3B
#define W25X_PageProgram 0x02
#define W25X_BlockErase 0xD8
#define W25X_SectorErase 0x20
#define W25X_ChipErase 0xC7
#define W25X_PowerDown 0xB9
#define W25X_ReleasePowerDown 0xAB
#define W25X_DeviceID 0xAB
#define W25X_ManufactDeviceID 0x90
#define W25X_JedecDeviceID 0x9F

void W25X40_ChipErase()
{
    u8 buffer[2] = {0};
    buffer[0] = W25X_WriteEnable;
    Spi2WriteReadNByte(buffer, 1, NULL, 0);
    buffer[0] = W25X_ChipErase;
    Spi2WriteReadNByte(buffer, 1, NULL, 0);
}

void W25X40_PowerDown()
{
    u8 buffer[2] = {0};
    buffer[0] = W25X_PowerDown;
    Spi2WriteReadNByte(buffer, 1, NULL, 0);
}

void W25X40_ReleasePowerDown()
{
    u8 buffer[5] = {0};
    buffer[0] = W25X_ReleasePowerDown;
    Spi2WriteReadNByte(buffer, 4, buffer + 4, 1);
    DEBUG_TRACE(LOG_TAG, "Read Nor Flash ID 0x%02x", buffer[4]);
    // TicksDelayUs(20);
}

void SpiDeint(void)
{
    CMU_PERCLK_SetableEx(SPI2CLK, DISABLE);
    SPIx_CR2_SPIEN_Setable(SPI2, DISABLE);

    // CloseIO(GPIOA, GPIO_Pin_0); // SSN
    CloseIO(GPIOA, GPIO_Pin_1); // SCK
    CloseIO(GPIOA, GPIO_Pin_2); // MISO
    CloseIO(GPIOA, GPIO_Pin_3); // MOSI
// CloseIO(GPIOA, GPIO_Pin_4); // WP
}

void SpiInit(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能
    OutputIO(SPI_FLASH_WP_PORT, SPI_FLASH_WP_PIN, OUT_PUSHPULL);
    SPI_FLASH_WP_L;

    CMU_PERCLK_SetableEx(SPI2CLK, ENABLE); // 开启SPI2总线时钟

    SPIx_CR1_IOSWAP_Set(SPI2, SPIx_CR1_IOSWAP_DEFAULT); // MISO、MOSI默认引脚 不交换
    SPIx_CR1_MM_Set(SPI2, SPIx_CR1_MM_MASTER);          // master模式
    SPIx_CR1_WAIT_Set(SPI2, SPIx_CR1_WAIT_1WAIT);       // 每发送完一帧后插入一个CLK
    SPIx_CR1_BAUD_Set(SPI2, SPIx_CR1_BAUD_DIV2);        // 波特率设置为外设时钟2分频
    SPIx_CR1_LSBF_Set(SPI2, SPIx_CR1_LSBF_MSB);         // 帧格式先发送MSB
    SPIx_CR1_CPHOL_Set(SPI2, SPIx_CR1_CPHOL_LOW);       // CLK停止在低电平
    SPIx_CR1_CPHA_Set(SPI2, SPIx_CR1_CPHA_1CLOCK);      // 第一个时钟边沿捕捉
    SPIx_CR2_SSNSEN_Setable(SPI2, ENABLE);              // SSN由软件自动控制
    // SPIx_CR2_SSNSEN_Setable(SPI2, DISABLE); 			// SSN由硬件自动控制

    SPIx_CR2_RXO_Setable(SPI2, DISABLE);                    // 关闭单接收模式
    SPIx_CR2_DLEN_Set(SPI2, SPIx_CR2_DLEN_8BIT);            // 通信数据字长8bit
    SPIx_CR2_HALFDUPLEX_Set(SPI2, SPIx_CR2_HALFDUPLEX_SPI); // SPI设置为标准SPI模式
    SPIx_CR2_SSNM_Set(SPI2, SPIx_CR2_SSNM_LOW);             // 每发送完一帧后Master保持SSN为低
    SPIx_CR2_TXO_AC_Setable(SPI2, DISABLE);                 // 关闭TXONLY自动清0
    SPIx_CR2_TXO_Setable(SPI2, DISABLE);                    // 关闭TXONLY模式
    // SPIx_CR2_SSNSEN_Setable(SPI2, DISABLE);					// SSN由硬件自动控制
    SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_HIGH);

    SPIx_CR3_SERRC_Clr(SPI2); // 清除从机错误标志
    SPIx_CR3_MERRC_Clr(SPI2); // 清除主机错误标志
    SPIx_CR3_RXBFC_Clr(SPI2); // 清除RXBUF
    SPIx_CR3_TXBFC_Clr(SPI2); // 清除TXBUF

    SPIx_CR2_SPIEN_Setable(SPI2, ENABLE); // 使能SPI2

    AltFunIO(GPIOA, GPIO_Pin_0, ALTFUN_NORMAL); // SSN
    AltFunIO(GPIOA, GPIO_Pin_1, ALTFUN_NORMAL); // SCK
    AltFunIO(GPIOA, GPIO_Pin_2, ALTFUN_NORMAL); // MISO
    AltFunIO(GPIOA, GPIO_Pin_3, ALTFUN_NORMAL); // MOSI

    DEBUG_TRACE(LOG_TAG, "%s Release", __func__);
    W25X40_ReleasePowerDown();
    // W25X40_ChipErase();
}

uint32_t SpiWriteAndRead(uint32_t data)
{
    SPIx_TXBUF_Write(SPI2, data);
    while (!SPIx_ISR_TXBE_Chk(SPI2))
        ;
    while (!SPIx_ISR_RXBF_Chk(SPI2))
        ;
    data = SPIx_RXBUF_Read(SPI2);

    return data;
}

void SpiWrite(uint8_t *data, uint32_t length)
{
    while (length--) {
        SpiWriteAndRead(*data);
        data++;
    }
}

void SpiRead(uint8_t *data, uint32_t length)
{
    while (length--) {
        *data = SpiWriteAndRead(0x00);
        data++;
    }
}

void Spi2WriteReadNByte(uint8_t *wdata, u16 wlength, uint8_t *rdata, u16 rlength)
{
    int i;
    int Txtimeout = 200;
    int Rxtimeout = 200;
    SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_LOW);
    SPI_FLASH_WP_H; // 写保护关闭
    if (wlength) {
        for (i = 0; i < wlength; i++) {
            SPIx_TXBUF_Write(SPI2, wdata[i]);
            do {
                if (SPIx_ISR_TXBE_Chk(SPI2))
                    break;
                TicksDelayUs(1);
            } while (Txtimeout--);
            do {
                if (SPIx_ISR_RXBF_Chk(SPI2))
                    break;
                TicksDelayUs(1);
            } while (Rxtimeout--);
            SPIx_RXBUF_Read(SPI2);
        }
        if (!Txtimeout) {
            INFO("\n Write Tx Timeout SPI2->ISR %04x %s\n", SPI2->ISR, __func__);
        }
        if (!Rxtimeout) {
            INFO("\n Write Recv Timeout SPI2->ISR %04x %s\n", SPI2->ISR, __func__);
        }
    }

    Txtimeout = 200;
    Rxtimeout = 200;
    if (rlength) {
        for (i = 0; i < rlength; i++) {
            SPIx_TXBUF_Write(SPI2, 0);
            do {
                if (SPIx_ISR_TXBE_Chk(SPI2))
                    break;
                TicksDelayUs(1);
            } while (Txtimeout--);
            do {
                if (SPIx_ISR_RXBF_Chk(SPI2))
                    break;
                TicksDelayUs(1);
            } while (Rxtimeout--);
            rdata[i] = SPIx_RXBUF_Read(SPI2);
        }
        if (!Txtimeout) {
            INFO("\n Write Tx Timeout SPI2->ISR %04x %s\n", SPI2->ISR, __func__);
        }
        if (!Rxtimeout) {
            INFO("\n Write Recv Timeout SPI2->ISR %04x %s\n", SPI2->ISR, __func__);
        }
    }
    SPI_FLASH_WP_L; // 写保护开
    SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_HIGH);
}

void Spi1Deint(void)
{
    CMU_PERCLK_SetableEx(SPI1CLK, DISABLE);
    SPIx_CR2_SPIEN_Setable(SPI1, DISABLE);

    GPIOx_DFS_Setable(CARD_NSS_PORT, CARD_NSS_PIN, DISABLE);   // 数字功能选择SPI1
    GPIOx_DFS_Setable(CARD_SCK_PORT, CARD_SCK_PIN, DISABLE);   // 数字功能选择SPI1
    GPIOx_DFS_Setable(CARD_MISO_PORT, CARD_MISO_PIN, DISABLE); // 数字功能选择SPI1
    GPIOx_DFS_Setable(CARD_MOSI_PORT, CARD_MOSI_PIN, DISABLE); // 数字功能选择SPI1

    // CloseIO(CARD_NSS_PORT, CARD_NSS_PIN);	// SSN
    // CloseIO(CARD_SCK_PORT, CARD_SCK_PIN);	// SCK
    // CloseIO(CARD_MISO_PORT, CARD_MISO_PIN); // MISO
    // CloseIO(CARD_MOSI_PORT, CARD_MOSI_PIN); // MOSI

    GPIO_OUTPUT_H(CARD_NSS_PORT, CARD_NSS_PIN);	// SSN
    GPIO_OUTPUT_L(CARD_SCK_PORT, CARD_SCK_PIN);	// SCK
    GPIO_OUTPUT_L(CARD_MISO_PORT, CARD_MISO_PIN); // MISO
    GPIO_OUTPUT_L(CARD_MOSI_PORT, CARD_MOSI_PIN); // MOSI
}

uint32_t Spi1WriteAndRead(uint32_t data)
{
    int Txtimeout = 200;
    int Rxtimeout = 200;
    SPIx_TXBUF_Write(SPI1, data);
    while (!SPIx_ISR_TXBE_Chk(SPI1)) {
        TicksDelayUs(1);
        Txtimeout--;
        if (!Txtimeout)
            break;
    }
    while (!SPIx_ISR_RXBF_Chk(SPI1)) {
        TicksDelayUs(1);
        Rxtimeout--;
        if (!Rxtimeout)
            break;
    }
    data = SPIx_RXBUF_Read(SPI1);
    if (!Txtimeout) {
        INFO("\n Write Tx Timeout SPI1->ISR %04x %s\n", SPI1->ISR, __func__);
    }
    if (!Rxtimeout) {
        INFO("\n Write Recv Timeout SPI1->ISR %04x %s\n", SPI1->ISR, __func__);
    }
    return data;
}

void Spi1Write(uint8_t *data, uint32_t length)
{
    while (length--) {
        Spi1WriteAndRead(*data);
        data++;
    }
}

void Spi1Read(uint8_t *data, uint32_t length)
{
    while (length--) {
        *data = Spi1WriteAndRead(0x00);
        data++;
    }
}

void Spi1Init(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);  // IO控制时钟寄存器使能
    CMU_PERCLK_SetableEx(SPI1CLK, ENABLE); // 开启SPI1总线时钟

    SPIx_CR1_IOSWAP_Set(SPI1, SPIx_CR1_IOSWAP_DEFAULT); // MISO、MOSI默认引脚 不交换
    SPIx_CR1_MM_Set(SPI1, SPIx_CR1_MM_MASTER);          // master模式
    SPIx_CR1_WAIT_Set(SPI1, SPIx_CR1_WAIT_1WAIT);       // 每发送完一帧后插入一个CLK
    SPIx_CR1_BAUD_Set(SPI1, SPIx_CR1_BAUD_DIV8);        // 波特率设置为外设时钟2分频
    SPIx_CR1_LSBF_Set(SPI1, SPIx_CR1_LSBF_MSB);         // 帧格式先发送MSB
    SPIx_CR1_CPHOL_Set(SPI1, SPIx_CR1_CPHOL_LOW);       // CLK停止在低电平
    SPIx_CR1_CPHA_Set(SPI1, SPIx_CR1_CPHA_1CLOCK);      // 第一个时钟边沿捕捉
    SPIx_CR2_SSNSEN_Setable(SPI1, ENABLE);              // SSN由硬件自动控制

    SPIx_CR2_RXO_Setable(SPI1, DISABLE);                    // 关闭单接收模式
    SPIx_CR2_DLEN_Set(SPI1, SPIx_CR2_DLEN_8BIT);            // 通信数据字长8bit
    SPIx_CR2_HALFDUPLEX_Set(SPI1, SPIx_CR2_HALFDUPLEX_SPI); // SPI设置为标准SPI模式
    SPIx_CR2_SSNM_Set(SPI1, SPIx_CR2_SSNM_LOW);             // 每次master发完后ssn保持低
    SPIx_CR2_TXO_AC_Setable(SPI1, DISABLE);                 // 关闭TXONLY自动清0
    SPIx_CR2_TXO_Setable(SPI1, DISABLE);                    // 关闭TXONLY模式
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_HIGH);

    SPIx_CR3_SERRC_Clr(SPI1); // 清除从机错误标志
    SPIx_CR3_MERRC_Clr(SPI1); // 清除主机错误标志
    SPIx_CR3_RXBFC_Clr(SPI1); // 清除RXBUF
    SPIx_CR3_TXBFC_Clr(SPI1); // 清除TXBUF

    SPIx_CR2_SPIEN_Setable(SPI1, ENABLE); // 使能SPI1

    AltFunIO(CARD_NSS_PORT, CARD_NSS_PIN, ALTFUN_NORMAL);   // SSN
    AltFunIO(CARD_SCK_PORT, CARD_SCK_PIN, ALTFUN_NORMAL);   // SCK
    AltFunIO(CARD_MISO_PORT, CARD_MISO_PIN, ALTFUN_NORMAL); // MISO
    AltFunIO(CARD_MOSI_PORT, CARD_MOSI_PIN, ALTFUN_NORMAL); // MOSI

    // 20uA
    GPIOx_DFS_Setable(CARD_NSS_PORT, CARD_NSS_PIN, ENABLE);   // 数字功能选择SPI1
    GPIOx_DFS_Setable(CARD_SCK_PORT, CARD_SCK_PIN, ENABLE);   // 数字功能选择SPI1
    GPIOx_DFS_Setable(CARD_MISO_PORT, CARD_MISO_PIN, ENABLE); // 数字功能选择SPI1
    GPIOx_DFS_Setable(CARD_MOSI_PORT, CARD_MOSI_PIN, ENABLE); // 数字功能选择SPI1
}
