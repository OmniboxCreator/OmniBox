
#include <string.h>
#include <stdio.h>
#include "pt_core.h"
#include "../shared/pt_frame.h"
#include "../shared/pt_proto.h"
#include "../shared/pt_wire.h"

void pt_init(pt_handle_t *h, pt_transport_t *t)
{
  memset(h, 0, sizeof(*h));
  h->t = t;
  snprintf(h->last_error, sizeof h->last_error, "no error");
}
const char *pt_last_error(pt_handle_t *h) { return h->last_error; }

static void set_err(pt_handle_t *h, const char *s) { snprintf(h->last_error, sizeof h->last_error, "%s", s); }


static int32_t pt_xfer(pt_handle_t *h, uint8_t cmd, const uint8_t *payload, uint16_t plen,
            const uint8_t **rdata, uint16_t *rdlen)
{
  if (!h->t) { set_err(h, "no transport"); return J2534_ERR_DEVICE_NOT_CONNECTED; }
  int flen = pt_frame_build(h->txbuf, sizeof h->txbuf, h->seq, cmd, payload, plen);
  if (flen < 0) { set_err(h, "request too large"); return J2534_ERR_FAILED; }
  if (h->t->send(h->t, h->txbuf, (size_t)flen) < 0) { set_err(h, "transport send failed"); return J2534_ERR_FAILED; }
  size_t got = 0;
  if (h->t->recv(h->t, h->rxbuf, sizeof h->rxbuf, &got, 2000) < 0) { set_err(h, "transport recv timeout"); return J2534_ERR_TIMEOUT; }
  uint8_t rseq, rcmd, status; const uint8_t *data; uint16_t dlen;
  if (pt_frame_parse(h->rxbuf, got, &rseq, &rcmd, &status, &data, &dlen) < 0) { set_err(h, "bad response frame"); return J2534_ERR_FAILED; }
  h->seq++;
  if (rdata) *rdata = data;
  if (rdlen) *rdlen = dlen;
  return status;
}


int32_t pt_open(pt_handle_t *h, const char *name, uint32_t *device_id)
{
  (void)name;
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_OPEN, 0, 0, &d, &dl);
  if (st == J2534_STATUS_NOERROR && device_id && dl >= 4) *device_id = pt_rd32(d);
  return st;
}

int32_t pt_close(pt_handle_t *h, uint32_t device_id)
{
  uint8_t p[4]; pt_wr32(p, device_id);
  return pt_xfer(h, CMD_CLOSE, p, 4, 0, 0);
}

int32_t pt_connect(pt_handle_t *h, uint32_t device_id, uint32_t protocol_id,
          uint32_t flags, uint32_t baud, uint32_t *channel_id)
{
  (void)device_id;
  uint8_t p[10];
  p[0] = (uint8_t)protocol_id; p[1] = (uint8_t)(protocol_id >> 8);  
  pt_wr32(p + 2, flags); pt_wr32(p + 6, baud);
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_CONNECT, p, sizeof p, &d, &dl);
  if (st == J2534_STATUS_NOERROR && channel_id && dl >= 4) *channel_id = pt_rd32(d);
  return st;
}

int32_t pt_disconnect(pt_handle_t *h, uint32_t channel_id)
{
  uint8_t p[4]; pt_wr32(p, channel_id);
  return pt_xfer(h, CMD_DISCONNECT, p, 4, 0, 0);
}

int32_t pt_write_msgs(pt_handle_t *h, uint32_t channel_id, const PASSTHRU_MSG *msgs,
           uint32_t *num, uint32_t timeout_ms)
{
  if (!msgs || !num) return J2534_ERR_NULL_PARAMETER;
  uint32_t want = *num, off = 12, sent = 0;
  pt_wr32(h->pbuf + 0, channel_id); pt_wr32(h->pbuf + 4, timeout_ms);
  for (uint32_t i = 0; i < want; i++) {
    int w = pt_wire_encode(h->pbuf + off, (uint32_t)sizeof(h->pbuf) - off, &msgs[i]);
    if (w < 0) break;            
    off += (uint32_t)w; sent++;
  }
  pt_wr32(h->pbuf + 8, sent);         
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_WRITE_MSGS, h->pbuf, (uint16_t)off, &d, &dl);
  if (st == J2534_STATUS_NOERROR && dl >= 4) *num = pt_rd32(d);
  return st;
}

