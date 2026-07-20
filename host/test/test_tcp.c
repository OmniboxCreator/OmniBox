
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../ptcore/pt_core.h"
#include "../shared/transport_tcp.h"
#include "../vdevice/vdevice.h"

static int g_fail = 0, g_tests = 0;
#define CHECK(cond, msg) do { g_tests++; if (!(cond)) { printf(" FAIL: %s\n", msg); g_fail++; } } while (0)

static tcp_sock_t g_listen;

static void *server_thread(void *arg)
{
  (void)arg;
  tcp_sock_t cli = tcp_accept(g_listen);
  if (cli == TCP_SOCK_INVALID) return 0;
  vdevice_reset();
  static uint8_t req[8192], resp[8192];
  size_t rn, sn;
  while (tcp_read_frame(cli, req, sizeof req, &rn) == 0) {
    if (vdevice_handle_frame(req, rn, resp, sizeof resp, &sn) == 0)
      if (tcp_write_all(cli, resp, sn) != 0) break;
  }
  tcp_close(cli);
  return 0;
}

static void mk_req(PASSTHRU_MSG *m, uint32_t id, const uint8_t *uds, uint16_t n)
{
  memset(m, 0, sizeof(*m));
  m->ProtocolID = J2534_ISO15765;
  m->Data[0] = (uint8_t)(id >> 24); m->Data[1] = (uint8_t)(id >> 16);
  m->Data[2] = (uint8_t)(id >> 8); m->Data[3] = (uint8_t)id;
  for (uint16_t i = 0; i < n; i++) m->Data[4 + i] = uds[i];
  m->DataSize = (uint32_t)(4 + n);
}

int main(void)
{
  printf("test_tcp\n");
  if (tcp_startup() != 0) { printf(" FAIL: tcp_startup\n"); return 1; }
  g_listen = tcp_listen(0);            
  if (g_listen == TCP_SOCK_INVALID) { printf(" FAIL: tcp_listen\n"); return 1; }
  int port = tcp_listen_port(g_listen);

  pthread_t th;
  pthread_create(&th, 0, server_thread, 0);

  pt_transport_t *t = transport_tcp_connect("127.0.0.1", port);
  CHECK(t != NULL, "client connected au device virtuel via TCP");
  if (t) {
    pt_handle_t h; pt_init(&h, t);
    uint32_t dev = 0, ch = 0, fid = 0;
    CHECK(pt_open(&h, "tcp", &dev) == J2534_STATUS_NOERROR, "PassThruOpen (TCP)");
    CHECK(pt_connect(&h, dev, J2534_ISO15765, 0, 500000, &ch) == J2534_STATUS_NOERROR, "Connect (TCP)");
    PASSTHRU_MSG mask, pat, flow;
    memset(&mask, 0, sizeof mask); memset(&pat, 0, sizeof pat); memset(&flow, 0, sizeof flow);
    mask.DataSize = 4; mask.Data[2] = 0x07; mask.Data[3] = 0xFF;
    pat.DataSize = 4; pat.Data[2] = 0x07; pat.Data[3] = 0xE8;
    flow.DataSize = 4; flow.Data[2] = 0x07; flow.Data[3] = 0xE0;
    CHECK(pt_start_filter(&h, ch, J2534_FLOW_CONTROL_FILTER, &mask, &pat, &flow, &fid) == J2534_STATUS_NOERROR, "StartFilter (TCP)");

    uint8_t uds[2] = { 0x3E, 0x00 };
    PASSTHRU_MSG req; mk_req(&req, 0x7E0, uds, 2);
    uint32_t num = 1;
    CHECK(pt_write_msgs(&h, ch, &req, &num, 100) == J2534_STATUS_NOERROR && num == 1, "WriteMsgs (TCP)");
    PASSTHRU_MSG resp; num = 1;
    int32_t st = pt_read_msgs(&h, ch, &resp, &num, 100);
    CHECK(st == J2534_STATUS_NOERROR && num == 1, "ReadMsgs (TCP)");
    CHECK(resp.Data[4] == 0x7E && resp.Data[5] == 0x00, "response ECU 0x7E 0x00 via TCP");
    transport_tcp_close(t);
  }
  pthread_join(th, 0);
  tcp_close(g_listen);
  tcp_cleanup();

  printf("\n%d/%d checks passed\n", g_tests - g_fail, g_tests);
  return g_fail ? 1 : 0;
}
