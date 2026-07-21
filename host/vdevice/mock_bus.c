
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../firmware/src/drivers/drivers.h"
#include "../../firmware/src/board/board.h"
#include "../../firmware/src/j2534/isotp.h"


static uint32_t g_ms;
uint32_t board_millis(void) { return g_ms++; }
uint32_t board_micros(void) { return g_ms * 1000u; }


#define ECU_REQ_ID 0x7E0u  
#define ECU_RSP_ID 0x7E8u  


#define ISO_NF_PHYS 0x18DA0000u  
#define ISO_NF_FUNC 0x18DB0000u  
#define TESTER_SA  0xF1u     
#define ECU_PHYS_29 0x0Eu     

typedef struct { uint32_t id; uint8_t data[8]; uint8_t len; } busframe_t;
static busframe_t ecu_q[256];     
static int ecu_qh, ecu_qt;

static isotp_t ecu_tp;
static uint8_t ecu_rx[4096], ecu_tx[4096];
static int   ecu_inited;
static uint32_t ecu_rsp_id = ECU_RSP_ID; 


static uint8_t ecu_session = 0x01;   
static uint8_t ecu_unlocked;      
static uint8_t ecu_xfer_upload;    
static uint32_t ecu_up_addr;      
static uint32_t ecu_up_rem;       

#define UDS_NRC_GENERIC   0x11u    
#define UDS_NRC_OUT_OF_RANGE 0x31u    
#define XFER_BLOCK_MAX    1024u    


static uint8_t *g_dump;
static uint32_t g_dump_size, g_dump_base;

int mock_bus_load_dump(const char *path, uint32_t base)
{
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz <= 0) { fclose(f); return -2; }
  uint8_t *buf = (uint8_t *)malloc((size_t)sz);
  if (!buf) { fclose(f); return -3; }
  size_t rd = fread(buf, 1, (size_t)sz, f);
  fclose(f);
  if (rd != (size_t)sz) { free(buf); return -4; }
  free(g_dump);
  g_dump = buf; g_dump_size = (uint32_t)sz; g_dump_base = base;
  return 0;
}

static uint32_t rd_be(const uint8_t *p, uint8_t n)
{
  uint32_t v = 0;
  for (uint8_t i = 0; i < n; i++) v = (v << 8) | p[i];
  return v;
}

static void ecu_queue(uint32_t id, const uint8_t *d, uint8_t n)
{
  int next = (ecu_qh + 1) % 256;
  if (next == ecu_qt) return;    
  ecu_q[ecu_qh].id = id; ecu_q[ecu_qh].len = n;
  memset(ecu_q[ecu_qh].data, 0, 8);
  for (uint8_t i = 0; i < n && i < 8; i++) ecu_q[ecu_qh].data[i] = d[i];
  ecu_qh = next;
}

void mock_bus_queue_can(uint32_t id, const uint8_t *data, uint8_t len)
{
  ecu_queue(id, data, len);
}


static uint16_t fill_pattern(uint8_t *p, uint16_t n)
{
  for (uint16_t i = 0; i < n; i++) p[i] = (uint8_t)(0xA0 + i);
  return n;
}


static uint16_t mem_serve(uint32_t addr, uint8_t *out, uint16_t len)
{
  if (g_dump) {
    if (addr < g_dump_base) return 0;
    uint32_t off = addr - g_dump_base;
    if (off >= g_dump_size) return 0;
    uint32_t avail = g_dump_size - off;
    if (len > avail) len = (uint16_t)avail;
    memcpy(out, g_dump + off, len);
    return len;
  }
  return fill_pattern(out, len);  
}


