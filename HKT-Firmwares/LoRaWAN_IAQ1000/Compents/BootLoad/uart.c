
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "uart.h"
#include "flash.h"
#include "crc.h"
#include "systick.h"
#include "gpio.h"

uart_t uartInfo;
u32 flashbuf[FLASH_ONE_PAGE_SIZE / 4];
time_t stamp_timeout;

/**
 * @brief  串口0接收中断事件,中断服务函数
 * @param
 * @retval
 */
void UART0_IRQHandler(void)
{
    // 接收中断处理
    if ((ENABLE == UARTx_IER_RXBF_IE_Getable(UART0)) && (SET == UARTx_ISR_RXBF_Chk(UART0)))
    {
        u8 DataReceived = UARTx_RXBUF_Read(UART0); // 读取出来接收到的数据,接收中断标志仅可通过读取RXBUF寄存器清除
        if (uartInfo.recvLen < uart_recv_max_len)
        {
            uartInfo.recvData[uartInfo.recvLen++] = DataReceived;
            uartInfo.recvTimeout = 100 / SYSTEM_TIMER_BASE; // 100ms超时
        }
    }

    // 发送中断处理
    if ((ENABLE == UARTx_IER_TXSE_IE_Getable(UART0)) && (SET == UARTx_ISR_TXSE_Chk(UART0)))
    {
        // 发送中断标志可通过写TXBUF寄存器清除或TXSE写1清除
        // 发送指定长度的数据
        if (uartInfo.txIndex < uartInfo.sendLen)
        {
            UARTx_TXBUF_Write(UART0, uartInfo.sendData[uartInfo.txIndex]); // 发送一个数据
            uartInfo.txIndex++;
        }
        UARTx_ISR_TXSE_Clr(UART0); // 清除发送中断标志
    }
}

/**
 * @brief  异步串口收发器初始化，默认用来打印日志信息
 * @param   UARTx: 串口号
 * @param   BaudRate: 波特率
 * @retval
 */
void Uartx_Init(UART_Type *UARTx, u32 BaudRate)
{

    UART_SInitTypeDef USART_InitStruct;
    CMU_ClocksType CMU_Clocks;
    u8 uartNumber;

    CMU_PERCLK_SetableEx(PADCLK, ENABLE);
    if (((uint32_t)UARTx - UART0_BASE) == 0)
        uartNumber = 0;
    else
        uartNumber = ((((uint32_t)UARTx - UART1_BASE) >> 10) + 1); // 获取uart

    switch (uartNumber)
    {
    case 0:                                                     // 进行UART0的初始化配置
        USART_InitStruct.ClockSrc = CMU_OPCCR1_UART0CKS_APBCLK; // UART0工作时钟选择
        /*UART0 IO 配置*/
        AltFunIO(GPIOF, GPIO_Pin_3, 0); // PF3 UART0 RX
        AltFunIO(GPIOF, GPIO_Pin_4, 0); // PF4 UART0 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART0_IRQn);
        NVIC_SetPriority(UART0_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART0_IRQn);
        break;
    case 1: // 进行UART1的初始化配置
        USART_InitStruct.ClockSrc = CMU_OPCCR1_UART1CKS_APBCLK;

        /*UART1 IO 配置*/
        AltFunIO(GPIOE, GPIO_Pin_3, 0); // PE3 UART1 RX
        AltFunIO(GPIOE, GPIO_Pin_4, 0); // PE4 UART1 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART1_IRQn);
        NVIC_SetPriority(UART1_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART1_IRQn);
        break;

    case 2: // 进行UART2的初始化配置
        /*UART2 IO 配置*/
        AltFunIO(GPIOB, GPIO_Pin_2, 0); // PB2 UART2 RX
        AltFunIO(GPIOB, GPIO_Pin_3, 0); // PB3 UART2 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART2_IRQn);
        NVIC_SetPriority(UART2_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART2_IRQn);
        break;
    case 3: // 进行UART3的初始化配置
        /*UART3 IO 配置*/
        AltFunIO(GPIOC, GPIO_Pin_10, 0); // PC10 UART3 RX
        AltFunIO(GPIOC, GPIO_Pin_11, 0); // PC11 UART3 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART3_IRQn);
        NVIC_SetPriority(UART3_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART3_IRQn);
        break;

    case 4: // 进行UART4的初始化配置
        /*UART4 IO 配置*/
        AltFunIO(GPIOD, GPIO_Pin_0, 0); // PD0 UART4 RX
        AltFunIO(GPIOD, GPIO_Pin_1, 0); // PD1 UART4 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART4_IRQn);
        NVIC_SetPriority(UART4_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART4_IRQn);
        break;

    case 5: // 进行UART5的初始化配置
        /*UART5 IO 配置*/
        AltFunIO(GPIOA, GPIO_Pin_8, 0); // PA8 UART5 RX
        AltFunIO(GPIOA, GPIO_Pin_9, 0); // PA9 UART5 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART5_IRQn);
        NVIC_SetPriority(UART5_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART5_IRQn);
        break;

    default:
        break;
    }

    // UART初始化配置
    USART_InitStruct.BaudRate = BaudRate; // 波特率
    USART_InitStruct.DataBit = Eight8Bit; // 数据位数
    USART_InitStruct.ParityBit = NONE;    // 奇偶校验
    USART_InitStruct.StopBit = OneBit;    // 停止位

    CMU_GetClocksFreq(&CMU_Clocks);
    UART_SInit(UARTx, &USART_InitStruct, &CMU_Clocks); // 初始化uart

    UARTx_IER_RXBF_IE_Setable(UARTx, ENABLE); // 接收中断使能
    UARTx_CSR_RXEN_Setable(UARTx, ENABLE);    // 接收使能
    UARTx_CSR_TXEN_Setable(UARTx, ENABLE);    // 发送使能
    UARTx->ISR = 0xFFFFFFFF;                  // 清除标志
}

