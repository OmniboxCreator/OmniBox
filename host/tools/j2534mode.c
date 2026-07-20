
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>  
#endif
#include "pt_frame.h"
#include "pt_proto.h"
#include "pt_transport.h"
#include "transport_tcp.h"
#ifdef _WIN32
#include "transport_winusb.h"
#endif


static const char *const MODE_NAMES[] = { "J2534+ELM327", "Empty slot 1",
                     "Empty slot 2", "Empty slot 3" };
#define MODE_MAX (int)(sizeof(MODE_NAMES) / sizeof(MODE_NAMES[0]))

static const char *mode_name(int m)
{
  return (m >= 0 && m < MODE_MAX) ? MODE_NAMES[m] : "?";
}


static pt_transport_t *open_transport(char *err, size_t err_cap)
{
  const char *spec = getenv("J2534_TCP");
  if (spec && *spec) {
    char host[64] = "127.0.0.1"; int port = 9000;
    const char *colon = strchr(spec, ':');
    if (colon) {
      size_t hl = (size_t)(colon - spec);
      if (hl && hl < sizeof host) { memcpy(host, spec, hl); host[hl] = 0; }
      port = atoi(colon + 1);
    } else {
      snprintf(host, sizeof host, "%s", spec);
    }
    tcp_startup();
    pt_transport_t *t = transport_tcp_connect(host, port);
    if (!t) snprintf(err, err_cap, "TCP connection to %s:%d failed", host, port);
    return t;
  }
#ifdef _WIN32
  return transport_winusb_connect(err, err_cap);
#else
  snprintf(err, err_cap, "set J2534_TCP=host:port (WinUSB is Windows-only)");
  return NULL;
#endif
}


static int do_cmd(pt_transport_t *t, uint8_t cmd, const uint8_t *payload, uint16_t plen,
         uint8_t *rdata, uint16_t *rlen)
{
  uint8_t frame[64];
  int n = pt_frame_build(frame, sizeof frame, 1, cmd, payload, plen);
  if (n < 0) return -1;
  if (t->send(t, frame, (size_t)n) != 0) return -1;

  uint8_t resp[64]; size_t got = 0;
  if (t->recv(t, resp, sizeof resp, &got, 2000) != 0) return -1;

  uint8_t seq, rcmd, status; const uint8_t *data; uint16_t dlen;
  if (pt_frame_parse(resp, got, &seq, &rcmd, &status, &data, &dlen) != 0) return -1;
  if (rdata && rlen) {
    uint16_t k = dlen < *rlen ? dlen : *rlen;
    memcpy(rdata, data, k);
    *rlen = k;
  }
  return status;
}

static int parse_mode_arg(const char *s)
{
  if (!strcasecmp(s, "j2534")) return 0;
  if (!strcasecmp(s, "slot1")) return 1;
  if (!strcasecmp(s, "slot2")) return 2;
  if (!strcasecmp(s, "slot3")) return 3;
  char *end; long v = strtol(s, &end, 10);
  if (*end == 0 && v >= 0 && v < MODE_MAX) return (int)v;
  return -1;
}

static int cmd_get(pt_transport_t *t)
{
  uint8_t data[8]; uint16_t dlen = sizeof data;
  int st = do_cmd(t, CMD_USB_MODE_GET, NULL, 0, data, &dlen);
  if (st < 0) { fprintf(stderr, "I/O error (device unreachable?)\n"); return 2; }
  if (st != 0) { fprintf(stderr, "device refused GET (status 0x%02X)\n", st); return 2; }
  if (dlen < 2) { fprintf(stderr, "GET response too short\n"); return 2; }
  int active = data[0], count = data[1];
  printf("Active USB mode: %d (%s)\n\n", active, mode_name(active));
  printf("Available modes:\n");
  for (int i = 0; i < count && i < MODE_MAX; i++)
    printf(" %d %-20s %s\n", i, mode_name(i), i == active ? "<= active" : "");
  return 0;
}

static int cmd_set(pt_transport_t *t, int mode)
{
  uint8_t req = (uint8_t)mode, data[4]; uint16_t dlen = sizeof data;
  int st = do_cmd(t, CMD_USB_MODE_SET, &req, 1, data, &dlen);
  if (st < 0) { fprintf(stderr, "I/O error (device unreachable?)\n"); return 2; }
  if (st != 0) { fprintf(stderr, "device refused SET (status 0x%02X)\n", st); return 2; }
  printf("Mode %d (%s) stored.\n", mode, mode_name(mode));
  printf("The device reboots and re-enumerates under this identity.\n");
  printf("Power-cycle if needed to return to J2534 mode.\n");
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr,
      "usage: %s get\n"
      "    %s set <mode>  (mode = 0..3 or j2534|slot1|slot2|slot3)\n"
      "transport: J2534_TCP=host:port (virtual device) or WinUSB (Windows).\n",
      argv[0], argv[0]);
    return 1;
  }

  char err[128] = {0};
  pt_transport_t *t = open_transport(err, sizeof err);
  if (!t) { fprintf(stderr, "transport: %s\n", err[0] ? err : "open failed"); return 2; }

  int rc;
  if (!strcmp(argv[1], "get")) {
    rc = cmd_get(t);
  } else if (!strcmp(argv[1], "set") && argc >= 3) {
    int mode = parse_mode_arg(argv[2]);
    if (mode < 0) { fprintf(stderr, "invalid mode: %s\n", argv[2]); rc = 1; }
    else rc = cmd_set(t, mode);
  } else {
    fprintf(stderr, "unknown command: %s\n", argv[1]);
    rc = 1;
  }

#ifdef _WIN32
  if (!getenv("J2534_TCP")) transport_winusb_close(t); else { transport_tcp_close(t); tcp_cleanup(); }
#else
  transport_tcp_close(t); tcp_cleanup();
#endif
  return rc;
}
