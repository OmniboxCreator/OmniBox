
#include "j2534_filter.h"

int j2534_id_match(uint32_t id, uint32_t mask, uint32_t pattern)
{
  return ((id ^ pattern) & mask) == 0u;
}

int j2534_filters_accept(const j2534_filter_t *f, uint32_t n, uint32_t id)
{
  int has_pass = 0, passed = 0;
  for (uint32_t i = 0; i < n; i++) {
    if (f[i].type == J2534_BLOCK_FILTER &&
      j2534_id_match(id, f[i].mask, f[i].pattern))
      return 0;            
    if (f[i].type == J2534_PASS_FILTER) {
      has_pass = 1;
      if (j2534_id_match(id, f[i].mask, f[i].pattern)) passed = 1;
    }
  }
  return has_pass ? passed : 1;      
}

int j2534_find_flow_by_rx(const j2534_filter_t *f, uint32_t n, uint32_t id)
{
  for (uint32_t i = 0; i < n; i++)
    if (f[i].type == J2534_FLOW_CONTROL_FILTER &&
      j2534_id_match(id, f[i].mask, f[i].pattern))
      return (int)i;
  return -1;
}

int j2534_find_flow_by_tx(const j2534_filter_t *f, uint32_t n, uint32_t id)
{
  for (uint32_t i = 0; i < n; i++)
    if (f[i].type == J2534_FLOW_CONTROL_FILTER && f[i].fc_id == id)
      return (int)i;
  return -1;
}
