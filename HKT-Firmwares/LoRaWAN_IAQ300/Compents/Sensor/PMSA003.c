#include "PMSA003.h"
#include "uart.h"
#include "control_center.h"

/*
pm2.5的空气质量可以分为6个等级
    每平米含有0~35ug的空气质量为优秀，
    每平米的空气含有35~75ug为良好，
    每平米的空气内含有75~115ug为轻度污染，
    每平米的空气含有115~150ug为重度污染
    每平米的空气含150~250ug为严重污染
    每平米的空气pm2.5含量大于250ug就是严重污染。
空气内的pm2.5含量在每平米115ug以为都为正常。
*/

const static u8 att_head[2] = {0x42, 0x4D};

/**
 * @brief  PM2.5 GPIO初始化
 * @param
 * @retval
 */
void PMSA003_PortCfg(void)
{
    CMU_OPCCR1_EXTICKSEL_Set(CMU_OPCCR1_EXTICKSEL_LSCLK); // EXTI中断采样时钟选择
    CMU_OPCCR1_EXTICKE_Setable(ENABLE);                   // EXTI工作时钟使能
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);                 // IO控制时钟寄存器使能

    /** 初始化 IO **/
    OutputIO(PMSA_SET_PORT, PMSA_SET_PIN, OUT_PUSHPULL);
    OutputIO(PMSA_RST_PORT, PMSA_RST_PIN, OUT_PUSHPULL);

    PMSA_SET_H;
    PMSA_RST_H;
}

// PM2.5传感器校验和计算
u16 PMSA_CRC_Calculate(u8 *data, u16 count)
{
    u16 sum = 0;
    for (int i = 0; i < count - 2; i++)
    {
        sum += data[i];
    }
    return sum;
}

/**
 * @brief  按规定协议封成数据包
 * @param  cmd：指令数据
 * @param  data：特征数据
 * @retval
 */
void sendPMSA_DataPackage(u16 cmd, u16 data)
{
    u8 dest[8] = {0};
    u8 dataLen = 8;

    /*  同步头  */
    memcpy(dest, att_head, 2);
    dest[2] = cmd << 8;
    dest[3] = cmd & 0xFF;

    dest[4] = data << 8;
    dest[5] = data & 0xFF;

    /*  校验  */
    u16 crc = PMSA_CRC_Calculate(dest, dataLen);
    dest[6] = crc << 8;
    dest[7] = crc & 0xFF;
    PMSA_SendData(dest, dataLen);
}

//读取数据
void PMSA003_SendReadCmd(void)
{
    // sendPMSA_DataPackage(0xE2, 0);
    PMSA_SET_H;
}

// PM2.5 传感器初始化
void PMSA003_Init(void)
{
    Uartx_Init(UART2, 9600);
    PMSA003_PortCfg();
    tx_thread_sleep(100);
    sendPMSA_DataPackage(0xE4, 1); //切换为正常模式
    tx_thread_sleep(100);
    // sendPMSA_DataPackage(0xE1, 0); //被动式度数
    sendPMSA_DataPackage(0xE1, 1); //主动式度数
}

// PM2.5 传感器串口数据处理
void PMSA_Cmd_Process(u8 *cmd, u16 cmd_size)
{
    u16 cmd_len;
    u16 cf_pm_1_0, cf_pm_2_5, cf_pm_10_0;                                                          // PM 浓度（CF=1，标准颗粒物）μg/m3
    u16 atmos_pm_1_0, atmos_pm_2_5, atmos_pm_10_0;                                                 // PM 浓度（大气环境下） μg/m3
    u16 number_pm_0_3, number_pm_0_5, number_pm_1_0, number_pm_2_5, number_pm_5_0, number_pm_10_0; // 0.1 升空气中直径在 xum 以上颗粒物个数
    u8 version, error_code;
    if (!memcmp(cmd, att_head, 2) && cmd_size == 32) //校验数据同步头
    {
        cmd_len = cmd[2] << 8 | cmd[3];
        //  DEBUG_TRACE(LOG_TAG, "PMSA Read Cmd Len: %d", cmd_len);
        if (cmd_len != 28) //数据长度异常
            return;
        cf_pm_1_0 = cmd[4] << 8 | cmd[5];
        cf_pm_2_5 = cmd[6] << 8 | cmd[7];
        cf_pm_10_0 = cmd[8] << 8 | cmd[9];
        atmos_pm_1_0 = cmd[10] << 8 | cmd[11];
        atmos_pm_2_5 = cmd[12] << 8 | cmd[13];
        atmos_pm_10_0 = cmd[14] << 8 | cmd[15];

        number_pm_0_3 = cmd[16] << 8 | cmd[17];
        number_pm_0_5 = cmd[18] << 8 | cmd[19];
        number_pm_1_0 = cmd[20] << 8 | cmd[21];
        number_pm_2_5 = cmd[22] << 8 | cmd[23];
        number_pm_5_0 = cmd[24] << 8 | cmd[25];
        number_pm_10_0 = cmd[26] << 8 | cmd[27];

        version = cmd[28];
        error_code = cmd[29];

        u16 crc = PMSA_CRC_Calculate(cmd, cmd_size);
        if (crc == (cmd[30] << 8 | cmd[31]))
        {
            device_t.sensor_t.atmos_pm_1_0 = atmos_pm_1_0;
            device_t.sensor_t.atmos_pm_2_5 = atmos_pm_2_5;
            device_t.sensor_t.atmos_pm_10_0 = atmos_pm_10_0;
            PMSA_SET_L; //休眠
#if PMSA_DEBUG_PRINTF
            DEBUG_TRACE(LOG_TAG, "PMSA Read Data, Cmd Len: %d", cmd_len);
            INFO("\n\t CF=1, PM1.0: %dug/m3, PM2.5: %dug/m3, PM10.0: %dug/m3", cf_pm_1_0, cf_pm_2_5, cf_pm_10_0);
            INFO("\n\t Atmos, PM1.0: %dug/m3, PM2.5: %dug/m3, PM10.0: %dug/m3", atmos_pm_1_0, atmos_pm_2_5, atmos_pm_10_0);
            INFO("\n\t CF=1, PM Number Count 0.3um: %dug/m3, 0.5um: %dug/m3, 1.0um: %dug/m3 ,2.5um: %dug/m3 ,5.0um: %dug/m3, 10um: %dug/m3",
                 number_pm_0_3, number_pm_0_5, number_pm_1_0, number_pm_2_5, number_pm_5_0, number_pm_10_0);
            INFO("\n\t Version: %d, Error Code: %d", version, error_code);
#endif
        }
        else
        {
            DEBUG_TRACE(ERROR_TAG, "PMSA Cmd Check CRC Error: %d", crc);
        }
    }
}
