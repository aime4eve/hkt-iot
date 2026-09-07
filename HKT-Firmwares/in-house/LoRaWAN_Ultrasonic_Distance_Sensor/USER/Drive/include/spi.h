
#ifndef __SPI_H__
#define __SPI_H__

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#ifdef __cplusplus
extern "C" {
#endif

enum Command {
    WREN = 0x06,    //!< Write Enable.
    WREVSR = 0x50,  //!< Write Enable for Volatile Status Register.
    WRDI = 0x04,    //!< Write Disable.
    RDSR = 0x05,    //!< Read Status Register.
    WRR = 0x01,     //!< Write Status Register.
    READ = 0x03,    //!< Read Data.
    FRD = 0x0b,     //!< Fast Read.
    FRDDIO = 0xbb,  //!< Fast Read Dual.
    PP = 0x02,      //!< Page Program.
    SER = 0x20,     //!< Sector Erase (4 kB).
    B32ER = 0x52,   //!< Block Erase (32 kB).
    B64ER = 0xd8,   //!< Block Erase (64kB).
    CER = 0x60,     //!< Chip Erase.
    PWD = 0xb9,     //!< Power-down
    RLPWD = 0xab,   //!< Release Power-down/Device ID.
    RDID = 0x90,    //!< Read Manufacturer/Device ID.
    RDIDDIO = 0x92, //!< Read Manufacturer/Device ID Dual I/O.
    RDJID = 0x9f,   //!< Read JEDEC ID.
    RDUID = 0x4b    //!< Read Unique ID.
};

void Spi3Deint(void);
void Spi2Deint(void);
void Spi2Init(void);
void Spi3Init(void);
void Spi3Read(uint8_t *data, uint32_t length);
void Spi3Write(uint8_t *data, uint32_t length);
uint32_t Spi3WriteAndRead(uint32_t data);
void Spi2WriteReadNByte(uint8_t *wdata, u16 wlength, uint8_t *rdata, u16 rlength);
void Spi3WriteReadNByte(uint8_t *wdata, u16 wlength, uint8_t *rdata, u16 rlength);

#ifdef __cplusplus
}
#endif

#endif