int32_t pt_read_msgs(pt_handle_t *h, uint32_t channel_id, PASSTHRU_MSG *msgs,
           uint32_t *num, uint32_t timeout_ms)
{
  if (!msgs || !num) return J2534_ERR_NULL_PARAMETER;
  uint8_t p[12];
  pt_wr32(p + 0, channel_id); pt_wr32(p + 4, *num); pt_wr32(p + 8, timeout_ms);
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_READ_MSGS, p, sizeof p, &d, &dl);
  uint32_t got = 0;
  if (dl >= 4) {
    uint32_t count = pt_rd32(d);
    uint32_t off = 4;
    for (uint32_t i = 0; i < count && i < *num; i++) {
      int r = pt_wire_decode(d + off, dl - off, &msgs[i]);
      if (r < 0) break;
      off += (uint32_t)r; got++;
    }
  }
  *num = got;
  return st;
}

int32_t pt_start_filter(pt_handle_t *h, uint32_t channel_id, uint32_t type,
            const PASSTHRU_MSG *mask, const PASSTHRU_MSG *pattern,
            const PASSTHRU_MSG *flow, uint32_t *filter_id)
{
  if (!mask || !pattern || !flow) return J2534_ERR_NULL_PARAMETER;
  uint8_t p[16 + 3 * (24 + J2534_DATA_MAX)];
  pt_wr32(p + 0, channel_id); pt_wr32(p + 4, type);
  uint32_t off = 8;
  int w = pt_wire_encode(p + off, (uint32_t)sizeof(p) - off, mask);
  if (w < 0) return J2534_ERR_INVALID_MSG;
  off += (uint32_t)w;
  w = pt_wire_encode(p + off, (uint32_t)sizeof(p) - off, pattern);
  if (w < 0) return J2534_ERR_INVALID_MSG;
  off += (uint32_t)w;
  w = pt_wire_encode(p + off, (uint32_t)sizeof(p) - off, flow);
  if (w < 0) return J2534_ERR_INVALID_MSG;
  off += (uint32_t)w;
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_START_FILTER, p, (uint16_t)off, &d, &dl);
  if (st == J2534_STATUS_NOERROR && filter_id && dl >= 4) *filter_id = pt_rd32(d);
  return st;
}

int32_t pt_stop_filter(pt_handle_t *h, uint32_t channel_id, uint32_t filter_id)
{
  uint8_t p[8]; pt_wr32(p, channel_id); pt_wr32(p + 4, filter_id);
  return pt_xfer(h, CMD_STOP_FILTER, p, sizeof p, 0, 0);
}

int32_t pt_set_prog_voltage(pt_handle_t *h, uint32_t pin, uint32_t millivolts)
{
  uint8_t p[8]; pt_wr32(p, pin); pt_wr32(p + 4, millivolts);
  return pt_xfer(h, CMD_SET_PROG_VOLT, p, sizeof p, 0, 0);
}

int32_t pt_read_vbatt(pt_handle_t *h, uint32_t *millivolts)
{
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_READ_VBATT, 0, 0, &d, &dl);
  if (st == J2534_STATUS_NOERROR && millivolts && dl >= 4) *millivolts = pt_rd32(d);
  return st;
}


int32_t pt_set_config(pt_handle_t *h, uint32_t channel_id, const SCONFIG *cfg, uint32_t n)
{
  if (n && !cfg) return J2534_ERR_NULL_PARAMETER;
  pt_wr32(h->pbuf + 0, channel_id); pt_wr32(h->pbuf + 4, J2534_SET_CONFIG);
  pt_wr32(h->pbuf + 8, n);             
  uint32_t off = 12;
  for (uint32_t i = 0; i < n; i++) { pt_wr32(h->pbuf + off, cfg[i].Parameter); pt_wr32(h->pbuf + off + 4, cfg[i].Value); off += 8; }
  return pt_xfer(h, CMD_IOCTL, h->pbuf, (uint16_t)off, 0, 0);
}

int32_t pt_get_config(pt_handle_t *h, uint32_t channel_id, SCONFIG *cfg, uint32_t n)
{
  if (n && !cfg) return J2534_ERR_NULL_PARAMETER;
  pt_wr32(h->pbuf + 0, channel_id); pt_wr32(h->pbuf + 4, J2534_GET_CONFIG);
  pt_wr32(h->pbuf + 8, n);
  uint32_t off = 12;
  for (uint32_t i = 0; i < n; i++) { pt_wr32(h->pbuf + off, cfg[i].Parameter); pt_wr32(h->pbuf + off + 4, 0); off += 8; }
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_IOCTL, h->pbuf, (uint16_t)off, &d, &dl);
  if (st == J2534_STATUS_NOERROR && dl >= 4) {    
    uint32_t cnt = pt_rd32(d);
    for (uint32_t i = 0; i < cnt && i < n; i++) cfg[i].Value = pt_rd32(d + 4 + i * 8 + 4);
  }
  return st;
}

