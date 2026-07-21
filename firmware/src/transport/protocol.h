
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define PROTO_SOF 0xA5u


enum {
  CMD_PING      = 0x00,
  CMD_OPEN      = 0x01, 
  CMD_CLOSE      = 0x02,
  CMD_CONNECT     = 0x03, 
  CMD_DISCONNECT   = 0x04, 
  CMD_READ_MSGS    = 0x05, 
  CMD_WRITE_MSGS   = 0x06, 
  CMD_START_PERIODIC = 0x07,
  CMD_STOP_PERIODIC  = 0x08,
  CMD_START_FILTER  = 0x09,
  CMD_STOP_FILTER   = 0x0A,
  CMD_IOCTL      = 0x0B, 
  CMD_SET_PROG_VOLT  = 0x0C, 
  CMD_READ_VERSION  = 0x0D, 
  CMD_READ_VBATT   = 0x0E, 
  CMD_TRACE      = 0x0F, 
  CMD_TEST_RELAY   = 0x10, 
  CMD_USB_MODE_GET  = 0x11, 
  CMD_USB_MODE_SET  = 0x12, 
  CMD_GET_CAPS    = 0x13,
};

#define OMNIBOX_PROTO_VERSION 2u

enum {
  OMNI_CAP_J2534_CAN        = 1u << 0,
  OMNI_CAP_J2534_ISO15765   = 1u << 1,
  OMNI_CAP_MULTI_CAN        = 1u << 2,
  OMNI_CAP_ROUTING_MATRIX   = 1u << 3,
  OMNI_CAP_ELM327_CAN       = 1u << 4,
  OMNI_CAP_KLINE_EXPERIMENTAL = 1u << 5,
  OMNI_CAP_J1850_EXPERIMENTAL = 1u << 6,
  OMNI_CAP_CANFD_EXPERIMENTAL = 1u << 7,
};


enum {
  TRACE_CTL_OFF = 0, TRACE_CTL_ON = 1, TRACE_CTL_CLEAR = 2, TRACE_CTL_READ = 3,
};


#define CMD_RESP_FLAG 0x80u

typedef struct {
  uint8_t seq;
  uint8_t cmd;
  uint16_t len;
  const uint8_t *payload;
} proto_frame_t;


uint16_t proto_crc16(const uint8_t *p, size_t n);

uint16_t proto_crc16_upd(uint16_t crc, const uint8_t *p, size_t n);


void transport_rx_byte(uint8_t b);
void transport_poll(void);


void proto_reply(uint8_t seq, uint8_t cmd, uint8_t status,
         const uint8_t *data, uint16_t n);


void proto_dispatch(const proto_frame_t *f);

#endif 
