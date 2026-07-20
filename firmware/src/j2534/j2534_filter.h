
#ifndef J2534_FILTER_H
#define J2534_FILTER_H

#include <stdint.h>
#include "j2534_defs.h"  

typedef struct {
  uint8_t type;    
  uint32_t mask;    
  uint32_t pattern;   
  uint32_t fc_id;    
  int8_t  link;    
} j2534_filter_t;


int j2534_id_match(uint32_t id, uint32_t mask, uint32_t pattern);


int j2534_filters_accept(const j2534_filter_t *f, uint32_t n, uint32_t id);


int j2534_find_flow_by_rx(const j2534_filter_t *f, uint32_t n, uint32_t id);


int j2534_find_flow_by_tx(const j2534_filter_t *f, uint32_t n, uint32_t id);

#endif 
