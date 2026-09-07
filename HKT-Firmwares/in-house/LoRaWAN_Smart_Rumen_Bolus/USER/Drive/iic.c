
/* Includes ------------------------------------------------------------------*/
#include "iic.h"
#include "sensirion_i2c_hal.h"
#include "systick.h"
#include "uart.h"

// 添加错误码定义
#define I2C_TIMEOUT_ERR 1
#define I2C_SUCCESS     0

// 添加超时计数器最大值
#define I2C_TIMEOUT_COUNT 10000

void I2C1_DeInit(void)
{
    /* 禁用I2C外设 */
    LL_I2C_Disable(I2C1);

    /* 关闭I2C时钟 */
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    LL_GPIO_DeInit(GPIOB);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    IIC_SDA_L;
    IIC_SCL_L;
    Delay_Us(4);
    IIC_SCL_H;
    IIC_SDA_H;
    Delay_Us(4);
}

void I2C1_Init(void)
{
    /* USER CODE BEGIN I2C1_Init 0 */
    I2C1_DeInit();

    /* USER CODE END I2C1_Init 0 */

    LL_I2C_InitTypeDef I2C_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    /**I2C1 GPIO Configuration
    PB8   ------> I2C1_SCL
    PB9   ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin        = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pull      = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Pull      = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    /** I2C Initialization
     */
    LL_I2C_EnableAutoEndMode(I2C1);
    LL_I2C_DisableOwnAddress2(I2C1);
    LL_I2C_DisableGeneralCall(I2C1);
    LL_I2C_EnableClockStretching(I2C1);
    I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
    I2C_InitStruct.Timing         = 0x00000708;
    //I2C_InitStruct.Timing = 0x00303d5b;
    //    if (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
    //        I2C_InitStruct.Timing = 0x0000;
    //    } else {
    //        I2C_InitStruct.Timing = 0x0004;
    //    }
    I2C_InitStruct.AnalogFilter    = LL_I2C_ANALOGFILTER_ENABLE;
    I2C_InitStruct.DigitalFilter   = 0;
    I2C_InitStruct.OwnAddress1     = 0;
    I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
    I2C_InitStruct.OwnAddrSize     = LL_I2C_OWNADDRESS1_7BIT;
    LL_I2C_Init(I2C1, &I2C_InitStruct);
    LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
}

uint8_t I2C1_WriteMemAddr(uint16_t mem_address, uint16_t mem_size)
{
    uint32_t timeout    = 0;
    uint8_t  MemAddrLSB = I2C_MEM_ADD_LSB(mem_address);
    uint8_t  MemAddrMSB = I2C_MEM_ADD_MSB(mem_address);

    // 发送器件寄存器地址
    if (mem_size == I2C_MEMADD_SIZE_8BIT) {
        LL_I2C_TransmitData8(I2C1, MemAddrLSB);
    } else {
        LL_I2C_TransmitData8(I2C1, MemAddrMSB);
        // 等待发送就绪
        timeout = 0;
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1)) {
            timeout++;
            if (timeout > I2C_TIMEOUT_COUNT) {
                return I2C_TIMEOUT_ERR;
            }
        }
        LL_I2C_TransmitData8(I2C1, MemAddrLSB);
    }
}

uint8_t I2C1_WriteMultipleData(uint8_t *pdata, uint32_t length)
{
    uint8_t  count;
    uint32_t timeout = 0;
    for (count = 0; count < length; count++) {
        timeout = 0;
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1)) {
            timeout++;
            if (timeout > I2C_TIMEOUT_COUNT) {
                DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
                return I2C_TIMEOUT_ERR;
            }
        }
        LL_I2C_TransmitData8(I2C1, *(pdata + count));
    }
    return I2C_SUCCESS;
}

uint8_t I2C1_ReadMultipleData(uint8_t *pdata, uint32_t length)
{
    uint8_t  count;
    uint32_t timeout = 0;
    for (count = 0; count < length; count++) {
        // 等待接收非空
        timeout = 0;
        while (!LL_I2C_IsActiveFlag_RXNE(I2C1)) {
            timeout++;
            if (timeout > I2C_TIMEOUT_COUNT) {
                DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
                return I2C_TIMEOUT_ERR;
            }
        }
        *(pdata + count) = LL_I2C_ReceiveData8(I2C1);
    }
    return I2C_SUCCESS;
}

