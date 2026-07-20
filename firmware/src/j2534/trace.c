
#include "trace.h"

static trace_evt_t s_ring[TRACE_DEPTH];
static uint16_t s_tail, s_count;  
static int   s_on;

void trace_init(void) { s_tail = 0; s_count = 0; s_on = 0; }
void trace_enable(int on) { s_on = on ? 1 : 0; }
int trace_enabled(void) { return s_on; }
void trace_clear(void) { s_tail = 0; s_count = 0; }
uint16_t trace_count(void) { return s_count; }

void trace_record(uint8_t tag, uint8_t channel, uint8_t code, uint32_t ts,
         const uint8_t *data, uint16_t len)
{
  if (!s_on) return;
  uint16_t head = (uint16_t)((s_tail + s_count) % TRACE_DEPTH);
  trace_evt_t *e = &s_ring[head];
  e->ts = ts; e->tag = tag; e->channel = channel; e->code = code;
  e->len = (len > 255u) ? 255u : (uint8_t)len;
  uint16_t n = (len > TRACE_DATA_MAX) ? TRACE_DATA_MAX : len;
  for (uint16_t i = 0; i < n; i++) e->data[i] = data ? data[i] : 0u;
  for (uint16_t i = n; i < TRACE_DATA_MAX; i++) e->data[i] = 0u;

  if (s_count < TRACE_DEPTH) s_count++;
  else s_tail = (uint16_t)((s_tail + 1) % TRACE_DEPTH);  
}

int trace_pop(trace_evt_t *out)
{
  if (s_count == 0u) return 0;
  if (out) *out = s_ring[s_tail];
  s_tail = (uint16_t)((s_tail + 1) % TRACE_DEPTH);
  s_count--;
  return 1;
}
