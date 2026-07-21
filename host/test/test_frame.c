
#include <stdio.h>
#include "../shared/pt_frame.h"
#include "../shared/pt_proto.h"

static int g_fail = 0, g_tests = 0;
#define CHECK(cond, msg) do { g_tests++; if (!(cond)) { printf(" FAIL: %s\n", msg); g_fail++; } } while (0)

static void test_crc_vector(void)
{
  printf("test_crc_vector\n");
  
  CHECK(pt_crc16((const uint8_t *)"123456789", 9) == 0x29B1, "known CRC vector");
}

static void test_build_parse(void)
{
  printf("test_build_parse\n");
  uint8_t payload[4] = { 0x00, 0xDE, 0xAD, 0xBE };  
  uint8_t frame[64];
  int len = pt_frame_build(frame, sizeof frame, 7, CMD_OPEN | PT_RESP_FLAG, payload, 4);
  CHECK(len == 5 + 4 + 2, "frame length");
  CHECK(frame[0] == PT_SOF, "SOF");

  uint8_t seq, cmd, status; const uint8_t *data; uint16_t dlen;
  int rc = pt_frame_parse(frame, (size_t)len, &seq, &cmd, &status, &data, &dlen);
  CHECK(rc == 0, "parse OK");
  CHECK(seq == 7, "seq");
  CHECK(cmd == CMD_OPEN, "cmd (response flag removed)");
  CHECK(status == 0x00, "status");
  CHECK(dlen == 3 && data[0] == 0xDE && data[2] == 0xBE, "data after status");
}

static void test_corruption(void)
{
  printf("test_corruption\n");
  uint8_t payload[2] = { 0, 0x42 };
  uint8_t frame[32];
  int len = pt_frame_build(frame, sizeof frame, 1, CMD_PING | PT_RESP_FLAG, payload, 2);
  frame[6] ^= 0xFF;                  
  uint8_t seq, cmd, status; const uint8_t *data; uint16_t dlen;
  CHECK(pt_frame_parse(frame, (size_t)len, &seq, &cmd, &status, &data, &dlen) == -1, "corrupted CRC -> rejected");
}

static void test_cap_guard(void)
{
  printf("test_cap_guard\n");
  uint8_t small[6];
  CHECK(pt_frame_build(small, sizeof small, 0, CMD_OPEN, (const uint8_t *)"xxxx", 4) == -1, "insufficient capacity -> -1");
}

int main(void)
{
  test_crc_vector();
  test_build_parse();
  test_corruption();
  test_cap_guard();
  printf("\n%d/%d checks passed\n", g_tests - g_fail, g_tests);
  return g_fail ? 1 : 0;
}
