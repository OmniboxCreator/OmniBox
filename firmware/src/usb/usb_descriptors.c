
#include "tusb.h"
#include "usb_mode.h"
#include <string.h>

#define OMNIBOX_VID 0x1209    
#define OMNIBOX_PID_J2534 0x5334
#define OMNIBOX_PID_SLOT1 0x5335
#define OMNIBOX_PID_SLOT2 0x5336
#define OMNIBOX_PID_SLOT3 0x5337
#define OMNIBOX_BCD 0x0201    

enum { ITF_VENDOR = 0, ITF_CDC, ITF_CDC_DATA, ITF_COUNT };
#define EPNUM_VENDOR_OUT 0x01
#define EPNUM_VENDOR_IN 0x81
#define EPNUM_CDC_OUT  0x02
#define EPNUM_CDC_IN   0x82
#define EPNUM_CDC_NOTIF 0x83

#define J2534_CFG_LEN (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN + TUD_CDC_DESC_LEN)
#define SLOT_CFG_LEN (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

static const tusb_desc_device_t desc_device_j2534 = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = OMNIBOX_BCD,
  .bDeviceClass = TUSB_CLASS_MISC,
  .bDeviceSubClass = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = OMNIBOX_VID,
  .idProduct = OMNIBOX_PID_J2534,
  .bcdDevice = 0x0100,
  .iManufacturer = 0x01,
  .iProduct = 0x02,
  .iSerialNumber = 0x03,
  .bNumConfigurations = 0x01,
};

static const uint8_t desc_cfg_j2534[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, J2534_CFG_LEN, 0x00, 100),
  TUD_VENDOR_DESCRIPTOR(ITF_VENDOR, 0, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64),
  TUD_CDC_DESCRIPTOR(ITF_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

static const char *const strings_j2534[] = {
  (const char[]){ 0x09, 0x04 },
  "OmniBox",
  "OmniBox J2534 PassThru",
  "0001",
  "OmniBox ELM327 Serial",
};

#define DEFINE_SLOT(NAME, PID, PRODUCT) \
static const tusb_desc_device_t desc_device_##NAME = { \
  .bLength = sizeof(tusb_desc_device_t), \
  .bDescriptorType = TUSB_DESC_DEVICE, \
  .bcdUSB = OMNIBOX_BCD, \
  .bDeviceClass = 0xFF, \
  .bDeviceSubClass = 0x00, \
  .bDeviceProtocol = 0x00, \
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE, \
  .idVendor = OMNIBOX_VID, \
  .idProduct = (PID), \
  .bcdDevice = 0x0100, \
  .iManufacturer = 0x01, \
  .iProduct = 0x02, \
  .iSerialNumber = 0x03, \
  .bNumConfigurations = 0x01, \
}; \
static const uint8_t desc_cfg_##NAME[] = { \
  TUD_CONFIG_DESCRIPTOR(1, 1, 0, SLOT_CFG_LEN, 0x00, 100), \
  TUD_VENDOR_DESCRIPTOR(0, 0, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64), \
}; \
static const char *const strings_##NAME[] = { \
  (const char[]){ 0x09, 0x04 }, \
  "OmniBox", \
  (PRODUCT), \
  "0001", \
}

DEFINE_SLOT(slot1, OMNIBOX_PID_SLOT1, "OmniBox Empty Slot 1");
DEFINE_SLOT(slot2, OMNIBOX_PID_SLOT2, "OmniBox Empty Slot 2");
DEFINE_SLOT(slot3, OMNIBOX_PID_SLOT3, "OmniBox Empty Slot 3");

typedef struct {
  const tusb_desc_device_t *device;
  const uint8_t      *config;
  const char *const    *strings;
  uint8_t          string_count;
  uint8_t          exposes_wcid;
} usb_desc_set_t;

static usb_desc_set_t current_desc_set(void)
{
  switch (usb_mode_get()) {
  case USB_MODE_SLOT1:
    return (usb_desc_set_t){ &desc_device_slot1, desc_cfg_slot1,
                 strings_slot1, TU_ARRAY_SIZE(strings_slot1), 0 };
  case USB_MODE_SLOT2:
    return (usb_desc_set_t){ &desc_device_slot2, desc_cfg_slot2,
                 strings_slot2, TU_ARRAY_SIZE(strings_slot2), 0 };
  case USB_MODE_SLOT3:
    return (usb_desc_set_t){ &desc_device_slot3, desc_cfg_slot3,
                 strings_slot3, TU_ARRAY_SIZE(strings_slot3), 0 };
  case USB_MODE_J2534:
  default:
    return (usb_desc_set_t){ &desc_device_j2534, desc_cfg_j2534,
                 strings_j2534, TU_ARRAY_SIZE(strings_j2534), 1 };
  }
}

uint8_t const *tud_descriptor_device_cb(void)
{
  return (uint8_t const *)current_desc_set().device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index;
  return current_desc_set().config;
}

static uint16_t desc_str[32];
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;
  usb_desc_set_t s = current_desc_set();
  uint8_t chr_count;

  if (index == 0) {
    desc_str[1] = 0x0409;
    chr_count = 1;
  } else {
    if (index >= s.string_count) return NULL;
    const char *str = s.strings[index];
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 31) chr_count = 31;
    for (uint8_t i = 0; i < chr_count; i++) desc_str[1 + i] = str[i];
  }

  desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return desc_str;
}


#define VENDOR_REQUEST_MICROSOFT 0x01
#define MS_OS_20_DESC_LEN 0xB2

static const uint8_t desc_bos[] = {
  TUD_BOS_DESCRIPTOR(TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN, 1),
  TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MICROSOFT),
};

uint8_t const *tud_descriptor_bos_cb(void)
{
  return current_desc_set().exposes_wcid ? desc_bos : NULL;
}

static const uint8_t desc_ms_os_20[] = {
  U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
  U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0,
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_VENDOR, 0,
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
  'W', 'I', 'N', 'U', 'S', 'B', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
  U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A),
  'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0,
  'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
  U16_TO_U8S_LE(0x0050),
  '{', 0, 'B', 0, '7', 0, 'B', 0, '3', 0, 'B', 0, '4', 0, 'E', 0, '0', 0, '-', 0,
  '1', 0, '2', 0, '0', 0, '9', 0, '-', 0, '5', 0, '3', 0, '3', 0, '4', 0, '-', 0,
  '9', 0, 'A', 0, '1', 0, '1', 0, '-', 0, '0', 0, 'E', 0, '1', 0, 'D', 0, '2', 0,
  'C', 0, '3', 0, 'B', 0, '4', 0, 'A', 0, '5', 0, '0', 0, '}', 0, 0, 0, 0, 0,
};
TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN, "bad MS OS 2.0 descriptor size");

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
  if (stage != CONTROL_STAGE_SETUP) return true;

  if (current_desc_set().exposes_wcid &&
    request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
    request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
    request->bRequest == VENDOR_REQUEST_MICROSOFT &&
    request->wIndex == 7) {
    return tud_control_xfer(rhport, request, (void *)(uintptr_t)desc_ms_os_20, MS_OS_20_DESC_LEN);
  }

  return false;
}
