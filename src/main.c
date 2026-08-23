#include "stm32f1xx.h"
#include "DMA1_cfg.h"
#include "globaldefinitions.h"
#include "rtcclock.h"
#include "systick.h"
#include "sysclock_basic.h"
#include "TIM2_cfg.h"
#include "USART1_cfg.h"

volatile bitfield_t gbl_flags = {0};

int main(void)
{
    SysTick_Init();             // 1. Настраиваем базовые механизмы таймера
    SysClock_Basic_Init();      // 2. Настраиваем систему тактирования
    RTC_Clock_Init();           // 3. Запускаем часы реального времени на стабильной частоте
    USART1_Init();               // 5. Настраиваем USART1 на 38400 бод
    DMA1_CH4_Init();             // 6. Настраиваем DMA контроллер на передачу строки времени в uart
    TIM2_Init_CTC();            // 7. Старт таймера TIM2 в режиме сравнения и сброса на 0,2 сек


    while (1)
    {
        
    }
}