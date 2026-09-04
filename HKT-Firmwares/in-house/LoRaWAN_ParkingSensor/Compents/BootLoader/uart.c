
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "crclib.h"
#include "flash.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

struct uart uartInfo;
struct uart uartBle;

/**
 * @brief  串口3接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART3_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART3)) // 检测是否接收中断
    {
        u8 DataReceived = LL_USART_ReceiveData8(USART3); // 读取出来接收到的数据
        if (uartInfo.recvLen < uart_recv_max_len) {
            uartInfo.recvData[uartInfo.recvLen++] = DataReceived;
            uartInfo.recvTimeout = 20; // 20ms超时
        }
    }
    if (LL_USART_IsActiveFlag_PE(USART3)) {
        LL_USART_ClearFlag_PE(USART3);
    }
    if (LL_USART_IsActiveFlag_ORE(USART3)) {
        LL_USART_ClearFlag_ORE(USART3);
    }
}

/**
 * @brief  失能异步串口2收发器
 * @param
 * @retval
 */
void App_Uart3_Deinit(void)
{
    LL_USART_DeInit(USART3);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    NVIC_DisableIRQ(USART3_IRQn);
}

/**
 * @brief  异步串口3收发器初始化，波特率115200，默认用来打印日志信息
 * @param
 * @retval
 */
void App_Uart3Cfg(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_PCLK1);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    /**USART3 GPIO Configuration
    PC4   ------> USART3_TX
    PC5   ------> USART3_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 1));
    NVIC_EnableIRQ(USART3_IRQn);

    /* USER CODE END USART3_Init 1 */
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;

    LL_USART_Init(USART3, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART3);
    LL_USART_DisableOverrunDetect(USART3);
    LL_USART_DisableDMADeactOnRxErr(USART3);
    LL_USART_Enable(USART3);

    LL_USART_EnableIT_RXNE(USART3);
    LL_USART_EnableIT_PE(USART3);
    LL_USART_ClearFlag_PE(USART3);
}

/**
 * @brief  串口1接收中断事件,中断服务函数
 * @param
 * @retval
 */
void LPUART1_IRQHandler(void)
{
    if (LL_LPUART_IsActiveFlag_RXNE(LPUART1)) // 检测是否接收中断
    {
        u8 DataReceived = LL_LPUART_ReceiveData8(LPUART1); // 读取出来接收到的数据
        if (uartBle.recvLen < uart_recv_max_len - 1) {
            uartBle.recvData[uartBle.recvLen++] = DataReceived;
            uartBle.recvTimeout = 50; // 50ms超时
        }
    }
    if (LL_LPUART_IsActiveFlag_PE(LPUART1)) {
        LL_LPUART_ClearFlag_PE(LPUART1);
    }
    if (LL_LPUART_IsActiveFlag_ORE(LPUART1)) {
        LL_LPUART_ClearFlag_ORE(LPUART1);
    }
}

void App_LpUart1DeInit(void)
{
    LL_LPUART_DeInit(LPUART1);
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    /**LPUART1 GPIO Configuration
    PB10   ------> LPUART1_TX
    PB11   ------> LPUART1_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    NVIC_DisableIRQ(LPUART1_IRQn);
}

/**
 * @brief  低功耗串口1收发器初始化，波特率115200，用于蓝牙芯片通讯
 * @param
 * @retval
 */
void App_LpUart1Cfg(void)
{
    LL_LPUART_InitTypeDef LPUART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    /**LPUART1 GPIO Configuration
    PB10   ------> LPUART1_TX
    PB11   ------> LPUART1_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_10 | LL_GPIO_PIN_11;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_8;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    // LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* LPUART1 interrupt Init */
    NVIC_SetPriority(LPUART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(LPUART1_IRQn);

    LPUART_InitStruct.BaudRate = 115200;
    LPUART_InitStruct.DataWidth = LL_LPUART_DATAWIDTH_8B;
    LPUART_InitStruct.StopBits = LL_LPUART_STOPBITS_1;
    LPUART_InitStruct.Parity = LL_LPUART_PARITY_NONE;
    LPUART_InitStruct.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
    LPUART_InitStruct.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
    LL_LPUART_Init(LPUART1, &LPUART_InitStruct);

    LL_LPUART_EnableIT_RXNE(LPUART1);
    LL_LPUART_EnableIT_PE(LPUART1);
    LL_LPUART_ClearFlag_PE(LPUART1);

    // 使能LPUART1
    LL_LPUART_Enable(LPUART1);
    // 查询LPUART1初始化
    while (!(LL_LPUART_IsActiveFlag_TEACK(LPUART1)) || (!(LL_LPUART_IsActiveFlag_REACK(LPUART1)))) {
    }
}

/**
 * @brief  通过LPUART1外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void BLE_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    // DEBUG_TRACE(LOG_TAG,"Send LoRa Data Len : %d, ",buffer_size);
    while (TxCounter < buffer_size) {
        // INFO("%02x ",buffptr[TxCounter]);
        /* Send one byte */
        LL_LPUART_TransmitData8(LPUART1, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_LPUART_IsActiveFlag_TC(LPUART1))
            ;
    }
    // if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
    //     device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    // batteryInitialCapacity++; // 按发包次数减少电量信息
}

