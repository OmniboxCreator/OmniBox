#include <stdio.h>
#include <string.h>
#include "../../firmware/src/elm327/elm327.h"

static int g_fail = 0, g_tests = 0;
#define CHECK(cond, msg) do { g_tests++; if (!(cond)) { printf(" FAIL: %s\n", msg); g_fail++; } } while (0)

typedef struct {
  char out[2048];
  size_t out_n;
  uint32_t ms;
  int opened;
  uint32_t tx_id[8];
  int tx_ext[8];
  uint8_t tx_data[8][8];
  uint8_t tx_len[8];
  uint8_t tx_count;
  struct { uint32_t id; int ext; uint8_t d[8]; uint8_t n; } rx[8];
  uint8_t rh, rt;
} elm_mock_t;

static void mock_write(void *user, const char *s, uint16_t n)
{
  elm_mock_t *m = (elm_mock_t *)user;
  if (m->out_n + n >= sizeof(m->out)) n = (uint16_t)(sizeof(m->out) - m->out_n - 1);
  memcpy(m->out + m->out_n, s, n);
  m->out_n += n;
  m->out[m->out_n] = 0;
}

static int mock_can_open(void *user, uint32_t baud)
{ (void)baud; ((elm_mock_t *)user)->opened = 1; return 0; }

static void mock_can_close(void *user)
{ ((elm_mock_t *)user)->opened = 0; }

static int mock_can_tx(void *user, uint32_t id, int ext, const uint8_t *d, uint8_t n)
{
  elm_mock_t *m = (elm_mock_t *)user;
  uint8_t k = m->tx_count++;
  if (k >= 8) return -1;
  m->tx_id[k] = id; m->tx_ext[k] = ext; m->tx_len[k] = n;
  memset(m->tx_data[k], 0, 8);
  memcpy(m->tx_data[k], d, n > 8 ? 8 : n);
  return 0;
}

static int mock_can_rx(void *user, uint32_t *id, int *ext, uint8_t *d, uint8_t *n)
{
  elm_mock_t *m = (elm_mock_t *)user;
  if (m->rt == m->rh) return 0;
  uint8_t k = m->rt++ & 7u;
  *id = m->rx[k].id; *ext = m->rx[k].ext; *n = m->rx[k].n;
  memcpy(d, m->rx[k].d, *n);
  return 1;
}

static int mock_vbatt(void *user, uint32_t *mv)
{ (void)user; *mv = 12500; return 0; }

static uint32_t mock_millis(void *user)
{ return ((elm_mock_t *)user)->ms++; }

static void queue_rx(elm_mock_t *m, uint32_t id, const uint8_t *d, uint8_t n)
{
  uint8_t k = m->rh++ & 7u;
  m->rx[k].id = id; m->rx[k].ext = id > 0x7FFu; m->rx[k].n = n;
  memset(m->rx[k].d, 0, 8);
  memcpy(m->rx[k].d, d, n);
}

static const elm327_ops_t OPS = {
  .write = mock_write,
  .can_open = mock_can_open,
  .can_close = mock_can_close,
  .can_tx = mock_can_tx,
  .can_rx = mock_can_rx,
  .read_vbatt_mv = mock_vbatt,
  .millis = mock_millis,
};

static void test_can_isotp_reassembly(void)
{
  printf("test_can_isotp_reassembly\n");
  elm_mock_t mock;
  memset(&mock, 0, sizeof(mock));
  elm327_ops_t ops = OPS;
  ops.user = &mock;
  elm327_t elm;
  elm327_init(&elm, &ops);

  const uint8_t echo_off[] = "ATE0\r";
  elm327_input(&elm, echo_off, sizeof(echo_off) - 1);
  mock.out_n = 0; mock.out[0] = 0;

  const uint8_t cmd[] = "22F190\r";
  elm327_input(&elm, cmd, sizeof(cmd) - 1);
  CHECK(mock.opened, "CAN opened");
  CHECK(mock.tx_count == 1, "one request transmitted");
  CHECK(mock.tx_id[0] == 0x7DF, "default functional request ID");
  CHECK(mock.tx_len[0] == 4 && mock.tx_data[0][0] == 0x03 &&
        mock.tx_data[0][1] == 0x22 && mock.tx_data[0][2] == 0xF1 &&
        mock.tx_data[0][3] == 0x90, "request encoded as ISO-TP single frame");

  const uint8_t ff[8] = { 0x10, 0x0A, 0x62, 0xF1, 0x90, 0x11, 0x22, 0x33 };
  const uint8_t cf[5] = { 0x21, 0x44, 0x55, 0x66, 0x77 };
  queue_rx(&mock, 0x7E8, ff, 8);
  queue_rx(&mock, 0x7E8, cf, 5);
  elm327_service(&elm);
  CHECK(mock.tx_count == 2 && mock.tx_data[1][0] == 0x30, "flow-control sent for first frame");
  CHECK(strstr(mock.out, "62 F1 90 11 22 33 44 55 66 77") != NULL,
        "multi-frame response reassembled for ELM output");
}

int main(void)
{
  test_can_isotp_reassembly();
  printf("\n%d/%d checks passed\n", g_tests - g_fail, g_tests);
  return g_fail ? 1 : 0;
}
