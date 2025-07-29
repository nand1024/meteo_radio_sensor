/*
 * i2c_util.h
 *
 *  Created on: Oct 1, 2024
 *      Author: Dus Oleh Viktorovych
 */

#ifndef INC_I2C_UTIL_H_
#define INC_I2C_UTIL_H_

#include <stdint.h>


typedef enum {
    i2c_op_succes,
    i2c_op_err_arg,
    i2c_op_err_start,
    i2c_op_err_restart,
    i2c_op_err_nack,
    i2c_op_err_timeout,
} i2c_op_res;

uint8_t i2c_is_device_conected (uint8_t val); //потрібно для деяких бібліотек, в цій реалізації просто повертається 1, незалежно від наявності пристрою

uint8_t i2c_is_timeout_err (void);  //повертає 1 якщо був таймаут очікування операції
void i2c_clear_timeout_err (void);  //скидає зміну таймаута очікування операції

i2c_op_res i2c_write255 (uint8_t adress, uint8_t *reg_adr, uint8_t size_reg_adr, uint8_t *data, uint32_t size);  //запис до 255 байтів з reg_adr включно
i2c_op_res i2c_read255 (uint8_t adress, uint8_t *reg_adr, uint8_t size_reg_adr, uint8_t *data, uint32_t size);   //читання до 255 байтів
void i2c_data_tx (uint8_t adress, uint8_t reg_adr, uint8_t *data, uint16_t size);                          //запис послідовності до 65535 байтів, версія колбеку для дисплеїв

#endif /* INC_I2C_UTIL_H_ */
