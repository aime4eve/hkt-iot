
#ifndef __SPI_H__
#define __SPI_H__

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void W25X40_ChipErase(void);
    void W25X40_PowerDown(void);
    void W25X40_ReleasePowerDown(void);

    void SpiDeint(void);
    void SpiInit(void);
    void SpiRead(uint8_t *data, uint32_t length);
    void SpiWrite(uint8_t *data, uint32_t length);
    uint32_t SpiWriteAndRead(uint32_t data);
    void Spi2WriteReadNByte(uint8_t *wdata, u16 wlength, uint8_t *rdata, u16 rlength);

    void Spi1Deint(void);
    void Spi1Init(void);
    void Spi1Read(uint8_t *data, uint32_t length);
    void Spi1Write(uint8_t *data, uint32_t length);
    uint32_t Spi1WriteAndRead(uint32_t data);

#ifdef __cplusplus
}
#endif

#endif
