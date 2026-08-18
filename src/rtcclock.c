#include "stm32f1xx.h"
#include "globaldefinitions.h"
#include "rtcclock.h"
#include "systick.h"

#define LED_PIN 13
#define LED_TOGGLE() (GPIOC->ODR ^= (1 << LED_PIN)) // Инвертировать состояние PC13

uint8_t RTC_WaitForLastTask(void)
{
    for (uint32_t i = 0; i < 2000; i++)
    {
        delay_ms(1);

        // Опрашиваем бит готовности RTOFF (Запись завершена)
        if (RTC->CRL & RTC_CRL_RTOFF)
        {
            return 1; // Успешно: аппаратные буферы свободны, можно писать дальше
        }
    }
    return 0; // Авария: тактирование RTC отсутствует
}

// Настройка внешнего часового кварца (LSE) и RTC
void RTC_Clock_Init(void)
{

    // 1. Включаем тактование PWR и BKP для доступа к Backup-зоне
    RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
    PWR->CR |= PWR_CR_DBP; // Снимаем защиту от записи в резервную зону

    // 2. Включаем часовой кварц (LSE)
    RCC->BDCR |= RCC_BDCR_LSEON;

    delay_ms(2200);

    // 3. Если часовой кварц готов, то настраиваем
    if (RCC->BDCR & RCC_BDCR_LSERDY)
    {

        // 4. Привязываем RTC к LSE и включаем модуль
        RCC->BDCR &= ~RCC_BDCR_RTCSEL;
        RCC->BDCR |= RCC_BDCR_RTCSEL_LSE;
        RCC->BDCR |= RCC_BDCR_RTCEN;

        // Сбрасываем флаг синхронизации и ждем, пока аппаратная часть установит его в 1
        RTC->CRL = (uint16_t)~RTC_CRL_RSF;

        for (volatile uint32_t i = 0; i < 500000; i++)
        {
            // Как только RSF стал равен 1 — теневые регистры обновлены, выходим
            if (RTC->CRL & RTC_CRL_RSF)
            {
                break;
            }
        }

        // Проверяем, произошла ли синхронизация (защита на случай, если часовой кварц умер)
        if (!(RTC->CRL & RTC_CRL_RSF))
        {
            // Авария: RTC не синхронизируется. Здесь можно обработать ошибку.
            return;
        }

        if (!RTC_WaitForLastTask())
            return; // Ждем окончания записи

        // 5. Режим конфигурации: задаем предделитель на 1 секунду (32768 - 1)
        RTC->CRL |= RTC_CRL_CNF;
        RTC->PRLH = 0;
        RTC->PRLL = 32767;
        RTC->CRH |= RTC_CRH_SECIE;
        RTC->CRL &= ~RTC_CRL_CNF; // Выход из режима конфигурации

        RTC_WaitForLastTask(); // Ждем окончания записи
    }
    else
    {
        return;
    }

    // 7. Устанавливаем приоритет прерывания RTC равным 1.
    // Это ниже, чем у TIM2 (у него будет 1), что позволит таймеру вытеснять код часов.
    NVIC_SetPriority(RTC_IRQn, 2);

    // 8. Физически разрешаем прохождение сигнала прерывания от RTC к ядру Cortex-M3
    NVIC_EnableIRQ(RTC_IRQn);
}

// 9. Функция чтения секунд из счетчика RTC
uint32_t RTC_Clock_GetCounter(void)
{
    uint32_t first_read;
    uint32_t second_read;

    do
    {
        // 1. Первое чтение счетчика (склеиваем половины)
        // Маски & 0xFFFF можно убрать — регистры CNTH/CNTL и так 16-битные
        first_read = ((uint32_t)RTC->CNTH << 16) | RTC->CNTL;

        // 2. Второе чтение счетчика сразу следом
        second_read = ((uint32_t)RTC->CNTH << 16) | RTC->CNTL;

        // Если значения совпали — значит, в момент чтения тика не было.
        // Если не совпали — цикл повторится и заберет актуальное стабильное значение.
    } while (first_read != second_read);

    return second_read;
}

