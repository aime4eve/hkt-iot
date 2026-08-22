
#include "stdio.h"
#include "string.h"

#include "LoRaWAN_APPLY.h"
#include "adc.h"
#include "communicate.h"
#include "gpio.h"
#include "iic.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

u8 I2C_Send_Bit(I2C_Type *I2Cx, u8 BIT_def)
{
    I2Cx_CFGR_MSPEN_Setable(I2Cx, ENABLE); // 使能I2C电路
    switch (BIT_def) {
    case STARTBIT:
        I2C_SEND_STARTBIT(I2Cx);
        while (RESET == I2Cx_ISR_S_Chk(I2Cx))
            ;
        break;

    case RESTARTBIT:
        I2C_SEND_RESTARTBIT(I2Cx);
        while (RESET == I2Cx_ISR_S_Chk(I2Cx))
            ;
        break;

    case STOPBIT:
        I2C_SEND_STOPBIT(I2Cx);
        while (RESET == I2Cx_ISR_P_Chk(I2Cx))
            ;
        break;

    default:
        break;
    }
    return 0; // ok
}

u8 I2C_Send_Byte(I2C_Type *I2Cx, u8 x_byte)
{
    I2Cx_BUF_Write(I2Cx, x_byte); // 写发送缓冲寄存器
    int Txtimeout = 10;
    while (!I2Cx_ISR_TXIF_Chk(I2Cx)) {
        tx_thread_sleep(1);
        if (Txtimeout) {
            Txtimeout--;
            if (!Txtimeout) {
                break;
            }
        }
    }
    // while (!I2Cx_ISR_TXIF_Chk(I2Cx));
    I2Cx_ISR_TXIF_Clr(I2Cx); // clr int flag
    if (!I2Cx_ISR_ACKSTA_Chk(I2Cx)) {
        return 0;
    } else {
        I2Cx_ISR_ACKSTA_Clr(I2Cx);
        return 1;
    }
}

u8 I2C_Receive_Byte(I2C_Type *I2Cx, u8 *x_byte)
{
    // i2c en, rcen
    I2Cx_CR_RCEN_Setable(I2Cx, ENABLE);
    int Rxtimeout = 10;
    while (!I2Cx_ISR_RXIF_Chk(I2Cx)) {
        tx_thread_sleep(1);
        if (Rxtimeout) {
            Rxtimeout--;
            if (!Rxtimeout) {
                break;
            }
        }
    }
    // while (!I2Cx_ISR_RXIF_Chk(I2Cx))
    //     ;
    I2Cx_ISR_RXIF_Clr(I2Cx);
    *x_byte = I2Cx_BUF_Read(I2Cx);
    return 0;
}

void I2Cx_IRQHandler(void)
{
    if (SET == I2Cx_ISR_TXIF_Chk(I2C0)) {
        I2Cx_ISR_TXIF_Clr(I2C0); // 清中断标志
    }
    if (SET == I2Cx_ISR_RXIF_Chk(I2C0)) {
        I2Cx_ISR_RXIF_Clr(I2C0); // 清中断标志
    }
    if (SET == I2Cx_ISR_RXIF_Chk(I2C0)) {
        I2Cx_ISR_RXIF_Clr(I2C0); // 清中断标志
    }
    if (SET == I2Cx_TO_Read(I2C0)) {
        I2Cx_TO_Write(I2C0, I2Cx_CFGR_TOEN_Pos);
        I2C_Init(I2C0);
    }
}

/**
 * @brief  I2C初始化
 * @param   I2Cx: I2C通道号
 * @retval
 */
