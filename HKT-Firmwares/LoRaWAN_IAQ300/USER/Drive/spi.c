
/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "EPD.h"
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
    // DEBUG_TRACE(LOG_TAG, "Read Nor Flash ID 0x%02x", buffer[4]);
    // TicksDelayUs(20);
}

void SpiDeint(void)
{
    CMU_PERCLK_SetableEx(SPI2CLK, DISABLE);
    SPIx_CR2_SPIEN_Setable(SPI2, DISABLE); // 使能SPI2
    CloseIO(GPIOA, GPIO_Pin_0);            // SSN
    CloseIO(GPIOA, GPIO_Pin_1);            // SCK
    // SPI_FLASH_CS_OUT;
    // SPI_FLASH_CS_H;

    // SPI_FLASH_CLK_OUT;
    // SPI_FLASH_CLK_L;

    CloseIO(GPIOA, GPIO_Pin_2); // MISO
    CloseIO(GPIOA, GPIO_Pin_3); // MOSI
    CloseIO(GPIOA, GPIO_Pin_4); // WP
                                // SPI_FLASH_WP_OUT;
                                // SPI_FLASH_WP_L;
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

    // DEBUG_TRACE(LOG_TAG, "%s Release", __func__);
    W25X40_ReleasePowerDown();
    // W25X40_ChipErase();
    // SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_LOW);

    // u32 DeviceID = 0, JedecDeviceID = 0;
    // u8 buffer[4] = {0};
    // memset(buffer, 0xFF, 4);
    // buffer[0] = W25X_DeviceID;
    // // SPI_FLASH_WP_H;
    // SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_LOW);
    // TicksDelayUs(20);

    // SpiWrite(buffer, 4); // 读取ID命令
    // SpiRead(buffer, 1);  // 读取1次
    // DeviceID = buffer[0];

    // SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_HIGH);
    // SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_LOW);

    // memset(buffer, 0xFF, 4);
    // buffer[0] = W25X_JedecDeviceID;

    // SpiWrite(buffer, 3); // 读取ID命令
    // SpiRead(buffer, 3);  // 读取3次
    // JedecDeviceID = buffer[0] << 16 | buffer[1] << 8 | buffer[2];

    // // SPI_FLASH_WP_L;
    // SPIx_CR2_SSN_Set(SPI2, SPIx_CR2_SSN_HIGH);

    // if (JedecDeviceID > 0 && JedecDeviceID < 0x00FFFFFF) // ID=0xEF3013
    // {
    //     DEBUG_TRACE(LOG_TAG, "Read W25X ID Success: %d %d\n", DeviceID, JedecDeviceID);
    // } else {
    //     DEBUG_TRACE(ERROR_TAG, "Read W25X ID Fail: %d %d\n", DeviceID, JedecDeviceID);
    // }
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

/*
功能：  W25X40芯片初始化
参数：  无
返回：  1成功，0失败
*/
void W25X_Init(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能
    // SPI_FLASH_WP_OUT;
    SPI_FLASH_CS_OUT;
    SPI_FLASH_CLK_OUT;
    SPI_FLASH_MOSI_OUT;
    SPI_FLASH_MISO_IN;

    // SPI_FLASH_WP_H; //写保护拉低,默认不能写入
    SPI_FLASH_CS_H; // 片选拉高,默认不使能读写

    SPI_FLASH_CS_L;
    SPI_FLASH_SendByte(W25X_DeviceID); // 读取ID命令
    SPI_FLASH_SendByte(0xFF);
    SPI_FLASH_SendByte(0xFF);
    SPI_FLASH_SendByte(0xFF);
    u32 id = SPI_FLASH_ReadByte(); // 读取1次
    SPI_FLASH_CS_H;

    SPI_FLASH_CS_L;                         // 片选拉低
    SPI_FLASH_SendByte(W25X_JedecDeviceID); // 读取ID命令
    id = SPI_FLASH_ReadByte();              // 读取3次
    id <<= 8;
    id |= SPI_FLASH_ReadByte();
    id <<= 8;
    id |= SPI_FLASH_ReadByte();
    SPI_FLASH_CS_H; // 片选拉高
    // SPI_FLASH_WP_L;

    if (id > 0 && id < 0x00FFFFFF) // ID=0xEF3013
    {
        DEBUG_TRACE(LOG_TAG, "Read W25X ID: %d\n", id);
    } else {
        DEBUG_TRACE(ERROR_TAG, "Read W25X ID Fail: %d\n", id);
    }
    tx_thread_sleep(1000);
}

/*
功能：  SPI发送一个字节
参数：  要发送的字节
返回：  无
*/
void SPI_FLASH_SendByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (byte & 0x80) {
            SPI_FLASH_MOSI_H;
        } else {
            SPI_FLASH_MOSI_L;
        }
        SPI_FLASH_CLK_H;
        byte <<= 1;

        SPI_FLASH_CLK_L;
    }
}

