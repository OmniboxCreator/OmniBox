
#include <stdio.h>
#include <string.h>
#include "vlog.h"
#include "../shared/pt_proto.h"
#include "../shared/pt_frame.h"
#include "../shared/pt_wire.h"
#include "../shared/j2534_abi.h"

static int g_on = 0;

void vlog_set(int on) { g_on = on ? 1 : 0; }
int vlog_enabled(void) { return g_on; }

void vlog_event(const char *msg)
{
  if (!g_on) return;
  printf("[log] %s\n", msg);
  fflush(stdout);
}

static const char *cmd_name(uint8_t c)
{
  switch (c) {
  case CMD_PING:      return "PING";
  case CMD_OPEN:      return "Open";
  case CMD_CLOSE:     return "Close";
  case CMD_CONNECT:    return "Connect";
  case CMD_DISCONNECT:   return "Disconnect";
  case CMD_READ_MSGS:   return "ReadMsgs";
  case CMD_WRITE_MSGS:   return "WriteMsgs";
  case CMD_START_PERIODIC: return "StartPeriodicMsg";
  case CMD_STOP_PERIODIC: return "StopPeriodicMsg";
  case CMD_START_FILTER:  return "StartMsgFilter";
  case CMD_STOP_FILTER:  return "StopMsgFilter";
  case CMD_IOCTL:     return "Ioctl";
  case CMD_SET_PROG_VOLT: return "SetProgrammingVoltage";
  case CMD_READ_VERSION:  return "ReadVersion";
  case CMD_READ_VBATT:   return "ReadVbatt";
  case CMD_TRACE:     return "Trace";
  default:         return "?";
  }
}

static const char *proto_name(uint32_t p)
{
  switch (p) {
  case J2534_J1850VPW:  return "J1850VPW";
  case J2534_J1850PWM:  return "J1850PWM";
  case J2534_ISO9141:   return "ISO9141";
  case J2534_ISO14230:  return "ISO14230";
  case J2534_CAN:     return "CAN";
  case J2534_ISO15765:  return "ISO15765";
  case J2534_CAN_PS:   return "CAN_PS";
  case J2534_ISO15765_PS: return "ISO15765_PS";
  default:        return "?";
  }
}

static const char *ioctl_name(uint32_t id)
{
  switch (id) {
  case J2534_GET_CONFIG:     return "GET_CONFIG";
  case J2534_SET_CONFIG:     return "SET_CONFIG";
  case J2534_READ_VBATT:     return "READ_VBATT";
  case J2534_FIVE_BAUD_INIT:   return "FIVE_BAUD_INIT";
  case J2534_FAST_INIT:      return "FAST_INIT";
  case J2534_CLEAR_TX_BUFFER:   return "CLEAR_TX_BUFFER";
  case J2534_CLEAR_RX_BUFFER:   return "CLEAR_RX_BUFFER";
  case J2534_CLEAR_PERIODIC_MSGS: return "CLEAR_PERIODIC_MSGS";
  case J2534_CLEAR_MSG_FILTERS:  return "CLEAR_MSG_FILTERS";
  default:            return "?";
  }
}

static const char *status_name(uint8_t s)
{
  switch (s) {
  case J2534_STATUS_NOERROR:      return "NOERROR";
  case J2534_ERR_NOT_SUPPORTED:     return "ERR_NOT_SUPPORTED";
  case J2534_ERR_INVALID_CHANNEL_ID:  return "ERR_INVALID_CHANNEL_ID";
  case J2534_ERR_INVALID_PROTOCOL_ID:  return "ERR_INVALID_PROTOCOL_ID";
  case J2534_ERR_NULL_PARAMETER:    return "ERR_NULL_PARAMETER";
  case J2534_ERR_TIMEOUT:        return "ERR_TIMEOUT";
  case J2534_ERR_INVALID_MSG:      return "ERR_INVALID_MSG";
  case J2534_ERR_BUFFER_EMPTY:     return "ERR_BUFFER_EMPTY";
  case J2534_ERR_BUFFER_FULL:      return "ERR_BUFFER_FULL";
  case J2534_ERR_BUFFER_OVERFLOW:    return "ERR_BUFFER_OVERFLOW";
  case J2534_ERR_DEVICE_NOT_CONNECTED: return "ERR_DEVICE_NOT_CONNECTED";
  case J2534_ERR_FAILED:        return "ERR_FAILED";
  case J2534_ERR_DEVICE_NOT_OPEN:    return "ERR_DEVICE_NOT_OPEN";
  default:               return "ERR_?";
  }
}


static void hex_dump(const uint8_t *p, uint32_t n, uint32_t max)
{
  uint32_t lim = (n < max) ? n : max;
  for (uint32_t i = 0; i < lim; i++) printf("%02X%s", p[i], (i + 1 < lim) ? " " : "");
  if (n > max) printf(" …(%u)", n);
}


static int frame_fields(const uint8_t *f, size_t n, uint8_t *cmd, const uint8_t **pl, uint16_t *plen)
{
  if (n < 7 || f[0] != PT_SOF) return -1;
  uint16_t len = (uint16_t)(f[1] | (f[2] << 8));
  if ((size_t)5 + len + 2 != n) return -1;
  *cmd = (uint8_t)(f[4] & ~PT_RESP_FLAG);
  *pl  = f + 5;
  *plen = len;
  return 0;
}

static void log_one_msg(const uint8_t *wire, uint32_t n, const char *tag)
{
  PASSTHRU_MSG m;
  int r = pt_wire_decode(wire, n, &m);
  if (r < 0) { printf("  %s <wire?>\n", tag); return; }
  printf("  %s proto=%s len=%u data=[", tag, proto_name(m.ProtocolID), m.DataSize);
  hex_dump(m.Data, m.DataSize, 16);
  printf("]\n");
}

