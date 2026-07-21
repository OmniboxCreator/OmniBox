
#ifndef DRIVERS_H
#define DRIVERS_H

#include <stdint.h>
#include <stddef.h>
#include "../j2534/j2534_defs.h"


void can_init(void);
void can_service(void);
int can_open(uint8_t ch, uint32_t nominal_baud, uint32_t data_baud, uint32_t flags);
int can_close(uint8_t ch);
int can_tx(uint8_t ch, uint32_t id, const uint8_t *data, uint8_t len, uint32_t flags);

int can_read(uint8_t ch, uint32_t *id, uint8_t *data, uint8_t *len, uint32_t *flags);
void can_irq(uint8_t ch);  


void kline_init(void);
void kline_service(void);
int kline_open(uint8_t ch, uint32_t baud);  
int kline_five_baud_init(uint8_t ch, uint8_t addr, uint8_t *keybytes);
int kline_fast_init(uint8_t ch, const uint8_t *tx, uint8_t n, uint8_t *rx, uint8_t *rn);
int kline_tx(uint8_t ch, const uint8_t *data, uint16_t n);
int kline_read(uint8_t ch, uint8_t *byte);  
void kline_irq(uint8_t ch);          


void j1850_init(void);
void j1850_service(void);
int j1850_set_mode_vpw(int vpw);   
int j1850_tx(const uint8_t *data, uint16_t n);
int j1850_read(uint8_t *buf, uint16_t *len); 
void j1850_capture(void);           


void swcan_init(void);
int swcan_set_mode(uint8_t mode);  


void feps_init(void);
int feps_set_voltage_mv(uint32_t mv);  
int feps_read_prog_mv(uint32_t *mv);   
int feps_read_vbatt_mv(uint32_t *mv);  
int feps_route_to_pin(uint8_t j1962_pin);
int feps_short_to_ground(uint8_t j1962_pin); 
void feps_off(void);           


int gpt_set(uint8_t gpt_idx, int on);  


void matrix_init(void);
int matrix_clear(void);            
int matrix_set_bits(uint8_t pca, uint16_t set_mask, uint16_t clr_mask); 
int matrix_test_relay(uint8_t pca_idx, uint8_t bit, uint8_t on);    
int matrix_can_connect(uint8_t can_ch);    
int matrix_can1_polarity_swap(int swapped);  
int matrix_can1_termination(int on);     
int matrix_kline_route(uint8_t kch, uint8_t j1962_pin); 
int matrix_read_fault_flags(uint8_t *ff);   

#endif 
