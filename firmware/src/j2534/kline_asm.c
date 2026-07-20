
#include "kline_asm.h"

void kline_asm_init(kline_asm_t *a) { a->len = 0; a->last_ms = 0; a->mode = KLINE_MODE_TIMING; }

void kline_asm_set_mode(kline_asm_t *a, uint8_t mode) { a->mode = mode; }


static uint16_t iso14230_expected(const uint8_t *b, uint16_t n)
{
  if (n < 1u) return 0;
  uint8_t fmt = b[0];
  uint8_t naddr = ((fmt >> 6) & 0x3u) ? 2u : 0u;
  uint8_t lenfield = fmt & 0x3Fu;
  uint16_t hdr = (uint16_t)(1u + naddr + (lenfield == 0u ? 1u : 0u));
  if (n < hdr) return 0;                
  uint16_t dlen = (lenfield != 0u) ? lenfield : b[1u + naddr];
  return (uint16_t)(hdr + dlen + 1u);         
}

int kline_asm_ready_len(const kline_asm_t *a)
{
  if (a->mode != KLINE_MODE_ISO14230) return 0;
  uint16_t exp = iso14230_expected(a->buf, a->len);
  return (exp != 0u && a->len >= exp);
}

void kline_asm_push(kline_asm_t *a, uint8_t b, uint32_t now)
{
  if (a->len >= KLINE_MSG_MAX) return;   
  a->buf[a->len++] = b;
  a->last_ms = now;
}

int kline_asm_ready(const kline_asm_t *a, uint32_t now, uint32_t gap_ms)
{
  if (a->len == 0u) return 0;
  if (a->len >= KLINE_MSG_MAX) return 1;        
  return (uint32_t)(now - a->last_ms) >= gap_ms;    
}

void kline_asm_reset(kline_asm_t *a) { a->len = 0; }
