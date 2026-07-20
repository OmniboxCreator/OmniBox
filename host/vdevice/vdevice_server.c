
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../shared/transport_tcp.h"
#include "vdevice.h"
#include "vlog.h"

int main(int argc, char **argv)
{
  int port = 9000;
  const char *dump = 0;
  uint32_t base = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0) vlog_set(1);
    else if (strcmp(argv[i], "--dump") == 0 && i + 1 < argc) dump = argv[++i];
    else if (strcmp(argv[i], "--base") == 0 && i + 1 < argc) base = (uint32_t)strtoul(argv[++i], 0, 0);
    else port = atoi(argv[i]);
  }
  const char *env = getenv("J2534_LOG");
  if (env && env[0] && strcmp(env, "0") != 0) vlog_set(1);

  if (dump) {
    int rc = mock_bus_load_dump(dump, base);
    if (rc != 0) { fprintf(stderr, "[vdevice] failed to load dump '%s' (rc=%d)\n", dump, rc); return 1; }
    printf("[vdevice] dump loaded: %s (base 0x%08X)\n", dump, base);
  }

  if (tcp_startup() != 0) { fprintf(stderr, "tcp_startup failed\n"); return 1; }

  tcp_sock_t srv = tcp_listen(port);
  if (srv == TCP_SOCK_INVALID) { fprintf(stderr, "listen on port %d failed\n", port); return 1; }
  printf("[vdevice] listening on port %d (simulated ECU)%s\n",
      tcp_listen_port(srv), vlog_enabled() ? " [log enabled]" : "");
  fflush(stdout);

  for (;;) {
    tcp_sock_t cli = tcp_accept(srv);
    if (cli == TCP_SOCK_INVALID) continue;
    printf("[vdevice] client connected\n"); fflush(stdout);
    vdevice_reset();             

    static uint8_t req[8192], resp[8192];
    size_t rn, sn;
    while (tcp_read_frame(cli, req, sizeof req, &rn) == 0) {
      vlog_request(req, rn);
      if (vdevice_handle_frame(req, rn, resp, sizeof resp, &sn) == 0) {
        vlog_response(resp, sn);
        if (tcp_write_all(cli, resp, sn) != 0) break;
      }
    }
    printf("[vdevice] client disconnected\n"); fflush(stdout);
    tcp_close(cli);
  }
}
