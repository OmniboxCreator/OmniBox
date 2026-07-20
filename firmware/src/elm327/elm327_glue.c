
#include "tusb.h"
#include "elm327.h"
#include "elm327_glue.h"
#include "../board/board.h"
#include "../drivers/drivers.h"

static elm327_t s_elm;


static void ops_write(void *u, const char *s, uint16_t n)
{
  (void)u;
  uint32_t sent = 0;
  if (!tud_cdc_connected()) return;     
  while (sent < n) {
    uint32_t k = tud_cdc_write(s + sent, n - sent);
    sent += k;
    tud_cdc_write_flush();
    if (k == 0) tud_task();        
  }
}


static int ops_can_open(void *u, uint32_t baud)
{ (void)u; return can_open(CAN_CH1, baud, 0, 0); }

static void ops_can_close(void *u)
{ (void)u; can_close(CAN_CH1); }

static int ops_can_tx(void *u, uint32_t id, int ext, const uint8_t *d, uint8_t n)
{ (void)u; return can_tx(CAN_CH1, id, d, n, ext ? J2534_CAN_29BIT_ID : 0u); }

static int ops_can_rx(void *u, uint32_t *id, int *ext, uint8_t *d, uint8_t *n)
{
  (void)u;
  uint8_t e8 = 0;
  if (!can_read(CAN_CH1, id, d, n, &e8)) return 0;
  *ext = e8;
  return 1;
}


static int ops_vbatt(void *u, uint32_t *mv) { (void)u; return feps_read_vbatt_mv(mv); }
static uint32_t ops_millis(void *u) { (void)u; return board_millis(); }

static const elm327_ops_t OPS = {
  .user = 0,
  .write = ops_write,
  .can_open = ops_can_open, .can_close = ops_can_close,
  .can_tx = ops_can_tx, .can_rx = ops_can_rx,
  .read_vbatt_mv = ops_vbatt, .millis = ops_millis,
};

void elm327_usb_init(void) { elm327_init(&s_elm, &OPS); }
void elm327_usb_service(void) { elm327_service(&s_elm); tud_cdc_write_flush(); }
void elm327_usb_rx(const uint8_t *d, uint32_t n) { elm327_input(&s_elm, d, n); }
