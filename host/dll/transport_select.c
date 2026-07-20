
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "transport_select.h"
#include "transport_winusb.h"
#include "../shared/transport_tcp.h"

static int g_kind;  

pt_transport_t *pt_open_transport(char *err, size_t err_cap)
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
    if (!t) { if (err) snprintf(err, err_cap, "TCP connection to %s:%d failed", host, port); tcp_cleanup(); return 0; }
    g_kind = 1;
    return t;
  }
  pt_transport_t *t = transport_winusb_connect(err, err_cap);  
  g_kind = t ? 2 : 0;
  return t;
}

void pt_close_transport(pt_transport_t *t)
{
  if (!t) return;
  if (g_kind == 1) { transport_tcp_close(t); tcp_cleanup(); }
  else if (g_kind == 2) transport_winusb_close(t);
  g_kind = 0;
}