static void ecu_build_response(const uint8_t *req, uint16_t n)
{
  uint8_t *r = ecu_tx; uint16_t rn = 0;
  uint8_t sid = (n > 0) ? req[0] : 0;
  uint8_t sub = (n > 1) ? req[1] : 0;
  switch (sid) {
  case 0x3E:                 
    r[0] = 0x7E; r[1] = sub; rn = 2; break;
  case 0x10:                 
    ecu_session = sub;
    r[0] = 0x50; r[1] = sub; r[2] = 0x00; r[3] = 0x32; r[4] = 0x01; r[5] = 0xF4; rn = 6; break;
  case 0x11:                 
    r[0] = 0x51; r[1] = sub; rn = 2; break;
  case 0x27:                 
    if (sub & 1u) {             
      r[0] = 0x67; r[1] = sub;
      if (ecu_unlocked) { r[2] = r[3] = r[4] = r[5] = 0x00; }  
      else { r[2] = 0xDE; r[3] = 0xAD; r[4] = 0xBE; r[5] = 0xEF; }
      rn = 6;
    } else {                
      ecu_unlocked = 1; r[0] = 0x67; r[1] = sub; rn = 2;
    }
    break;
  case 0x22: {                
    if (n < 3) { r[0] = 0x7F; r[1] = sid; r[2] = 0x13; rn = 3; break; }  
    uint16_t did = (uint16_t)((req[1] << 8) | req[2]);
    r[0] = 0x62; r[1] = req[1]; r[2] = req[2];
    uint16_t dl = (did == 0xF190u) ? 100u : 4u;  
    rn = (uint16_t)(3 + fill_pattern(r + 3, dl));
    break; }
  case 0x2E:                 
    r[0] = 0x6E; r[1] = (n > 1) ? req[1] : 0; r[2] = (n > 2) ? req[2] : 0; rn = 3; break;
  case 0x23: {                
    uint8_t alfid = sub, alen = (uint8_t)(alfid & 0x0F), slen = (uint8_t)((alfid >> 4) & 0x0F);
    uint32_t addr = 0, size = 0;
    if ((2u + alen + slen) <= n) { addr = rd_be(req + 2, alen); size = rd_be(req + 2 + alen, slen); }
    if (size == 0) size = 16u;
    if (size > XFER_BLOCK_MAX) size = XFER_BLOCK_MAX;  
    uint16_t got = mem_serve(addr, r + 1, (uint16_t)size);
    if (got == 0) { r[0] = 0x7F; r[1] = sid; r[2] = UDS_NRC_OUT_OF_RANGE; rn = 3; }  
    else { r[0] = 0x63; rn = (uint16_t)(1 + got); }
    break; }
  case 0x31:                 
    r[0] = 0x71; r[1] = sub; r[2] = (n > 2) ? req[2] : 0; r[3] = (n > 3) ? req[3] : 0; rn = 4; break;
  case 0x34: case 0x35: {           
    ecu_xfer_upload = (sid == 0x35) ? 1u : 0u;
    ecu_up_addr = 0; ecu_up_rem = 0;
    if (n >= 3) {              
      uint8_t alfid = req[2], alen = (uint8_t)(alfid & 0x0F), slen = (uint8_t)((alfid >> 4) & 0x0F);
      if ((3u + alen + slen) <= n) { ecu_up_addr = rd_be(req + 3, alen); ecu_up_rem = rd_be(req + 3 + alen, slen); }
    }
    r[0] = (uint8_t)(sid + 0x40); r[1] = 0x20; r[2] = 0x04; r[3] = 0x02; rn = 4; break; }
  case 0x36:                 
    r[0] = 0x76; r[1] = sub; rn = 2;
    if (ecu_xfer_upload && ecu_up_rem) {
      uint16_t chunk = (ecu_up_rem > XFER_BLOCK_MAX) ? (uint16_t)XFER_BLOCK_MAX : (uint16_t)ecu_up_rem;
      uint16_t got = mem_serve(ecu_up_addr, r + 2, chunk);
      rn = (uint16_t)(2 + got);
      ecu_up_addr += got; ecu_up_rem -= got;
    }
    break;
  case 0x37:                 
    r[0] = 0x77; rn = 1; break;
  case 0x14:                 
    r[0] = 0x54; rn = 1; break;
  case 0x85:                 
    r[0] = 0xC5; r[1] = sub; rn = 2; break;
  case 0x28:                 
    r[0] = 0x68; r[1] = sub; rn = 2; break;
  default:                  
    r[0] = 0x7F; r[1] = sid; r[2] = UDS_NRC_GENERIC; rn = 3; break;
  }
  isotp_frame_t f;
  if (isotp_send_start(&ecu_tp, ecu_tx, rn, &f)) ecu_queue(ecu_rsp_id, f.data, f.len);
}


static int ecu_accept(uint32_t id)
{
  if (id == ECU_REQ_ID) { ecu_rsp_id = ECU_RSP_ID; return 1; }    
  if ((id & 0xFFFF0000u) == ISO_NF_PHYS && (id & 0xFFu) == TESTER_SA) {
    uint8_t ta = (uint8_t)((id >> 8) & 0xFFu);           
    if (ta != ECU_PHYS_29) return 0;                
    ecu_rsp_id = ISO_NF_PHYS | ((uint32_t)TESTER_SA << 8) | ta;   
    return 1;
  }
  return 0;  
}


