/**
 ******************************************************************************
 * @file    fm33a0xxev_gpio.c
 * @author  FM33A0XXEV Application Team
 * @version V1.0.0
 * @date    16-April-2020
 * @brief   This file provides firmware functions to manage the following
 *          functionalities of....:
 *
 */

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_gpio.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
 * @{
 */

/** @defgroup GPIO
 * @brief GPIO driver modules
 * @{
 */

#ifdef USE_FULL_ASSERT

/**
 * @brief  The assert_param macro is used for function's parameters check.
 * @param  expr: If expr is false, it calls assert_failed function
 *   which reports the name of the source file and the source
 *   line number of the call that failed.
 *   If expr is true, it returns no value.
 * @retval None
 */
#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
/* Exported functions ------------------------------------------------------- */
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */

/********************************
 PortX输出数据寄存器配置函数
功能: 配置PortX输出数据
输入：GPIOx_Type* GPIOx（管脚号）,  uint32_t SetValue（输出值）
输出:无
********************************/
void GPIOx_DO_Write(GPIO_Type *GPIOx, uint32_t SetValue)
{
	GPIOx->DO = (SetValue & GPIOx_DO_DO_Msk);
}
/********************************
读PortX输出数据状态函数
功能:读PortX输出数据状态
输入：无
输出:PortX输出值
********************************/
uint32_t GPIOx_DO_Read(GPIO_Type *GPIOx)
{
	return (GPIOx->DO & GPIOx_DO_DO_Msk);
}

/********************************
PortX输出数据置位寄存器函数
功能:PortX输出数据置位，如向PADSET写0x0000_8000，则PADO[15]置位，其余位保持不变。
输入：GPIOx_Type* GPIOx（管脚号）,  uint32_t SetValue（输出值）
输出:无
********************************/
void GPIOx_DSET_Write(GPIO_Type *GPIOx, uint32_t SetValue)
{
	GPIOx->DSET = (SetValue & GPIOx_DSET_DSET_Msk);
}

/********************************
PortX输出数据复位寄存器函数
功能:PortX输出数据复位，如向PADRST写0x0000_8000，则PADO[15]清零，其余位保持不变。
输入：GPIOx_Type* GPIOx（管脚号）,  uint32_t SetValue（输出值）
输出:无
********************************/
void GPIOx_DRST_Write(GPIO_Type *GPIOx, uint32_t SetValue)
{
	GPIOx->DRST = (SetValue & GPIOx_DRST_DRESET_Msk);
}

/********************************
PortX输入数据寄存器函数
功能:PortX输入数据，此寄存器仅占用地址空间，无物理实现。软件读此寄存器直接返回引脚输入信号，芯片并不对引脚输入进行锁存
输入：GPIOx_Type* GPIOx（管脚号）,  uint32_t SetValue（输入值）
输出:无
********************************/
uint32_t GPIOx_DIN_Read(GPIO_Type *GPIOx)
{
	return (GPIOx->DIN & GPIOx_DIN_DIN_Msk);
}

/* PortX额外数字功能寄存器 相关函数 */
void GPIOx_DFS_Setable(GPIO_Type *GPIOx, uint32_t GPIO_Pin, FunState NewState)
{
	uint32_t tmpreg;

	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GET_GPIO_PIN(GPIO_Pin));

	tmpreg = GPIOx->DFS;
	if (NewState == ENABLE)
	{
		tmpreg |= (GPIO_Pin);
	}
	else
	{
		tmpreg &= ~(GPIO_Pin);
	}
	GPIOx->DFS = tmpreg;
}

FunState GPIOx_DFS_Getable(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GET_GPIO_PIN(GPIO_Pin));

	if (GPIOx->DFS & GPIO_Pin)
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* PortX模拟开关使能寄存器 相关函数 */
void GPIOx_ANEN_Setable(GPIO_Type *GPIOx, uint32_t GPIO_Pin, FunState NewState)
{
	uint32_t tmpreg;

	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GET_GPIO_PIN(GPIO_Pin));

	tmpreg = GPIOx->ANEN;
	if (NewState == ENABLE)
	{
		tmpreg |= (GPIO_Pin);
	}
	else
	{
		tmpreg &= ~(GPIO_Pin);
	}
	GPIOx->ANEN = tmpreg;
}

