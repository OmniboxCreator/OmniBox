
#ifndef USB_MODE_H
#define USB_MODE_H

#include <stdint.h>


typedef enum {
  USB_MODE_J2534 = 0,  
  USB_MODE_SLOT1 = 1,  
  USB_MODE_SLOT2 = 2,  
  USB_MODE_SLOT3 = 3,  
  USB_MODE_COUNT
} usb_mode_t;

#define USB_MODE_DEFAULT USB_MODE_J2534

usb_mode_t usb_mode_get(void);

void usb_mode_force(usb_mode_t mode);

void usb_mode_init(void);

int usb_mode_store(usb_mode_t mode);

void usb_mode_request_reboot(void);

void usb_mode_poll_reboot(void);

const char *usb_mode_name(usb_mode_t mode);

#endif 
