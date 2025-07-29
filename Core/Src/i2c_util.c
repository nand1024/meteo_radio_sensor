/*
 * i2c_util.c
 *
 *  Created on: Oct 1, 2024
 *      Author: Dus Oleh Viktorovych
 *
 *  Для stm32l0xx, в яких передача порції байтів обмежена 255 байтами за сессію
 */
#include <stdint.h>
#include <stddef.h>
#include "stm32l0xx_ll_i2c.h"
#include "stm32l0xx_ll_utils.h"
#include "timer_util.h"
#include <stdio.h>
#include "i2c_util.h"
#define I2C_PERIPH    I2C1

static uint8_t timeout_err = 0;


//очікує флагу tx empty, no ack або настання таймауту.
static i2c_op_res wait_signal_txe (uint16_t timeout)
{
    uint16_t time_ticks;
    i2c_op_res res;

    timer_fix_cnt();

    do {
        time_ticks = timer_get_ticks();
    } while (time_ticks < timeout && LL_I2C_IsActiveFlag_TXE(I2C_PERIPH) == 0 && LL_I2C_IsActiveFlag_NACK(I2C1) == 0);

    if (LL_I2C_IsActiveFlag_NACK(I2C1) != 0 ) {
    	res = i2c_op_err_nack;
    } else if (time_ticks >= timeout) {
    	res = i2c_op_err_timeout;
    } else {
    	res = i2c_op_succes;
    }

    return res;
}



//очікує флагу rx not empty або настання таймауту. 
//повертає 1 якщо настав таймаут очікування і 0 якщо флаг виставився раніше
static i2c_op_res wait_signal_rxe (uint16_t timeout)
{
    uint16_t time_ticks;
    i2c_op_res res;

    timer_fix_cnt();

    do {
        time_ticks = timer_get_ticks();
    } while (time_ticks < timeout && LL_I2C_IsActiveFlag_RXNE(I2C_PERIPH) == 0);

    if (time_ticks >= timeout) {
    	res = i2c_op_err_timeout;
    } else {
    	res = i2c_op_succes;
    }

    return res;
}


static i2c_op_res wait_signal_ack (uint16_t timeout)
{
    uint16_t time_ticks;
    i2c_op_res res;

    timer_fix_cnt();

    do {
        time_ticks = timer_get_ticks();
    } while (time_ticks < timeout && LL_I2C_IsActiveFlag_NACK(I2C_PERIPH) != 0);

    if (time_ticks >= timeout) {
    	res = i2c_op_err_timeout;
    } else {
    	res = i2c_op_succes;
    }

    return res;
}


//потрібно для деяких бібліотек, поки не реалізована і працює як "заглушка"
uint8_t i2c_is_device_conected (uint8_t val)
{
    return 1;
}



//повертає 1 якщо був таймаут очікування операції
uint8_t i2c_is_timeout_err (void)
{
    return timeout_err;
}



//скидає зміну таймаута очікування операції
void i2c_clear_timeout_err (void)
{
    timeout_err = 0;
}



//запис до 255 байтів з reg_adr включно
i2c_op_res i2c_write255 (uint8_t adress, uint8_t *reg_adr, uint8_t size_reg_adr, uint8_t *data, uint32_t size)
{
    const uint16_t _max_time_out = 10; //1ms
    i2c_op_res res = i2c_op_succes;
    if (size_reg_adr == 0 || size_reg_adr + size > 255) {
        return i2c_op_err_arg;
    }

    //початок сессії запису
    LL_I2C_HandleTransfer(I2C_PERIPH,
                          adress << 1,
                          LL_I2C_ADDRSLAVE_7BIT,
                          size_reg_adr + size,
                          LL_I2C_MODE_SOFTEND,
                          LL_I2C_GENERATE_START_WRITE);
    res = wait_signal_txe(_max_time_out);
    if (res != i2c_op_succes) {
        //timeout_err = 1;
        LL_I2C_GenerateStopCondition(I2C_PERIPH);
        return i2c_op_err_start;
    }

    //відправляємо reg_adr
    for (uint8_t i = 0; i < size_reg_adr; i++) {
        LL_I2C_TransmitData8(I2C_PERIPH, reg_adr[i]);
        res = wait_signal_txe(_max_time_out);
        if (res != i2c_op_succes) {
            //timeout_err = 1;
            LL_I2C_GenerateStopCondition(I2C_PERIPH);
            return res;
        }
    }

    //відправляємо дані
    if (data != NULL && size > 0) {
        for (uint8_t i = 0; i < size; i++) {
            LL_I2C_TransmitData8(I2C_PERIPH, data[i]);
            res = wait_signal_txe(_max_time_out);
            if (res != i2c_op_succes) {
                //timeout_err = 1;
                LL_I2C_GenerateStopCondition(I2C_PERIPH);
                return res;
            }
        }
    }

    LL_I2C_GenerateStopCondition(I2C_PERIPH);
    return res;
}