FunState GPIOx_ANEN_Getable(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GET_GPIO_PIN(GPIO_Pin));

	if (GPIOx->ANEN & GPIO_Pin)
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/********************************
GPIO初始化配置函数
功能:GPIO初始化配置
输入：GPIO端口  GPIO_PIN引脚
输出: 无
********************************/
void GPIO_Init(GPIO_Type *GPIOx, GPIO_InitTypeDef *para)
{
	uint32_t pinpos = 0x00, pinbit = 0x00, currentpin = 0x00;
	uint32_t tmpreg;

	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));

	for (pinpos = 0; pinpos < 16; pinpos++)
	{
		pinbit = ((uint32_t)0x01) << pinpos;
		currentpin = (para->Pin) & pinbit;

		if (currentpin == pinbit)
		{
			tmpreg = GPIOx->INEN;
			tmpreg &= ~pinbit;
			tmpreg |= (para->PxINEN) << pinpos;
			GPIOx->INEN = tmpreg;

			tmpreg = GPIOx->PUEN;
			tmpreg &= ~pinbit;
			tmpreg |= (para->PxPUEN) << pinpos;
			GPIOx->PUEN = tmpreg;

			tmpreg = GPIOx->ODEN;
			tmpreg &= ~pinbit;
			tmpreg |= (para->PxODEN) << pinpos;
			GPIOx->ODEN = tmpreg;

			pinbit = (((uint32_t)0x00000003)) << (pinpos * 2);
			tmpreg = GPIOx->FCR;
			tmpreg &= ~pinbit;
			tmpreg |= (para->PxFCR) << (pinpos * 2);
			GPIOx->FCR = tmpreg;
		}
	}
}

/* 获取一个IO口的配置参数结构体
	注意一次只能读取一个IO的配置
*/
/********************************
获取一个IO配置参数结构体函数
功能:获取一个IO配置参数结构体
输入：GPIO端口  GPIO_PIN引脚 引脚类型
输出: 无
********************************/
void GPIO_Get_InitPara(GPIO_Type *GPIOx, uint32_t GPIO_Pin, GPIO_InitTypeDef *para)
{
	uint32_t pinbit = 0x00;
	uint32_t i, temp;

	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));

	for (i = 0; i < 16; i++)
	{
		pinbit = GPIO_Pin & (1 << i);
		if (pinbit > 0)
			break;
	}

	if (i < 16)
	{
		pinbit = i;
		para->Pin = GPIO_Pin;

		temp = (GPIOx->INEN & GPIO_Pin) >> pinbit;
		if (temp == (uint32_t)GPIO_IN_En)
		{
			para->PxINEN = GPIO_IN_En;
		}
		else
		{
			para->PxINEN = GPIO_IN_Dis;
		}

		temp = (GPIOx->PUEN & GPIO_Pin) >> pinbit;
		if (temp == (uint32_t)GPIO_PU_En)
		{
			para->PxPUEN = GPIO_PU_En;
		}
		else
		{
			para->PxPUEN = GPIO_PU_Dis;
		}

		temp = (GPIOx->ODEN & GPIO_Pin) >> pinbit;
		if (temp == (uint32_t)GPIO_OD_En)
		{
			para->PxODEN = GPIO_OD_En;
		}
		else
		{
			para->PxODEN = GPIO_OD_Dis;
		}

		temp = (GPIOx->FCR >> (pinbit * 2)) & 0x00000003;
		if (temp == (uint32_t)GPIO_FCR_IN)
		{
			para->PxFCR = GPIO_FCR_IN;
		}
		else if (temp == (uint32_t)GPIO_FCR_OUT)
		{
			para->PxFCR = GPIO_FCR_OUT;
		}
		else if (temp == (uint32_t)GPIO_FCR_DIG)
		{
			para->PxFCR = GPIO_FCR_DIG;
		}
		else
		{
			para->PxFCR = GPIO_FCR_ANA;
		}
	}
}

/********************************
读取GPIOx输入数据寄存器函数
功能:读取GPIOx输入数据寄存器
输入：GPIOx_PIN引脚
输出: 数据寄存器值
********************************/
uint32_t GPIO_ReadInputData(GPIO_Type *GPIOx)
{
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	return (GPIOx->DIN);
}

/********************************
读取GPIOx输入数据寄存器bit函数
功能:GPIOx输入数据寄存器bit
输入：GPIO端口  GPIO_PIN引脚
输出: bit状态
********************************/
uint8_t GPIO_ReadInputDataBit(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	uint8_t bitstatus = 0x00;

	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GET_GPIO_PIN(GPIO_Pin));

	if ((GPIOx->DIN & GPIO_Pin) != (uint32_t)Bit_RESET)
	{
		bitstatus = (uint8_t)Bit_SET;
	}
	else
	{
		bitstatus = (uint8_t)Bit_RESET;
	}
	return bitstatus;
}

