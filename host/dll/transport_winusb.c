
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "transport_winusb.h"

#ifdef _WIN32
#include <windows.h>
#include <winusb.h>
#include <setupapi.h>
#include "../shared/pt_proto.h"


static const GUID J2534_IF_GUID =
  { 0xB7B3B4E0, 0x1209, 0x5334, { 0x9A, 0x11, 0x0E, 0x1D, 0x2C, 0x3B, 0x4A, 0x50 } };

#define WU_READ_TIMEOUT_MS 3000u  
#define WU_RBUF 4224u       

typedef struct {
  pt_transport_t base;
  HANDLE dev;
  WINUSB_INTERFACE_HANDLE winusb;
  UCHAR in_pipe, out_pipe;
  uint8_t rb[WU_RBUF];
  size_t rblen, rbpos;
} wu_ctx_t;

#define WU_RESYNC_MAX 8192u  


static HANDLE wu_open_device(char *err, size_t err_cap)
{
  HDEVINFO di = SetupDiGetClassDevs(&J2534_IF_GUID, NULL, NULL,
                   DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (di == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

  HANDLE h = INVALID_HANDLE_VALUE;
  SP_DEVICE_INTERFACE_DATA ifd; ifd.cbSize = sizeof ifd;
  if (SetupDiEnumDeviceInterfaces(di, NULL, &J2534_IF_GUID, 0, &ifd)) {
    DWORD need = 0;
    SetupDiGetDeviceInterfaceDetailA(di, &ifd, NULL, 0, &need, NULL); 
    if (need) {
      SP_DEVICE_INTERFACE_DETAIL_DATA_A *det = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)malloc(need);
      if (det) {
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        if (SetupDiGetDeviceInterfaceDetailA(di, &ifd, det, need, NULL, NULL)) {
          
          h = CreateFileA(det->DevicePath, GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
          if (h == INVALID_HANDLE_VALUE && err && err_cap)
            snprintf(err, err_cap, "CreateFile echoue (err %lu) — device deja ouvert ?",
                 (unsigned long)GetLastError());
        }
        free(det);
      } else if (err && err_cap) {
        snprintf(err, err_cap, "allocation of %lu bytes failed", (unsigned long)need);
      }
    }
  } else if (err && err_cap) {
    snprintf(err, err_cap, "no J2534 device found (interface GUID)");
  }
  SetupDiDestroyDeviceInfoList(di);
  return h;
}


static int wu_find_pipes(wu_ctx_t *c)
{
  USB_INTERFACE_DESCRIPTOR id;
  if (!WinUsb_QueryInterfaceSettings(c->winusb, 0, &id)) return -1;
  int have_in = 0, have_out = 0;
  for (UCHAR i = 0; i < id.bNumEndpoints; i++) {
    WINUSB_PIPE_INFORMATION pi;
    if (!WinUsb_QueryPipe(c->winusb, 0, i, &pi)) continue;
    if (pi.PipeType != UsbdPipeTypeBulk) continue;
    if (pi.PipeId & 0x80) { c->in_pipe = pi.PipeId; have_in = 1; }
    else         { c->out_pipe = pi.PipeId; have_out = 1; }
  }
  return (have_in && have_out) ? 0 : -1;
}


static int wu_fill(wu_ctx_t *c)
{
  ULONG got = 0;
  if (!WinUsb_ReadPipe(c->winusb, c->in_pipe, c->rb, (ULONG)sizeof c->rb, &got, NULL)) return -1;
  c->rblen = got; c->rbpos = 0;
  return got > 0 ? 0 : -1;
}


static int wu_read(wu_ctx_t *c, uint8_t *dst, size_t n)
{
  size_t off = 0;
  while (off < n) {
    if (c->rbpos >= c->rblen) { if (wu_fill(c)) return -1; }
    size_t avail = c->rblen - c->rbpos;
    size_t take = (n - off < avail) ? (n - off) : avail;
    memcpy(dst + off, c->rb + c->rbpos, take);
    c->rbpos += take; off += take;
  }
  return 0;
}

static int wu_send(pt_transport_t *t, const uint8_t *buf, size_t n)
{
  wu_ctx_t *c = (wu_ctx_t *)t;
  size_t off = 0;
  while (off < n) {             
    ULONG sent = 0;
    if (!WinUsb_WritePipe(c->winusb, c->out_pipe, (PUCHAR)(buf + off), (ULONG)(n - off), &sent, NULL))
      return -1;
    if (sent == 0) return -1;
    off += sent;
  }
  return 0;
}


static int wu_recv(pt_transport_t *t, uint8_t *buf, size_t cap, size_t *got, int timeout_ms)
{
  (void)timeout_ms;             
  wu_ctx_t *c = (wu_ctx_t *)t;
  uint8_t b; unsigned guard = 0;
  do {                   
    if (wu_read(c, &b, 1)) return -1;
    if (++guard > WU_RESYNC_MAX) return -1;
  } while (b != PT_SOF);
  if (cap < 8) return -1;
  buf[0] = b;
  if (wu_read(c, buf + 1, 2)) return -1;             
  uint16_t L = (uint16_t)(buf[1] | (buf[2] << 8));
  size_t total = (size_t)5 + L + 2;
  if (total > cap) return -1;
  if (wu_read(c, buf + 3, total - 3)) return -1;         
  *got = total;
  return 0;
}

pt_transport_t *transport_winusb_connect(char *err, size_t err_cap)
{
  HANDLE dev = wu_open_device(err, err_cap);
  if (dev == INVALID_HANDLE_VALUE) {
    if (err && err_cap && !err[0])
      snprintf(err, err_cap, "USB device open failed (board connected? WinUSB driver installed?)");
    return 0;
  }
  wu_ctx_t *c = (wu_ctx_t *)calloc(1, sizeof *c);
  if (!c) { CloseHandle(dev); return 0; }
  c->dev = dev;
  if (!WinUsb_Initialize(dev, &c->winusb)) {
    if (err && err_cap) snprintf(err, err_cap, "WinUsb_Initialize echoue (err %lu)", GetLastError());
    CloseHandle(dev); free(c); return 0;
  }
  if (wu_find_pipes(c)) {
    if (err && err_cap) snprintf(err, err_cap, "pipes bulk IN/OUT introuvables");
    WinUsb_Free(c->winusb); CloseHandle(dev); free(c); return 0;
  }
  ULONG to = WU_READ_TIMEOUT_MS;
  WinUsb_SetPipePolicy(c->winusb, c->in_pipe, PIPE_TRANSFER_TIMEOUT, sizeof to, &to);
  c->base.send = wu_send; c->base.recv = wu_recv; c->base.ctx = c;
  return &c->base;
}

void transport_winusb_close(pt_transport_t *t)
{
  if (!t) return;
  wu_ctx_t *c = (wu_ctx_t *)t;
  if (c->winusb) WinUsb_Free(c->winusb);
  if (c->dev != INVALID_HANDLE_VALUE) CloseHandle(c->dev);
  free(c);
}

#else 

pt_transport_t *transport_winusb_connect(char *err, size_t err_cap)
{
  if (err && err_cap) snprintf(err, err_cap, "WinUSB is unavailable outside Windows");
  return 0;
}
void transport_winusb_close(pt_transport_t *t) { (void)t; }

#endif
