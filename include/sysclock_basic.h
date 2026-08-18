#ifndef SYSCLOCK_BASIC_H
#define SYSCLOCK_BASIC_H

#include "stm32f1xx.h"

typedef enum
{
    SYSCLK_OK_HSI_SRC = 0,             // Система тактирования инициализирована успешно
    SYSCLK_OK_HSE_SRC = 1,     // Система тактирования работает от источника HSE (внешний кварц)
    SYSCLK_OK_PLL_SRC = 2,     // Система тактирования работает от источника PLL
    SYSCLK_ERR_HSE_START = 3,  // Статус ошибки: внешний кварц (HSE) не запустился за 1 мс
    SYSCLK_ERR_PLL_SYNC = 4,   // Статус ошибки: умножитель частоты (PLL) не стабилизировался
    SYSCLK_ERR_HSE_SWITCH = 5, // Статус ошибки: процессор не смог переключиться на источник HSE
    SYSCLK_ERR_PLL_SWITCH = 6, // Статус ошибки: процессор не смог переключиться на источник PLL
    SYSCLK_ERR_HSI_FAIL = 7,   // Статус ошибки: внутренний генератор HSI нестабилен (на всякий случай)
    SYSCLK_ERR_UNKNOWN = 8     // Резервный код неопределенной ошибки
} SYSCLK_Error_TypeDef;

extern volatile SYSCLK_Error_TypeDef SYSCLK_Status;

uint8_t SysClock_Basic_Init(void);

#endif /* SYSCLOCK_H */