/********************************
GPIOx输出置1函数
功能: GPIOx输出置1
输入：GPIO端口  GPIO_PIN引脚
输出: 无
********************************/
void GPIO_SetBits(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GPIO_PIN(GPIO_Pin));

	GPIOx->DSET = GPIO_Pin;
}

/********************************
GPIOx输出置0函数
功能: GPIOx输出置0
输入：GPIO端口  GPIO_PIN引脚
输出: 无
********************************/
void GPIO_ResetBits(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GPIO_PIN(GPIO_Pin));

	GPIOx->DRST = GPIO_Pin;
}

/********************************
GPIOx输出翻转函数
功能: GPIOx输出置0
输入：GPIO端口  GPIO_PIN引脚
输出: 无
********************************/
void GPIO_ToggleBits(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GPIO_PIN(GPIO_Pin));

	if (GPIO_Pin & GPIOx->DO)
	{
		GPIOx->DRST = GPIO_Pin;
	}
	else
	{
		GPIOx->DSET = GPIO_Pin;
	}
}

/********************************
读取GPIOx输出数据寄存器函数
功能:读取GPIOx输出数据寄存器
输入：GPIOx_PIN引脚
输出: 数据寄存器值
********************************/
uint32_t GPIO_ReadOutputData(GPIO_Type *GPIOx)
{
	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));

	return (GPIOx->DO);
}

/********************************
读取GPIOx输出数据寄存器bit函数
功能:GPIOx输出数据寄存器bit
输入：GPIO端口  GPIO_PIN引脚
输出: bit状态
********************************/
uint8_t GPIO_ReadOutputDataBit(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	uint8_t bitstatus = 0x00;

	assert_param(IS_GPIO_ALL_PERIPH(GPIOx));
	assert_param(IS_GET_GPIO_PIN(GPIO_Pin));

	if (((GPIOx->DO) & GPIO_Pin) != (uint32_t)Bit_RESET)
	{
		bitstatus = (uint8_t)Bit_SET;
	}
	else
	{
		bitstatus = (uint8_t)Bit_RESET;
	}
	return bitstatus;
}

/********************************
WKUP引脚配置函数
功能：WKUP引脚配置
输入：WKUP GPIO_PIN引脚
	  ENABLE 使能WKUP功能
	  DISABLE 关闭WKUP功能
输出: 无
********************************/
void GPIO_PINWKEN_SetableEx(uint32_t NWKPinDef, FunState NewState)
{
	if (NewState == ENABLE)
	{
		GPIO->PINWKEN |= NWKPinDef;
	}
	else
	{
		GPIO->PINWKEN &= ~NWKPinDef;
	}
}

/********************************
WKUP边沿选择函数
功能WKUP边沿选择
输入：WKUP GPIO_PIN引脚
	  ENABLE 上升沿
	  DISABLE 下降沿
输出: 无
********************************/
void GPIO_PINWKSEL_SetableEx(uint32_t NWKPinDef, FunState NewState)
{
	if (NewState == ENABLE)
	{
		GPIO->PINWKEN |= (NWKPinDef << 8);
	}
	else
	{
		GPIO->PINWKEN &= ~(NWKPinDef << 8);
	}
}

/********************************
WKUP中断入口选择函数
功能WKUP中断入口选择
输入：GPIO_PINWKEN_WKISEL_NMI: NMI中断
	  GPIO_PINWKEN_WKISEL_46：#46中断
输出: 无
********************************/
void GPIO_PINWKEN_WKISEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = GPIO->PINWKEN;
	tmpreg &= ~(GPIO_PINWKEN_WKISEL_Msk);
	tmpreg |= (SetValue & GPIO_PINWKEN_WKISEL_Msk);
	GPIO->PINWKEN = tmpreg;
}
/********************************
读取WKUP中断入口选择函数
功能：读取WKUP中断入口选择
输入：无
输出: GPIO_PINWKEN_WKISEL_NMI: NMI中断
	  GPIO_PINWKEN_WKISEL_46：#46中断
********************************/
uint32_t GPIO_PINWKEN_WKISEL_Get(void)
{
	return (GPIO->PINWKEN & GPIO_PINWKEN_WKISEL_Msk);
}

