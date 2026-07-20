
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "j2534.h"
#include "transport_select.h"
#include "../ptcore/pt_core.h"

static pt_handle_t  g_h;
static pt_transport_t *g_t;
static int      g_open;


static FILE *g_log;
static int  g_log_init;

static void dlog(const char *fmt, ...)
{
  if (!g_log_init) {
    g_log_init = 1;
    const char *p = getenv("J2534_DLL_LOG");
    if (p && p[0]) g_log = fopen(p, "a");
  }
  if (!g_log) return;
  va_list ap; va_start(ap, fmt);
  vfprintf(g_log, fmt, ap);
  va_end(ap);
  fputc('\n', g_log);
  fflush(g_log);
}

#define NEED_OPEN() do { if (!g_open) { dlog(" ! call without an open device"); return J2534_ERR_DEVICE_NOT_OPEN; } } while (0)

long WINAPI PassThruOpen(void *pName, unsigned long *pDeviceID)
{
  if (g_open) { if (pDeviceID) *pDeviceID = 1; return J2534_STATUS_NOERROR; }
  char err[128] = "";
  g_t = pt_open_transport(err, sizeof err);
  if (!g_t) return J2534_ERR_DEVICE_NOT_CONNECTED;
  pt_init(&g_h, g_t);
  if (err[0]) { }
  uint32_t dev = 0;
  long st = pt_open(&g_h, (const char *)pName, &dev);
  if (st != J2534_STATUS_NOERROR) { pt_close_transport(g_t); g_t = 0; dlog("PassThruOpen -> %ld (transport)", st); return st; }
  g_open = 1;
  if (pDeviceID) *pDeviceID = dev;
  dlog("PassThruOpen -> 0 (devid=%u)", dev);
  return J2534_STATUS_NOERROR;
}

long WINAPI PassThruClose(unsigned long DeviceID)
{
  dlog("PassThruClose(devid=%lu)", DeviceID);
  NEED_OPEN();
  long st = pt_close(&g_h, (uint32_t)DeviceID);
  pt_close_transport(g_t); g_t = 0; g_open = 0;
  return st;
}

long WINAPI PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID,
              unsigned long Flags, unsigned long BaudRate, unsigned long *pChannelID)
{
  NEED_OPEN();
  dlog("PassThruConnect(proto=%lu flags=0x%lX baud=%lu)", ProtocolID, Flags, BaudRate);
  uint32_t ch = 0;
  long st = pt_connect(&g_h, (uint32_t)DeviceID, (uint32_t)ProtocolID, (uint32_t)Flags, (uint32_t)BaudRate, &ch);
  if (pChannelID) *pChannelID = ch;
  dlog(" PassThruConnect -> %ld (ch=%u)", st, ch);
  return st;
}

long WINAPI PassThruDisconnect(unsigned long ChannelID)
{
  NEED_OPEN();
  return pt_disconnect(&g_h, (uint32_t)ChannelID);
}

long WINAPI PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg,
               unsigned long *pNumMsgs, unsigned long Timeout)
{
  NEED_OPEN();
  uint32_t num = pNumMsgs ? (uint32_t)*pNumMsgs : 0;
  long st = pt_read_msgs(&g_h, (uint32_t)ChannelID, pMsg, &num, (uint32_t)Timeout);
  if (pNumMsgs) *pNumMsgs = num;
  return st;
}

long WINAPI PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg,
               unsigned long *pNumMsgs, unsigned long Timeout)
{
  NEED_OPEN();
  uint32_t num = pNumMsgs ? (uint32_t)*pNumMsgs : 0;
  long st = pt_write_msgs(&g_h, (uint32_t)ChannelID, pMsg, &num, (uint32_t)Timeout);
  if (pNumMsgs) *pNumMsgs = num;
  return st;
}

long WINAPI PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG *pMsg,
                   unsigned long *pMsgID, unsigned long TimeInterval)
{
  NEED_OPEN();
  uint32_t mid = 0;
  long st = pt_start_periodic(&g_h, (uint32_t)ChannelID, pMsg, (uint32_t)TimeInterval, &mid);
  if (pMsgID) *pMsgID = mid;
  return st;
}

