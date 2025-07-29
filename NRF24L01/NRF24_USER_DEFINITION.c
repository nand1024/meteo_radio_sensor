#include <stdint.h>
#include "NRF24_HAL_CROSS.h"
#include "tim.h"
#include "spi.h"
#include "gpio.h"

static void csEnable()
{
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);
}

static void csDisable()
{
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4);
}

static uint8_t transferByte(uint8_t value)
{
    LL_SPI_TransmitData8(SPI1, value);
    while(!LL_SPI_IsActiveFlag_RXNE(SPI1));
    return LL_SPI_ReceiveData8(SPI1);
}

static void transferBuff(uint8_t *data, uint8_t len)
{
  for (uint8_t i = 0; i < len; i++) {
      data[i] = transferByte(data[i]);
  }
}

void nrf24l01_DelayMs(uint16_t ms)
{
    LL_mDelay(ms);
}

void nrf24l01_DelayUs(uint16_t us)
{
    delay_us(us);
}

void chipEnable(void)
{
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_1);
    //nrf24l01_DelayUs(10);
}

void chipDisable(void)
{
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_1);
    //nrf24l01_DelayUs(5);
}

void nrf24l01_spi_init(void)
{
  //функції зворотнього виклику SPI
  SPI_controls_CB SpiCB;

  SpiCB.csEnable = csEnable;
  SpiCB.csDisable = csDisable;
  SpiCB.transferByte = transferByte;
  SpiCB.transferByteArray = transferBuff;

  //ініціалізація nrf24_hal
  hal_nrf_init(&SpiCB);

  //відключення чіпа
  chipDisable();
}



