#ifndef __NRF24L01_H__
#define __NRF24L01_H__

#include "NRF24_HAL_CROSS.h"
#include "NRF24_USER_DEFINITION.h"

typedef enum{
	tx_ok,	//ack received
	tx_err  //ack no received
}nrf24l01_tx_res;

void nrf24l01_power_down();

nrf24l01_tx_res nrf24l01_transmit_data(uint8_t tx[], uint8_t size);

#endif