/*
功能：  SPI读取一个字节
参数：  无
返回：  读取的字节
*/
uint8_t SPI_FLASH_ReadByte(void)
{
    uint8_t i, byte;

    byte = 0;
    for (i = 0; i < 8; i++) {
        byte <<= 1;
        SPI_FLASH_CLK_H;

        if (READ_SPI_FLASH_MISO_STATUS) {
            byte |= 1;
        }
        SPI_FLASH_CLK_L;
    }

    return byte;
}

/*
功能：  等待W25X40操作执行完毕
参数：  无
返回：  无
*/
void SPI_FLASH_WaitForWriteEnd(void)
{
    uint8_t FLASH_Status = 0;
    uint32_t overTim = 0;

    SPI_FLASH_CS_L;
    SPI_FLASH_SendByte(W25X_ReadStatusReg); // 读取状态
    do {
        FLASH_Status = SPI_FLASH_ReadByte();
        overTim++;
        if (overTim > 0xFFFF) // 避免死机
        {
            break;
        }
    } while ((FLASH_Status & 0x01) == SET);

    SPI_FLASH_CS_H;
}

/*
功能：  W25X40写入一页
参数：  数据,地址,长度
返回：  无
*/
static void SPI_FLASH_PageWrite(u8 *pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
    SPI_FLASH_CS_L;
    SPI_FLASH_SendByte(W25X_WriteEnable); // 写使能命令
    SPI_FLASH_CS_H;

    SPI_FLASH_CS_L;
    SPI_FLASH_SendByte(W25X_PageProgram);             // 写数据命令
    SPI_FLASH_SendByte((WriteAddr & 0xFF0000) >> 16); // 地址
    SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);
    SPI_FLASH_SendByte(WriteAddr & 0xFF);
    while (NumByteToWrite--) // 数据
    {
        SPI_FLASH_SendByte(*pBuffer);
        pBuffer++;
    }
    SPI_FLASH_CS_H;

    SPI_FLASH_WaitForWriteEnd(); // 等待完成
}

/*
功能：  W25X40写入数据
参数：  地址,数据,长度
返回：  无
*/
void W25X_Write(uint32_t addr, uint8_t *dat, uint16_t len)
{
    uint8_t NumOfPage = 0, NumOfSingle = 0, AddrT = 0, count = 0, temp = 0;

    SPI_FLASH_WP_H; // 写保护关闭

    AddrT = addr % W25X_PageSize;      // 非对齐地址偏移
    count = W25X_PageSize - AddrT;     // 页剩余空间
    NumOfPage = len / W25X_PageSize;   // 存放数据要使用的页数
    NumOfSingle = len % W25X_PageSize; // 多余的数据(不满一页)

    if (AddrT == 0) // 正好对齐
    {
        if (NumOfPage == 0) // 只用一页
        {
            SPI_FLASH_PageWrite(dat, addr, len);
        } else {
            while (NumOfPage--) // 循环写入
            {
                SPI_FLASH_PageWrite(dat, addr, W25X_PageSize); // 每次写一页
                addr += W25X_PageSize;
                dat += W25X_PageSize;
            }
            SPI_FLASH_PageWrite(dat, addr, NumOfSingle); // 最后把不满一页的数据写入
        }
    } else // 没有对齐
    {
        if (NumOfPage == 0) // 数据<一页
        {
            if (NumOfSingle > count) // 数据长度>页剩余空间
            {
                temp = NumOfSingle - count;

                SPI_FLASH_PageWrite(dat, addr, count); // 把剩余空间写满
                addr += count;
                dat += count;

                SPI_FLASH_PageWrite(dat, addr, temp); // 其他数据写入新的一页
            } else                                    // 数据长度!>页剩余空间
            {
                SPI_FLASH_PageWrite(dat, addr, len); // 直接写入
            }
        } else // 数据>一页
        {
            len -= count;                      // 因为页不对齐,所以先减去前面要填满一页的数据
            NumOfPage = len / W25X_PageSize;   // 重新计算需要的页数
            NumOfSingle = len % W25X_PageSize; // 多余的数据(不满一页)

            SPI_FLASH_PageWrite(dat, addr, count); // 将要填满一页的数据先写入
            addr += count;
            dat += count;

            while (NumOfPage--) // 循环写入各页
            {
                SPI_FLASH_PageWrite(dat, addr, W25X_PageSize);
                addr += W25X_PageSize;
                dat += W25X_PageSize;
            }

            if (NumOfSingle != 0) // 有多余的数据
            {
                SPI_FLASH_PageWrite(dat, addr, NumOfSingle); // 单独写入新的一页
            }
        }
    }

    SPI_FLASH_WP_L; // 打开写保护
}

/*
功能：  W25X40读取数据
参数：  地址,数据,长度
返回：  无
*/
void W25X_Read(uint32_t addr, uint8_t *dat, uint16_t len)
{
    SPI_FLASH_CS_L;

    SPI_FLASH_SendByte(W25X_ReadData);           // 读命令
    SPI_FLASH_SendByte((addr & 0xFF0000) >> 16); // 地址
    SPI_FLASH_SendByte((addr & 0xFF00) >> 8);
    SPI_FLASH_SendByte(addr & 0xFF);
    while (len--) // 数据
    {
        *dat = SPI_FLASH_ReadByte();
        dat++;
    }

    SPI_FLASH_CS_H;
}

