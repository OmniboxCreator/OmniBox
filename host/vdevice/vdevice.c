
#include <string.h>
#include "vdevice.h"
#include "../shared/pt_frame.h"
#include "../../firmware/src/transport/protocol.h"  
#include "../../firmware/src/j2534/j2534_core.h"

void mock_bus_reset(void);

static uint8_t g_reply[8192];
static size_t g_reply_len;


void proto_reply(uint8_t seq, uint8_t cmd, uint8_t status, const uint8_t *data, uint16_t n)
{
  static uint8_t pl[8192];
  if ((size_t)1 + n > sizeof pl) n = (uint16_t)(sizeof pl - 1);
  pl[0] = status;
  if (n) memcpy(pl + 1, data, n);
  int len = pt_frame_build(g_reply, sizeof g_reply, seq,
               (uint8_t)(cmd | CMD_RESP_FLAG), pl, (uint16_t)(1 + n));
  g_reply_len = (len > 0) ? (size_t)len : 0;
}


static void vdevice_process(const uint8_t *req, size_t n)
{
  g_reply_len = 0;
  for (size_t i = 0; i < n; i++) transport_rx_byte(req[i]);  
  for (int k = 0; k < 64; k++) j2534_service();        
}

int vdevice_handle_frame(const uint8_t *req, size_t n, uint8_t *resp, size_t cap, size_t *resp_len)
{
  vdevice_process(req, n);
  if (g_reply_len == 0 || g_reply_len > cap) return -1;
  memcpy(resp, g_reply, g_reply_len);
  *resp_len = g_reply_len;
  return 0;
}

static int vd_send(pt_transport_t *t, const uint8_t *buf, size_t n)
{
  (void)t; vdevice_process(buf, n); return 0;
}
static int vd_recv(pt_transport_t *t, uint8_t *buf, size_t cap, size_t *got, int timeout_ms)
{
  (void)t; (void)timeout_ms;
  if (g_reply_len == 0 || g_reply_len > cap) return -1;
  memcpy(buf, g_reply, g_reply_len);
  *got = g_reply_len;
  return 0;
}

static pt_transport_t g_vt = { vd_send, vd_recv, 0 };

void vdevice_reset(void)
{
  mock_bus_reset();
  j2534_init();
  g_reply_len = 0;
}

pt_transport_t *vdevice_init(void)
{
  vdevice_reset();
  return &g_vt;
}
