
#include "pt_frame.h"
#include "pt_proto.h"

uint16_t pt_crc16(const uint8_t *p, size_t n)
{
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < n; i++) {
    crc ^= (uint16_t)p[i] << 8;
    for (int b = 0; b < 8; b++)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}

int pt_frame_build(uint8_t *out, size_t cap, uint8_t seq, uint8_t cmd,
          const uint8_t *payload, uint16_t n)
{
  size_t total = (size_t)5 + n + 2;     
  if (cap < total) return -1;
  out[0] = PT_SOF;
  out[1] = (uint8_t)(n & 0xFF);
  out[2] = (uint8_t)(n >> 8);
  out[3] = seq;
  out[4] = cmd;
  for (uint16_t i = 0; i < n; i++) out[5 + i] = payload ? payload[i] : 0;
  
  uint16_t crc = pt_crc16(out + 3, (size_t)(2 + n));
  out[5 + n]   = (uint8_t)(crc & 0xFF);
  out[5 + n + 1] = (uint8_t)(crc >> 8);
  return (int)total;
}

int pt_frame_parse(const uint8_t *buf, size_t n, uint8_t *seq, uint8_t *cmd,
          uint8_t *status, const uint8_t **data, uint16_t *dlen)
{
  if (n < 8) return -1;            
  if (buf[0] != PT_SOF) return -1;
  uint16_t len = (uint16_t)(buf[1] | (buf[2] << 8));  
  if ((size_t)5 + len + 2 != n) return -1;       
  uint16_t crc = (uint16_t)(buf[5 + len] | (buf[5 + len + 1] << 8));
  if (crc != pt_crc16(buf + 3, (size_t)(2 + len))) return -1;
  if (seq)  *seq = buf[3];
  if (cmd)  *cmd = (uint8_t)(buf[4] & ~PT_RESP_FLAG);
  if (status) *status = buf[5];
  if (data)  *data = buf + 6;
  if (dlen)  *dlen = (uint16_t)(len - 1);       
  return 0;
}
