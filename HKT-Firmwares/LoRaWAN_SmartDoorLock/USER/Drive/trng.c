#include "trng.h"

/* crc32实现函数 = CRC-32/MPEG-2       x32+x26+x23+x22+x16+x12+x11+x10+x8+x7+x5+x4+x2+x+1 */
uint32_t Calc_Crc32_MPEG2(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0XFFFFFFFF;
    uint32_t i = 0, j = 0;
    while ((len--) != 0)
    {
        crc ^= (uint32_t) * (data + j) << 24;
        j++;
        for (i = 0; i < 8; i++)
        {
            if ((crc & 0x80000000) != 0)
            {
                crc = (crc << 1) ^ 0x4C11DB7;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/********************************
RNG时钟计算函数
功能:根据RCHFClk，计算RNG分频系数，固定输出4MHZ
输入：RCHFClk
输出：RNG时钟工作在4MHZ及以下的分频系数
********************************/
uint32_t RNG_ClkCalc(uint32_t RCHFClk)
{
    uint32_t div = 0;
    switch (RCHFClk)
    {
    case 16000000:
        div = CMU_OPCCR2_RNGPRSC_DIV4;
        break;
    case 24000000:
        div = CMU_OPCCR2_RNGPRSC_DIV8;
        break;
    case 8000000:
    default:
        div = CMU_OPCCR2_RNGPRSC_DIV2;
        break;
    }
    return div;
}

//获取一个随机数
uint32_t TRNG_Get_RandNum(void)
{
    uint32_t RandomNum;

    RNG_SR_RNF_Clr();
    RNG_CR_EN_Setable(ENABLE);
    TicksDelayUs(12); // LSFR移位需要32个时钟周期
    RNG_CR_EN_Setable(DISABLE);
    if (SET == RNG_SR_RNF_Chk()) //随机数未能通过质量检测
    {
        RNG_SR_RNF_Clr();
        return FAIL;
    }

    RandomNum = RNG_DOR_Read();
    return RandomNum;
}

// crc32运算
uint32_t TRNG_Get_Crc32(uint32_t DataIn)
{
    uint16_t i;
    uint32_t Result;

    RNG_CRC_DIR_Write(DataIn);
    RNG_CRC_CR_CRCEN_Setable(ENABLE);

    // crc运算需要约32个时钟周期
    for (i = 0; i < 100; i++)
    {
        if (SET == RNG_CRC_SR_CRCDONE_Chk())
            break;
    }
    RNG_CRC_SR_CRCDONE_Clr();

    Result = RNG_DOR_Read();

    return Result;
}

void TRNG_Init(void)
{
    volatile u8 Result;
    CMU_ClocksType RCC_Clocks;
    uint32 RngClk;
    //随机数生成测试
    CMU_PERCLK_SetableEx(ANACCLK, ENABLE); //模拟电路总线时钟使能;//随机数电路需要用到ana模块
    CMU_PERCLK_SetableEx(TRNGCLK, ENABLE); // TRNG时钟使能

    /* 一般建议应用使用4M时钟作为随机数工作时钟 */
    CMU_GetClocksFreq(&RCC_Clocks);                  //获取RCHF时钟频率
    RngClk = RNG_ClkCalc(RCC_Clocks.RCHF_Frequency); //获取随机数发生器工作时钟分频
    CMU_OPCCR2_RNGPRSC_Set(RngClk);                  //设置随机数发生器工作时钟分频
    CMU_OPCCR2_RNGCKE_Setable(ENABLE);               //随机数发生器工作时钟使能

    RNG_SR_RNF_Clr();
}