/**
 * @brief  通过USART0外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size)
    {
        /* Send one byte from UART0 to USARTx */
        UARTx_TXBUF_Write(UART0, buffptr[TxCounter++]);
        /* Loop until register is empty */
        while (!UARTx_ISR_TXBE_Chk(UART0))
            ;
    }
}

/**
 * @brief  格式化打印日志信息，打印字符串最大限制长度在600字节以内
 * @param  str：待发送字符串源地址
 * @retval
 */
void INFO(const char *str, ...)
{
    static unsigned char s[2200]; // This needs to be large enough to store the string TODO Change magic number
    unsigned char *p;
    va_list args;
    int str_len;

    if ((str == NULL) || (strlen(str) == 0))
    {
        return;
    }

    va_start(args, str);
    str_len = (unsigned int)vsprintf((char *)s, str, args);
    va_end(args);
    p = s;

    INFO_SendData(p, str_len);
}

void DebugHexInfo(char *tag, u8 *data, int len)
{
    INFO("\nUart Data [%s]:", tag);
    for (int i = 0; i < len; i++)
    {
        INFO(" %02x", data[i]);
    }
    INFO("\n");
}

u8 start_flag;
u16 flash_write_count; // flash 写入计数
u32 firmware_size;
u32 firmware_write_size;

void InfoUartAck(u8 cmd, u8 data)
{
    /*hkt cmd data bootload*/
    u8 buffer[] = {0x68, 0x6B, 0x74, 0x00, 0x00, 0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61, 0x64};
    buffer[3] = cmd;
    buffer[4] = data;
    INFO_SendData(buffer, sizeof(buffer) / sizeof(u8));
}

void reget_flash_data(void)
{
    if (stamp_timeout < Timestamp && start_flag == 1)
    {
        stamp_timeout = Timestamp + 1;
        InfoUartAck(2, flash_write_count);
    }
}

__IO uint32_t AppSpInitVal;
__IO uint32_t AppJumpAddr;
void (*pAppFun)(void); // 定义一个函数指针.用于指向APP程序入口.

/* 跳转到应用程序位置 */
void AppProgramRun(void)
{
    AppSpInitVal = *(__IO uint32_t *)(FLASH_APP_START_ADDR);    // 取APP的SP初值.
    AppJumpAddr = *(__IO uint32_t *)(FLASH_APP_START_ADDR + 4); // 取程序入口.
    if ((AppSpInitVal & 0x2FFE0000) == 0x20000000)              // 检查栈顶地址是否合法.
    {
        __set_MSP(AppSpInitVal);               // 设置SP.
        pAppFun = (void (*)(void))AppJumpAddr; // 生成跳转函数.
        (*pAppFun)();                          // 跳转.不再返回.
    }
}

/**
 * @brief  处理来自debug口的数据
 * @param
 * @retval
 */
