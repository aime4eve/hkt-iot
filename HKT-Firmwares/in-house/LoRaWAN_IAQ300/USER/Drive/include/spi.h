
#ifndef __SPI_H__
#define __SPI_H__

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#ifdef __cplusplus
extern "C"
{
#endif


#define SPI_FLASH_CS_PORT GPIOA
#define SPI_FLASH_CS_PIN GPIO_Pin_0
#define SPI_FLASH_CS_H GPIO_SetBits(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN)
#define SPI_FLASH_CS_L GPIO_ResetBits(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN)
#define SPI_FLASH_CS_OUT OutputIO(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN, OUT_PUSHPULL)
#define SPI_FLASH_CS_IN InputtIO(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN, IN_NORMAL)
#define READ_SPI_FLASH_CS_STATUS GPIO_ReadInputDataBit(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN)

#define SPI_FLASH_CLK_PORT GPIOA
#define SPI_FLASH_CLK_PIN GPIO_Pin_1
#define SPI_FLASH_CLK_H GPIO_SetBits(SPI_FLASH_CLK_PORT, SPI_FLASH_CLK_PIN)
#define SPI_FLASH_CLK_L GPIO_ResetBits(SPI_FLASH_CLK_PORT, SPI_FLASH_CLK_PIN)
#define SPI_FLASH_CLK_OUT OutputIO(SPI_FLASH_CLK_PORT, SPI_FLASH_CLK_PIN, OUT_PUSHPULL)
#define SPI_FLASH_CLK_IN InputtIO(SPI_FLASH_CLK_PORT, SPI_FLASH_CLK_PIN, IN_NORMAL)
#define READ_SPI_FLASH_CLK_STATUS GPIO_ReadInputDataBit(SPI_FLASH_CLK_PORT, SPI_FLASH_CLK_PIN)

#define SPI_FLASH_MISO_PORT GPIOA
#define SPI_FLASH_MISO_PIN GPIO_Pin_2
#define SPI_FLASH_MISO_H GPIO_SetBits(SPI_FLASH_MISO_PORT, SPI_FLASH_MISO_PIN)
#define SPI_FLASH_MISO_L GPIO_ResetBits(SPI_FLASH_MISO_PORT, SPI_FLASH_MISO_PIN)
#define SPI_FLASH_MISO_OUT OutputIO(SPI_FLASH_MISO_PORT, SPI_FLASH_MISO_PIN, OUT_PUSHPULL)
#define SPI_FLASH_MISO_IN InputtIO(SPI_FLASH_MISO_PORT, SPI_FLASH_MISO_PIN, IN_NORMAL)
// #define SPI_FLASH_MISO_IN InputtIO(SPI_FLASH_MISO_PORT, SPI_FLASH_MISO_PIN, IN_PULLUP)
#define READ_SPI_FLASH_MISO_STATUS GPIO_ReadInputDataBit(SPI_FLASH_MISO_PORT, SPI_FLASH_MISO_PIN)

#define SPI_FLASH_MOSI_PORT GPIOA
#define SPI_FLASH_MOSI_PIN GPIO_Pin_3
#define SPI_FLASH_MOSI_H GPIO_SetBits(SPI_FLASH_MOSI_PORT, SPI_FLASH_MOSI_PIN)
#define SPI_FLASH_MOSI_L GPIO_ResetBits(SPI_FLASH_MOSI_PORT, SPI_FLASH_MOSI_PIN)
#define SPI_FLASH_MOSI_OUT OutputIO(SPI_FLASH_MOSI_PORT, SPI_FLASH_MOSI_PIN, OUT_PUSHPULL)
#define SPI_FLASH_MOSI_IN InputtIO(SPI_FLASH_MOSI_PORT, SPI_FLASH_MOSI_PIN, IN_NORMAL)
#define READ_SPI_FLASH_MOSI_STATUS GPIO_ReadInputDataBit(SPI_FLASH_MOSI_PORT, SPI_FLASH_MOSI_PIN)


#define SPI_FLASH_WP_PORT GPIOA
#define SPI_FLASH_WP_PIN GPIO_Pin_4
#define SPI_FLASH_WP_H GPIO_SetBits(SPI_FLASH_WP_PORT, SPI_FLASH_WP_PIN)
#define SPI_FLASH_WP_L GPIO_ResetBits(SPI_FLASH_WP_PORT, SPI_FLASH_WP_PIN)
#define SPI_FLASH_WP_OUT OutputIO(SPI_FLASH_WP_PORT, SPI_FLASH_WP_PIN, OUT_PUSHPULL)


    void W25X40_PowerDown(void);
    void W25X40_ReleasePowerDown(void);

    void SpiDeint(void);
    void SpiInit(void);
    void SpiRead(uint8_t *data, uint32_t length);
    void SpiWrite(uint8_t *data, uint32_t length);
    uint32_t SpiWriteAndRead(uint32_t data);
    void Spi2WriteReadNByte(uint8_t *wdata, u16 wlength, uint8_t *rdata, u16 rlength);

    void W25X_Init(void);
    void SPI_FLASH_SendByte(uint8_t byte);
    uint8_t SPI_FLASH_ReadByte(void);
    void SPI_FLASH_WaitForWriteEnd(void);
    void W25X_Write(uint32_t addr, uint8_t *dat, uint16_t len);
    void W25X_Read(uint32_t addr, uint8_t *dat, uint16_t len);

    void Spi4Deint(void);
    uint32_t Spi4WriteAndRead(uint32_t data);
    void Spi4Write(uint8_t *data, uint32_t length);
    void Spi4Read(uint8_t *data, uint32_t length);
    void Spi4Init(void);
    


#ifdef __cplusplus
}
#endif

#endif
