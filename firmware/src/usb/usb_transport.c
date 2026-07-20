
#include "tusb.h"
#include "../transport/protocol.h"
#include "../elm327/elm327_glue.h"
#include "usb_mode.h"

void usb_device_init(void) { tud_init(BOARD_TUD_RHPORT); }
void usb_device_task(void) { tud_task(); }


void tud_cdc_rx_cb(uint8_t itf)
{
  (void)itf;
  uint8_t buf[CFG_TUD_CDC_EP_BUFSIZE];
  uint32_t n;
  while ((n = tud_cdc_read(buf, sizeof buf)) > 0) elm327_usb_rx(buf, n);
}


void tud_vendor_rx_cb(uint8_t itf)
{
  (void)itf;
  uint8_t buf[CFG_TUD_VENDOR_EPSIZE];
  uint32_t n;
  while ((n = tud_vendor_read(buf, sizeof buf)) > 0) {
    if (usb_mode_get() != USB_MODE_J2534) continue;
    for (uint32_t i = 0; i < n; i++) transport_rx_byte(buf[i]);
  }
}


static void vendor_write_all(const uint8_t *p, uint32_t n)
{
  uint32_t sent = 0;
  while (sent < n) {
    uint32_t k = tud_vendor_write(p + sent, n - sent);
    sent += k;
    tud_vendor_write_flush();
    if (k == 0) tud_task();    
  }
}


void proto_reply(uint8_t seq, uint8_t cmd, uint8_t status,
         const uint8_t *data, uint16_t n)
{
  uint8_t rcmd = (uint8_t)(cmd | CMD_RESP_FLAG);
  uint16_t plen = (uint16_t)(1 + n);      
  uint8_t hdr[5] = { PROTO_SOF, (uint8_t)(plen & 0xFF), (uint8_t)(plen >> 8), seq, rcmd };

  uint8_t pre[3] = { seq, rcmd, status };
  uint16_t crc = proto_crc16(pre, 3);     
  crc = proto_crc16_upd(crc, data, n);     
  uint8_t crcb[2] = { (uint8_t)(crc & 0xFF), (uint8_t)(crc >> 8) };

  vendor_write_all(hdr, sizeof(hdr));
  vendor_write_all(&status, 1);
  if (n) vendor_write_all(data, n);
  vendor_write_all(crcb, 2);
  tud_vendor_write_flush();
}