void fromInfoDataHandle(void)
{
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        u8 *data = uartInfo.recvData;
        u16 len = uartInfo.recvLen;

        /* 确认为上位机通过串口下发数据 */
        // if (strstr((const char *)data, "hkt") && strstr((const char *)data, "bootload"))
        if (data[0] == 0x68 && data[1] == 0x6B && data[2] == 0x74)
        {
            /* hkt(3) len(2)(cmd+packnum+data) cmd(1) data(n) crc(2) bootload(8) */
            u16 buflen = (data[3] << 8 | data[4]);
            u8 cmd = data[5];
            // 校验数据包长度
            if (len - 15 == buflen)
            {
                u8 *buffer = data + 5;
                u16 crc = CalCRC16_CCITT(0, buffer, buflen);
                if (crc == (data[len - 10] << 8 | data[len - 9]))
                {
                    DEBUG_TRACE(LOG_TAG, "BootLoad Event: %d, Crc: %d", cmd, crc);
                    // buflen += 2;
                    switch (cmd)
                    {
                    case 1:
                        firmware_size = data[6] << 24 | data[7] << 16 | data[8] << 8 | data[9];
                        flash_write_count = 0;
                        firmware_write_size = 0;
                        DEBUG_TRACE(LOG_TAG, "Update Start, Firmware Length: %dbytes", firmware_size);
                        InfoUartAck(cmd + 1, flash_write_count);
                        start_flag = 1;
                        break;
                    case 2:
                        buffer += 1;
                        if (flash_write_count == buffer[0])
                        {
                            memset(&flashbuf, 0, FLASH_ONE_PAGE_SIZE);
                            u32 temp;
                            u8 *dfu = buffer + 1;
                            u16 i = 0;
                            for (int t = 0; t < buflen - 2; t += 4) // 上位机下发除最后一包外，其余每包固定长度2048bytes
                            {
                                temp = dfu[t + 3];
                                temp <<= 8;
                                temp |= dfu[t + 2];
                                temp <<= 8;
                                temp |= dfu[t + 1];
                                temp <<= 8;
                                temp |= dfu[t];
                                flashbuf[i++] = temp;
                            }
                            u32 Address = FLASH_ONE_PAGE_SIZE * flash_write_count + FLASH_APP_START_ADDR;
                            FlashPageErase(Address);
                            FlashPageErase(Address + MCU_PAGE_SIZE);
                            FlashPageErase(Address + MCU_PAGE_SIZE * 2);
                            FlashPageErase(Address + MCU_PAGE_SIZE * 3);
                            if (!FLASH_WriteCheck_NWord(Address, flashbuf, (buflen - 2) / 4))
                            {
                                firmware_write_size += (buflen - 2);
                                mDelay(100);
                                InfoUartAck(cmd, ++flash_write_count);
                                stamp_timeout = Timestamp + 1;
                            }
                            else
                                stamp_timeout = Timestamp;
                        }
                        else
                        {
                            // InfoUartAck(cmd, 0); //计数错误从0开始重新更新
                            InfoUartAck(cmd, flash_write_count); // 重新获取
                        }
                        DEBUG_TRACE(LOG_TAG, "Write[%d] Firmware Length: %dbytes", flash_write_count, firmware_write_size);
                        break;
                    case 3:
                    {
                        start_flag = 0;
                        DEBUG_TRACE(LOG_TAG, "Update Finish");
                        InfoUartAck(cmd, 0);
                        mDelay(100);
                        __disable_irq(); // 关闭全局中断使能

                        CMU_PERCLK_SetableEx(PADCLK, DISABLE);
                        CloseIO(LED_GREEN_PORT, LED_GREEN_PIN);
                        CloseIO(LED_ORANGE_PORT, LED_ORANGE_PIN);
                        CloseIO(LED_RED_PORT, LED_RED_PIN);

                        NVIC_DisableIRQ(BSTIM_IRQn);
                        BSTIM_CR1_CEN_Setable(DISABLE); // 关闭定时器
                        CMU_PERCLK_SetableEx(BSTIMCLK, DISABLE);

                        NVIC_DisableIRQ(UART0_IRQn);
                        CloseIO(GPIOF, GPIO_Pin_3);                // PF3 UART0 RX
                        CloseIO(GPIOF, GPIO_Pin_4);                // PF4 UART0 TX
                        UARTx_IER_TXSE_IE_Setable(UART0, DISABLE); // 关闭发送中断
                        UARTx_IER_RXBF_IE_Setable(UART0, DISABLE); // 关闭接收中断
                        UARTx_CSR_RXEN_Setable(UART0, DISABLE);    // 关闭接收使能
                        UARTx_CSR_TXEN_Setable(UART0, DISABLE);    // 关闭发送使能

                        CMU_PERCLK_SetableEx(CRCCLK, DISABLE);
                        AppProgramRun();
                    }
                    break;
                    default:
                        DEBUG_TRACE(WARN_TAG, "Unknow Cmd");
                        break;
                    }
                }
                else
                {
                    DEBUG_TRACE(ERROR_TAG, "Recv Crc ERROR: %d, %d", crc, data[len - 9]);
                }
            }
            else
            {
                DEBUG_TRACE(ERROR_TAG, "Recv Len ERROR Buffer Len: %d, Data Len: %d", buflen, len);
            }
        }
        else
        {
            DEBUG_TRACE(WARN_TAG, "Recv Unknow Data");
        }
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
    reget_flash_data();
}
