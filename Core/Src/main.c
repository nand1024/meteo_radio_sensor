/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "main.h"
#include "i2c.h"

#include "lptim.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_util.h"
#include "NRF24_HAL_CROSS.h"
#include "NRF24_USER_DEFINITION.h"
#include "NRF24L01_API.h"
#include "sht20.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SIZE_PAYLOAD 32
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)//колбек для роботи функції printf з виводом по uart, необхідно лише під час трасування
{
    LL_USART_TransmitData8(USART2, ch);
    while(!LL_USART_IsActiveFlag_TXE(USART2));
    return ch;
}

void serialize(int16_t val, uint8_t *arr)
{
    arr[0] = val >> 8;
    arr[1] = val & 0xFF;
}

void deserialize(int16_t *val, uint8_t *arr)
{
    *val = arr[0];
    *val <<= 8;
    *val |= arr[1];
}

void enter_stop_mode()
{
    //конфігурація PWR живлення для deep sleep
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE3);     //1.2v напруга живлення регулятора
    LL_PWR_SetRegulModeDS(LL_PWR_REGU_DSMODE_LOW_POWER);           //переключення на режим низького споживання, коли переходимо в deep sleep
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);                         //режим mode stop при входження в deep sleep
    //-------------------
    LL_PWR_ClearFlag_WU();                                         //очищаємо прапорець переривання Wake-up
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29);                        //вмикажмо EXTI переривання для LPTIM1
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;                             //Увімкнути режим Sleep Deep
    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_MSI);   //при пробудженні задіюємо MSI як джерело тактування
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_29);                       //знімаємо прапорець EXTI переривання для LPTIM1
    //запускаємо таймер LPTIM1 по перериванню якого будемо пробуджуватися
    LL_LPTIM_Enable(LPTIM1);                                       //вмикаємо LPTIM1
    LL_LPTIM_SetAutoReload(LPTIM1, 0xFFFF);                        //для цього проекту ставимо максимальне значення відліку
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_ONESHOT);//запуск одиночного відліку
    __WFI();                                                       //Зупиняємо все і очікуємо переривання
}


uint8_t check_diff_values(int16_t a, int16_t b, int16_t diff_vall)//повертає 1 якщо різниця між двома значеннями >= diff_vall
{
    int16_t c = a - b;
    if (c >= diff_vall || c <= -diff_vall) {
        return 1;
    }
    return 0;
}

void init_all_periph()
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    /* SysTick_IRQn interrupt configuration */
    NVIC_SetPriority(SysTick_IRQn, 3);
    /* Configure the system clock */
    SystemClock_Config();
    /** PVD Configuration
    */
    LL_PWR_SetPVDLevel(LL_PWR_PVDLEVEL_2);//встановлюємо детектор напруги на 2.3 вольти

    /** Enable the PVD Output
    */
    LL_PWR_EnablePVD();//вмикаємо детектор напруги
    MX_LPTIM1_Init();  //low power таймер для пробудження з sleep deep
    MX_GPIO_Init();
    MX_I2C1_Init();
    //MX_USART2_UART_Init();
    MX_SPI1_Init();
    MX_TIM2_Init();
    nrf24l01_spi_init();
    nrf24l01_power_down();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
    int16_t now_temp, prew_temp;
    int16_t now_hum, prew_hum;
    now_temp = prew_temp = -9999; //ініціалізуємо не існуючим значенням
    now_hum = prew_hum = -9999;   //ініціалізуємо не існуючим значенням
    uint8_t low_battery_trigger = 0;
    uint8_t tx_buffer[SIZE_PAYLOAD] = {0};
    nrf24l01_tx_res res_tx = tx_err;

    //ініціалізуємо колбеки i2c для датчика sht20
    SHT20CB sht20CB = {
          .i2c_read = i2c_read255,
          .i2c_write = i2c_write255,
    };

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, 3);

  /** PVD Configuration
  */
  LL_PWR_SetPVDLevel(LL_PWR_PVDLEVEL_2);//встановлюємо детектор напруги на 2.3 вольти

  /** Enable the PVD Output
  */
  LL_PWR_EnablePVD();//вмикаємо детектор напруги

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_LPTIM1_Init();
  MX_GPIO_Init();
  MX_I2C1_Init();
  //MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  nrf24l01_spi_init();
  nrf24l01_power_down();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //світлодіодна індикація успішного включення пристрою
  for (uint8_t i = 0; i < 2; i++) {
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1);
    LL_mDelay(50);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
    LL_mDelay(450);
  }
  sht20_get_data(&sht20CB, &now_temp, &now_hum);          //перший запуск вимірювань
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      sht20_get_data(&sht20CB, &now_temp, &now_hum);    //отримуємо дані температури та вологсті повітря
    if (LL_PWR_IsActiveFlag_PVDO()) {                   //якщо спрацював детектор напруги
        low_battery_trigger = 1;
    }
    if (res_tx == tx_err                                //якщо попередня передача даних була невдалою
        || check_diff_values(now_temp, prew_temp, 25)   //якщо температура змінилась на 0.25 градуси або більше
        || check_diff_values(now_hum, prew_hum, 100)) { //якщо волога змінилась на 1% або більше

        prew_temp = now_temp;
        prew_hum = now_hum;
        tx_buffer[0] = 0xAB;                            //"магічне" число. просто для позначення початку пакету.
        serialize(now_temp, &tx_buffer[1]);
        serialize(now_hum, &tx_buffer[3]);
        tx_buffer[5] = low_battery_trigger ? 'L' : 'H';
        res_tx = nrf24l01_transmit_data(tx_buffer, SIZE_PAYLOAD);//відправляємо оновленні дані на метеостанцію
    }

    enter_stop_mode();//засинаємо... на ~3 хвилин 46 секунд

    init_all_periph();//ініціалізація після пробудження
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_0)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  while (LL_PWR_IsActiveFlag_VOS() != 0)
  {
  }
  LL_RCC_LSI_Enable();

   /* Wait till LSI is ready */
  while(LL_RCC_LSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_Enable();

   /* Wait till MSI is ready */
  while(LL_RCC_MSI_IsReady() != 1)
  {

  }
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_1);
  LL_RCC_MSI_SetCalibTrimming(0);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI)
  {

  }

  LL_Init1msTick(131072);

  LL_SetSystemCoreClock(131072);
  LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
  LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSI);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
