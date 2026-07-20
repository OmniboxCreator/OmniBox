
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>


#define BOARD_HSE_HZ   25000000u  
#define BOARD_SYSCLK_HZ  550000000u  




enum { CAN_CH1 = 0, CAN_CH2, CAN_CH3, CAN_CH4_MCP1, CAN_CH5_MCP2, SWCAN_MCP3, CAN_CH_COUNT };

enum { KLINE_CH1 = 0, KLINE_CH2, KLINE_CH3, KLINE_CH_COUNT };


void board_clock_init(void);   
void board_gpio_init(void);    
void board_periph_init(void);   
uint32_t board_millis(void);   
void   board_micros_init(void); 
uint32_t board_micros(void);   
void board_led_fault(int on);
void board_iso_power_enable(int on); 


int board_recovery_strapped(void);


#include <stddef.h>
int board_i2c_write(uint8_t addr7, const uint8_t *buf, size_t n);
int board_i2c_read(uint8_t addr7, const uint8_t *reg, size_t reg_n,
          uint8_t *buf, size_t n);


int board_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t n);

#endif 
