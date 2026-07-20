
#include <stdlib.h>
#include <string.h>
#include "transport_tcp.h"
#include "pt_proto.h"

#ifdef _WIN32
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #define close_sock closesocket
#else
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <netinet/tcp.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <netdb.h>
 #define close_sock close
#endif

int tcp_startup(void)
{
#ifdef _WIN32
  WSADATA w; return WSAStartup(MAKEWORD(2, 2), &w) == 0 ? 0 : -1;
#else
  return 0;
#endif
}
void tcp_cleanup(void)
{
#ifdef _WIN32
  WSACleanup();
#endif
}

int tcp_write_all(tcp_sock_t s, const uint8_t *buf, size_t n)
{
  size_t off = 0;
  while (off < n) {
    int k = (int)send((int)s, (const char *)buf + off, (int)(n - off), 0);
    if (k <= 0) return -1;
    off += (size_t)k;
  }
  return 0;
}

static int read_n(tcp_sock_t s, uint8_t *buf, size_t n)
{
  size_t off = 0;
  while (off < n) {
    int k = (int)recv((int)s, (char *)buf + off, (int)(n - off), 0);
    if (k <= 0) return -1;
    off += (size_t)k;
  }
  return 0;
}

int tcp_read_frame(tcp_sock_t s, uint8_t *buf, size_t cap, size_t *len)
{
  uint8_t b;
  do { if (read_n(s, &b, 1)) return -1; } while (b != PT_SOF);  
  if (cap < 8) return -1;
  buf[0] = b;
  if (read_n(s, buf + 1, 2)) return -1;             
  uint16_t L = (uint16_t)(buf[1] | (buf[2] << 8));
  size_t total = (size_t)5 + L + 2;
  if (total > cap) return -1;
  if (read_n(s, buf + 3, total - 3)) return -1;         
  *len = total;
  return 0;
}


typedef struct { pt_transport_t base; tcp_sock_t sock; } tcp_ctx_t;

static int tcp_send(pt_transport_t *t, const uint8_t *buf, size_t n)
{
  return tcp_write_all(((tcp_ctx_t *)t)->sock, buf, n);
}
static int tcp_recv(pt_transport_t *t, uint8_t *buf, size_t cap, size_t *got, int timeout_ms)
{
  (void)timeout_ms;
  return tcp_read_frame(((tcp_ctx_t *)t)->sock, buf, cap, got);
}

pt_transport_t *transport_tcp_connect(const char *host, int port)
{
  tcp_sock_t s = (tcp_sock_t)socket(AF_INET, SOCK_STREAM, 0);
  if (s == TCP_SOCK_INVALID) return 0;
  struct sockaddr_in a; memset(&a, 0, sizeof a);
  a.sin_family = AF_INET;
  a.sin_port = htons((unsigned short)port);
  a.sin_addr.s_addr = inet_addr(host ? host : "127.0.0.1");
  if (connect((int)s, (struct sockaddr *)&a, sizeof a) != 0) { close_sock((int)s); return 0; }
  int one = 1; setsockopt((int)s, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
  tcp_ctx_t *c = (tcp_ctx_t *)calloc(1, sizeof *c);
  if (!c) { close_sock((int)s); return 0; }
  c->base.send = tcp_send; c->base.recv = tcp_recv; c->base.ctx = c; c->sock = s;
  return &c->base;
}
void transport_tcp_close(pt_transport_t *t)
{
  if (!t) return;
  tcp_ctx_t *c = (tcp_ctx_t *)t;
  close_sock((int)c->sock);
  free(c);
}


tcp_sock_t tcp_listen(int port)
{
  tcp_sock_t s = (tcp_sock_t)socket(AF_INET, SOCK_STREAM, 0);
  if (s == TCP_SOCK_INVALID) return TCP_SOCK_INVALID;
  int one = 1; setsockopt((int)s, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one);
  struct sockaddr_in a; memset(&a, 0, sizeof a);
  a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY);
  a.sin_port = htons((unsigned short)port);
  if (bind((int)s, (struct sockaddr *)&a, sizeof a) != 0) { close_sock((int)s); return TCP_SOCK_INVALID; }
  if (listen((int)s, 4) != 0) { close_sock((int)s); return TCP_SOCK_INVALID; }
  return s;
}
int tcp_listen_port(tcp_sock_t s)
{
  struct sockaddr_in a; socklen_t n = sizeof a;
  if (getsockname((int)s, (struct sockaddr *)&a, &n) != 0) return -1;
  return ntohs(a.sin_port);
}
tcp_sock_t tcp_accept(tcp_sock_t listener)
{
  tcp_sock_t c = (tcp_sock_t)accept((int)listener, 0, 0);
  if (c == TCP_SOCK_INVALID) return TCP_SOCK_INVALID;
  int one = 1; setsockopt((int)c, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof one);
  return c;
}
void tcp_close(tcp_sock_t s) { close_sock((int)s); }
