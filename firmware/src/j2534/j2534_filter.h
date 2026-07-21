
#ifndef J2534_FILTER_H
#define J2534_FILTER_H

#include <stdint.h>
#include "j2534_defs.h"  

#define J2534_FILTER_DATA_MAX 64u

typedef struct {
  uint8_t type;    
  uint32_t mask;    
  uint32_t pattern;   
  uint32_t fc_id;    
  int8_t  link;    
  uint16_t mask_len;
  uint16_t pattern_len;
  uint16_t flow_len;
  uint8_t mask_data[J2534_FILTER_DATA_MAX];
  uint8_t pattern_data[J2534_FILTER_DATA_MAX];
  uint8_t flow_data[J2534_FILTER_DATA_MAX];
} j2534_filter_t;


int j2534_id_match(uint32_t id, uint32_t mask, uint32_t pattern);


int j2534_filters_accept(const j2534_filter_t *f, uint32_t n, uint32_t id);
int j2534_filters_accept_msg(const j2534_filter_t *f, uint32_t n, uint32_t id,
                const uint8_t *data, uint16_t len);


int j2534_find_flow_by_rx(const j2534_filter_t *f, uint32_t n, uint32_t id);


int j2534_find_flow_by_tx(const j2534_filter_t *f, uint32_t n, uint32_t id);

#endif 
