
#include "../../firmware/src/usb/usb_mode.h"

static usb_mode_t g_mode = USB_MODE_DEFAULT;

usb_mode_t usb_mode_get(void)     { return g_mode; }
void    usb_mode_init(void)     { g_mode = USB_MODE_DEFAULT; }
int    usb_mode_store(usb_mode_t m){ if (m >= USB_MODE_COUNT) return -1; g_mode = m; return 0; }
void    usb_mode_request_reboot(void) { }
void    usb_mode_poll_reboot(void)  { }

const char *usb_mode_name(usb_mode_t m)
{
  switch (m) {
  case USB_MODE_J2534: return "J2534+ELM327";
  case USB_MODE_SLOT1: return "Empty slot 1";
  case USB_MODE_SLOT2: return "Empty slot 2";
  case USB_MODE_SLOT3: return "Empty slot 3";
  default:       return "?";
  }
}