uint8_t I2C1_MasterWrite(uint8_t device_7BitAddr, uint16_t mem_address, uint16_t mem_size, uint8_t *pdata, uint32_t length)
{
    uint32_t timeout = 0;
    uint8_t  status  = I2C_SUCCESS;

    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1)) {
        timeout++;
        if (timeout > I2C_TIMEOUT_COUNT) {
            DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
            return I2C_TIMEOUT_ERR;
        }
    }

    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置为写
    LL_I2C_HandleTransfer(I2C1, device_7BitAddr << 1, LL_I2C_ADDRSLAVE_7BIT, mem_size, I2C_RELOAD_MODE, I2C_GENERATE_START_WRITE);

    // 等待发送就绪
    timeout = 0;
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1)) {
        timeout++;
        if (timeout > I2C_TIMEOUT_COUNT) {
            DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
            return I2C_TIMEOUT_ERR;
        }
    }

    // 写入从设备寄存器地址
    I2C1_WriteMemAddr(mem_address, mem_size);

   timeout = 0;
   while (!LL_I2C_IsActiveFlag_TCR(I2C1)) {
       timeout++;
       if (timeout > I2C_TIMEOUT_COUNT) {
           DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
           return I2C_TIMEOUT_ERR;
       }
   }

    // 重新配置为自动停止模式
    LL_I2C_HandleTransfer(I2C1, device_7BitAddr << 1, LL_I2C_ADDRSLAVE_7BIT, length, I2C_AUTOEND_MODE, I2C_NO_STARTSTOP);

    // 写数据
    I2C1_WriteMultipleData(pdata, length);

    // 等待停止完成
    timeout = 0;
    while (!LL_I2C_IsActiveFlag_STOP(I2C1)) {
        timeout++;
        if (timeout > I2C_TIMEOUT_COUNT) {
            DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
            return I2C_TIMEOUT_ERR;
        }
    }

    LL_I2C_ClearFlag_STOP(I2C1);

    return 0;
}

uint8_t I2C1_MasterRead(uint8_t device_7BitAddr, uint16_t mem_address, uint16_t mem_size, uint8_t *pdata, uint32_t length)
{
    uint32_t timeout = 0;
    uint8_t  status  = I2C_SUCCESS;

    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1)) {
        timeout++;
        if (timeout > I2C_TIMEOUT_COUNT) {
            DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
            return I2C_TIMEOUT_ERR;
        }
    }

    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置
    LL_I2C_HandleTransfer(I2C1, device_7BitAddr << 1, LL_I2C_ADDRSLAVE_7BIT, mem_size, I2C_SOFTEND_MODE, I2C_GENERATE_START_WRITE);

    // 等待发送就绪
    timeout = 0;
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1)) {
        timeout++;
        if (timeout > I2C_TIMEOUT_COUNT) {
            DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
            return I2C_TIMEOUT_ERR;
        }
    }
    // 写入从设备寄存器地址
    I2C1_WriteMemAddr(mem_address, mem_size);

   timeout = 0;
   while (!LL_I2C_IsActiveFlag_TC(I2C1)) {
       timeout++;
       if (timeout > I2C_TIMEOUT_COUNT) {
           DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
           return I2C_TIMEOUT_ERR;
       }
   }

    // 配置
    LL_I2C_HandleTransfer(I2C1, (device_7BitAddr << 1) | 1, LL_I2C_ADDRSLAVE_7BIT, length, I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);

    // 读取数据
    I2C1_ReadMultipleData(pdata, length);

    // 等待停止完成
    timeout = 0;
    while (!LL_I2C_IsActiveFlag_STOP(I2C1)) {
        timeout++;
        if (timeout > I2C_TIMEOUT_COUNT) {
            DEBUG_TRACE(ERROR_TAG, "%s I2C_TIMEOUT", __func__);
            return I2C_TIMEOUT_ERR;
        }
    }

    LL_I2C_ClearFlag_STOP(I2C1);

    return 0;
}

void IIC_Config(void)
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    LL_GPIO_DeInit(GPIOB);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SDA_OUT()
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SDA_IN()
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Pin  = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SCL_OUT()
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SCL_IN()
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Pin  = LL_GPIO_PIN_8;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 *  功能：IIC开始信号 SCL保持高电平，SDA从高电平跳变到低电平
 *  入口参数：无
 *  返回值：无
 */
void IIC_Start(void)
{
    SDA_OUT();
    IIC_SDA_H;
    IIC_SCL_H;
    Delay_Us(4);
    IIC_SDA_L;
    Delay_Us(4);
    IIC_SCL_L; // 钳制IIC总线，准备发送或接受数据
}

/**
 *  功能：IIC结束信号 SCL保持高电平，SDA从低电平跳变到高电平
 *  入口参数：无
 *  返回值：无
 */
void IIC_Stop(void)
{
    SDA_OUT();
    IIC_SDA_L;
    IIC_SCL_L;
    Delay_Us(4);
    IIC_SCL_H;
    IIC_SDA_H;
    Delay_Us(4);
}

/**
 *  功能：等待应答信号ACK
 *  入口参数：无
 *  返回值：0，接受应答成功；1，接受应答失败
 */
u8 IIC_Wait_Ack(void)
{
    u32 ucErrTime = 0;
    SDA_IN();
    IIC_SDA_H;
    Delay_Us(1);
    IIC_SCL_H;
    Delay_Us(1);
    while (READ_SDA) {
        ucErrTime++;
        if (ucErrTime > 10000) {
            IIC_Stop();
            return 1;
        }
    }
    IIC_SCL_L;
    return 0;
}

