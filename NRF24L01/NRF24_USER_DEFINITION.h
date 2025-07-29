#ifndef __NRF24_USER_DEFINITION_
#define __NRF24_USER_DEFINITION_

#include <stdint.h>

void nrf24l01_DelayMs(uint16_t ms);
void nrf24l01_DelayUs(uint16_t us);

void chipEnable(void);
void chipDisable(void);
void nrf24l01_spi_init(void);

#endif