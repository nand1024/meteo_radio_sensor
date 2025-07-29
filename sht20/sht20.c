/*
 * author Dus Oleh Viktorovych
 * created 11.01.2023
 * cross platform library for SHT20
 */

#include <stdlib.h>
#include "tim.h"
#include "i2c_util.h"
#include "sht20.h"



//команди сенсора
#define CMD_T_MEAS_HOLD_BY_MASTER     0b11100011  //команда на запуск вимірювання температури з блокуванням scl на час виміру
#define CMD_RH_MEAS_HOLD_BY_MASTER    0b11100101  //команда на запуск вимірювання вологості з блокуванням scl на час виміру
#define CMD_T_MEAS_NO_HOLD            0b11110011  //команда на запуск вимірювання температури без блокування
#define CMD_RH_MEAS_NO_HOLD           0b11110101  //команда на запуск вимірювання вологості без блокування
#define CMD_W_USER_REG                0b11100110  //команда на запис конфігураційного регистру користувача
#define CMD_R_USER_REG                0b11100111  //команда на читання конфігураційного регистру користувача
#define CMD_SOFT_RES                  0b11111110  //команда скидання

//розрядність ацп сенсора
#define SHT20_RESOL_RH_12_BIT_T_14BIT    0b00000000   //температура 14 біт вологість 12 біт
#define SHT20_RESOL_RH_8_BIT_T_12BIT     0b00000001   //температура 12 біт вологість 8 біт
#define SHT20_RESOL_RH_10_BIT_T_13BIT    0b10000000   //температура 13 біт вологість 10 біт
#define SHT20_RESOL_RH_11_BIT_T_11BIT    0b10000001   //температура 11 біт вологість 11 біт
#define SHT20_RESOL_MASK                 (~0b10000001)

//налаштування нагрівача сенсора
#define SHT20_POWER_DISABLE    0b00000000    //нагрівач чіпа вимкненно
#define SHT20_POWER_ENABLE     0b00000100    //нагрівач чіпа увімкненно,
#define SHT20_POWER_MASK       (~0b00000100)

//налаштування otp
#define SHT20_OTP_ENABLE     0b00000000      //otp увімкненно
#define SHT20_OTP_DISABLE    0b00000010      //otp вимкненно(за замовченням)
#define SHT20_OTP_MASK       (~0b00000010)

//флаг статусу живлення
#define SHT20_BATTERY_IS_HIGH    0b00000000  //напруга вище 2.25 вольт
#define SHT20_BATTERY_IS_LOW     0b01000000  //напруга нижче 2.25 вольт



#if SHT20_DOUBLE_CALC

static double calc_temperature_from_raw (uint16_t raw_temperature)
{
    return -46.85 + 175.72 * ((double)raw_temperature / 65536);
}

static double calc_humidity_from_raw (uint16_t raw_humidity)
{
    return -6 + 125 * ((double)raw_humidity / 65536);
}

#else

static int16_t calc_temperature_from_raw (uint32_t raw_temperature)
{
    int32_t res = 17572 * raw_temperature;
    res /= 65536;
    res += -4685;
    return (int16_t)res;
}

static int16_t calc_humidity_from_raw (uint32_t raw_humidity)
{
    int32_t res = 12500 * raw_humidity;
    res /= 65536;
    res += -600;
    return (int16_t)res;
}

#endif



static void set_user_register (SHT20CB *shtCB, uint8_t config_user_reg)
{
    uint8_t addr_register = CMD_W_USER_REG;
    
    shtCB->i2c_write(SHT20_I2C_ADDRESS, &addr_register, sizeof(addr_register), &config_user_reg, sizeof(config_user_reg));
}

static uint8_t get_user_register (SHT20CB *shtCB)
{
    uint8_t addr_register = CMD_R_USER_REG;
    uint8_t user_reg;
    
    shtCB->i2c_read(SHT20_I2C_ADDRESS, &addr_register, sizeof(addr_register), &user_reg, sizeof(user_reg));
    
    return user_reg;
}

