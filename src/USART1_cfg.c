#include "stm32f1xx.h"
#include "USART1_cfg.h"

void USART1_Init(void) {
    // 1. Включаем тактование порта GPIOA и модуля USART1 на шине APB2
    RCC->APB2ENR |= (RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN);

    // 2. Настройка пина PA9 (TX) как Alternate Function Push-Pull, 50MHz
    // Регистр CRH управляет пинами 8-15. Сдвиг для пина 9: (9-8)*4 = 4 бита
    GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
    GPIOA->CRH |= (GPIO_CRH_MODE9_0 | GPIO_CRH_MODE9_1) | GPIO_CRH_CNF9_1;

    // 3. Настройка пина PA10 (RX) как Input Floating (Вход без подтяжки)
    // Сдвиг для пина 10: (10-8)*4 = 8 бит
    GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
    GPIOA->CRH |= GPIO_CRH_CNF10_0; // MODE10 = 00 (Input), CNF10 = 01 (Floating)

    // Подстройка Baudrate в зависимости от источника тактирования

    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL)
    {    
        // Частота шины 72 МГц (PLL)
        USART1->BRR = 1875; // 0x0753
    }
    else
    {
        // Частота 8 МГц от внутреннего или внешнего источника
        USART1->BRR = 208;  // 0x00D0
    }

    // 4. ЯВНАЯ НАСТРОЙКА ФОРМАТА КАДРА: 8 Data, 1 Stop, No Parity
    USART1->CR1 &= ~USART_CR1_M;    // 8 бит данных
    USART1->CR1 &= ~USART_CR1_PCE;  // Без контроля четности
    USART1->CR2 &= ~USART_CR2_STOP; // 1 стоп-бит
            
    // 5. Включаем передатчик (TE), приемник (RE) и сам модуль USART (UE)
    USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
}

void USART1_SendChar(char ch) {
    // Ждем, пока освободится буфер передачи (флаг TXE — Transmit Data Register Empty)
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = ch;
}

void USART1_SendString(const char* str) {
    while (*str) {
        USART1_SendChar(*str++);
    }
}
