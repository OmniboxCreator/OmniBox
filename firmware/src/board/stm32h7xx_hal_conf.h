
#ifndef STM32H7xx_HAL_CONF_H
#define STM32H7xx_HAL_CONF_H


#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_FDCAN_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_PCD_MODULE_ENABLED     


#if !defined(HSE_VALUE)
#define HSE_VALUE    25000000UL   
#endif
#define HSE_STARTUP_TIMEOUT 100UL
#define CSI_VALUE    4000000UL
#define HSI_VALUE    64000000UL
#define LSI_VALUE    32000UL
#define LSE_VALUE    32768UL
#define LSE_STARTUP_TIMEOUT 5000UL
#define EXTERNAL_CLOCK_VALUE 12288000UL


#define VDD_VALUE        3300UL
#define TICK_INT_PRIORITY    0x0FUL
#define USE_RTOS         0U
#define USE_SPI_CRC       0U
#define USE_HAL_FDCAN_REGISTER_CALLBACKS 0U
#define USE_HAL_SPI_REGISTER_CALLBACKS  0U
#define USE_HAL_I2C_REGISTER_CALLBACKS  0U
#define USE_HAL_UART_REGISTER_CALLBACKS 0U
#define USE_HAL_TIM_REGISTER_CALLBACKS  0U
#define USE_HAL_ADC_REGISTER_CALLBACKS  0U
#define USE_HAL_PCD_REGISTER_CALLBACKS  0U
#define PREFETCH_ENABLE     0U
#define USE_SPI_CRC       0U



#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif


#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_gpio.h"
#include "stm32h7xx_hal_dma.h"
#include "stm32h7xx_hal_cortex.h"
#include "stm32h7xx_hal_flash.h"
#include "stm32h7xx_hal_pwr.h"
#include "stm32h7xx_hal_fdcan.h"
#include "stm32h7xx_hal_spi.h"
#include "stm32h7xx_hal_i2c.h"
#include "stm32h7xx_hal_uart.h"
#include "stm32h7xx_hal_tim.h"
#include "stm32h7xx_hal_adc.h"
#include "stm32h7xx_hal_pcd.h"

#endif 
