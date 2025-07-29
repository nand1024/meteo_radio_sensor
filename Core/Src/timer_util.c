/*
 * timer_util.c
 * 
 * author Dus Oleh Viktorovych
 * created 11.03.2025
 */
#include "stm32l0xx_ll_tim.h"
#include <stdint.h>

#define TIMER    TIM2

static uint16_t fix_count = 0;

//фіксуємо значення початку відліку
void timer_fix_cnt (void)
{
    fix_count = LL_TIM_GetCounter(TIMER);
}

//повертаємо кількість тіків таймера після останьої фіксації відліку
uint16_t timer_get_ticks (void)
{
    const uint16_t to_count = LL_TIM_GetAutoReload(TIMER);
    uint16_t now = LL_TIM_GetCounter(TIMER);
    uint16_t ticks;

    if (now >= fix_count) {
        ticks = now - fix_count;
    } else {
        ticks = to_count - fix_count + now;
    }
    
    return ticks;
}