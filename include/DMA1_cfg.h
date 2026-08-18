#ifndef DMA1_CFG_H
#define DMA1_CFG_H

#include "stm32f1xx.h"

// Прототипы функций, доступных для вызова из других файлов проекта
void DMA1_CH4_Init(void);
void DMA1_SendTo_USART1_Single(void);

#endif /* DMA1_CFG_H */