/**
 * @brief  通过LPUART1外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void BLE_SendCMD(const char *cmd)
{
    BLE_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART3外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* Send one byte from USART2 to USARTx */
        LL_USART_TransmitData8(USART3, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_USART_IsActiveFlag_TC(USART3))
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
    static unsigned char s[1100]; // This needs to be large enough to store the string TODO Change magic number
    unsigned char *p;
    va_list args;
    int str_len;

    if ((str == NULL) || (strlen(str) == 0)) {
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
    for (int i = 0; i < len; i++) {
        INFO(" %02x", data[i]);
    }
    INFO("\n");
}

u8 start_flag;
u8 recv_fail_cnt;
u16 flash_write_count; // flash 写入计数
u16 flash_cache_count;
u32 firmware_size;
u32 firmware_write_size;
u8 firmware_write_count;
uint64_t flashbuf[FLASH_ONE_PAGE_SIZE / 8];
time_t stamp_timeout;

void InfoUartAck(u8 cmd, u16 data)
{
    /*hkt cmd data bootload*/
    u8 buffer[] = {0x68, 0x6B, 0x74, 0x00, 0x00, 0x00, 0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61, 0x64};
    buffer[3] = cmd;
    buffer[4] = (data >> 8) & 0xFF;
    buffer[5] = data & 0xFF;
    BLE_SendData(buffer, sizeof(buffer) / sizeof(u8));
}

void reget_flash_data(void)
{
    if (stamp_timeout < Timestamp && start_flag == 1) {
        stamp_timeout = Timestamp + 1;
        recv_fail_cnt++;
        if (recv_fail_cnt > 10) {
            recv_fail_cnt = 0;
            flash_write_count = 0;
            flash_cache_count = 0;
            firmware_write_count = 0;
            start_flag = 0;
            InfoUartAck(1, flash_write_count);
            memset(&flashbuf, 0, FLASH_ONE_PAGE_SIZE);
        } else {
            InfoUartAck(2, flash_write_count);
        }
    }
}

