#include "stm32f1xx.h"
#include "DMA1_cfg.h"
#include "rtcclock.h"
#include "globaldefinitions.h" // Подключаем, чтобы иметь доступ к gbl_flags

// Инициализация канала DMA1 Channel 4 для работы с USART1_TX
void DMA1_CH4_Init(void) 
{
    // 1. Включаем тактирование контроллера DMA1 на шине AHB
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    
    // 2. Гарантированно выключаем канал перед его конфигурацией
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    
    // 3. Указываем жесткий адрес приемника — регистр данных USART1
    DMA1_Channel4->CPAR = (uint32_t)&(USART1->DR);
       
    // Побитовая конфигурация регистра управления DMA1 Channel 4
    DMA1_Channel4->CCR = 0
                       |  DMA_CCR_DIR              // bit 4  = 1: Направление — из памяти в периферию (Memory-to-Peripheral)
                       |  DMA_CCR_MINC             // bit 7  = 1: Инкремент адреса ПАМЯТИ включен (идем вперед по строке)
                       // DMA_CCR_PINC             // bit 6  = 0: Инкремент адреса ПЕРИФЕРИИ выключен (адрес USART1->DR фиксирован)
                       |  DMA_CCR_TCIE             // bit 1  = 1: Прерывание по ПОЛНОМУ окончанию передачи (Transfer Complete)
                       // DMA_CCR_HTIE             // bit 2  = 0: Прерывание по ПОЛОВИНЕ передачи (Half Transfer) выключено
                       // DMA_CCR_TEIE             // bit 3  = 0: Прерывание по ОШИБКЕ передачи (Transfer Error) выключено
                       // DMA_CCR_CIRC             // bit 5  = 0: Режим кольцевого буфера (Circular) выключен -> Single Shot
                       // DMA_CCR_MEM2MEM          // bit 14 = 0: Режим Память-в-Память выключен (работаем с периферией)
                       // Разрядность данных в памяти (MSIZE = 00 -> 8 бит / 1 байт)
                       // DMA_CCR_MSIZE_0          // bit 10 = 0
                       // DMA_CCR_MSIZE_1          // bit 11 = 0
                       // Разрядность данных в периферии (PSIZE = 00 -> 8 бит / 1 байт)
                       // DMA_CCR_PSIZE_0          // bit 8  = 0
                       // DMA_CCR_PSIZE_1          // bit 9  = 0
                       // Программный приоритет канала (PL = 01 -> Medium / Средний)
                       |  DMA_CCR_PL_0             // bit 12 = 1
                       // DMA_CCR_PL_1             // bit 13 = 0
                       ;

    NVIC_SetPriority(DMA1_Channel4_IRQn,1);

    // 5. Разрешаем прерывание данного канала DMA в контроллере NVIC ядра ARM
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

// Функция однократного запуска (Single Shot) передачи строки времени
void DMA1_SendTo_USART1_Single(void) 
{
    // Если прошлый пакет еще передаётся по UART, игнорируем новый вызов
    if (FLG_DMA_TX_BSY) return; 
    
    FLG_DMA_TX_BSY = 1; // Выставляем флаг занятости

    DMA1_Channel4->CCR &= ~DMA_CCR_EN;                              // 1. Выключаем канал для обновления параметров
    for (volatile uint32_t i = 0; i < 100; i++)
    {
        if ((DMA1_Channel4->CCR & DMA_CCR_EN) == 0) break;
    }
    DMA1_Channel4->CMAR = (uint32_t)RTC_Clock_GetTimeString();      // 2. Загружаем адрес начала вашей строки времени
    DMA1_Channel4->CNDTR = 9;                                       // 3. Передаем ровно 8 байт ("ЧЧ:ММ:СС", без '\0')
    DMA1_Channel4->CCR |= DMA_CCR_EN;                               // 4. Включаем обратно — вся строка монолитно улетает в порт!
    USART1->CR3 |= USART_CR3_DMAT;                                  // 5. Включаем генерацию запросов от USART1 к DMA  
}

// Переопределяем функцию аппаратного прерывания для DMA1 Channel 4 из таблицы векторов CMSIS
void DMA1_Channel4_IRQHandler(void) 
{
    // Проверяем флаг завершения передачи всего пакета (TCIF4)
    if (DMA1->ISR & DMA_ISR_TCIF4) 
    {
        // СБРОС ВСЕХ ФЛАГОВ КАНАЛА: пишем единицу в Global Clear для 4-го канала.
        // Это гарантированно потушит TCIF4, HTIF4 и GIF4.
        // Сбрасываем все три флага событий 4-го канала, чтобы полностью опустить линию прерывания
        DMA1->IFCR = DMA_IFCR_CTCIF4 | DMA_IFCR_CHTIF4 | DMA_IFCR_CGIF4;
        USART1->CR3 &= ~USART_CR3_DMAT;
        
        FLG_DMA_TX_BSY = 0; // Снимаем флаг занятости. Теперь прерывание отпустит шину!
    }
}