long WINAPI PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
  NEED_OPEN();
  return pt_stop_periodic(&g_h, (uint32_t)ChannelID, (uint32_t)MsgID);
}

long WINAPI PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType,
                  PASSTHRU_MSG *pMaskMsg, PASSTHRU_MSG *pPatternMsg,
                  PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
  NEED_OPEN();
  uint32_t fid = 0;
  long st = pt_start_filter(&g_h, (uint32_t)ChannelID, (uint32_t)FilterType,
               pMaskMsg, pPatternMsg, pFlowControlMsg, &fid);
  if (pMsgID) *pMsgID = fid;
  return st;
}

long WINAPI PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
  NEED_OPEN();
  return pt_stop_filter(&g_h, (uint32_t)ChannelID, (uint32_t)MsgID);
}

long WINAPI PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long PinNumber,
                     unsigned long Voltage)
{
  (void)DeviceID;
  NEED_OPEN();
  return pt_set_prog_voltage(&g_h, (uint32_t)PinNumber, (uint32_t)Voltage);
}

long WINAPI PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion,
                char *pDllVersion, char *pApiVersion)
{
  (void)DeviceID;
  NEED_OPEN();
  long st = pt_read_version(&g_h, pFirmwareVersion, pDllVersion, pApiVersion);
  
  const char *e;
  if (pFirmwareVersion && (e = getenv("J2534_FW"))   && e[0]) snprintf(pFirmwareVersion, 80, "%s", e);
  if (pDllVersion   && (e = getenv("J2534_DLLVER")) && e[0]) snprintf(pDllVersion, 80, "%s", e);
  if (pApiVersion   && (e = getenv("J2534_API"))  && e[0]) snprintf(pApiVersion, 80, "%s", e);
  dlog("PassThruReadVersion -> %ld fw=\"%s\" dll=\"%s\" api=\"%s\"", st,
     pFirmwareVersion ? pFirmwareVersion : "", pDllVersion ? pDllVersion : "",
     pApiVersion ? pApiVersion : "");
  return st;
}

long WINAPI PassThruGetLastError(char *pErrorDescription)
{
  if (pErrorDescription)
    snprintf(pErrorDescription, 80, "%s", g_open ? pt_last_error(&g_h) : "device not open");
  dlog("PassThruGetLastError -> \"%s\"", pErrorDescription ? pErrorDescription : "");
  return J2534_STATUS_NOERROR;
}

long WINAPI PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID,
             void *pInput, void *pOutput)
{
  dlog("PassThruIoctl(ch=%lu id=0x%lX)", ChannelID, IoctlID);
  NEED_OPEN();
  switch (IoctlID) {
  case J2534_SET_CONFIG: {
    SCONFIG_LIST *l = (SCONFIG_LIST *)pInput;
    if (!l) return J2534_ERR_NULL_PARAMETER;
    return pt_set_config(&g_h, (uint32_t)ChannelID, l->ConfigPtr, l->NumOfParams); }
  case J2534_GET_CONFIG: {
    SCONFIG_LIST *l = (SCONFIG_LIST *)pInput;
    if (!l) return J2534_ERR_NULL_PARAMETER;
    return pt_get_config(&g_h, (uint32_t)ChannelID, l->ConfigPtr, l->NumOfParams); }
  case J2534_CLEAR_RX_BUFFER: case J2534_CLEAR_TX_BUFFER:
  case J2534_CLEAR_MSG_FILTERS: case J2534_CLEAR_PERIODIC_MSGS:
    return pt_ioctl_clear(&g_h, (uint32_t)ChannelID, (uint32_t)IoctlID);
  case J2534_READ_VBATT: {
    uint32_t mv = 0; long st = pt_read_vbatt(&g_h, &mv);
    if (pOutput) *(unsigned long *)pOutput = mv;
    return st; }
  default:
    dlog(" ! Ioctl id=0x%lX is not supported", IoctlID);
    return J2534_ERR_NOT_SUPPORTED;
  }
}