/**
 * @brief  处理来自debug口的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromBleDataHandle(u8 *data, u16 len)
{
    /* 确认为上位机通过串口下发数据 */
    // if (strstr((const char *)data, "hkt") && strstr((const char *)data, "bootload"))
    if (data[0] == 0x68 && data[1] == 0x6B && data[2] == 0x74) {
        u16 buflen = (data[3] << 8 | data[4]);
        u8 cmd = data[5];
        // 校验数据包长度
        if (len - 15 == buflen) {
            u8 *buffer = data + 5;
            u16 crc = crc16_ccitt(buffer, buflen);
            if (crc == (data[len - 10] << 8 | data[len - 9])) {
                // DEBUG_TRACE(LOG_TAG, "BootLoad Event: %d, Crc: %04X", cmd, crc);
                switch (cmd) { // hkt(3) len(2)(cmd+data) cmd(1) data(4) crc(2) bootloader(8)
                case 1:
                    firmware_size = data[6] << 24 | data[7] << 16 | data[8] << 8 | data[9];
                    flash_write_count = 0;
                    flash_cache_count = 0;
                    firmware_write_size = 0;
                    DEBUG_TRACE(LOG_TAG, "Update Start, Firmware Length: %d bytes", firmware_size);
                    InfoUartAck(cmd + 1, flash_write_count);
                    start_flag = 1;
                    break;
                case 0xFF:
                case 2:
                    buffer += 1;
                    u16 count = buffer[0] << 8 | buffer[1];
                    if (flash_write_count == count) { // hkt(3) len(2)(cmd+packnum+data) cmd(1) packnum(2) data(n) crc(2) bootloader(8)
                        uint64_t temp = 0;
                        u8 *dfu = buffer + 2;
                        for (int t = 0; t < buflen - 3; t += 8) { // 上位机下发除最后一包外，其余每包固定长度128bytes
                            temp = dfu[t + 7];
                            temp <<= 8;
                            temp |= dfu[t + 6];
                            temp <<= 8;
                            temp |= dfu[t + 5];
                            temp <<= 8;
                            temp |= dfu[t + 4];
                            temp <<= 8;
                            temp |= dfu[t + 3];
                            temp <<= 8;
                            temp |= dfu[t + 2];
                            temp <<= 8;
                            temp |= dfu[t + 1];
                            temp <<= 8;
                            temp |= dfu[t];
                            flashbuf[flash_cache_count++] = temp;
                        }
                        if ((flash_cache_count == (FLASH_ONE_PAGE_SIZE / 8)) || cmd == 0xFF) {
                            u32 Address = FLASH_ONE_PAGE_SIZE * firmware_write_count + FLASH_APP_START_ADDR;
                            if (FLASH_Write_OTA_Data(Address, flashbuf, FLASH_ONE_PAGE_SIZE / 8) == SUCCESS) {
                                memset(&flashbuf, 0, FLASH_ONE_PAGE_SIZE);
                                firmware_write_count++;
                                firmware_write_size += flash_cache_count * 8;

                                if (cmd == 0xFF || firmware_write_size >= firmware_size) {
                                    DEBUG_TRACE(LOG_TAG, "Write Count[%d][%d] Firmware Write Flash Length: %d bytes, %d bytes", firmware_write_count, flash_write_count, firmware_write_size, firmware_size);
                                    InfoUartAck(3, 0);
                                    return;
                                }
                            }
                            flash_cache_count = 0;
                        }
                        recv_fail_cnt = 0;
                        // mDelay(100);
                        InfoUartAck(cmd, ++flash_write_count);
                        stamp_timeout = Timestamp + 1;
                    } else {
                        // mDelay(100);
                        // InfoUartAck(cmd, 0); //计数错误从0开始重新更新
                        InfoUartAck(cmd, flash_write_count); // 重新获取
                        stamp_timeout = Timestamp + 1;
                    }
                    DEBUG_TRACE(LOG_TAG, "Write Count[%d][%d] Firmware Write Flash Length: %d bytes, %d bytes", firmware_write_count, flash_write_count, firmware_write_size, firmware_size);
                    break;
                case 3: { // hkt(3) len(2)(cmd) cmd(1) crc(2) bootloader(8)
                __ota_finish:
                    start_flag = 0;
                    DEBUG_TRACE(LOG_TAG, "Update Finish");
                    InfoUartAck(cmd, 0);
                    mDelay(100);
                    LL_IWDG_ReloadCounter(IWDG);
                    App_Uart3_Deinit();
                    App_LpUart1DeInit();
                    LL_TIM_DeInit(TIM16);
                    NVIC_DisableIRQ(TIM1_UP_TIM16_IRQn);
                    NVIC_DisableIRQ(SysTick_IRQn);
                    memset(&flashbuf, 0, FLASH_ONE_PAGE_SIZE);
                    __disable_irq(); // 关闭全局中断使能
                    AppProgramRun();
                } break;
                default:
                    DEBUG_TRACE(WARN_TAG, "Unknow Cmd");
                    break;
                }
            } else {
                DEBUG_TRACE(ERROR_TAG, "Recv Crc ERROR: %d, %d", crc, data[len - 9]);
            }
        } else {
            DEBUG_TRACE(ERROR_TAG, "Recv Len ERROR Buffer Len: %d, Data Len: %d", buflen, len);
        }
    } else {
        DEBUG_TRACE(WARN_TAG, "Recv Unknow Data");
        if (flash_write_count > 0) {
            __disable_irq(); // 关闭全局中断使能
            memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
            uartBle.recvLen = 0;
            __enable_irq(); // 关闭全局中断使能
            mDelay(100);
            InfoUartAck(2, flash_write_count);
        }
    }
}

/**
 * @brief  处理来自debug口的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromInfoDataHandle(u8 *data, u16 len)
{
    char *valid;
    // 同步设备时间
    if (strstr((const char *)data, "syncDeviceTimestamp")) {
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
        if (stamp != 0) {
            stamp += 8 * 60 * 60;
            struct tm *timeP;
            timeP = localtime(&stamp); // 转换
            systime.tm_year = timeP->tm_year + 1900;
            systime.tm_mon = timeP->tm_mon + 1;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);
            getTimestamp();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else {
        DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
    }
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromUartDataHandle(void)
{
    if (!uartBle.recvTimeout && uartBle.recvLen) {
        // DEBUG_TRACE(LOG_TAG, "Recv Uart BLE Data: %s", uartBle.recvData);
        // DebugHexInfo("BLE Recv", uartBle.recvData, uartBle.recvLen);
        fromBleDataHandle(uartBle.recvData, uartBle.recvLen);
        memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
        uartBle.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
    reget_flash_data();
}