/*************************************************************************
 函数名称：GPIO_EXTI_Select_Pin
 功能说明：GPIO EXTI 外部引脚选择
 输入参数：GPIOx 端口 GPIO_Pinpin 端口内引脚
 输出参数：无
 返回参数无
 *************************************************************************/
void GPIO_EXTI_Select_Pin(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	uint32_t pin_num = 0, i;
	uint32_t pin_shift;
	uint32_t tmpreg;

	for (i = 0; i < 16; i++)
	{
		if (GPIO_Pin & (1 << i))
		{
			pin_num = i;
			break;
		}
	}

	if (i == 16)
		return;

	switch ((uint32_t)GPIOx)
	{
	case (uint32_t)GPIOE: //
	case (uint32_t)GPIOF: //
		if (pin_num > 11)
			break;

		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 2;
		}
		else
		{
			pin_shift = 4;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOF)
		{
			pin_shift += 8;
		}
		pin_num = pin_num % 4;
		tmpreg = GPIO->EXTISEL1;
		tmpreg &= (~(0x00000003 << pin_shift));
		tmpreg |= (pin_num << pin_shift);
		GPIO->EXTISEL1 = tmpreg;
		break;

	case (uint32_t)GPIOA: //
	case (uint32_t)GPIOB: //
	case (uint32_t)GPIOC: //
	case (uint32_t)GPIOD: //
		if (((uint32_t)GPIOx == (uint32_t)GPIOA) || ((uint32_t)GPIOx == (uint32_t)GPIOB) || ((uint32_t)GPIOx == (uint32_t)GPIOC))
			if (pin_num > 15)
				break;

		if ((uint32_t)GPIOx == (uint32_t)GPIOD)
			if (pin_num > 11)
				break;

		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 2;
		}
		else if ((pin_num >= 8) && (pin_num <= 11))
		{
			pin_shift = 4;
		}
		else
		{
			pin_shift = 6;
		}

		if ((uint32_t)GPIOx == (uint32_t)GPIOB)
		{
			pin_shift += 8;
		}

		if ((uint32_t)GPIOx == (uint32_t)GPIOC)
		{
			pin_shift += 16;
		}

		if ((uint32_t)GPIOx == (uint32_t)GPIOD)
		{
			pin_shift += 24;
		}

		pin_num = pin_num % 4;
		tmpreg = GPIO->EXTISEL0;
		tmpreg &= (~(0x00000003 << pin_shift));
		tmpreg |= (pin_num << pin_shift);
		GPIO->EXTISEL0 = tmpreg;
		break;
	default:
		break;
	}
}
/*************************************************************************
 函数名称：GPIO_EXTI_Select_Edge
 功能说明：GPIO EXTI_Select_Edge 外部中断边沿选择
 输入参数：GPIOx 端口 ，GPIO_Pin 端口内引脚 ,edge_select 触发沿
 输出参数：无
 返回参数无
 *************************************************************************/
void GPIO_EXTI_Select_Edge(GPIO_Type *GPIOx, uint32_t GPIO_Pin, GPIOExtiEdge edge_select)
{
	uint32_t pin_num = 0, i;
	uint32_t pin_shift;
	uint32_t tmpreg;
	for (i = 0; i < 16; i++)
	{
		if (GPIO_Pin & (1 << i))
		{
			pin_num = i;
			break;
		}
	}
	if (i == 16)
		return;

	switch ((uint32_t)GPIOx)
	{
	case (uint32_t)GPIOA:
	case (uint32_t)GPIOB:
	case (uint32_t)GPIOC:
	case (uint32_t)GPIOD:
		if (pin_num > 15)
			break;
		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 2;
		}
		else if ((pin_num >= 8) && (pin_num <= 11))
		{
			pin_shift = 4;
		}
		else
		{
			pin_shift = 6;
		}

		if (((uint32_t)GPIOx == (uint32_t)GPIOB))
		{
			pin_shift += 8;
		}

		if (((uint32_t)GPIOx == (uint32_t)GPIOC))
		{
			pin_shift += 16;
		}

		if (((uint32_t)GPIOx == (uint32_t)GPIOD))
		{
			pin_shift += 24;
		}
		tmpreg = GPIO->EXTIEDS0;
		tmpreg &= (~(0x00000003 << pin_shift));
		tmpreg |= (edge_select << pin_shift);
		GPIO->EXTIEDS0 = tmpreg;
		break;

	case (uint32_t)GPIOE:
	case (uint32_t)GPIOF:
		if (pin_num > 11)
			break;
		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 2;
		}
		else
		{
			pin_shift = 4;
		}

		if (((uint32_t)GPIOx == (uint32_t)GPIOF))
		{
			pin_shift += 8;
		}
		tmpreg = GPIO->EXTIEDS1;
		tmpreg &= (~(0x00000003 << pin_shift));
		tmpreg |= (edge_select << pin_shift);
		GPIO->EXTIEDS1 = tmpreg;
	default:
		break;
	}
}

