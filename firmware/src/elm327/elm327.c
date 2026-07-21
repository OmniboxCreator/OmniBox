
#include <string.h>
#include <stdio.h>
#include "elm327.h"

#define ST_TO_MS(st)    ((uint32_t)(st) * 4u)
#define AT1_SILENCE_MS   100u
#define CAN_DATA_MAX    8u


static void out(elm327_t *e, const char *s, uint16_t n) { e->ops->write(e->ops->user, s, n); }
static void out_str(elm327_t *e, const char *s) { out(e, s, (uint16_t)strlen(s)); }
static void out_eol(elm327_t *e) { out_str(e, e->linefeeds ? "\r\n" : "\r"); }
static void out_line(elm327_t *e, const char *s) { out_str(e, s); out_eol(e); }
static void out_prompt(elm327_t *e) { out_eol(e); out_str(e, ">"); }
static void reply(elm327_t *e, const char *s) { out_line(e, s); out_prompt(e); }


static int hex_val(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static int hex_u32(const char *s, uint16_t n, uint32_t *val)
{
  uint32_t v = 0;
  if (n == 0 || n > 8) return -1;
  for (uint16_t i = 0; i < n; i++) {
    int d = hex_val(s[i]);
    if (d < 0) return -1;
    v = (v << 4) | (uint32_t)d;
  }
  *val = v;
  return 0;
}


static int hex_bytes(const char *s, uint16_t n, uint8_t *buf, uint16_t cap)
{
  if (n & 1u) return -1;
  if (n / 2 > cap) return -1;
  for (uint16_t i = 0; i < n; i += 2) {
    int hi = hex_val(s[i]), lo = hex_val(s[i + 1]);
    if (hi < 0 || lo < 0) return -1;
    buf[i / 2] = (uint8_t)((hi << 4) | lo);
  }
  return (int)(n / 2);
}


static void close_proto(elm327_t *e)
{
  if (e->proto_open && e->ops->can_close) e->ops->can_close(e->ops->user);
  e->proto_open = 0;
  e->req_state = ELM_REQ_IDLE;
}

static void set_defaults(elm327_t *e)
{
  e->echo = 1; e->linefeeds = 0; e->spaces = 1; e->headers = 0;
  e->caf = 1; e->cfc = 1; e->responses = 1; e->adaptive = 1;
  e->st = ELM327_ST_DEFAULT; e->dlc_display = 0; e->allow_long = 0;
  e->kw_check = 1;
  e->tx_id = 0x7DF; e->tx_ext = 0; e->can_prio = 0x18;
  e->rx_id = ELM_CRA_NONE; e->rx_ext = 0;
  e->fc_mode = 0; e->fc_id = ELM_CRA_NONE; e->fc_ext = 0;
  e->fc_data[0] = 0x30; e->fc_data[1] = 0x00; e->fc_data[2] = 0x00; e->fc_len = 3;
  e->iso_baud = 10; e->iso_init_addr = 0x33; e->wakeup_period = 0x92;
  e->wm_len = 0;
}

void elm327_init(elm327_t *e, const elm327_ops_t *ops)
{
  memset(e, 0, sizeof(*e));
  e->ops = ops;
  set_defaults(e);
  e->proto = ELM_PROTO_AUTO;
}


static int proto_is_can(uint8_t p)
{ return p == ELM_PROTO_AUTO || (p >= ELM_PROTO_CAN11_500 && p <= ELM_PROTO_CAN29_250); }
static int proto_is_ext(uint8_t p)
{ return p == ELM_PROTO_CAN29_500 || p == ELM_PROTO_CAN29_250; }
static uint32_t proto_baud(uint8_t p)
{ return (p == ELM_PROTO_CAN11_250 || p == ELM_PROTO_CAN29_250) ? 250000u : 500000u; }

static int ensure_can_open(elm327_t *e)
{
  if (e->proto_open) return 0;
  if (!e->ops->can_open || e->ops->can_open(e->ops->user, proto_baud(e->proto)) != 0)
    return -1;
  e->proto_open = 1;
  return 0;
}


static uint32_t effective_tx_id(const elm327_t *e, int *ext)
{
  if (proto_is_ext(e->proto)) {
    *ext = 1;
    return e->tx_ext ? e->tx_id : (((uint32_t)e->can_prio << 24) | (e->tx_id & 0xFFFFFFu));
  }
  *ext = 0;
  return e->tx_id & 0x7FFu;
}


static int rx_match(const elm327_t *e, uint32_t id, int ext)
{
  if (e->req_state == ELM_REQ_MONITOR) return 1;
  if (e->rx_id != ELM_CRA_NONE)
    return id == e->rx_id && (ext ? 1 : 0) == (e->rx_ext ? 1 : 0);
  if (proto_is_ext(e->proto)) return 1;
  if (ext) return 0;
  if (e->tx_id == 0x7DF) return id >= 0x7E8 && id <= 0x7EF;
  return id == e->tx_id + 8;
}


static void display_frame(elm327_t *e, uint32_t id, int ext, const uint8_t *d, uint8_t n)
{
  char buf[64];
  int k = 0;
  if (e->headers)
    k += snprintf(buf + k, sizeof(buf) - (size_t)k, ext ? "%08X" : "%03X",
           (unsigned)id);
  if (e->headers && e->dlc_display)
    k += snprintf(buf + k, sizeof(buf) - (size_t)k, e->spaces ? " %X" : "%X", n);
  for (uint8_t i = 0; i < n; i++) {
    if ((e->spaces && k) || (e->spaces && e->headers))
      k += snprintf(buf + k, sizeof(buf) - (size_t)k, " ");
    k += snprintf(buf + k, sizeof(buf) - (size_t)k, "%02X", d[i]);
  }
  out_line(e, buf);
}

static void display_payload(elm327_t *e, uint32_t id, int ext, const uint8_t *d, uint16_t n)
{
  char buf[96];
  uint16_t pos = 0;
  if (e->headers) {
    pos += (uint16_t)snprintf(buf + pos, sizeof(buf) - pos, ext ? "%08X" : "%03X", (unsigned)id);
  }
  for (uint16_t i = 0; i < n; i++) {
    if (pos > sizeof(buf) - 5u) { out_line(e, buf); pos = 0; }
    if ((e->spaces && pos) || (e->spaces && e->headers))
      pos += (uint16_t)snprintf(buf + pos, sizeof(buf) - pos, " ");
    pos += (uint16_t)snprintf(buf + pos, sizeof(buf) - pos, "%02X", d[i]);
  }
  out_line(e, buf);
}


static void send_auto_fc(elm327_t *e)
{
  int ext = 0;
  uint32_t id;
  if (e->fc_id != ELM_CRA_NONE) { id = e->fc_id; ext = e->fc_ext; }
  else id = effective_tx_id(e, &ext);
  if (e->ops->can_tx)
    e->ops->can_tx(e->ops->user, id, ext, e->fc_data, e->fc_len);
}

static void count_response(elm327_t *e)
{
  e->req_got++;
  e->req_last_rx_ms = e->ops->millis(e->ops->user);
}

static int handle_isotp_frame(elm327_t *e, uint32_t id, int ext, const uint8_t *d, uint8_t n)
{
  if (!e->caf || n == 0) return 0;
  uint8_t pci = d[0] & 0xF0u;
  if (pci == 0x00u) {
    uint8_t plen = d[0] & 0x0Fu;
    if (plen > n - 1u) return 0;
    display_payload(e, id, ext, d + 1, plen);
    count_response(e);
    return 1;
  }
  if (pci == 0x10u) {
    if (n < 2u) return 0;
    uint16_t total = (uint16_t)(((uint16_t)(d[0] & 0x0Fu) << 8) | d[1]);
    if (total == 0u || total > ELM327_ISOTP_MAX) return 0;
    e->tp_active = 1;
    e->tp_sn = 1;
    e->tp_len = total;
    e->tp_pos = 0;
    uint8_t copy = (uint8_t)(n - 2u);
    if (copy > total) copy = (uint8_t)total;
    memcpy(e->tp_buf, d + 2, copy);
    e->tp_pos = copy;
    if (e->cfc) send_auto_fc(e);
    if (e->tp_pos >= e->tp_len) {
      display_payload(e, id, ext, e->tp_buf, e->tp_len);
      e->tp_active = 0;
      count_response(e);
    }
    return 1;
  }
  if (pci == 0x20u && e->tp_active) {
    uint8_t sn = d[0] & 0x0Fu;
    if (sn != (e->tp_sn & 0x0Fu)) { e->tp_active = 0; return 1; }
    e->tp_sn = (uint8_t)((e->tp_sn + 1u) & 0x0Fu);
    uint16_t rem = (uint16_t)(e->tp_len - e->tp_pos);
    uint8_t copy = (uint8_t)(n - 1u);
    if (copy > rem) copy = (uint8_t)rem;
    memcpy(e->tp_buf + e->tp_pos, d + 1, copy);
    e->tp_pos = (uint16_t)(e->tp_pos + copy);
    if (e->tp_pos >= e->tp_len) {
      display_payload(e, id, ext, e->tp_buf, e->tp_len);
      e->tp_active = 0;
      count_response(e);
    }
    return 1;
  }
  return 0;
}


static void req_finish(elm327_t *e, int stopped)
{
  if (stopped) out_line(e, "STOPPED");
  else if (e->req_got == 0) out_line(e, "NO DATA");
  out_prompt(e);
  e->req_state = ELM_REQ_IDLE;
}


void elm327_service(elm327_t *e)
{
  uint32_t id; int ext; uint8_t d[8], n;

  if (e->req_state == ELM_REQ_IDLE) {

    if (e->proto_open && e->ops->can_rx)
      while (e->ops->can_rx(e->ops->user, &id, &ext, d, &n)) { }
    return;
  }

	  while (e->ops->can_rx && e->ops->can_rx(e->ops->user, &id, &ext, d, &n)) {
	    if (!rx_match(e, id, ext)) continue;
	    if (e->req_state != ELM_REQ_MONITOR && handle_isotp_frame(e, id, ext, d, n)) {
	      if (e->req_state == ELM_REQ_WAIT_RESP && e->req_expect && e->req_got >= e->req_expect) { req_finish(e, 0); return; }
	      continue;
	    }
	    display_frame(e, id, ext, d, n);
	    count_response(e);
	    if (e->req_state == ELM_REQ_WAIT_RESP) {
	      if (e->cfc && n >= 1 && (d[0] & 0xF0u) == 0x10u)
	        send_auto_fc(e);
      if (e->req_expect && e->req_got >= e->req_expect) { req_finish(e, 0); return; }
    }
  }

  if (e->req_state == ELM_REQ_WAIT_RESP) {
    uint32_t now = e->ops->millis(e->ops->user);
    uint32_t st_ms = ST_TO_MS(e->st);
    if (e->req_got == 0) {
      if (now - e->req_start_ms >= st_ms) req_finish(e, 0);
    } else {
      uint32_t silence = e->adaptive ? (st_ms < AT1_SILENCE_MS ? st_ms : AT1_SILENCE_MS)
                      : st_ms;
      if (now - e->req_last_rx_ms >= silence) req_finish(e, 0);
    }
  }
}


static void do_data(elm327_t *e, const char *s, uint16_t n)
{
  uint8_t payload[CAN_DATA_MAX];
  uint8_t expect = 0;

  if (n & 1u) {
    int d = hex_val(s[n - 1]);
    if (d < 0) { reply(e, "?"); return; }
    expect = (uint8_t)d;
    n--;
  }
  int cnt = hex_bytes(s, n, payload, CAN_DATA_MAX);
  if (cnt <= 0) { reply(e, "?"); return; }

  if (!proto_is_can(e->proto)) {
    reply(e, "BUS INIT: ...ERROR");
    return;
  }
  if (ensure_can_open(e) != 0) { reply(e, "CAN ERROR"); return; }

  int ext = 0;
  uint32_t id = effective_tx_id(e, &ext);
  uint8_t txbuf[8];
  const uint8_t *tx = payload;
  uint8_t txlen = (uint8_t)cnt;
  if (e->caf) {
    if (cnt > 7) { reply(e, "CAN ERROR"); return; }
    txbuf[0] = (uint8_t)cnt;
    memcpy(txbuf + 1, payload, (uint8_t)cnt);
    tx = txbuf;
    txlen = (uint8_t)(cnt + 1);
  }
  if (!e->ops->can_tx ||
    e->ops->can_tx(e->ops->user, id, ext, tx, txlen) != 0) {
    reply(e, "CAN ERROR");
    return;
  }
  if (!e->responses) { out_prompt(e); return; }

  e->req_state = ELM_REQ_WAIT_RESP;
  e->req_expect = expect;
  e->req_got = 0;
  e->req_start_ms = e->req_last_rx_ms = e->ops->millis(e->ops->user);
}


static int starts(const char *s, const char *pfx) { return strncmp(s, pfx, strlen(pfx)) == 0; }


static int at_flag(const char *s, const char *pfx, uint8_t *flag)
{
  size_t l = strlen(pfx);
  if (strncmp(s, pfx, l) != 0 || (s[l] != '0' && s[l] != '1') || s[l + 1] != '\0')
    return 0;
  *flag = (uint8_t)(s[l] - '0');
  return 1;
}

static void at_describe_proto(elm327_t *e, int by_number)
{
  static const char *const names[] = {
    [ELM_PROTO_ISO9141]  = "ISO 9141-2",
    [ELM_PROTO_KWP_SLOW] = "ISO 14230-4 (KWP 5BAUD)",
    [ELM_PROTO_KWP_FAST] = "ISO 14230-4 (KWP FAST)",
    [ELM_PROTO_CAN11_500] = "ISO 15765-4 (CAN 11/500)",
    [ELM_PROTO_CAN29_500] = "ISO 15765-4 (CAN 29/500)",
    [ELM_PROTO_CAN11_250] = "ISO 15765-4 (CAN 11/250)",
    [ELM_PROTO_CAN29_250] = "ISO 15765-4 (CAN 29/250)",
  };
  if (by_number) {
    char b[3] = { "0123456789ABCDEF"[e->proto & 0xF], 0, 0 };
    reply(e, b);
  } else if (e->proto == ELM_PROTO_AUTO) {
    reply(e, "AUTO");
  } else if (e->proto < sizeof(names)/sizeof(names[0]) && names[e->proto]) {
    reply(e, names[e->proto]);
  } else {
    reply(e, "?");
  }
}


static void at_set_header(elm327_t *e, const char *a, uint16_t n)
{
  uint32_t v;
  if (hex_u32(a, n, &v) != 0 || (n != 3 && n != 6 && n != 8)) { reply(e, "?"); return; }
  if (n == 3)   { e->tx_id = v & 0x7FFu; e->tx_ext = 0; }
  else if (n == 6) { e->tx_id = v & 0xFFFFFFu; e->tx_ext = 0; }
  else       { e->tx_id = v & 0x1FFFFFFFu; e->tx_ext = 1; }
  reply(e, "OK");
}

static void at_command(elm327_t *e, const char *s)
{
  uint32_t v;
  uint16_t n = (uint16_t)strlen(s);


  if (!strcmp(s, "Z") || !strcmp(s, "WS")) {
    close_proto(e);
    uint8_t p = e->proto;
    set_defaults(e);
    if (!strcmp(s, "WS")) e->proto = p;
    else e->proto = ELM_PROTO_AUTO;
    reply(e, ELM327_ID_STRING);
    return;
  }
  if (!strcmp(s, "I")) { reply(e, ELM327_ID_STRING); return; }
  if (!strcmp(s, "@1") || !strcmp(s, "@2")) { reply(e, ELM327_DESC_STRING); return; }
  if (!strcmp(s, "D")) { set_defaults(e); reply(e, "OK"); return; }


  if (at_flag(s, "E", &e->echo) || at_flag(s, "L", &e->linefeeds) ||
    at_flag(s, "S", &e->spaces) || at_flag(s, "H", &e->headers) ||
    at_flag(s, "R", &e->responses) || at_flag(s, "D", &e->dlc_display) ||
    at_flag(s, "CAF", &e->caf) || at_flag(s, "CFC", &e->cfc) ||
    at_flag(s, "KW", &e->kw_check)) { reply(e, "OK"); return; }


  if (!strcmp(s, "AT0") || !strcmp(s, "AT1") || !strcmp(s, "AT2")) {
    e->adaptive = (uint8_t)(s[2] - '0'); reply(e, "OK"); return;
  }
  if (starts(s, "ST") && hex_u32(s + 2, (uint16_t)(n - 2), &v) == 0 && n == 4) {
    e->st = (uint8_t)(v ? v : ELM327_ST_DEFAULT); reply(e, "OK"); return;
  }


  if (starts(s, "SP") || starts(s, "TP")) {
    const char *a = s + 2;
    if (*a == 'A') a++;
    if (!strcmp(a, "") || !strcmp(a, "00")) { close_proto(e); e->proto = ELM_PROTO_AUTO; reply(e, "OK"); return; }
    int d = (strlen(a) == 1) ? hex_val(a[0]) : -1;
    if (d < 0 || d > 0xC) { reply(e, "?"); return; }
    close_proto(e);
    e->proto = (uint8_t)d;
    reply(e, "OK");
    return;
  }
  if (!strcmp(s, "DP")) { at_describe_proto(e, 0); return; }
  if (!strcmp(s, "DPN")) { at_describe_proto(e, 1); return; }
  if (!strcmp(s, "PC")) { close_proto(e); reply(e, "OK"); return; }


  if (starts(s, "SH")) { at_set_header(e, s + 2, (uint16_t)(n - 2)); return; }
  if (starts(s, "CP") && n == 4 && hex_u32(s + 2, 2, &v) == 0) {
    e->can_prio = (uint8_t)v; reply(e, "OK"); return;
  }
  if (!strcmp(s, "CRA")) { e->rx_id = ELM_CRA_NONE; reply(e, "OK"); return; }
  if (starts(s, "CRA") && (n == 6 || n == 11) && hex_u32(s + 3, (uint16_t)(n - 3), &v) == 0) {
    e->rx_id = v; e->rx_ext = (n == 11); reply(e, "OK"); return;
  }

  if ((starts(s, "CM") || starts(s, "CF")) && n > 2 &&
    hex_u32(s + 2, (uint16_t)(n - 2), &v) == 0) { reply(e, "OK"); return; }
  if (starts(s, "CEA")) { reply(e, "OK"); return; }


  if (starts(s, "FCSM") && n == 5 && (s[4] == '0' || s[4] == '1' || s[4] == '2')) {
    e->fc_mode = (uint8_t)(s[4] - '0'); reply(e, "OK"); return;
  }
  if (starts(s, "FCSH") && (n == 7 || n == 12) && hex_u32(s + 4, (uint16_t)(n - 4), &v) == 0) {
    e->fc_id = v; e->fc_ext = (n == 12); reply(e, "OK"); return;
  }
  if (starts(s, "FCSD")) {
    int c = hex_bytes(s + 4, (uint16_t)(n - 4), e->fc_data, sizeof(e->fc_data));
    if (c < 1) { reply(e, "?"); return; }
    e->fc_len = (uint8_t)c; reply(e, "OK"); return;
  }


  if (starts(s, "IB") && (n == 4)) { e->iso_baud = (uint8_t)((s[2]-'0')*10 + (s[3]-'0')); reply(e, "OK"); return; }
  if (starts(s, "IIA") && hex_u32(s + 3, (uint16_t)(n - 3), &v) == 0) {
    e->iso_init_addr = (uint8_t)v; reply(e, "OK"); return;
  }
  if (starts(s, "SW") && hex_u32(s + 2, (uint16_t)(n - 2), &v) == 0 && n == 4) {
    e->wakeup_period = (uint8_t)v; reply(e, "OK"); return;
  }
  if (starts(s, "WM")) {
    int c = hex_bytes(s + 2, (uint16_t)(n - 2), e->wm, sizeof(e->wm));
    if (c < 1) { reply(e, "?"); return; }
    e->wm_len = (uint8_t)c; reply(e, "OK"); return;
  }
  if (!strcmp(s, "SI") || !strcmp(s, "FI")) {

    reply(e, "BUS INIT: ...ERROR");
    return;
  }
  if (!strcmp(s, "KW")) { reply(e, "KW1:E9 KW2:8F"); return; }


  if (!strcmp(s, "MA") || starts(s, "MR") || starts(s, "MT")) {
    if (proto_is_can(e->proto) && ensure_can_open(e) != 0) { reply(e, "CAN ERROR"); return; }
    out_eol(e);
    e->req_state = ELM_REQ_MONITOR;
    e->req_got = 0; e->req_expect = 0;
    e->req_start_ms = e->req_last_rx_ms = e->ops->millis(e->ops->user);
    return;
  }


  if (!strcmp(s, "RV")) {
    uint32_t mv = 0;
    if (e->ops->read_vbatt_mv && e->ops->read_vbatt_mv(e->ops->user, &mv) == 0) {
      char b[16];
      snprintf(b, sizeof(b), "%u.%uV", (unsigned)(mv / 1000u), (unsigned)((mv % 1000u) / 100u));
      reply(e, b);
    } else reply(e, "0.0V");
    return;
  }
  if (!strcmp(s, "IGN")) { reply(e, "ON"); return; }
  if (starts(s, "BRD") || starts(s, "BRT")) { reply(e, "OK"); return; }


  static const char *const ACK_EXACT[] = {
    "AL", "NL", "AR", "BD", "BI", "CS", "FE", "JE", "JS", "PPS", "RD",
    "RTR", "SS", "LP", "CSM0", "CSM1", "JHF0", "JHF1", "V0", "V1",
    "M0", "M1", "IFR0", "IFR1", "IFR2", "IFRH", "IFRS", "DM1",
  };
  for (unsigned i = 0; i < sizeof(ACK_EXACT)/sizeof(ACK_EXACT[0]); i++)
    if (!strcmp(s, ACK_EXACT[i])) { reply(e, "OK"); return; }
  static const char *const ACK_PFX[] = {
    "CV", "PP", "PB", "TA", "SR", "RA", "SD", "MP", "JTM",
  };
  for (unsigned i = 0; i < sizeof(ACK_PFX)/sizeof(ACK_PFX[0]); i++)
    if (starts(s, ACK_PFX[i]) && n > strlen(ACK_PFX[i])) { reply(e, "OK"); return; }

  reply(e, "?");
}


static void run_line(elm327_t *e, const char *s, uint16_t n)
{
  if (n == 0) { out_prompt(e); return; }
  if (n >= 2 && s[0] == 'A' && s[1] == 'T') at_command(e, s + 2);
  else do_data(e, s, n);
}

static void process_line(elm327_t *e)
{
  char norm[ELM327_LINE_MAX];
  uint16_t k = 0;

  for (uint16_t i = 0; i < e->line_len && k < ELM327_LINE_MAX - 1; i++) {
    char c = e->line[i];
    if (c == ' ' || c == '\t') continue;
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    norm[k++] = c;
  }
  norm[k] = '\0';
  e->line_len = 0;

  if (k == 0) {
    if (e->last_len) run_line(e, e->last_line, e->last_len);
    else out_prompt(e);
    return;
  }
  memcpy(e->last_line, norm, (size_t)k + 1);
  e->last_len = k;
  run_line(e, norm, k);
}

void elm327_input(elm327_t *e, const uint8_t *data, uint32_t n)
{
  for (uint32_t i = 0; i < n; i++) {
    uint8_t c = data[i];


    if (e->req_state != ELM_REQ_IDLE) req_finish(e, 1);

    if (e->echo) out(e, (const char *)&c, 1);
    if (c == '\r') { process_line(e); continue; }
    if (c == '\n') continue;
    if (c == 0x08) { if (e->line_len) e->line_len--; continue; }
    if (e->line_len < ELM327_LINE_MAX - 1) e->line[e->line_len++] = (char)c;
  }
}
