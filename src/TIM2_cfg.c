#include "stm32f1xx.h"
#include "TIM2_cfg.h"
#include "DMA1_cfg.h"
#define LED_PIN 13
// Функция предварительной настройки TIM2. Таймер готовится, но не запускается.
void TIM2_Init_CTC(void)
{
    // 1. Включаем тактирование модуля TIM2 на шине APB1
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 2. Настройка предделителя (Prescaler)
    // Частота шины APB1 если PLL 72 МГц то делится на 2, если HSE/HSI 8МГц то тактируется напрямую (до 24МГц) 
    
    TIM2->ARR = 999;

    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL)
    {
        TIM2->PSC = 14399;
    }
    else
    {
        TIM2->PSC = 1599;
    }
       

    // 4. Включаем буферизацию регистра ARR (Auto-reload preload enable)
    // Это защищает таймер от аппаратных сбоев фазы, если период будет меняться на лету.
    TIM2->CR1 |= TIM_CR1_ARPE;

    // 5. Разрешаем прерывание по обновлению/переполнению (Update Interrupt Enable)
    // Оно будет срабатывать каждый раз, когда таймер дойдет до 199 и сбросится в 0.
    TIM2->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(TIM2_IRQn,3);

    // 6. Разрешаем прерывание таймера TIM2 в контроллере прерываний ядра NVIC
    NVIC_EnableIRQ(TIM2_IRQn);

    // ВНИМАНИЕ: Бит TIM2->CR1 |= TIM_CR1_CEN (Counter Enable) здесь НЕ взводим!
    // Таймер находится в режиме ожидания. Его запустит и синхронизирует RTC_IRQHandler.
}

void TIM2_IRQHandler(void)
{
    // Проверяем, что прерывание вызвано именно флагом обновления счета (UIF)
    //if (TIM2->SR & TIM_SR_UIF)
    //{
        
        // СБРОС ФЛАГА: В таймерах STM32 флаг сбрасывается ручной записью нуля в бит.
        // Если этого не сделать, микроконтроллер намертво зациклится в прерывании.
        TIM2->SR = (uint16_t)~TIM_SR_UIF;
      
        // Запуск процедуры отправки по DMA
        // Включаем Single Shot DMA Канал 4, который отправляет строку "ЧЧ:ММ:СС\n" в USART1.
        // (Функцию UART1_SendTime_DMA мы спроектировали на прошлых шагах).
        DMA1_SendTo_USART1_Single(); 
  //  }
}