// 10. Функция принудительной установки времени
void RTC_Clock_SetCounter(uint32_t count)
{

    // 11. Ждем окончания предыдущих операций
    if (!RTC_WaitForLastTask())
        return;

    // 12. Входим в режим конфигурации регистров RTC
    RTC->CRL |= RTC_CRL_CNF;

    // 13. Записываем новые данные раздельно в регистры HIGH и LOW
    RTC->CNTH = (count >> 16) & 0xFFFF;
    RTC->CNTL = count & 0xFFFF;

    // 14. Выходим из режима конфигурации (железо начинает применять изменения)
    RTC->CRL &= ~RTC_CRL_CNF;

    RTC_WaitForLastTask();
}

// 15. Функция обновляет статический буфер rtc_time_str текущим временем
// Статические переменные хранят состояние между вызовами прерывания.
// При старте они инициализируются нулями (или актуальным временем при старте платы)
static uint8_t tmp_hours = 0;
static uint8_t tmp_minutes = 0;
static uint8_t tmp_seconds = 0;

// Глобальный статический буфер в SRAM для хранения текстового времени.
// Размер: 8 символов под "ЧЧ:ММ:СС" + байт перевод строки (LF) '\n' = 9 байт.
static char rtc_time_str[10];

void RTC_Clock_UpdateString(void)
{
    // ======================================
    // 1. Инкремент переменных каждую секунду
    // ======================================
    tmp_seconds++;
    if (tmp_seconds >= 60)
    {
        tmp_seconds = 0;
        tmp_minutes++;
        if (tmp_minutes >= 60)
        {
            tmp_minutes = 0;
            tmp_hours++;
            if (tmp_hours >= 24)
            {
                tmp_hours = 0;
            }
        }
    }

    // ==================================
    // 2. ПЕРЕВОД В ASCII ЧЕРЕЗ ВЫЧИТАНИЕ
    // ==================================

    // Переводим ЧАСЫ (число гарантированно от 0 до 23)
    uint8_t h_tens = 0;
    uint8_t h_units = tmp_hours;
    while (h_units >= 10)
    {
        h_units -= 10;
        h_tens++;
    } // Выполнится максимум 2 раза
    rtc_time_str[0] = h_tens + '0';
    rtc_time_str[1] = h_units + '0';
    rtc_time_str[2] = ':';

    // Переводим МИНУТЫ (число гарантированно от 0 до 59)
    uint8_t m_tens = 0;
    uint8_t m_units = tmp_minutes;
    while (m_units >= 10)
    {
        m_units -= 10;
        m_tens++;
    } // Выполнится максимум 5 раз
    rtc_time_str[3] = m_tens + '0';
    rtc_time_str[4] = m_units + '0';
    rtc_time_str[5] = ':';

    // Переводим СЕКУНДЫ (число гарантированно от 0 до 59)
    uint8_t s_tens = 0;
    uint8_t s_units = tmp_seconds;
    while (s_units >= 10)
    {
        s_units -= 10;
        s_tens++;
    } // Выполнится максимум 5 раз
    rtc_time_str[6] = s_tens + '0';
    rtc_time_str[7] = s_units + '0';

    rtc_time_str[8] = '\n';
    rtc_time_str[9] = '\0';
}

// 19. Функция для безопасного получения указателя на буфер из main.c
char *RTC_Clock_GetTimeString(void)
{
    return rtc_time_str;
}

void RTC_IRQHandler(void)
{
    // 1. Проверяем, что прерывание вызвано именно секундным тиком (SECF)
    if (RTC->CRL & RTC_CRL_SECF)
    {

        // 2. СБРОС ФЛАГА
        RTC->CRL = (uint16_t)~RTC_CRL_SECF;

        // Обнуляем счетчик обычного таймера TIM2, выравнивая фазу
        TIM2->CNT = 0;

        // Если это самый первый тик после сброса — запускаем таймер TIM2 в работу
        if (!(TIM2->CR1 & TIM_CR1_CEN))
        {
            TIM2->CR1 |= TIM_CR1_CEN;
        }
    }
    RTC_Clock_UpdateString();

    // Финально дожидаемся окончания всех операций
    RTC_WaitForLastTask();

    // Переключаем светодиод. Теперь он будет мерно и бесконечно мигать раз в секунду!
    LED_TOGGLE();
}
