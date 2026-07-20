
#ifndef ELM327_H
#define ELM327_H

#include <stdint.h>


typedef struct {
  void *user;
  
  void (*write)(void *user, const char *s, uint16_t n);
  
  int (*can_open)(void *user, uint32_t baud);
  void (*can_close)(void *user);
  int (*can_tx)(void *user, uint32_t id, int ext, const uint8_t *d, uint8_t n);
  int (*can_rx)(void *user, uint32_t *id, int *ext, uint8_t *d, uint8_t *n);
  
  int (*read_vbatt_mv)(void *user, uint32_t *mv);  
  uint32_t (*millis)(void *user);
} elm327_ops_t;


#define ELM327_ID_STRING  "ELM327 v1.5"
#define ELM327_DESC_STRING "OBDII to RS232 Interpreter"
#define ELM327_LINE_MAX  64   
#define ELM327_ST_DEFAULT 0x32  


enum {
  ELM_PROTO_AUTO   = 0x0,
  ELM_PROTO_ISO9141  = 0x3,  
  ELM_PROTO_KWP_SLOW = 0x4,  
  ELM_PROTO_KWP_FAST = 0x5,  
  ELM_PROTO_CAN11_500 = 0x6,
  ELM_PROTO_CAN29_500 = 0x7,
  ELM_PROTO_CAN11_250 = 0x8,
  ELM_PROTO_CAN29_250 = 0x9,
};


enum { ELM_REQ_IDLE = 0, ELM_REQ_WAIT_RESP, ELM_REQ_MONITOR };

typedef struct {
  const elm327_ops_t *ops;

  
  uint8_t echo;     
  uint8_t linefeeds;  
  uint8_t spaces;    
  uint8_t headers;   
  uint8_t caf;     
  uint8_t cfc;     
  uint8_t responses;  
  uint8_t adaptive;   
  uint8_t st;      
  uint8_t dlc_display; 
  uint8_t allow_long;  
  uint8_t kw_check;   
  uint8_t proto;    
  uint8_t proto_open;  
  
  uint32_t tx_id;    
  uint8_t tx_ext;    
  uint8_t can_prio;   
  uint32_t rx_id;    
  uint8_t rx_ext;
  
  uint8_t fc_mode;   
  uint32_t fc_id;    
  uint8_t fc_ext;
  uint8_t fc_data[5];  
  uint8_t fc_len;
  
  uint8_t iso_baud;   
  uint8_t iso_init_addr;
  uint8_t wakeup_period;
  uint8_t wm[6], wm_len;

  
  char   line[ELM327_LINE_MAX];
  uint16_t line_len;
  char   last_line[ELM327_LINE_MAX];  
  uint16_t last_len;

  
  uint8_t req_state;
  uint8_t req_expect;  
  uint8_t req_got;
  uint32_t req_start_ms;
  uint32_t req_last_rx_ms;
} elm327_t;

#define ELM_CRA_NONE 0xFFFFFFFFu

void elm327_init(elm327_t *e, const elm327_ops_t *ops);

void elm327_input(elm327_t *e, const uint8_t *data, uint32_t n);

void elm327_service(elm327_t *e);

#endif 