static void ecu_on_frame(uint32_t id, const uint8_t *d, uint8_t n)
{
  if (!ecu_inited) { isotp_init(&ecu_tp, ecu_rx, sizeof ecu_rx, 0, 0); ecu_inited = 1; }
  if (!ecu_accept(id)) return;        
  isotp_frame_t out; int send = 0;
  int done = isotp_rx(&ecu_tp, d, n, &out, &send);  
  if (send) ecu_queue(ecu_rsp_id, out.data, out.len);     
  if (done) ecu_build_response(ecu_rx, ecu_tp.rx_len);     
  if (ecu_tp.tx_state == ISOTP_TX_SEND_CF) {          
    isotp_frame_t f;
    while (isotp_send_next(&ecu_tp, &f)) ecu_queue(ecu_rsp_id, f.data, f.len);
  }
}

void mock_bus_reset(void)
{
  g_ms = 0; ecu_qh = ecu_qt = 0; ecu_inited = 0;
  ecu_rsp_id = ECU_RSP_ID;
  ecu_session = 0x01; ecu_unlocked = 0; ecu_xfer_upload = 0;
  ecu_up_addr = 0; ecu_up_rem = 0;    
  memset(&ecu_tp, 0, sizeof ecu_tp);
}


void can_init(void) {}
void can_service(void) {}
int can_open(uint8_t c, uint32_t a, uint32_t b, uint32_t f) { (void)c;(void)a;(void)b;(void)f; return 0; }
int can_close(uint8_t c) { (void)c; return 0; }
void can_irq(uint8_t c) { (void)c; }
int can_tx(uint8_t ch, uint32_t id, const uint8_t *data, uint8_t len, uint32_t flags)
{
  (void)ch; (void)flags; ecu_on_frame(id, data, len); return 0;
}
int can_read(uint8_t ch, uint32_t *id, uint8_t *data, uint8_t *len, uint8_t *ext)
{
  (void)ch;
  if (ecu_qt == ecu_qh) return 0;
  busframe_t *f = &ecu_q[ecu_qt];
  *id = f->id; *len = f->len; *ext = (f->id > 0x7FFu) ? 1u : 0u;  
  for (uint8_t i = 0; i < f->len; i++) data[i] = f->data[i];
  ecu_qt = (ecu_qt + 1) % 256;
  return 1;
}


void kline_init(void) {}
void kline_service(void) {}
int kline_open(uint8_t c, uint32_t b) { (void)c;(void)b; return 0; }
int kline_five_baud_init(uint8_t c, uint8_t a, uint8_t *k) { (void)c;(void)a;(void)k; return 0; }
int kline_fast_init(uint8_t c, const uint8_t *t, uint8_t n, uint8_t *r, uint8_t *rn) { (void)c;(void)t;(void)n;(void)r;(void)rn; return 0; }
int kline_tx(uint8_t c, const uint8_t *d, uint16_t n) { (void)c;(void)d;(void)n; return 0; }
int kline_read(uint8_t c, uint8_t *b) { (void)c;(void)b; return 0; }
void kline_irq(uint8_t c) { (void)c; }
void j1850_init(void) {}
void j1850_service(void) {}
int j1850_set_mode_vpw(int v) { (void)v; return 0; }
int j1850_tx(const uint8_t *d, uint16_t n) { (void)d;(void)n; return 0; }
int j1850_read(uint8_t *b, uint16_t *l) { (void)b;(void)l; return 0; }
void j1850_capture(void) {}
void swcan_init(void) {}
int swcan_set_mode(uint8_t m) { (void)m; return 0; }
void feps_init(void) {}
int feps_set_voltage_mv(uint32_t mv) { (void)mv; return 0; }
int feps_read_prog_mv(uint32_t *mv) { *mv = 0; return 0; }
int feps_read_vbatt_mv(uint32_t *mv) { *mv = 12500; return 0; }
int feps_route_to_pin(uint8_t p) { (void)p; return 0; }
int feps_short_to_ground(uint8_t p) { (void)p; return 0; }
void feps_off(void) {}
int gpt_set(uint8_t i, int on) { (void)i;(void)on; return 0; }
void matrix_init(void) {}
int matrix_clear(void) { return 0; }
int matrix_set_bits(uint8_t p, uint16_t s, uint16_t c) { (void)p;(void)s;(void)c; return 0; }
int matrix_can_connect(uint8_t c) { (void)c; return 0; }
int matrix_can1_polarity_swap(int s) { (void)s; return 0; }
int matrix_can1_termination(int on) { (void)on; return 0; }
int matrix_kline_route(uint8_t k, uint8_t p) { (void)k;(void)p; return 0; }
int matrix_read_fault_flags(uint8_t *ff) { *ff = 0x07; return 0; }
int matrix_test_relay(uint8_t pca, uint8_t bit, uint8_t on) { (void)pca;(void)bit;(void)on; return 0; }
