#include "delay_us.h"

static uint32_t cycles_per_us = 0;

void delay_us_init(void)
{
    /* Włącz TRC (Trace) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Wyzeruj licznik */
    DWT->CYCCNT = 0;

    /* Włącz licznik cykli CPU */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    cycles_per_us = SystemCoreClock / 1000000UL;
}

void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * cycles_per_us;

    while ((DWT->CYCCNT - start) < ticks)
    {
        __NOP();
    }
}