/**
 *  功能：产生ACK应答
 *  入口参数：无
 *  返回值：无
 */
void IIC_Ack(void)
{
    IIC_SCL_L;
    SDA_OUT();
    IIC_SDA_L;
    Delay_Us(2);
    IIC_SCL_H;
    Delay_Us(2);
    IIC_SCL_L;
}

/**
 *  功能：产生非ACK应答
 *  入口参数：无
 *  返回值：无
 */
void IIC_NAck(void)
{
    IIC_SCL_L;
    SDA_OUT();
    IIC_SDA_H;
    Delay_Us(2);
    IIC_SCL_H;
    Delay_Us(2);
    IIC_SCL_L;
}

/**
 *  功能：IIC发送一个字节（8 bit）
 *  入口参数：无
 *  返回值：返回从机有无应答。0，无应答；1，有应答
 */
void IIC_Send_Byte(u8 txd)
{
    u8 t;
    SDA_OUT();
    IIC_SCL_L; // 拉低IIC总线时钟开始数据传输
    for (t = 0; t < 8; t++) {
        if (txd & 0x80)
            IIC_SDA_H;
        else
            IIC_SDA_L;
        txd <<= 1;
        Delay_Us(2);
        IIC_SCL_H;
        Delay_Us(2);
        IIC_SCL_L;
        Delay_Us(2);
    }
}

/**
 *  功能：IIC读取一个字节（8 bit）
 *  入口参数：ack，应答
 *  返回值：一个字节（8 bit）
 */
u8 IIC_Read_Byte(unsigned char ack)
{
    unsigned char i, receive = 0;
    SDA_IN();
    for (i = 0; i < 8; i++) {
        IIC_SCL_L;
        Delay_Us(2);
        IIC_SCL_H;
        receive <<= 1;
        if (READ_SDA)
            receive++;
        Delay_Us(1);
    }
    if (!ack)
        IIC_NAck(); // 发送NACK
    else
        IIC_Ack(); // 发送ACK
    return receive;
}

// u8 i2c_slave_read(u8 addr, u8 reg, u8 *data, int len)
//{
//     u8 temp = 0;
//     IIC_Start();
//     IIC_Send_Byte(addr << 1);
//     IIC_Wait_Ack();
//     IIC_Send_Byte(reg); //发送低地址
//     IIC_Wait_Ack();
//     IIC_Stop(); //产生一个停止条件
//     Delay_Ms(10);

//    IIC_Start();
//    IIC_Send_Byte((addr << 1) | 1); //进入接收模式
//    IIC_Wait_Ack();
//    temp = IIC_Read_Byte(0);
//    IIC_Stop(); //产生一个停止条件

//    *data = temp;
//    return 0;
//}

// u8 i2c_slave_write(u8 addr, u8 reg, u8 *data, int len)
//{
//     int i = 0;
//     IIC_Start();
//     IIC_Send_Byte(addr << 1); //发送器件地址0XA0,写数据

//    IIC_Wait_Ack();
//    IIC_Send_Byte(reg); //发送低地址
//    IIC_Wait_Ack();
//    for (int i = 0; i < len; i++)
//    {
//        IIC_Send_Byte(data[i++]); //发送字节
//        IIC_Wait_Ack();
//    }
//    IIC_Stop(); //产生一个停止条件
//    Delay_Ms(10);
//    return 0;
//}

u8 i2c_slave_read(u8 addr, u8 reg, u8 *bufp, int len)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置
    LL_I2C_HandleTransfer(I2C1, (addr << 1), LL_I2C_ADDRSLAVE_7BIT, 1, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE);

    // 等待发送完成
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
        ;
    // 写数据
    LL_I2C_TransmitData8(I2C1, reg);

    LL_I2C_HandleTransfer(I2C1, (addr << 1) | 1, LL_I2C_ADDRSLAVE_7BIT, len, I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);

    // 读取数据
    for (int i = 0; i < len; i++) {
        // 等待接收非空
        while (!LL_I2C_IsActiveFlag_RXNE(I2C1))
            ;
        *(bufp + i) = LL_I2C_ReceiveData8(I2C1);
    }

    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);

    return 0;
}

u8 i2c_slave_write(u8 addr, u8 reg, u8 *bufp, int len)
{
    int write_len = len + 1;
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置为写
    LL_I2C_HandleTransfer(I2C1, addr << 1, LL_I2C_ADDRSLAVE_7BIT, write_len, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE);
    // 等待发送完成
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
        ;
    LL_I2C_TransmitData8(I2C1, reg);
    // 写数据
    for (int i = 0; i < write_len - 1; i++) {
        // 等待发送完成
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
            ;
        LL_I2C_TransmitData8(I2C1, *(bufp + i));
    }
    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);
    return 0;
}
