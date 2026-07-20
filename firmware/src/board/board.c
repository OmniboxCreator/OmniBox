
#include "stm32h7xx_hal.h"
#include "board.h"
#include "../drivers/drivers.h"  


static I2C_HandleTypeDef hi2c2;
static SPI_HandleTypeDef hspi1;


#define I2C2_TIMINGR 0x10707DBCu


void board_clock_init(void)
{
  
  if (HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY) != HAL_OK) board_led_fault(1);
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) { }

  
  RCC_OscInitTypeDef osc = {0};
  osc.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
  osc.HSEState  = RCC_HSE_ON;     
  osc.HSI48State = RCC_HSI48_ON;    
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLM = 5;           
  osc.PLL.PLLN = 110;          
  osc.PLL.PLLP = 1;           
  osc.PLL.PLLQ = 5;           
  osc.PLL.PLLR = 2;
  osc.PLL.PLLRGE  = RCC_PLL1VCIRANGE_2;  
  osc.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;   
  osc.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) board_led_fault(1);

  
  RCC_ClkInitTypeDef clk = {0};
  clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 |
          RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1;
  clk.SYSCLKSource  = RCC_SYSCLKSOURCE_PLLCLK;
  clk.SYSCLKDivider = RCC_SYSCLK_DIV1;   
  clk.AHBCLKDivider = RCC_HCLK_DIV2;    
  clk.APB1CLKDivider = RCC_APB1_DIV2;    
  clk.APB2CLKDivider = RCC_APB2_DIV2;
  clk.APB3CLKDivider = RCC_APB3_DIV2;
  clk.APB4CLKDivider = RCC_APB4_DIV2;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_3) != HAL_OK) board_led_fault(1);

  
  RCC_PeriphCLKInitTypeDef pk = {0};
  pk.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_FDCAN |
               RCC_PERIPHCLK_I2C2 | RCC_PERIPHCLK_USART234578;
  pk.PLL2.PLL2M = 5; pk.PLL2.PLL2N = 64; pk.PLL2.PLL2Q = 4;
  pk.PLL2.PLL2P = 2; pk.PLL2.PLL2R = 2; pk.PLL2.PLL2FRACN = 0;
  pk.PLL2.PLL2RGE  = RCC_PLL2VCIRANGE_2;  
  pk.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;   
  pk.UsbClockSelection     = RCC_USBCLKSOURCE_HSI48;
  pk.FdcanClockSelection    = RCC_FDCANCLKSOURCE_PLL2;  
  pk.I2c123ClockSelection   = RCC_I2C123CLKSOURCE_HSI;
  pk.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&pk) != HAL_OK) board_led_fault(1);

  
  __HAL_RCC_CRS_CLK_ENABLE();
  RCC_CRSInitTypeDef crs = {0};
  crs.Prescaler       = RCC_CRS_SYNC_DIV1;
  crs.Source        = RCC_CRS_SYNC_SOURCE_USB1;
  crs.Polarity       = RCC_CRS_SYNC_POLARITY_RISING;
  crs.ReloadValue      = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000U, 1000U);
  crs.ErrorLimitValue    = RCC_CRS_ERRORLIMIT_DEFAULT;
  crs.HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT;
  HAL_RCCEx_CRSConfig(&crs);

  board_micros_init();  
}

uint32_t board_millis(void) { return HAL_GetTick(); }


void board_micros_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
uint32_t board_micros(void) { return DWT->CYCCNT / (BOARD_SYSCLK_HZ / 1000000u); }


