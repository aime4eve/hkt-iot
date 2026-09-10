//-----------------------------------------------------------------------------
// nz3801-ab_com.c
//-----------------------------------------------------------------------------
// Copyright 2017 nationz Ltd, Inc.
// http://www.nationz.com
//
// Program Description:
//
// driver definitions for the nfc reader.
//
//
//      PROJECT:   NZ3801-AB firmware
//      $Revision: $
//      LANGUAGE:  ANSI C
//
//
// Release 1.0
//    -Initial Revision (NZ)
//    -14 Feb 2017
//    -Latest release before new firmware coding standard
//

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/

#include "nz3801-ab_com.h"
#include "spi.h"

/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/

/**
 * @brief WriteRawRC
 *        write value at the specified address
 * @param address
 * @param value
 */
static void WriteRawRC(u8 address, u8 value)
{
    u8 buffer[2];
    buffer[0] = (address << 1) & 0x7E;
    buffer[1] = value;
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_LOW);
    Spi1Write(buffer, 2);
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_HIGH);
}

/**
 * @brief ReadRawRC
 *        read value from the specified address
 * @param address
 * @param
 */
static u8 ReadRawRC(u8 address)
{
    u8 txbuffer[2];
    u8 rxbuffer[2];
    txbuffer[0] = ((address << 1) & 0x7E) | 0x80;
    rxbuffer[0] = 0xFF;
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_LOW);
    Spi1Write(txbuffer, 1);
    Spi1Read(rxbuffer, 1);
    SPIx_CR2_SSN_Set(SPI1, SPIx_CR2_SSN_HIGH);
    return rxbuffer[0];
}

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/

/**
 * @brief G E N E R I C    W R I T E
 *        write value at the specified address
 * @param address
 * @param value
 */
void nzWriteReg(u8 reg, u8 value)
{
    WriteRawRC(reg, value);
}

/**
 * @brief G E N E R I C    R E A D
 *        read value from the specified address
 * @param address
 * @param
 */
u8 nzReadReg(u8 reg)
{
    return ReadRawRC(reg);
}

/**
 * @brief S E T   A   B I T   M A S K
 *
 * @param address
 * @param mask
 */
void nzSetBitMask(u8 reg, u8 mask)
{
    u8 tmp;
    tmp = nzReadReg(reg);
    nzWriteReg(reg, (tmp | mask));
}

/**
 * @brief C L E A R   A   B I T   M A S K
 *
 * @param address
 * @param mask
 */
void nzClearBitMask(u8 reg, u8 mask)
{
    u8 tmp;
    tmp = nzReadReg(reg);
    nzWriteReg(reg, (tmp & (~mask)));
}

/**
 * @brief SET EXT REG DATA
 *
 * @param extRegAddr
 * @param extRegData
 */
void nzSetRegExt(u8 extRegAddr, u8 extRegData)
{
    u8 addr, regdata;

    addr = BFL_JREG_EXT_REG_ENTRANCE;
    regdata = BFL_JBIT_EXT_REG_WR_ADDR + extRegAddr;
    nzWriteReg(addr, regdata);

    addr = BFL_JREG_EXT_REG_ENTRANCE;
    regdata = BFL_JBIT_EXT_REG_WR_DATA + extRegData;
    nzWriteReg(addr, regdata);
}

/**
 * @brief Clear Fifo
 *
 * @param
 * @param
 */
void nzClearFifo(void)
{
    while (nzReadReg(FIFOLEVEL & 0x7f) != 0)
    {
        nzSetBitMask(FIFOLEVEL, 0x00 | BFL_JBIT_FLUSHBUFFER);
    }
}
/**
 * @brief Clear Flag
 *
 * @param
 * @param
 */
void nzClearFlag(void)
{
    nzWriteReg(COMMIRQ, 0x7f);
    nzWriteReg(DIVIRQ, 0x7f);
}

/**
 * @brief Start Cmd
 *
 * @param cmd
 * @param
 */
void nzStartCmd(u8 cmd)
{
    nzWriteReg(COMMAND, cmd);
}

/**
 * @brief Stop Cmd
 *
 * @param
 * @param
 */
void nzStopCmd(void)
{
    nzWriteReg(COMMAND, 0x00);
}

/**
 * @brief Set CRC enable or disable
 *
 * @param bEN
 * @param
 */
void nzSetCRC(bool bEN)
{
    if (bEN)
    {
        nzSetBitMask(TXMODE, BFL_JBIT_CRCEN);
        nzSetBitMask(RXMODE, BFL_JBIT_CRCEN);
    }
    else
    {
        nzClearBitMask(TXMODE, BFL_JBIT_CRCEN);
        nzClearBitMask(RXMODE, BFL_JBIT_CRCEN);
    }
}
/**
 * @brief Set PARITY enable or disable
 *
 * @param bEN
 * @param
 */
void nzSetPARITY(bool bEN)
{
    ;
}

/**
 * @brief check errors flag if Set or Not
 *
 * @param
 * @param
 */
bool nzFlagOK(void)
{
    if ((nzReadReg(REGERROR) & (BFL_JBIT_CRCERR | BFL_JBIT_PROTERR /*|BFL_JBIT_COLLERR|BFL_JBIT_PARITYERR*/)) == 0)
    {
        return true;
    }
    return false;
}

/**
 * @brief check crc flag if Set or Not
 *
 * @param
 * @param
 */
bool nzCrcOK(void)
{
    if ((nzReadReg(REGERROR) & (BFL_JBIT_CRCERR)) == 0)
    {
        return true;
    }
    return false;
}