void I2C_Init(I2C_Type *I2Cx)
{
    I2Cx_Deinit(I2Cx); // 复位I2C
    // I2CIO口配置
    if (I2Cx == I2C0) {
        CMU_PERCLK_SetableEx(I2C0CLK, ENABLE); // I2C总线时钟使能
        CMU_OPCCR1_I2C0CKE_Setable(ENABLE);    // I2C工作时钟使能
        CMU_OPCCR1_I2C0CKS_Set(CMU_OPCCR1_I2C0CKS_APBCLK);
        AltFunIO(GPIOA, GPIO_Pin_14, ALTFUN_OPENDRAIN); // PA14;//SCL
        AltFunIO(GPIOA, GPIO_Pin_15, ALTFUN_OPENDRAIN); // PA15;//SDA
    }

    if (I2Cx == I2C1) {
        CMU_PERCLK_SetableEx(I2C1CLK, ENABLE); // I2C总线时钟使能
        CMU_OPCCR1_I2C1CKE_Setable(ENABLE);    // I2C工作时钟使能
        CMU_OPCCR1_I2C1CKS_Set(CMU_OPCCR1_I2C1CKS_APBCLK);
        AltFunIO(GPIOE, GPIO_Pin_5, ALTFUN_OPENDRAIN); // PE5;//SCL
        AltFunIO(GPIOE, GPIO_Pin_6, ALTFUN_OPENDRAIN); // PE6;//SDA
    }

    I2Cx_BRG_MSPBRGH_Set(I2Cx, (I2C_BaudREG_Calc(100000, RCHFCLKCFG * 1000000)) << 16); // 100k@8M
    I2Cx_BRG_MSPBRGL_Set(I2Cx, I2C_BaudREG_Calc(100000, RCHFCLKCFG * 1000000));         // 100k@8M
    I2Cx_TIMING_Write(I2Cx, I2Cx_BRG_MSPBRGL_Get(I2Cx) / 2);
    I2Cx_CFGR_TOEN_Setable(I2Cx, ENABLE);
    NVIC_DisableIRQ(I2Cx_IRQn);
    // NVIC_SetPriority(I2Cx_IRQn, 2);
    // NVIC_EnableIRQ(I2Cx_IRQn);
}

u8 i2c_hard_write(uint8_t address, uint8_t *data, uint16_t count)
{
    u8 result, i;
    //-------------- start bit -------------
    result = I2C_Send_Bit(I2C0, STARTBIT); // 发送起始位
    if (result != 0) {
        return 1; // failure.
    }

    //-------------- disable read -------------
    I2Cx_CR_RCEN_Setable(I2C0, DISABLE);

    //-------------- device addr -------------
    result = I2C_Send_Byte(I2C0, (address << 1)); // 发送器件地址
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 2; // failure.
    }

    //--------------- data --------------
    for (i = 0; i < count; i++) {
        result = I2C_Send_Byte(I2C0, data[i]);
        if (result != 0) {
            I2C_Send_Bit(I2C0, STOPBIT);
            return 3; // failure.
        }
    }
    result = I2C_Send_Bit(I2C0, STOPBIT);
    if (result != 0) {
        return 4; // failure.
    }
    return 0; // ok
}

u8 i2c_hard_read(uint8_t address, uint8_t *data, uint16_t count)
{
    u8 result, i;
    //-------------- start bit -------------
    result = I2C_Send_Bit(I2C0, STARTBIT); // 发送起始位
    if (result != 0) {
        return 1; // failure.
    }

    //-------------- disable read -------------
    I2Cx_CR_RCEN_Setable(I2C0, DISABLE);

    //-------------- device addr -------------
    result = I2C_Send_Byte(I2C0, (address << 1) | 1); // 发送器件地址
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 2; // failure.
    }

    //--------------- data --------------
    for (i = 0; i < count; i++) {
        I2C_SEND_ACK_0(I2C0);
        I2C_Receive_Byte(I2C0, &data[i]);
    }
    I2C_SEND_ACK_1(I2C0);
    result = I2C_Send_Bit(I2C0, STOPBIT);
    if (result != 0) {
        return 5; // failure.
    }
    return 0; // ok
}

u8 i2c_hard_write_onebyte(uint8_t address, uint8_t reg, uint8_t data)
{
    u8 result;
    //-------------- start bit -------------
    result = I2C_Send_Bit(I2C0, STARTBIT); // 发送起始位
    if (result != 0) {
        return 1; // failure.
    }

    //-------------- disable read -------------
    I2Cx_CR_RCEN_Setable(I2C0, DISABLE);

    //-------------- device addr -------------
    result = I2C_Send_Byte(I2C0, (address << 1)); // 发送器件地址
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 2; // failure.
    }

    result = I2C_Send_Byte(I2C0, reg);
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 3; // failure.
    }

    result = I2C_Send_Byte(I2C0, data);
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 3; // failure.
    }

    result = I2C_Send_Bit(I2C0, STOPBIT);
    if (result != 0) {
        return 4; // failure.
    }
    return 0; // ok
}

