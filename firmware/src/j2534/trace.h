
#ifndef TRACE_H
#define TRACE_H

#include <stdint.h>

#define TRACE_DATA_MAX 16u  
#define TRACE_DEPTH  128u  


enum {
  TRACE_USB_RX = 1,  
  TRACE_BUS_TX = 2,  
  TRACE_BUS_RX = 3,  
  TRACE_EVT  = 4,  
};


enum {
  TRACE_EVT_NBS_TIMEOUT = 1,  
  TRACE_EVT_NCR_TIMEOUT = 2,  
  TRACE_EVT_RX_OVERFLOW = 3,  
};

typedef struct {
  uint32_t ts;          
  uint8_t tag;          
  uint8_t channel;        
  uint8_t code;         
  uint8_t len;          
  uint8_t data[TRACE_DATA_MAX]; 
} trace_evt_t;

void   trace_init(void);
void   trace_enable(int on);
int   trace_enabled(void);
void   trace_clear(void);


void   trace_record(uint8_t tag, uint8_t channel, uint8_t code, uint32_t ts,
           const uint8_t *data, uint16_t len);

uint16_t trace_count(void);
int   trace_pop(trace_evt_t *out);  

#endif 