/* EXTI数字滤波控制寄存器 相关函数 */
void GPIO_EXTI_EXTIDF_Setable(GPIO_Type *GPIOx, uint32_t GPIO_Pin, FunState NewState)
{
	uint32_t pin_num = 0, i;
	uint32_t pin_shift;
	uint32_t tmpreg;
	for (i = 0; i < 16; i++)
	{
		if (GPIO_Pin & (1 << i))
		{
			pin_num = i;
			break;
		}
	}
	if (i == 16)
		return;

	switch ((uint32_t)GPIOx)
	{
	case (uint32_t)GPIOA:
	case (uint32_t)GPIOB:
	case (uint32_t)GPIOC:
	case (uint32_t)GPIOD:
	case (uint32_t)GPIOE:
	case (uint32_t)GPIOF:
		if (((uint32_t)GPIOx == (uint32_t)GPIOE) || ((uint32_t)GPIOx == (uint32_t)GPIOF))
		{
			if (pin_num > 11)
				break;
		}
		else
		{
			if (pin_num > 15)
				break;
		}

		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 1;
		}
		else if ((pin_num >= 8) && (pin_num <= 11))
		{
			pin_shift = 2;
		}
		else
		{
			pin_shift = 3;
		}

		if ((uint32_t)GPIOx == (uint32_t)GPIOB)
		{
			pin_shift += 4;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOC)
		{
			pin_shift += 8;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOD)
		{
			pin_shift += 12;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOE)
		{
			pin_shift += 16;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOF)
		{
			pin_shift += 20;
		}

		tmpreg = GPIO->EXTIDF;
		if (NewState == ENABLE)
		{
			tmpreg |= 1 << pin_shift;
		}
		else
		{
			tmpreg &= (~(1 << pin_shift));
		}
		GPIO->EXTIDF = tmpreg;
		break;

	default:
		break;
	}
}

/* 读取EXTI输入信号寄存器 */
FunState GPIO_EXTI_EXTIDI_Getable(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	uint32_t pin_num = 0, i;
	uint32_t pin_shift;
	FunState Result = DISABLE;
	for (i = 0; i < 16; i++)
	{
		if (GPIO_Pin & (1 << i))
		{
			pin_num = i;
			break;
		}
	}

	if (i == 16)
		return Result;

	switch ((uint32_t)GPIOx)
	{
	case (uint32_t)GPIOA:
	case (uint32_t)GPIOB:
	case (uint32_t)GPIOC:
	case (uint32_t)GPIOD:
	case (uint32_t)GPIOE:
	case (uint32_t)GPIOF:
		if (((uint32_t)GPIOx == (uint32_t)GPIOE) || ((uint32_t)GPIOx == (uint32_t)GPIOF))
		{
			if (pin_num > 11)
				break;
		}
		else
		{
			if (pin_num > 15)
				break;
		}

		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 1;
		}
		else if ((pin_num >= 8) && (pin_num <= 11))
		{
			pin_shift = 2;
		}
		else
		{
			pin_shift = 3;
		}

		if ((uint32_t)GPIOx == (uint32_t)GPIOB)
		{
			pin_shift += 4;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOC)
		{
			pin_shift += 8;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOD)
		{
			pin_shift += 12;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOE)
		{
			pin_shift += 16;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOF)
		{
			pin_shift += 20;
		}

		if ((GPIO->EXTIDI & (1 << pin_shift)) == (1 << pin_shift))
		{
			Result = ENABLE;
		}
		else
		{
			Result = DISABLE;
		}
		break;
	default:
		break;
	}

	return Result;
}

/*************************************************************************
 函数名称：GPIO_EXTI_EXTIISR_ClrEx
 功能说明：GPIO_EXTI_EXTIISR_ClrEx 清除中断标志
 输入参数：GPIOx 端口 ，GPIO_Pin 端口内引脚
 输出参数：无
 返回参数无
 *************************************************************************/
