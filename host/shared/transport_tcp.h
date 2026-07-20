
#ifndef TRANSPORT_TCP_H
#define TRANSPORT_TCP_H

#include <stdint.h>
#include <stddef.h>
#include "pt_transport.h"

#ifdef _WIN32
 typedef unsigned long long tcp_sock_t;  
#else
 typedef int tcp_sock_t;
#endif
#define TCP_SOCK_INVALID ((tcp_sock_t)~0)


int tcp_startup(void);
void tcp_cleanup(void);


pt_transport_t *transport_tcp_connect(const char *host, int port);
void transport_tcp_close(pt_transport_t *t);


int tcp_write_all(tcp_sock_t s, const uint8_t *buf, size_t n);
int tcp_read_frame(tcp_sock_t s, uint8_t *buf, size_t cap, size_t *len);


tcp_sock_t tcp_listen(int port);
int    tcp_listen_port(tcp_sock_t s);  
tcp_sock_t tcp_accept(tcp_sock_t listener);
void    tcp_close(tcp_sock_t s);

#endif 
