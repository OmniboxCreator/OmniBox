
#ifndef PT_FRAME_H
#define PT_FRAME_H

#include <stdint.h>
#include <stddef.h>


uint16_t pt_crc16(const uint8_t *p, size_t n);


int pt_frame_build(uint8_t *out, size_t cap, uint8_t seq, uint8_t cmd,
          const uint8_t *payload, uint16_t n);


int pt_frame_parse(const uint8_t *buf, size_t n, uint8_t *seq, uint8_t *cmd,
          uint8_t *status, const uint8_t **data, uint16_t *dlen);

#endif 
