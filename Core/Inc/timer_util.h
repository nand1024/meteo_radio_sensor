/*
 * timer_util.h
 * 
 * author Dus Oleh Viktorovych
 * created 11.03.2025
 */

 #ifndef _TIMER_UTIL_H_
 #define _TIMER_UTIL_H_

#include <stdint.h>

 //фіксуємо значення початку відліку
 void timer_fix_cnt (void);

 //повертаємо кількість тіків таймера після останьої фіксації відліку
 uint16_t timer_get_ticks (void);
 
 #endif