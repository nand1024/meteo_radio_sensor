#include "NRF24L01_API.h"
#include <stdint.h>
#include <stdlib.h>

#define CHANNEL 20

static void init_tx()
{
    //очищаємо буфери rx tx
    hal_nrf_flush_rx();
    hal_nrf_flush_tx();

    //простий та фіксований формат пакету
    hal_nrf_enable_dynamic_payload(false);
    hal_nrf_enable_ack_payload(false);
    hal_nrf_enable_dynamic_ack(false);

    hal_nrf_set_rf_channel(CHANNEL);
    hal_nrf_set_datarate(HAL_NRF_1MBPS);
    hal_nrf_set_crc_mode(HAL_NRF_CRC_16BIT);

    hal_nrf_set_operation_mode(HAL_NRF_PTX);
    hal_nrf_config_tx(NULL, HAL_NRF_18DBM, 3, 1500);//для економії енергії обираємо найнижчу потужність передавача
}

void nrf24l01_power_down()
{
    hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
}


nrf24l01_tx_res nrf24l01_transmit_data(uint8_t tx[], uint8_t size)
{
    nrf24l01_tx_res result;
    uint8_t irq_flg;

    //вмикаємо радіомодуль
    hal_nrf_set_power_mode(HAL_NRF_PWR_UP);
    nrf24l01_DelayUs(1500);

    //ініціалізуємо радіомодуль в режим передавача
    init_tx();

    //відправляємо дані для передачі радіомодулю
    hal_nrf_write_tx_payload(tx, size <= NRF_MAX_PL ? size : NRF_MAX_PL);

    hal_nrf_get_clear_irq_flags();

    // робимо пульс на лінії CE для передачі даних
    chipEnable();
    nrf24l01_DelayUs(100);
    chipDisable();

    // очікуємо до 2мс, поки не завершиться передача даних
    int i;
    for (i = 0; i < 20; i++) {
        irq_flg = hal_nrf_get_clear_irq_flags();
        if (irq_flg & ((1U << HAL_NRF_TX_DS) | (1U << HAL_NRF_MAX_RT))) break;
        nrf24l01_DelayUs(100);
    }

    //перевірка успішності передачі даних
    if ((irq_flg & (1U << HAL_NRF_MAX_RT)) || i >= 20) {
        hal_nrf_flush_tx();
        result = tx_err;
    } else {// tx ok
        result = tx_ok;
    }

    // переводим радіомодуль в режим "сну"
    hal_nrf_set_power_mode(HAL_NRF_PWR_DOWN);
    return result;
}