void vlog_request(const uint8_t *frame, size_t n)
{
  if (!g_on) return;
  uint8_t cmd; const uint8_t *p; uint16_t plen;
  if (frame_fields(frame, n, &cmd, &p, &plen) < 0) { printf("-> <invalid request frame>\n"); fflush(stdout); return; }

  printf("-> %s", cmd_name(cmd));
  switch (cmd) {
  case CMD_CONNECT:
    if (plen >= 10) {
      uint32_t proto = (uint32_t)(p[0] | (p[1] << 8));
      printf(" proto=%s flags=0x%X baud=%u", proto_name(proto), pt_rd32(p + 2), pt_rd32(p + 6));
    }
    break;
  case CMD_DISCONNECT:
    if (plen >= 4) printf(" cid=%u", pt_rd32(p));
    break;
  case CMD_CLOSE:
    if (plen >= 4) printf(" devid=%u", pt_rd32(p));
    break;
  case CMD_READ_MSGS:
    if (plen >= 12) printf(" cid=%u num=%u timeout=%u", pt_rd32(p), pt_rd32(p + 4), pt_rd32(p + 8));
    break;
  case CMD_WRITE_MSGS:
    if (plen >= 12) {
      uint32_t cnt = pt_rd32(p + 8);
      printf(" cid=%u timeout=%u count=%u\n", pt_rd32(p), pt_rd32(p + 4), cnt);
      uint32_t off = 12;
      for (uint32_t i = 0; i < cnt && off + PT_WIRE_HDR <= plen; i++) {
        uint32_t ds = pt_rd32(p + off + 16);
        log_one_msg(p + off, plen - off, "TX");
        off += PT_WIRE_HDR + ds;
      }
      fflush(stdout);
      return;
    }
    break;
  case CMD_START_FILTER:
    if (plen >= 20)
      printf(" cid=%u type=%u mask=0x%X pattern=0x%X flow=0x%X",
          pt_rd32(p), pt_rd32(p + 4), pt_rd32(p + 8), pt_rd32(p + 12), pt_rd32(p + 16));
    break;
  case CMD_STOP_FILTER:
    if (plen >= 8) printf(" cid=%u filter=%u", pt_rd32(p), pt_rd32(p + 4));
    break;
  case CMD_IOCTL:
    if (plen >= 8) {
      uint32_t id = pt_rd32(p + 4);
      printf(" cid=%u id=%s", pt_rd32(p), ioctl_name(id));
      if ((id == J2534_SET_CONFIG || id == J2534_GET_CONFIG) && plen >= 12) {
        uint32_t cnt = pt_rd32(p + 8);
        printf(" count=%u {", cnt);
        for (uint32_t i = 0; i < cnt && 12 + i * 8 + 8 <= plen; i++)
          printf("%s0x%X=0x%X", i ? ", " : "", pt_rd32(p + 12 + i * 8), pt_rd32(p + 12 + i * 8 + 4));
        printf("}");
      }
    }
    break;
  case CMD_SET_PROG_VOLT:
    if (plen >= 8) printf(" pin=%u mV=%u", pt_rd32(p), pt_rd32(p + 4));
    break;
  case CMD_START_PERIODIC:
    if (plen >= 12) printf(" cid=%u interval=%u txflags=0x%X", pt_rd32(p), pt_rd32(p + 4), pt_rd32(p + 8));
    break;
  case CMD_STOP_PERIODIC:
    if (plen >= 8) printf(" cid=%u msgid=%u", pt_rd32(p), pt_rd32(p + 4));
    break;
  default:
    break;
  }
  printf("\n");
  fflush(stdout);
}

void vlog_response(const uint8_t *frame, size_t n)
{
  if (!g_on) return;
  uint8_t seq, cmd, status; const uint8_t *data; uint16_t dlen;
  if (pt_frame_parse(frame, n, &seq, &cmd, &status, &data, &dlen) < 0) {
    printf("<- <invalid response frame>\n"); fflush(stdout); return;
  }
  printf("<- %s %s", cmd_name(cmd), status_name(status));

  if (status == J2534_STATUS_NOERROR) {
    switch (cmd) {
    case CMD_OPEN:  if (dlen >= 4) printf(" devid=%u", pt_rd32(data)); break;
    case CMD_CONNECT: if (dlen >= 4) printf(" cid=%u", pt_rd32(data)); break;
    case CMD_START_FILTER:  if (dlen >= 4) printf(" filter=%u", pt_rd32(data)); break;
    case CMD_START_PERIODIC: if (dlen >= 4) printf(" msgid=%u", pt_rd32(data)); break;
    case CMD_READ_VBATT:   if (dlen >= 4) printf(" mV=%u", pt_rd32(data)); break;
    case CMD_WRITE_MSGS:   if (dlen >= 4) printf(" written=%u", pt_rd32(data)); break;
    case CMD_READ_MSGS:
      if (dlen >= 4) {
        uint32_t cnt = pt_rd32(data);
        printf(" count=%u\n", cnt);
        uint32_t off = 4;
        for (uint32_t i = 0; i < cnt && off + PT_WIRE_HDR <= dlen; i++) {
          uint32_t ds = pt_rd32(data + off + 16);
          log_one_msg(data + off, dlen - off, "RX");
          off += PT_WIRE_HDR + ds;
        }
        fflush(stdout);
        return;
      }
      break;
    case CMD_READ_VERSION:
      if (dlen) { printf(" fw=\"%s\"", (const char *)data); }
      break;
    default: break;
    }
  }
  printf("\n");
  fflush(stdout);
}
