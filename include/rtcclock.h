#ifndef RTCCLOCK_H
#define RTCCLOCK_H

#include "stm32f1xx.h"

void RTC_Clock_Init(void);
uint32_t RTC_Clock_GetCounter(void);
void RTC_Clock_SetCounter(uint32_t count);
void RTC_Clock_UpdateString(void);
char* RTC_Clock_GetTimeString(void);

#endif /* RTCCLOCK_H */