//читання до 255 байтів
i2c_op_res i2c_read255 (uint8_t adress, uint8_t *reg_adr, uint8_t size_reg_adr, uint8_t *data, uint32_t size)
{
//    uint16_t time_ticks;
    const uint16_t _max_time_out = 100; //10ms
    i2c_op_res res = i2c_op_succes;

    if (size_reg_adr == 0 || size == 0) {
        return res = i2c_op_err_arg;
    }

    //початок сессії запису
    LL_I2C_HandleTransfer(I2C_PERIPH,
                          adress << 1,
                          LL_I2C_ADDRSLAVE_7BIT,
                          size_reg_adr,
                          LL_I2C_MODE_SOFTEND,
                          LL_I2C_GENERATE_START_WRITE);
    res = wait_signal_txe(_max_time_out);
    if (res != i2c_op_succes) {
        //timeout_err = 1;
        LL_I2C_GenerateStopCondition(I2C_PERIPH);
        return i2c_op_err_start;
    }

    //відправляємо reg_adr
    for(uint8_t i = 0; i < size_reg_adr; i++) {
        LL_I2C_TransmitData8(I2C_PERIPH, reg_adr[i]);
        res = wait_signal_txe(_max_time_out);
        if (res != i2c_op_succes) {
            //timeout_err = 1;
            LL_I2C_GenerateStopCondition(I2C_PERIPH);
            return res;
        }
    }

    //початок сессії читання та очікування відповіді від slave
	LL_I2C_HandleTransfer(I2C_PERIPH,
						  adress << 1,
						  LL_I2C_ADDRSLAVE_7BIT,
						  size,
						  LL_I2C_MODE_SOFTEND,
						  LL_I2C_GENERATE_RESTART_7BIT_READ);
	res = wait_signal_ack(_max_time_out);
    if (res != i2c_op_succes) {
        //timeout_err = 1;
        LL_I2C_GenerateStopCondition(I2C_PERIPH);
        return i2c_op_err_restart;
    }

    //отримання даних
    for (uint8_t i = 0; i < size; i++) {
    	res = wait_signal_rxe(_max_time_out);
        if (res != i2c_op_succes) {
            //timeout_err = 1;
            LL_I2C_GenerateStopCondition(I2C_PERIPH);
            return res;
        }
        data[i] = LL_I2C_ReceiveData8(I2C_PERIPH);
    }
    LL_I2C_GenerateStopCondition(I2C_PERIPH);
    return res;
}



//запис послідовності до 65535 байтів, версія колбеку для oled дисплеїв
void i2c_data_tx (uint8_t adress, uint8_t reg_adr, uint8_t *data, uint16_t size)
{
    const uint8_t _max_chunk = 255;

    if (data == NULL || size == 0) {
        return;
    }

    uint8_t last_chunk = size % _max_chunk;
    uint16_t blocks = size / _max_chunk;
    uint16_t i;

    for (i = 0; i < blocks; i++) {
        i2c_write255(adress, &reg_adr, 1, &data[i * _max_chunk], _max_chunk);
    }

    if (last_chunk) {
        i2c_write255(adress, &reg_adr, 1, &data[i * _max_chunk], last_chunk);
    }
}