void GPIO_EXTI_EXTIISR_ClrEx(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	uint32_t pin_num = 0, i;
	uint32_t pin_shift;
	uint32_t tmpreg;
	for (i = 0; i < 16; i++)
	{
		if (GPIO_Pin & (1 << i))
		{
			pin_num = i;
			break;
		}
	}

	if (i == 16)
		return;

	switch ((uint32_t)GPIOx)
	{
	case (uint32_t)GPIOA:
	case (uint32_t)GPIOB:
	case (uint32_t)GPIOC:
	case (uint32_t)GPIOD:
	case (uint32_t)GPIOE:
	case (uint32_t)GPIOF:
		if (((uint32_t)GPIOx == (uint32_t)GPIOE) || ((uint32_t)GPIOx == (uint32_t)GPIOF))
		{
			if (pin_num > 11)
				break;
		}
		else
		{
			if (pin_num > 15)
				break;
		}

		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 1;
		}
		else if ((pin_num >= 8) && (pin_num <= 11))
		{
			pin_shift = 2;
		}
		else
		{
			pin_shift = 3;
		}

		if ((uint32_t)GPIOx == (uint32_t)GPIOB)
		{
			pin_shift += 4;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOC)
		{
			pin_shift += 8;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOD)
		{
			pin_shift += 12;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOE)
		{
			pin_shift += 16;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOF)
		{
			pin_shift += 20;
		}

		tmpreg = GPIO->EXTIISR;
		tmpreg |= (1 << pin_shift);
		GPIO->EXTIISR = tmpreg;
		break;

	default:
		break;
	}
}

/*************************************************************************
 函数名称：GPIO_EXTI_EXTIISR_ChkEx
 功能说明：GPIO_EXTI_EXTIISR_ChkEx 读取中断标志状态
 输入参数：GPIOx 端口 ，GPIO_Pin 端口内引脚
 输出参数：无
 返回参数FlagStatus 中断标志状态
 *************************************************************************/
FlagStatus GPIO_EXTI_EXTIISR_ChkEx(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	uint32_t pin_num = 0, i;
	uint32_t pin_shift;
	FlagStatus Result = RESET;

	for (i = 0; i < 16; i++)
	{
		if (GPIO_Pin & (1 << i))
		{
			pin_num = i;
			break;
		}
	}

	if (i == 16)
		return Result;

	switch ((uint32_t)GPIOx)
	{
	case (uint32_t)GPIOA:
	case (uint32_t)GPIOB:
	case (uint32_t)GPIOC:
	case (uint32_t)GPIOD:
	case (uint32_t)GPIOE:
	case (uint32_t)GPIOF:
		if (((uint32_t)GPIOx == (uint32_t)GPIOE) || ((uint32_t)GPIOx == (uint32_t)GPIOF))
		{
			if (pin_num > 11)
				break;
		}
		else
		{
			if (pin_num > 15)
				break;
		}

		if ((pin_num <= 3)) //
		{
			pin_shift = 0;
		}
		else if ((pin_num >= 4) && (pin_num <= 7))
		{
			pin_shift = 1;
		}
		else if ((pin_num >= 8) && (pin_num <= 11))
		{
			pin_shift = 2;
		}
		else
		{
			pin_shift = 3;
		}

		if ((uint32_t)GPIOx == (uint32_t)GPIOB)
		{
			pin_shift += 4;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOC)
		{
			pin_shift += 8;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOD)
		{
			pin_shift += 12;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOE)
		{
			pin_shift += 16;
		}
		if ((uint32_t)GPIOx == (uint32_t)GPIOF)
		{
			pin_shift += 20;
		}
		if ((GPIO->EXTIISR & (1 << pin_shift)) == (1 << pin_shift))
			Result = SET;
		break;
	default:
		break;
	}

	return Result;
}

/********************************
 FOUT1输出配置函数
功能: 配置FOUT1输出
输入：输出频率选择信号
输出:无
********************************/
void GPIO_FOUTSEL_FOUT1SEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = GPIO->FOUTSEL;
	tmpreg &= ~(GPIO_FOUTSEL_FOUT1SEL_Msk);
	tmpreg |= (SetValue & GPIO_FOUTSEL_FOUT1SEL_Msk);
	GPIO->FOUTSEL = tmpreg;
}
/********************************
读FOUT1输出状态函数
功能:读FOUT1输出状态
输入：无
输出:FOUT输出选择值
********************************/
uint32_t GPIO_FOUTSEL_FOUT1SEL_Get(void)
{
	return (GPIO->FOUTSEL & GPIO_FOUTSEL_FOUT1SEL_Msk);
}