void Spi4Deint(void)
{
    CMU_PERCLK_SetableEx(SPI1CLK, DISABLE);
    SPIx_CR2_SPIEN_Setable(SPI1, DISABLE);
    //	CloseIO(EPD_CSB_PORT, EPD_CSB_PIN);	// SSN
    //	CloseIO(EPD_SCL_PORT, EPD_SCL_PIN); // SCK
    //	CloseIO(EPD_SDA_PORT, EPD_SDA_PIN); // MOSI
}

uint32_t Spi4WriteAndRead(uint32_t data)
{
    SPIx_TXBUF_Write(SPI4, data);
    while (!SPIx_ISR_TXBE_Chk(SPI4))
        ;
    //	while (!SPIx_ISR_RXBF_Chk(SPI4))
    //		;
    //	data = SPIx_RXBUF_Read(SPI4);

    return data;
}

void Spi4Write(uint8_t *data, uint32_t length)
{
    SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_LOW);
    while (length--) {
        Spi4WriteAndRead(*data);
        data++;
    }
    SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_HIGH);
}

void Spi4Read(uint8_t *data, uint32_t length)
{
    SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_LOW);
    while (length--) {
        *data = Spi4WriteAndRead(0x00);
        data++;
    }
    SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_HIGH);
}

void Spi4Init(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);  // IO控制时钟寄存器使能
    CMU_PERCLK_SetableEx(SPI4CLK, ENABLE); // 开启SPI4总线时钟

    SPIx_CR1_IOSWAP_Set(SPI4, SPIx_CR1_IOSWAP_DEFAULT); // MISO、MOSI默认引脚 不交换
    SPIx_CR1_MM_Set(SPI4, SPIx_CR1_MM_MASTER);          // master模式
    SPIx_CR1_WAIT_Set(SPI4, SPIx_CR1_WAIT_1WAIT);       // 每发送完一帧后插入一个CLK
    SPIx_CR1_BAUD_Set(SPI4, SPIx_CR1_BAUD_DIV32);       // 波特率设置为外设时钟2分频
    SPIx_CR1_LSBF_Set(SPI4, SPIx_CR1_LSBF_MSB);         // 帧格式先发送MSB
    SPIx_CR1_CPHOL_Set(SPI4, SPIx_CR1_CPHOL_LOW);       // CLK停止在低电平
    SPIx_CR1_CPHA_Set(SPI4, SPIx_CR1_CPHA_1CLOCK);      // 第一个时钟边沿捕捉
    SPIx_CR2_SSNSEN_Setable(SPI4, ENABLE);              // SSN由硬件自动控制
    // SPIx_CR2_SSNSEN_Setable(SPI4, DISABLE);

    SPIx_CR2_RXO_Setable(SPI4, DISABLE);                    // 关闭单接收模式
    SPIx_CR2_DLEN_Set(SPI4, SPIx_CR2_DLEN_8BIT);            // 通信数据字长8bit
    SPIx_CR2_HALFDUPLEX_Set(SPI4, SPIx_CR2_HALFDUPLEX_SPI); // SPI设置为标准SPI模式
    // SPIx_CR2_HALFDUPLEX_Set(SPI4, SPIx_CR2_HALFDUPLEX_DCN); // SPI设置为标准SPI模式
    SPIx_CR2_SSNM_Set(SPI4, SPIx_CR2_SSNM_LOW); // 每次master发完后ssn保持低
    SPIx_CR2_TXO_AC_Setable(SPI4, DISABLE);     // 关闭TXONLY自动清0
    SPIx_CR2_TXO_Setable(SPI4, DISABLE);        // 关闭TXONLY模式
    SPIx_CR2_SSN_Set(SPI4, SPIx_CR2_SSN_HIGH);

    SPIx_CR3_SERRC_Clr(SPI4); // 清除从机错误标志
    SPIx_CR3_MERRC_Clr(SPI4); // 清除主机错误标志
    SPIx_CR3_RXBFC_Clr(SPI4); // 清除RXBUF
    SPIx_CR3_TXBFC_Clr(SPI4); // 清除TXBUF

    SPIx_CR2_SPIEN_Setable(SPI4, ENABLE); // 使能SPI4

    AltFunIO(EPD_CSB_PORT, EPD_CSB_PIN, ALTFUN_NORMAL); // SSN
    AltFunIO(EPD_SCL_PORT, EPD_SCL_PIN, ALTFUN_NORMAL); // SCK
    AltFunIO(EPD_SDA_PORT, EPD_SDA_PIN, ALTFUN_NORMAL); // MOSI
    AltFunIO(GPIOE, GPIO_Pin_6, ALTFUN_NORMAL);         // MISO
}
