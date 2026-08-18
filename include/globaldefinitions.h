#ifndef GLOBALDEFINITIONS_H
#define GLOBALDEFINITIONS_H

#include "stm32f1xx.h"

// Объявление глобального массива бит
typedef union
{
    uint32_t bit_fld; // Переменная целиком (все 32 бита)
    struct
    {
        uint32_t flg_systick_int : 1;   // Bit 0: ваш флаг переполнения SysTick
        uint32_t flg_rtc_sync : 1;             // Bit 1: следующий флаг
        uint32_t flg_dma_tx_bsy : 1;             // Bit 2: следующий флаг
                                        // ... здесь вы можете добавлять другие флаги по очереди
        uint32_t reserved : 29;         // Оставшиеся свободные биты до 32-х
    } flags;
} bitfield_t;

// Объявляем внешнюю переменную флагов (сама память выделится в файлах *.c)
extern volatile bitfield_t gbl_flags;

// (Макрос) Формула пересчета адреса бита SRAM в адрес зоны Bit-band
#define BITBAND_SRAM(var_addr, bit) ((volatile uint32_t *)(0x22000000 + ((uint32_t)(var_addr) - 0x20000000) * 32 + (bit) * 4))

// (Макрос) Регистровый макрос для прямого управления Bit 0 (flg_systick_ovf)
#define FLG_SYSTICK_INT (*BITBAND_SRAM(&gbl_flags.bit_fld, 0))
#define FLG_DELAY_DONE (*BITBAND_SRAM(&gbl_flags.bit_fld, 1))
#define FLG_RTC_SYNC (*BITBAND_SRAM(&gbl_flags.bit_fld, 2))
#define FLG_DMA_TX_BSY (*BITBAND_SRAM(&gbl_flags.bit_fld, 3))

#endif  /* GLOBALDEFINITIONS_H */