/********************************
 FOUT0输出配置函数
功能: 配置FOUT0输出
输入：输出频率选择信号
输出:无
********************************/
void GPIO_FOUTSEL_FOUT0SEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = GPIO->FOUTSEL;
	tmpreg &= ~(GPIO_FOUTSEL_FOUT0SEL_Msk);
	tmpreg |= (SetValue & GPIO_FOUTSEL_FOUT0SEL_Msk);
	GPIO->FOUTSEL = tmpreg;
}
/********************************
读FOUT0输出状态函数
功能:读FOUT0输出状态
输入：无
输出:FOUT输出选择值
********************************/
uint32_t GPIO_FOUTSEL_FOUT0SEL_Get(void)
{
	return (GPIO->FOUTSEL & GPIO_FOUTSEL_FOUT0SEL_Msk);
}

/*************************************************************************
 函数名称：GPIO_EXTI_Init
 功能说明：GPIO_EXTI_Init 外部中断初始化
 输入参数：port 端口 ，pin 端口内引脚 ,edge 触发沿
 输出参数：无
 返回参数无
 *************************************************************************/
void GPIO_EXTI_Init(GPIO_Type *GPIOx, uint32_t GPIO_Pin, GPIOExtiEdge edge_select, FunState DFState)
{
	GPIO_EXTI_Select_Edge(GPIOx, GPIO_Pin, EXTIDISABLE); //关闭边沿触发
	GPIO_EXTI_Select_Pin(GPIOx, GPIO_Pin);				 //外部引脚选择
	GPIO_EXTI_Select_Edge(GPIOx, GPIO_Pin, edge_select); //修改边沿选择
	GPIO_EXTI_EXTIISR_ClrEx(GPIOx, GPIO_Pin);			 //清除中断标志
	GPIO_EXTI_EXTIDF_Setable(GPIOx, GPIO_Pin, DFState);
}

/*************************************************************************
 函数名称：GPIO_EXTI_Close
 功能说明：GPIO_EXTI_Close 外部中断关闭
 输入参数：port 端口 ，pin 端口内引脚
 输出参数：无
 返回参数无
 *************************************************************************/
void GPIO_EXTI_Close(GPIO_Type *GPIOx, uint32_t GPIO_Pin)
{
	GPIO_EXTI_Select_Edge(GPIOx, GPIO_Pin, EXTIDISABLE); //关闭边沿触发
}

/********************************
GPIOH初始化配置函数
功能:GPIO初始化配置
输入：GPIO端口  GPIO_PIN引脚
输出: 无
********************************/
void GPIOH_Init(GPIO_InitTypeDef *para)
{
	uint32_t pinpos = 0x00, pinbit = 0x00, currentpin = 0x00;
	uint32_t tmpreg;

	for (pinpos = 0; pinpos < 16; pinpos++)
	{
		pinbit = ((uint32_t)0x01) << pinpos;
		currentpin = (para->Pin) & pinbit;

		if (currentpin == pinbit)
		{
			tmpreg = GPIOH->INEN;
			tmpreg &= ~pinbit;
			tmpreg |= (para->PxINEN) << pinpos;
			GPIOH->INEN = tmpreg;

			tmpreg = GPIOH->PUEN;
			tmpreg &= ~pinbit;
			tmpreg |= (para->PxPUEN) << pinpos;
			GPIOH->PUEN = tmpreg;

			pinbit = (((uint32_t)0x00000003)) << (pinpos * 2);
			tmpreg = GPIOH->FCR;
			tmpreg &= ~pinbit;
			tmpreg |= (para->PxFCR) << (pinpos * 2);
			GPIOH->FCR = tmpreg;
		}
	}
}