int32_t pt_ioctl_clear(pt_handle_t *h, uint32_t channel_id, uint32_t ioctl_id)
{
  uint8_t p[8]; pt_wr32(p, channel_id); pt_wr32(p + 4, ioctl_id);
  return pt_xfer(h, CMD_IOCTL, p, sizeof p, 0, 0);
}

int32_t pt_ioctl_bytes(pt_handle_t *h, uint32_t channel_id, uint32_t ioctl_id,
             const uint8_t *in, uint32_t in_len, uint8_t *out, uint32_t *out_len)
{
  if (in_len > sizeof(h->pbuf) - 8u) return J2534_ERR_INVALID_MSG;
  if (in_len && !in) return J2534_ERR_NULL_PARAMETER;
  pt_wr32(h->pbuf + 0, channel_id);
  pt_wr32(h->pbuf + 4, ioctl_id);
  if (in_len) memcpy(h->pbuf + 8, in, in_len);
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_IOCTL, h->pbuf, (uint16_t)(8u + in_len), &d, &dl);
  if (st == J2534_STATUS_NOERROR && out_len) {
    uint32_t copy = dl;
    if (out && copy > *out_len) copy = *out_len;
    if (out && copy) memcpy(out, d, copy);
    *out_len = copy;
  }
  return st;
}

int32_t pt_start_periodic(pt_handle_t *h, uint32_t channel_id, const PASSTHRU_MSG *msg,
             uint32_t interval_ms, uint32_t *msg_id)
{
  if (!msg) return J2534_ERR_NULL_PARAMETER;
  pt_wr32(h->pbuf + 0, channel_id); pt_wr32(h->pbuf + 4, interval_ms);
  pt_wr32(h->pbuf + 8, msg->TxFlags);
  uint32_t off = 12, ds = msg->DataSize;
  if (ds > sizeof(h->pbuf) - off) ds = (uint32_t)(sizeof(h->pbuf) - off);
  for (uint32_t i = 0; i < ds; i++) h->pbuf[off + i] = msg->Data[i];
  off += ds;
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_START_PERIODIC, h->pbuf, (uint16_t)off, &d, &dl);
  if (st == J2534_STATUS_NOERROR && msg_id && dl >= 4) *msg_id = pt_rd32(d);
  return st;
}

int32_t pt_stop_periodic(pt_handle_t *h, uint32_t channel_id, uint32_t msg_id)
{
  uint8_t p[8]; pt_wr32(p, channel_id); pt_wr32(p + 4, msg_id);
  return pt_xfer(h, CMD_STOP_PERIODIC, p, sizeof p, 0, 0);
}


static size_t pull_str(char *dst, size_t cap, const uint8_t *src, size_t avail)
{
  size_t n = 0;
  while (n < avail && src[n]) n++;
  size_t copy = (cap && n < cap - 1) ? n : (cap ? cap - 1 : 0);
  for (size_t i = 0; i < copy; i++) dst[i] = (char)src[i];
  if (cap) dst[copy] = 0;
  return (n < avail) ? n + 1 : n;
}

int32_t pt_read_version(pt_handle_t *h, char *fw, char *dll, char *api)
{
  char tmp[80];
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_READ_VERSION, 0, 0, &d, &dl);
  if (st == J2534_STATUS_NOERROR) {        
    size_t off = 0;
    off += pull_str(fw ? fw : tmp, 80, d + off, (size_t)dl - off);
    pull_str(api ? api : tmp, 80, d + off, (size_t)dl - off);
  }
  if (dll) snprintf(dll, 80, "OmniBox DLL 0.1 dev");
  return st;
}

int32_t pt_get_caps(pt_handle_t *h, pt_caps_t *caps)
{
  if (!caps) return J2534_ERR_NULL_PARAMETER;
  memset(caps, 0, sizeof(*caps));
  const uint8_t *d; uint16_t dl;
  int32_t st = pt_xfer(h, CMD_GET_CAPS, 0, 0, &d, &dl);
  if (st == J2534_STATUS_NOERROR && dl >= 16) {
    caps->proto_version = pt_rd32(d + 0);
    caps->caps = pt_rd32(d + 4);
    caps->can_channels = pt_rd32(d + 8);
    caps->kline_channels = pt_rd32(d + 12);
  }
  return st;
}