u8 i2c_hard_write_read(uint8_t address, uint8_t reg, uint8_t *data, uint16_t count)
{
    u8 result, i;
    //-------------- start bit -------------
    result = I2C_Send_Bit(I2C0, STARTBIT); // 发送起始位
    if (result != 0) {
        return 1; // failure.
    }

    //-------------- disable read -------------
    I2Cx_CR_RCEN_Setable(I2C0, DISABLE);

    //-------------- device addr -------------
    result = I2C_Send_Byte(I2C0, (address << 1)); // 发送器件地址
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 2; // failure.
    }

    //--------------- data --------------
    result = I2C_Send_Byte(I2C0, reg);
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 2; // failure.
    }

    if (count > 0) {
        //-------------- start bit -------------
        result = I2C_Send_Bit(I2C0, RESTARTBIT); // 发送起始位
        if (result != 0) {
            // TX_RESTORE
            return 3; // failure.
        }
        //-------------- device addr -------------
        result = I2C_Send_Byte(I2C0, (address << 1) | 1); // 发送器件地址
        if (result != 0) {
            return 4; // failure.
        }

        //--------------- data --------------
        for (i = 0; i < count; i++) {
            if (i < (count - 1))
                I2C_SEND_ACK_0(I2C0);
            else
                I2C_SEND_ACK_1(I2C0);
            result = I2C_Receive_Byte(I2C0, &data[i]);
            if (result) {
                I2C_Send_Bit(I2C0, STOPBIT);
                return 5;
            }
        }
    }
    result = I2C_Send_Bit(I2C0, STOPBIT);
    if (result != 0) {
        return 6; // failure.
    }
    return 0; // ok
}

u8 i2c_hard_write_read_n(uint8_t address, uint8_t *send, uint8_t send_len, uint8_t *recv, uint8_t recv_len)
{
    u8 result, i;
    //-------------- start bit -------------
    result = I2C_Send_Bit(I2C0, STARTBIT); // 发送起始位
    if (result != 0) {
        return 1; // failure.
    }

    //-------------- disable read -------------
    I2Cx_CR_RCEN_Setable(I2C0, DISABLE);

    //-------------- device addr -------------
    result = I2C_Send_Byte(I2C0, (address << 1)); // 发送器件地址
    if (result != 0) {
        I2C_Send_Bit(I2C0, STOPBIT);
        return 2; // failure.
    }

    //--------------- data --------------
    for (i = 0; i < send_len; i++) {
        result = I2C_Send_Byte(I2C0, send[i]);
        if (result != 0) {
            I2C_Send_Bit(I2C0, STOPBIT);
            return 3; // failure.
        }
    }

    if (recv_len > 0) {
        //-------------- start bit -------------
        result = I2C_Send_Bit(I2C0, RESTARTBIT); // 发送起始位
        if (result != 0) {
            return 1; // failure.
        }
        //-------------- device addr -------------
        result = I2C_Send_Byte(I2C0, (address << 1) | 1); // 发送器件地址
        if (result != 0) {
            return 4; // failure.
        }

        //--------------- data --------------
        for (i = 0; i < recv_len; i++) {
            if (i < (recv_len - 1))
                I2C_SEND_ACK_0(I2C0);
            else
                I2C_SEND_ACK_1(I2C0);
            result = I2C_Receive_Byte(I2C0, &recv[i]);
            if (result) {
                I2C_Send_Bit(I2C0, STOPBIT);
                return 5;
            }
        }
    }
    result = I2C_Send_Bit(I2C0, STOPBIT);
    if (result != 0) {
        return 6; // failure.
    }
    return 0; // ok
}

u8 I2C_Test(void)
{
    int16_t error;
    uint8_t buffer[6];
    uint16_t offset = 0;
    buffer[offset++] = (uint8_t)0x94;
    error = i2c_hard_write(0x44, &buffer[0], offset);
    if (error) {
        return error;
    }
    tx_thread_sleep(10);

    offset = 0;
    buffer[offset++] = (uint8_t)0x89;
    error = i2c_hard_write(0x44, &buffer[0], offset);
    if (error) {
        return error;
    }
    tx_thread_sleep(10);

    error = i2c_hard_read(0x44, &buffer[0], 6);
    if (error) {
        return error;
    }
    return 0;
}