//увімкнення та вимкнення нагрівача сенсора
void sht20_set_power_heater (SHT20CB *shtCB, sht20_set chip_power)
{
    if (chip_power < sht20_set_cnt) {

        uint8_t user_reg = get_user_register(shtCB);

        user_reg &= SHT20_POWER_MASK;
        user_reg |= chip_power == sht20_set_enable ? SHT20_POWER_ENABLE : SHT20_POWER_DISABLE;
        
        set_user_register(shtCB, user_reg);
    }
}

//программне скидання
void sht20_soft_reset (SHT20CB *shtCB)
{
    uint8_t cmd = CMD_SOFT_RES;
    
    shtCB->i2c_write(SHT20_I2C_ADDRESS, &cmd, sizeof(cmd), NULL, 0);
}

//налаштування розрядності ацп
void sht20_set_resolution (SHT20CB *shtCB, sht20_res_set resolution)
{
    if (resolution < sht20_resol_cnt) {

        uint8_t user_reg = get_user_register(shtCB);
        
        user_reg &= SHT20_RESOL_MASK;

        switch (resolution) {

        case sht20_resol_rh12_t14:
            user_reg |= SHT20_RESOL_RH_12_BIT_T_14BIT;
            break;

        case sht20_resol_rh8_t12:
            user_reg |= SHT20_RESOL_RH_8_BIT_T_12BIT;
            break;

        case sht20_resol_rh10_t13:
            user_reg |= SHT20_RESOL_RH_10_BIT_T_13BIT;
            break;

        case sht20_resol_rh11_t11:
            user_reg |= SHT20_RESOL_RH_11_BIT_T_11BIT;
            break;

        default:
            break;

        }

        set_user_register(shtCB, user_reg);
    }
}

//увімкнення та вимкнення otp reload
void sht20_set_OTP (SHT20CB *shtCB, sht20_set otp)
{
    if (otp < sht20_set_cnt) {

        uint8_t user_reg = get_user_register(shtCB);
        
        user_reg &= SHT20_OTP_MASK;
        user_reg |= otp == sht20_set_enable ? SHT20_OTP_ENABLE : SHT20_OTP_DISABLE;
        
        set_user_register(shtCB, user_reg);
    }
}

//повертає 1, якщо живлення сенсора менше 2.25 вольт
uint8_t sht20_is_battery_low (SHT20CB *shtCB)
{
    uint8_t user_reg = get_user_register(shtCB);

    return user_reg & SHT20_BATTERY_IS_LOW ? 1 : 0;
}

#if SHT20_DOUBLE_CALC
//повертає значення температури та вологості в вигляді числа з плаваючою точкою
void sht20_get_data(SHT20CB *shtCB, double *temperature, double *humidity)
#else
//повертає значення температури та вологості в цілочисленому вигляді
void sht20_get_data (SHT20CB *shtCB, int16_t *temperature, int16_t *humidity)
#endif
{
    for (uint8_t try = 0; try < 5; try++) {
        i2c_op_res res;

        uint8_t addr_register;
        uint8_t data[2];
        uint16_t raw_t, raw_rh;

        addr_register = CMD_T_MEAS_HOLD_BY_MASTER;
        res = shtCB->i2c_read(SHT20_I2C_ADDRESS, &addr_register, sizeof(addr_register), data, sizeof(data));
        if (res != i2c_op_succes) {
            delay_us(100);
            continue;
        }
        raw_t = data[0];
        raw_t <<= 8;
        raw_t |= data[1];

        *temperature = calc_temperature_from_raw(raw_t);

        addr_register = CMD_RH_MEAS_HOLD_BY_MASTER;
        res = shtCB->i2c_read(SHT20_I2C_ADDRESS, &addr_register, sizeof(addr_register), data, sizeof(data));
        if (res != i2c_op_succes) {
            delay_us(100);
            continue;
        }
        raw_rh = data[0];
        raw_rh <<= 8;
        raw_rh |= data[1];

        *humidity = calc_humidity_from_raw(raw_rh);
        break;
    }
}