/********************************
获取一个IO配置参数结构体函数
功能:获取一个IO配置参数结构体
输入：GPIO端口  GPIO_PIN引脚 引脚类型
输出: 无
********************************/
void GPIOH_Get_InitPara(uint32_t GPIO_Pin, GPIO_InitTypeDef *para)
{
	uint32_t pinbit = 0x00;
	uint32_t i, temp;

	assert_param(IS_GET_GPIOH_PIN(GPIO_Pin));
	for (i = 0; i < 16; i++)
	{
		pinbit = GPIO_Pin & (1 << i);
		if (pinbit > 0)
			break;
	}

	if (i < 16)
	{
		pinbit = i;
		para->Pin = GPIO_Pin;

		temp = (GPIOH->INEN & GPIO_Pin) >> pinbit;
		if (temp == (uint32_t)GPIO_IN_En)
		{
			para->PxINEN = GPIO_IN_En;
		}
		else
		{
			para->PxINEN = GPIO_IN_Dis;
		}

		temp = (GPIOH->PUEN & GPIO_Pin) >> pinbit;
		if (temp == (uint32_t)GPIO_PU_En)
		{
			para->PxPUEN = GPIO_PU_En;
		}
		else
		{
			para->PxPUEN = GPIO_PU_Dis;
		}

		temp = (GPIOH->FCR >> (pinbit * 2)) & 0x00000003;
		if (temp == (uint32_t)GPIO_FCR_IN)
		{
			para->PxFCR = GPIO_FCR_IN;
		}
		else if (temp == (uint32_t)GPIO_FCR_OUT)
		{
			para->PxFCR = GPIO_FCR_OUT;
		}
		else if (temp == (uint32_t)GPIO_FCR_DIG)
		{
			para->PxFCR = GPIO_FCR_DIG;
		}
		else
		{
			para->PxFCR = GPIO_FCR_ANA;
		}
	}
}

/********************************
 PortH输出数据寄存器配置函数
功能: 配置PortH输出数据
输入：uint32_t SetValue（输出值）
输出:无
********************************/
void GPIOH_DO_Write(uint32_t SetValue)
{
	GPIOH->DO = (SetValue & GPIOH_DO_PHDO_Msk);
}
/* 读取GPIOH输出寄存器 */
uint32_t GPIOH_ReadOutputData(void)
{
	return (GPIOH->DO & GPIOH_DO_PHDO_Msk);
}

/* 按管脚Pin读取输出寄存器 */
uint8_t GPIOH_ReadOutputDataBit(uint32_t GPIO_Pin)
{
	uint8_t bitstatus = 0x00;
	assert_param(IS_GET_GPIOH_PIN(GPIO_Pin));
	if ((GPIOH->DO & GPIO_Pin) != (uint32_t)Bit_RESET)
	{
		bitstatus = (uint8_t)Bit_SET;
	}
	else
	{
		bitstatus = (uint8_t)Bit_RESET;
	}
	return bitstatus;
}

/* 读取GPIOH输入寄存器 */
uint32_t GPIOH_ReadInputData(void)
{
	return (GPIOH->DIN & GPIOH_DIN_PHDIN_Msk);
}

/* 按管脚Pin读取输入寄存器 */
uint8_t GPIOH_ReadInputDataBit(uint32_t GPIO_Pin)
{
	uint8_t bitstatus = 0x00;
	assert_param(IS_GET_GPIOH_PIN(GPIO_Pin));
	if ((GPIOH->DIN & GPIO_Pin) != (uint32_t)Bit_RESET)
	{
		bitstatus = (uint8_t)Bit_SET;
	}
	else
	{
		bitstatus = (uint8_t)Bit_RESET;
	}
	return bitstatus;
}

/********************************
 IOMCR四选一输出配置函数
功能: 配置IO四选一输出
输入：输出IO通道选择配置
输出:无
********************************/
void GPIO_IOMCR_IOMUXSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = GPIO->IOMCR;
	tmpreg &= ~(GPIO_IOMCR_IOMUXSEL_Msk);
	tmpreg |= (SetValue & GPIO_IOMCR_IOMUXSEL_Msk);
	GPIO->IOMCR = tmpreg;
}
/********************************
读IOMCR四选一配置函数
功能:读IOMCR四选一配置状态
输入：无
输出:IOMCR四选一配置选择值
********************************/
uint32_t GPIO_IOMCR_IOMUXSEL_Get(void)
{
	return (GPIO->IOMCR & GPIO_IOMCR_IOMUXSEL_Msk);
}

/* GPIO 四选一使能 函数 */
void GPIO_IOMCR_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		GPIO->IOMCR |= GPIO_IOMCR_IOMCUEN_Msk;
	}
	else
	{
		GPIO->IOMCR &= ~(GPIO_IOMCR_IOMCUEN_Msk);
	}
}

FunState GPIO_IOMCR_Getable(void)
{
	if (GPIO->IOMCR & (GPIO_IOMCR_IOMCUEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/******END OF FILE****/
