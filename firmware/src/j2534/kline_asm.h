
#ifndef KLINE_ASM_H
#define KLINE_ASM_H

#include <stdint.h>

#define KLINE_MSG_MAX 264u  


enum { KLINE_MODE_TIMING = 0, KLINE_MODE_ISO14230 = 1 };

typedef struct {
  uint8_t buf[KLINE_MSG_MAX];
  uint16_t len;
  uint32_t last_ms;    
  uint8_t mode;      
} kline_asm_t;

void kline_asm_init(kline_asm_t *a);
void kline_asm_set_mode(kline_asm_t *a, uint8_t mode);


void kline_asm_push(kline_asm_t *a, uint8_t b, uint32_t now);


int kline_asm_ready(const kline_asm_t *a, uint32_t now, uint32_t gap_ms);


int kline_asm_ready_len(const kline_asm_t *a);


void kline_asm_reset(kline_asm_t *a);

#endif 
