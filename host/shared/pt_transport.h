
#ifndef PT_TRANSPORT_H
#define PT_TRANSPORT_H

#include <stdint.h>
#include <stddef.h>

typedef struct pt_transport {
  
  int (*send)(struct pt_transport *t, const uint8_t *buf, size_t n);
  
  int (*recv)(struct pt_transport *t, uint8_t *buf, size_t cap, size_t *got, int timeout_ms);
  void *ctx;
} pt_transport_t;

#endif 
