#ifndef	__USER_INIT_H__
#define __USER_INIT_H__

#include "define_all.h"  

typedef int (*ConditionHook)(void);

extern void Init_SysTick(void);
extern void TicksDelay(uint32_t ClkNum);
extern void TicksDelayMs(uint32_t ms , ConditionHook Hook);
extern void TicksDelayUs(uint32_t us);
extern unsigned char CheckSysReg( __IO uint32_t *RegAddr, uint32_t Value );
extern unsigned char CheckNvicIrqEn( IRQn_Type IRQn );

extern void AnalogIO( GPIO_Type* GPIOx, uint32_t GPIO_Pin );
extern void InputtIO( GPIO_Type* GPIOx, uint32_t GPIO_Pin, uint8_t Type );
extern void OutputIO( GPIO_Type* GPIOx, uint32_t GPIO_Pin, uint8_t Type );
extern void AltFunIO( GPIO_Type* GPIOx, uint32_t GPIO_Pin, uint8_t Type );
extern void CloseIO( GPIO_Type* GPIOx, uint32_t GPIO_Pin );
extern void CloseHIO(GPIOH_Type *GPIOx, uint32_t GPIO_Pin);

extern void AnalogIO_H(uint32_t GPIO_Pin );
extern void InputtIO_H(uint32_t GPIO_Pin, uint8_t Type );
extern void OutputIO_H(uint32_t GPIO_Pin);
extern void AltFunIO_H(uint32_t GPIO_Pin, uint8_t Type  );

extern void Init_SysClk_Gen(void);
extern void IWDT_Init(void);

extern void Init_System(void);

extern void LED0_Flash(uint8_t Times);
extern void IWDT_Clr(void);
 
#endif
