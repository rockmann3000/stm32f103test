#ifndef SYSTICK_H
#define SYSTICK_H

#include "stm32f1xx.h"

void SysTick_Init(void);

// Блок 2: Управление состоянием (Инлайн-макросы)
#define SysTick_Start()  (SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk)
#define SysTick_Stop()   (SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk)
#define SysTick_Reset()  (SysTick->VAL = 0)

void delay_ms(volatile uint32_t ms);
void SysTick_Handler(void);

#endif /* SYSTICK_H */