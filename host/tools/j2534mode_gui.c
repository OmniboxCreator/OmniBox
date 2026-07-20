
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "pt_frame.h"
#include "pt_proto.h"
#include "pt_transport.h"
#include "transport_tcp.h"
#include "transport_winusb.h"


static const char *const MODE_NAMES[] = {
  "J2534 + ELM327", "Empty slot 1", "Empty slot 2", "Empty slot 3" };
#define MODE_COUNT ((int)(sizeof(MODE_NAMES) / sizeof(MODE_NAMES[0])))

enum { ID_BTN_BASE = 1000, ID_REFRESH = 1100, ID_LOG = 1101, ID_STATUS = 1102 };
static HWND g_log, g_status, g_btn[MODE_COUNT];


static void logln(const char *fmt, ...)
{
  char line[512];
  va_list ap; va_start(ap, fmt);
  int n = vsnprintf(line, sizeof line - 2, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  strcpy(line + (n < (int)sizeof line - 2 ? n : (int)sizeof line - 2), "\r\n");
  int len = GetWindowTextLength(g_log);
  SendMessage(g_log, EM_SETSEL, len, len);
  SendMessage(g_log, EM_REPLACESEL, FALSE, (LPARAM)line);
}


static pt_transport_t *open_transport(char *err, size_t cap)
{
  return transport_winusb_connect(err, cap);
}
static void close_transport(pt_transport_t *t)
{
  if (t) transport_winusb_close(t);
}


static int do_cmd(pt_transport_t *t, uint8_t cmd, const uint8_t *pl, uint16_t pn,
         uint8_t *rd, uint16_t *rn)
{
  uint8_t frame[64];
  int n = pt_frame_build(frame, sizeof frame, 1, cmd, pl, pn);
  if (n < 0 || t->send(t, frame, (size_t)n) != 0) return -1;
  uint8_t resp[64]; size_t got = 0;
  if (t->recv(t, resp, sizeof resp, &got, 2000) != 0) return -1;
  uint8_t seq, rc, st; const uint8_t *d; uint16_t dl;
  if (pt_frame_parse(resp, got, &seq, &rc, &st, &d, &dl) != 0) return -1;
  if (rd && rn) { uint16_t k = dl < *rn ? dl : *rn; memcpy(rd, d, k); *rn = k; }
  return st;
}


static void refresh(void)
{
  char err[128] = {0};
  pt_transport_t *t = open_transport(err, sizeof err);
  if (!t) { SetWindowText(g_status, "Device unreachable"); logln("transport: %s", err[0] ? err : "failed"); return; }
  uint8_t d[8]; uint16_t dn = sizeof d;
  int st = do_cmd(t, CMD_USB_MODE_GET, NULL, 0, d, &dn);
  close_transport(t);
  if (st != 0 || dn < 2) { SetWindowText(g_status, "GET refused"); logln("GET status 0x%02X", st < 0 ? 0xFF : st); return; }
  int active = d[0];
  char s[128]; snprintf(s, sizeof s, "Active mode: %d - %s", active, active < MODE_COUNT ? MODE_NAMES[active] : "?");
  SetWindowText(g_status, s);
  logln("GET -> mode %d (%s)", active, active < MODE_COUNT ? MODE_NAMES[active] : "?");
}

static void set_mode(int mode)
{
  char err[128] = {0};
  pt_transport_t *t = open_transport(err, sizeof err);
  if (!t) { logln("transport: %s", err[0] ? err : "failed (is the device in J2534 mode?)"); return; }
  uint8_t req = (uint8_t)mode, d[4]; uint16_t dn = sizeof d;
  int st = do_cmd(t, CMD_USB_MODE_SET, &req, 1, d, &dn);
  close_transport(t);
  if (st != 0) { logln("SET %d refused (status 0x%02X)", mode, st < 0 ? 0xFF : st); return; }
  logln("SET -> mode %d (%s) stored. The device reboots and re-enumerates.", mode, MODE_NAMES[mode]);
  SetWindowText(g_status, "Switching mode - reconnect if needed");
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
  switch (m) {
  case WM_CREATE: {
    for (int i = 0; i < MODE_COUNT; i++)
      g_btn[i] = CreateWindow("BUTTON", MODE_NAMES[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  20, 20 + i * 44, 360, 36, h, (HMENU)(INT_PTR)(ID_BTN_BASE + i), NULL, NULL);
    CreateWindow("BUTTON", "Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
           20, 20 + MODE_COUNT * 44, 360, 30, h, (HMENU)ID_REFRESH, NULL, NULL);
    g_status = CreateWindow("STATIC", "Mode actif : ?", WS_CHILD | WS_VISIBLE,
                20, 26 + MODE_COUNT * 44 + 30, 360, 22, h, (HMENU)ID_STATUS, NULL, NULL);
    g_log = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
               20, 54 + MODE_COUNT * 44 + 30, 360, 150, h, (HMENU)ID_LOG, NULL, NULL);
    return 0;
  }
  case WM_COMMAND: {
    int id = LOWORD(w);
    if (id >= ID_BTN_BASE && id < ID_BTN_BASE + MODE_COUNT) set_mode(id - ID_BTN_BASE);
    else if (id == ID_REFRESH) refresh();
    return 0;
  }
  case WM_DESTROY: PostQuitMessage(0); return 0;
  }
  return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show)
{
  (void)hp; (void)cmd;
  WNDCLASS wc = {0};
  wc.lpfnWndProc = WndProc; wc.hInstance = hi; wc.lpszClassName = "j2534mode_gui";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  RegisterClass(&wc);
  HWND h = CreateWindow("j2534mode_gui", "OmniBox - USB Mode Selector",
             WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
             CW_USEDEFAULT, CW_USEDEFAULT, 420, 340 + MODE_COUNT * 44 - 176,
             NULL, NULL, hi, NULL);
  ShowWindow(h, show); UpdateWindow(h);
  refresh();  
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessage(&msg); }
  return 0;
}