void board_gpio_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE(); __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE(); __HAL_RCC_GPIOH_CLK_ENABLE();

  GPIO_InitTypeDef g = {0};

  
  g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  g.Alternate = GPIO_AF5_SPI1; g.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  HAL_GPIO_Init(GPIOA, &g);

  
  g.Alternate = GPIO_AF10_OTG1_FS; g.Pin = GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOA, &g);

  
  g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Alternate = GPIO_AF9_FDCAN1; g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_12 | GPIO_PIN_13;
  HAL_GPIO_Init(GPIOD, &g);
  
  g.Alternate = GPIO_AF9_FDCAN2; g.Pin = GPIO_PIN_12 | GPIO_PIN_13;
  HAL_GPIO_Init(GPIOB, &g);

  
  g.Mode = GPIO_MODE_AF_OD; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Alternate = GPIO_AF4_I2C2; g.Pin = GPIO_PIN_10 | GPIO_PIN_11;
  HAL_GPIO_Init(GPIOB, &g);

  
  g.Mode = GPIO_MODE_AF_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Alternate = GPIO_AF7_USART2;
  g.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9;
  HAL_GPIO_Init(GPIOD, &g);
  
  g.Alternate = GPIO_AF8_UART4; g.Pin = GPIO_PIN_10 | GPIO_PIN_11;
  HAL_GPIO_Init(GPIOC, &g);

  
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);
  g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Pin = GPIO_PIN_4 | GPIO_PIN_5; HAL_GPIO_Init(GPIOC, &g);
  g.Pin = GPIO_PIN_0 | GPIO_PIN_1; HAL_GPIO_Init(GPIOB, &g);

  
  g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_PULLUP;
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8; HAL_GPIO_Init(GPIOC, &g);

  
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_RESET);
  g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Pin = GPIO_PIN_2 | GPIO_PIN_3; HAL_GPIO_Init(GPIOE, &g);

  
  g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_NOPULL;
  g.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6; HAL_GPIO_Init(GPIOE, &g);
}


void board_periph_init(void)
{
  
  __HAL_RCC_I2C2_CLK_ENABLE();
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = I2C2_TIMINGR;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK) board_led_fault(1);

  
  __HAL_RCC_SPI1_CLK_ENABLE();
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;   
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;         
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE; 
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) board_led_fault(1);

  
  HAL_PWREx_EnableUSBVoltageDetector();
  
  __HAL_RCC_USB1_OTG_HS_FORCE_RESET();
  __HAL_RCC_USB1_OTG_HS_RELEASE_RESET();
  
  __HAL_RCC_USB1_OTG_HS_CLK_ENABLE();
  
  __HAL_RCC_USB1_OTG_HS_ULPI_CLK_SLEEP_DISABLE();

  
  kline_init();

  
}


void board_iso_power_enable(int on)
{
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


void board_led_fault(int on)
{
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


int board_recovery_strapped(void)
{
  __HAL_RCC_GPIOE_CLK_ENABLE();
  GPIO_InitTypeDef g = {0};
  g.Pin = GPIO_PIN_1; g.Mode = GPIO_MODE_INPUT; g.Pull = GPIO_PULLUP; g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &g);
  for (volatile int i = 0; i < 20000; i++) { }  
  return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_RESET;  
}


#define I2C_TIMEOUT_MS 50u

int board_i2c_write(uint8_t addr7, const uint8_t *buf, size_t n)
{
  HAL_StatusTypeDef s = HAL_I2C_Master_Transmit(&hi2c2, (uint16_t)(addr7 << 1),
                         (uint8_t *)buf, (uint16_t)n, I2C_TIMEOUT_MS);
  return (s == HAL_OK) ? 0 : -1;
}

int board_i2c_read(uint8_t addr7, const uint8_t *reg, size_t reg_n,
          uint8_t *buf, size_t n)
{
  if (reg && reg_n) {
    HAL_StatusTypeDef s = HAL_I2C_Master_Transmit(&hi2c2, (uint16_t)(addr7 << 1),
                           (uint8_t *)reg, (uint16_t)reg_n,
                           I2C_TIMEOUT_MS);
    if (s != HAL_OK) return -1;
  }
  HAL_StatusTypeDef s = HAL_I2C_Master_Receive(&hi2c2, (uint16_t)(addr7 << 1),
                         buf, (uint16_t)n, I2C_TIMEOUT_MS);
  return (s == HAL_OK) ? 0 : -1;
}


#define SPI_TIMEOUT_MS 50u
static uint8_t s_spi_scratch[64];
int board_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t n)
{
  if (n == 0) return 0;
  if (n > sizeof(s_spi_scratch)) return -1;
  const uint8_t *ptx = tx;
  uint8_t *prx = rx;
  if (!ptx) { for (size_t i = 0; i < n; i++) s_spi_scratch[i] = 0xFF; ptx = s_spi_scratch; }
  if (!prx) prx = s_spi_scratch;    
  HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)ptx, prx,
                         (uint16_t)n, SPI_TIMEOUT_MS);
  return (s == HAL_OK) ? 0 : -1;
}
