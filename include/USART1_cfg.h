#ifndef USART1_CFG_H
#define USART1_CFG_H

#include "stm32f1xx.h"

void USART1_Init(void);
void USART1_SendChar(char ch);
void USART1_SendString(const char* str);

#endif /* UART_H */
