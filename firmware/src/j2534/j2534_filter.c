
#include "j2534_filter.h"

int j2534_id_match(uint32_t id, uint32_t mask, uint32_t pattern)
{
  return ((id ^ pattern) & mask) == 0u;
}

static int data_match(const j2534_filter_t *f, const uint8_t *data, uint16_t len)
{
  if (f->mask_len <= 4u && f->pattern_len <= 4u) return 1;
  uint16_t mlen = f->mask_len > 4u ? (uint16_t)(f->mask_len - 4u) : 0u;
  uint16_t plen = f->pattern_len > 4u ? (uint16_t)(f->pattern_len - 4u) : 0u;
  uint16_t n = mlen > plen ? mlen : plen;
  if (n > len) return 0;
  if (n > J2534_FILTER_DATA_MAX - 4u) n = J2534_FILTER_DATA_MAX - 4u;
  for (uint16_t i = 0; i < n; i++) {
    uint8_t mask = (i + 4u < f->mask_len) ? f->mask_data[i + 4u] : 0u;
    uint8_t pat = (i + 4u < f->pattern_len) ? f->pattern_data[i + 4u] : 0u;
    if (((data[i] ^ pat) & mask) != 0u) return 0;
  }
  return 1;
}

int j2534_filters_accept(const j2534_filter_t *f, uint32_t n, uint32_t id)
{
  return j2534_filters_accept_msg(f, n, id, 0, 0);
}

int j2534_filters_accept_msg(const j2534_filter_t *f, uint32_t n, uint32_t id,
                const uint8_t *data, uint16_t len)
{
  int has_pass = 0, passed = 0;
  for (uint32_t i = 0; i < n; i++) {
    if (f[i].type == J2534_BLOCK_FILTER &&
      j2534_id_match(id, f[i].mask, f[i].pattern) &&
      data_match(&f[i], data, len))
      return 0;            
    if (f[i].type == J2534_PASS_FILTER) {
      has_pass = 1;
      if (j2534_id_match(id, f[i].mask, f[i].pattern) &&
        data_match(&f[i], data, len))
        passed = 1;
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
