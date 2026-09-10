
/* Includes ------------------------------------------------------------------*/
#include "iic.h"
#include "systick.h"

void I2C1_DeInit(void)
{
    LL_I2C_DeInit(I2C1);
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_9); // SDA
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_8);   // SCL

    Delay_Us(100);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9);
    Delay_Us(100);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_8);

    // LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_9); // SDA
    // LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_8); // SCL

    // Delay_Us(100);
    // LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9);
    // Delay_Us(100);
    // for (int i = 0; i < 9; i++) {
    //     LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_8);
    //     Delay_Us(100);
    //     LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_8);
    //     Delay_Us(100);
    // }
    Delay_Ms(10);
}

void I2C1_Init(void)
{
    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */

    LL_I2C_InitTypeDef I2C_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    /**I2C1 GPIO Configuration
    PB8   ------> I2C1_SCL
    PB9   ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /** I2C Initialization
     */
    LL_I2C_EnableAutoEndMode(I2C1);
    LL_I2C_DisableOwnAddress2(I2C1);
    LL_I2C_DisableGeneralCall(I2C1);
    LL_I2C_EnableClockStretching(I2C1);
    I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
    I2C_InitStruct.Timing = 0x00303D5B;
    I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
    I2C_InitStruct.DigitalFilter = 0;
    I2C_InitStruct.OwnAddress1 = 0;
    I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
    I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
    LL_I2C_Init(I2C1, &I2C_InitStruct);
    LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
}

void I2C1_WriteMemAddr(uint16_t mem_address, uint16_t mem_size)
{
    uint8_t MemAddrLSB = I2C_MEM_ADD_LSB(mem_address);
    uint8_t MemAddrMSB = I2C_MEM_ADD_MSB(mem_address);

    // 发送器件寄存器地址
    if (mem_size == I2C_MEMADD_SIZE_8BIT) {
        LL_I2C_TransmitData8(I2C1, MemAddrLSB);
    } else {
        LL_I2C_TransmitData8(I2C1, MemAddrMSB);
        // 等待发送完成
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
            ;
        LL_I2C_TransmitData8(I2C1, MemAddrLSB);
    }
}

void I2C1_WriteMultipleData(uint8_t *pdata, uint8_t length)
{
    uint8_t count;
    for (count = 0; count < length; count++) {
        // 等待发送完成
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
            ;
        LL_I2C_TransmitData8(I2C1, *(pdata + count));
    }
}

void I2C1_ReadMultipleData(uint8_t *pdata, uint8_t length)
{
    uint8_t count;
    for (count = 0; count < length; count++) {
        // 等待接收非空
        while (!LL_I2C_IsActiveFlag_RXNE(I2C1))
            ;
        *(pdata + count) = LL_I2C_ReceiveData8(I2C1);
    }
}

void I2C1_MasterWrite(uint8_t device_7BitAddr, uint16_t mem_address, uint16_t mem_size, uint8_t *pdata, uint8_t length)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置为写
    LL_I2C_HandleTransfer(I2C1, device_7BitAddr << 1, LL_I2C_ADDRSLAVE_7BIT, mem_size, I2C_RELOAD_MODE, I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXE(I2C1))
        ;

    // 写入从设备寄存器地址
    I2C1_WriteMemAddr(mem_address, mem_size);
    while (!LL_I2C_IsActiveFlag_TCR(I2C1))
        ;

    // 重新配置为自动停止模式
    LL_I2C_HandleTransfer(I2C1, device_7BitAddr << 1, LL_I2C_ADDRSLAVE_7BIT, length, I2C_AUTOEND_MODE, I2C_NO_STARTSTOP);

    // 写数据
    I2C1_WriteMultipleData(pdata, length);

    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);
}

void I2C1_MasterRead(uint8_t device_7BitAddr, uint16_t mem_address, uint16_t mem_size, uint8_t *pdata, uint8_t length)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置
    LL_I2C_HandleTransfer(I2C1, device_7BitAddr << 1, LL_I2C_ADDRSLAVE_7BIT, mem_size, I2C_SOFTEND_MODE, I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXE(I2C1))
        ;

    // 写入从设备寄存器地址
    I2C1_WriteMemAddr(mem_address, mem_size);
    while (!LL_I2C_IsActiveFlag_TC(I2C1))
        ;

    // 配置
    LL_I2C_HandleTransfer(I2C1, (device_7BitAddr << 1) | 1, LL_I2C_ADDRSLAVE_7BIT, length, I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);

    // 读取数据
    I2C1_ReadMultipleData(pdata, length);

    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);
}

u8 i2c_slave_read(u8 addr, u8 reg, u8 *data, int len)
{
    I2C1_MasterRead(addr, reg, 1, data, len);
    return 0;
}

u8 i2c_slave_write(u8 addr, u8 reg, u8 *data, int len)
{
    I2C1_MasterWrite(addr, reg, 1, data, len);
    return 0;
}

int8_t i2c_hal_read(uint8_t address, uint8_t *data, uint16_t count)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置
    LL_I2C_HandleTransfer(I2C1, (address << 1) | 1, LL_I2C_ADDRSLAVE_7BIT, count, I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);

    // 读取数据
    for (int i = 0; i < count; i++) {
        // 等待接收非空
        while (!LL_I2C_IsActiveFlag_RXNE(I2C1))
            ;
        *(data + i) = LL_I2C_ReceiveData8(I2C1);
    }

    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);

    return 0;
}

int8_t i2c_hal_write(uint8_t address, const uint8_t *data, uint16_t count)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置为写
    LL_I2C_HandleTransfer(I2C1, address << 1, LL_I2C_ADDRSLAVE_7BIT, count, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
        ;
    // 写数据
    for (int i = 0; i < count; i++) {
        // 等待发送完成
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
            ;
        LL_I2C_TransmitData8(I2C1, *(data + i));
    }
    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);

    return 0;
}

int8_t i2c_hal_write_read(uint8_t address, uint8_t *tdata, uint16_t tcount, uint8_t *rdata, uint16_t rcount)
{
    // 等待iic空闲
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        ;
    // 确认iic是否使能
    if (!LL_I2C_IsEnabled(I2C1)) {
        LL_I2C_Enable(I2C1);
    }
    // 配置为写
    LL_I2C_HandleTransfer(I2C1, address << 1, LL_I2C_ADDRSLAVE_7BIT, tcount, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
        ;
    // 写数据
    for (int i = 0; i < tcount; i++) {
        // 等待发送完成
        while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
            ;
        LL_I2C_TransmitData8(I2C1, *(tdata + i));
    }

    // 配置
    LL_I2C_HandleTransfer(I2C1, (address << 1) | 1, LL_I2C_ADDRSLAVE_7BIT, rcount, I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);

    // 读取数据
    for (int i = 0; i < rcount; i++) {
        // 等待接收非空
        while (!LL_I2C_IsActiveFlag_RXNE(I2C1))
            ;
        *(rdata + i) = LL_I2C_ReceiveData8(I2C1);
    }

    // 等待停止完成
    while (!LL_I2C_IsActiveFlag_STOP(I2C1))
        ;
    LL_I2C_ClearFlag_STOP(I2C1);

    return 0;
}

uint16_t i2c_add_command_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t command)
{
    buffer[offset++] = (uint8_t)((command & 0xFF00) >> 8);
    buffer[offset++] = (uint8_t)((command & 0x00FF) >> 0);
    return offset;
}
