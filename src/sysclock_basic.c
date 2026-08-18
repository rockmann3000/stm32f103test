// Функция автоматической настройки источника тактирования и настройки тактирования памяти и периферии;
// Можно использовать только при работе с платой BluePill с внешним кварцем 8МГц или с голым кристаллом от кварца 8МГц;

#include "globaldefinitions.h"
#include "sysclock_basic.h"
#include "systick.h"


#define LED_PIN 13

volatile SYSCLK_Error_TypeDef SYSCLK_Status = SYSCLK_OK_HSI_SRC;

// Попытка включения внешнего кварца и старта PLL

uint8_t SysClock_Basic_Init(void)
{

    // Подключаем GPIO (PORTС) для индикации состояния системы тактирования

    // Разрешаем тактирование порта С
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    // 2. Настраиваем пин PC13 на выход (Push-Pull, 50 МГц)
    // Сдвиг для пина 13 рассчитывается как: (13 - 8) * 4 = 20 бит
    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);     // Очищаем старые биты
    GPIOC->CRH |= (GPIO_CRH_MODE13_0 | GPIO_CRH_MODE13_1); // MODE13 = 11, CNF13 = 00

    // Опционально: гасим светодиод при старте (записываем 1)
    GPIOC->BSRR = (1 << LED_PIN);

    // 1. Включаем внешний кварц (HSE)
    RCC->CR |= RCC_CR_HSEON;

    // 1.1. Ожидание готовности кварца

    delay_ms(3); // Усреднённое время в течение которого кварц точно запустится

    if (!(RCC->CR & RCC_CR_HSERDY))
    {
        // Внешний кварц не запустился, записываем код ошибки в статус
        SYSCLK_Status = SYSCLK_ERR_HSE_START;

        // Включаем светодиод на 3 с (подать 0 на PC13 через BR13 - биты сброса находятся со сдвигом +16)
        GPIOC->BSRR = (1 << (LED_PIN + 16));
        delay_ms(3000);

        // Выключить светодиод (подать 1 на PC13 через BS13)
        GPIOC->BSRR = (1 << LED_PIN);
    }

    // 2. Настраиваем тактирование периферии от PLL на 72 МГц и шины USB на 48 МГц от внешнего кварца (HSE)
    if (SYSCLK_OK_HSI_SRC == SYSCLK_Status)
    {

        // 2.1. Предделители шин
        RCC->CFGR |= RCC_CFGR_HPRE_DIV1;  // AHB = 72 МГц
        RCC->CFGR |= RCC_CFGR_PPRE2_DIV1; // APB2 = 72 МГц
        RCC->CFGR |= RCC_CFGR_PPRE1_DIV2; // APB1 = 36 МГц

        // 2.2. Настраиваем задержку Flash-памяти (2 цикла для 72 МГц) и буфер предвыборки
        FLASH->ACR |= FLASH_ACR_PRFTBE;
        FLASH->ACR &= ~FLASH_ACR_LATENCY;
        FLASH->ACR |= FLASH_ACR_LATENCY_2;

        // 2.3. НАСТРОЙКА USB: деление частоты PLL на 1.5 (72 / 1.5 = 48 МГц)
        RCC->CFGR &= ~RCC_CFGR_USBPRE;

        // 2.4. Конфигурация и запуск PLL (HSE * 9 = 72 МГц)
        RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL); // Очищаем оба поля управления источником и умножителем
        RCC->CFGR |= (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9); // Задаем: источник HSE, множитель x9
        RCC->CR |= RCC_CR_PLLON;

        delay_ms(1);

        // 2.5. Проверка работы PLL
        if (!(RCC->CR & RCC_CR_PLLRDY))
        {
            // PLL не смог стабилизироваться, записываем код ошибки в статус
            SYSCLK_Status = SYSCLK_ERR_PLL_SYNC;

            // Индикация состояния: 
            GPIOC->BSRR = (1 << (LED_PIN + 16));
            delay_ms(500);

            // Выключить светодиод (подать 1 на PC13 через BS13)
            GPIOC->BSRR = (1 << LED_PIN);
            delay_ms(500);

            GPIOC->BSRR = (1 << (LED_PIN + 16));
            delay_ms(500);

            // Выключить светодиод (подать 1 на PC13 через BS13)
            GPIOC->BSRR = (1 << LED_PIN);
            delay_ms(500);
        }
        else
        {

            // 2.6. Переключаем мультиплексор на источник PLL
            RCC->CFGR &= ~RCC_CFGR_SW;
            RCC->CFGR |= RCC_CFGR_SW_PLL;

            delay_ms(1);

            // 2.7. Подтверждение переключения на источник тактирования

            if ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
            {
                // Мультиплексор не смог переключиться на источник тактирования PLL, записываем код ошибки в статус
                SYSCLK_Status = SYSCLK_ERR_PLL_SWITCH;
            }
            else
            {
                // PLL синхронизирован, записываем код в статус
                SYSCLK_Status = SYSCLK_OK_PLL_SRC;

                // Переинициализация таймера systick (загрузка новой константы в регистр сравнения)
                SysTick_Init();

                // Включаем светодиод на 1 с затем на 0,25с (подать 0 на PC13 через BR13 - биты сброса находятся со сдвигом +16)
                GPIOC->BSRR = (1 << (LED_PIN + 16));
                delay_ms(1000);

                // Выключить светодиод (подать 1 на PC13 через BS13)
                GPIOC->BSRR = (1 << LED_PIN);
                delay_ms(1000);

                GPIOC->BSRR = (1 << (LED_PIN + 16));
                delay_ms(250);

                // Выключить светодиод (подать 1 на PC13 через BS13)
                GPIOC->BSRR = (1 << LED_PIN);
            }
        }
    }

    // 3. Переключение на внешний источник тактирования HSE если PLL не заработал

    if (SYSCLK_ERR_PLL_SYNC == SYSCLK_Status)
    {

        // 3.1. Производим переключение на HSE (тактирование генератор уже включено и работает - включение не требуется)

        RCC->CFGR &= ~RCC_CFGR_SW;    // Очищаем биты выбора источника (сброс в HSI)
        RCC->CFGR |= RCC_CFGR_SW_HSE; // Записываем маску HSE (биты 01)

        delay_ms(2);

        // 3.2. Отключаем PLL, чтобы полностью сбросить аппаратное состояние
        RCC->CR &= ~RCC_CR_PLLON;

        delay_ms(2);

        // 3.3. Настройка предделителей шин для 8 МГц
        // Принудительно очищаем старые биты делителей шин, чтобы избежать наложения масок
        RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
        // Все шины (AHB, APB2, APB1) теперь работают на частоте 8 МГц напрямую без деления (DIV1)
        RCC->CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV1);

        // 3.4. Настройка задержек для Flash-памяти под частоту 8 МГц
        FLASH->ACR |= FLASH_ACR_PRFTBE;    // Включаем буфер предвыборки для стабильности
        FLASH->ACR &= ~FLASH_ACR_LATENCY;  // Очищаем биты задержки
        FLASH->ACR |= FLASH_ACR_LATENCY_0; // 0 циклов задержки (максимальная скорость чтения Flash на 8 МГц)

        SYSCLK_Status = SYSCLK_OK_HSE_SRC;


        // Индикация состояния: вкл 2 раза по 0,25с

        GPIOC->BSRR = (1 << (LED_PIN + 16));
        delay_ms(250);

        // Выключить светодиод (подать 1 на PC13 через BS13)
        GPIOC->BSRR = (1 << LED_PIN);
        delay_ms(750);

        GPIOC->BSRR = (1 << (LED_PIN + 16));
        delay_ms(250);

        // Выключить светодиод (подать 1 на PC13 через BS13)
        GPIOC->BSRR = (1 << LED_PIN);
    }

    // 4. Задействуем внутренний генератор для тактирования (HSI), если все источники не запустились

    if (SYSCLK_Status == SYSCLK_ERR_HSE_START)
    {
        // 1. Ручное переключение на внутренний RC генератор (если он был выключен)
        RCC->CR |= RCC_CR_HSION;

        delay_ms(2);

        // 2. Переключаем источник SYSCLK на чистый HSI (без использования PLL)
        RCC->CFGR &= ~RCC_CFGR_SW; // Очистка битов SW (00) выбирает источник HSI напрямую

        delay_ms(2);

        // Отключаем неисправный внешний кварц и PLL, чтобы полностью сбросить их аппаратное состояние
        RCC->CR &= ~RCC_CR_HSEON;
        RCC->CR &= ~RCC_CR_PLLON;

        delay_ms(2);

        // 3. Настройка предделителей шин под базовые 8 МГц
        // Принудительно очищаем старые биты делителей шин, чтобы избежать наложения масок
        RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
        // Все шины (AHB, APB2, APB1) теперь работают на частоте 8 МГц напрямую без деления (DIV1)
        RCC->CFGR |= (RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV1);

        // 4. Настройка задержек для Flash-памяти под частоту 8 МГц
        FLASH->ACR |= FLASH_ACR_PRFTBE;    // Включаем буфер предвыборки для стабильности
        FLASH->ACR &= ~FLASH_ACR_LATENCY;  // Очищаем биты задержки
        FLASH->ACR |= FLASH_ACR_LATENCY_0; // 0 циклов задержки (максимальная скорость чтения Flash на 8 МГц)

        SYSCLK_Status = SYSCLK_OK_HSI_SRC;


        // Индикация состояния: вкл 3 раза по 0,25с

        GPIOC->BSRR = (1 << (LED_PIN + 16));
        delay_ms(250);

        GPIOC->BSRR = (1 << LED_PIN);
        delay_ms(750);

        GPIOC->BSRR = (1 << (LED_PIN + 16));
        delay_ms(250);

        GPIOC->BSRR = (1 << LED_PIN);
        delay_ms(750);

        GPIOC->BSRR = (1 << (LED_PIN + 16));
        delay_ms(250);

        GPIOC->BSRR = (1 << LED_PIN);
    }

    // Возвращаем итоговый статус (SYSCLK_OK при успехе на 72 МГц, либо код ошибки при работе на 64 МГц)
    return SYSCLK_Status;
}