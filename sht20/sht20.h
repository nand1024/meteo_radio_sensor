/*
 * author Dus Oleh Viktorovych
 * created 11.01.2023
 * cross platform library for SHT20
 */

#ifndef _SHT_20_H_
#define _SHT_20_H_

#include <stdint.h>

#define SHT20_DOUBLE_CALC    0          //1 працюємо з double.
                                        //0 працюємо з int16, значення зміщаютьюся на 2 десяткові знаки вліво 22.50 відображається як 2250
#define SHT20_I2C_ADDRESS    0b1000000

typedef enum {
    sht20_set_enable,      //опція увімкненна
    sht20_set_disable,     //опція вимкненна
    sht20_set_cnt,
} sht20_set;

typedef enum {
    sht20_resol_rh12_t14,  //температура 14 біт вологість 12 біт
    sht20_resol_rh8_t12,   //температура 12 біт вологість 8 біт
    sht20_resol_rh10_t13,  //температура 13 біт вологість 10 біт
    sht20_resol_rh11_t11,  //температура 11 біт вологість 11 біт
    sht20_resol_cnt,
} sht20_res_set;

/* колбеки для i2c функцій запису та читання
 *
 * adress - адреса пристрою
 *
 * reg_cmd - вказівник на змінну з командою (відправляється першою)
 *
 * size_reg - розмір змінної, для випадків коли команда регістру i2c має розмір більший за один байт
 * (для цього датчика таких випадків немає, але можуть бути для інших пристроїв, з якими працює i2c через той самий інтерфейс)
 *
 * data - вказівник на массив данних для запису/читання (відправляються слідом за reg_cmd)
 *
 * size - розмір массиву данних для запису/читання
 *
 * для i2c_write data може бути NULL, а size = 0, якщо нам треба відправити лише один байт команди,
 * який буде передаватися через параметр reg_cmd
 */
typedef struct {
    i2c_op_res (*i2c_write) (uint8_t adress, uint8_t *reg_cmd, uint8_t size_reg, uint8_t *data, uint32_t size);
    i2c_op_res (*i2c_read) (uint8_t adress, uint8_t *reg_cmd, uint8_t size_reg, uint8_t *data, uint32_t size);
} SHT20CB;


void sht20_set_power_heater (SHT20CB *shtCB, sht20_set chip_power);             //увімкнення та вимкнення нагрівача сенсора(для діагностичних цілей)

void sht20_soft_reset (SHT20CB *shtCB);                                         //программне скидання
void sht20_set_resolution (SHT20CB *shtCB, sht20_res_set resolution);           //налаштування розрядності ацп
void sht20_set_OTP (SHT20CB *shtCB, sht20_set otp);                             //увімкнення та вимкнення otp reload

uint8_t sht20_is_battery_low (SHT20CB *shtCB);                                  //повертає 1, якщо живлення сенсора менше 2.25 вольт

#if SHT20_DOUBLE_CALC
void sht20_get_data (SHT20CB *shtCB, double *temperature, double *humidity);    //повертає значення температури та вологості в вигляді числа з плаваючою точкою
#else
void sht20_get_data (SHT20CB *shtCB, int16_t *temperature, int16_t *humidity);  //повертає значення температури та вологості в цілочисленому вигляді
                                                                                //27.35 буде у вигляді 2735, тобто зміщенно на 2 десяткові розряди вліво
#endif



#endif
