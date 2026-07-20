
#include "usb_mode.h"
#include "stm32h7xx_hal.h"


#define USB_MODE_RAM_MAGIC 0xB007A534u
static __attribute__((section(".noinit"))) struct {
  uint32_t magic;
  uint8_t mode;
  uint8_t mode_inv;
} g_mode_ram;

static usb_mode_t g_active_mode = USB_MODE_DEFAULT;
static uint8_t  g_reboot_pending;
static uint32_t  g_reboot_at_ms;

static int ram_valid(void)
{
  return g_mode_ram.magic == USB_MODE_RAM_MAGIC
    && g_mode_ram.mode < USB_MODE_COUNT
    && (uint8_t)~g_mode_ram.mode_inv == g_mode_ram.mode;
}

usb_mode_t usb_mode_get(void) { return g_active_mode; }

void usb_mode_force(usb_mode_t mode) { if (mode < USB_MODE_COUNT) g_active_mode = mode; }

void usb_mode_init(void)
{
  
  int cold = __HAL_RCC_GET_FLAG(RCC_FLAG_PORRST) || __HAL_RCC_GET_FLAG(RCC_FLAG_BORRST);
  __HAL_RCC_CLEAR_RESET_FLAGS();

  if (!cold && ram_valid()) {
    g_active_mode = (usb_mode_t)g_mode_ram.mode;
  } else {
    g_active_mode = USB_MODE_DEFAULT;
    (void)usb_mode_store(USB_MODE_DEFAULT);
  }
}

int usb_mode_store(usb_mode_t mode)
{
  if (mode >= USB_MODE_COUNT) return -1;
  g_mode_ram.mode   = (uint8_t)mode;
  g_mode_ram.mode_inv = (uint8_t)~(uint8_t)mode;
  g_mode_ram.magic  = USB_MODE_RAM_MAGIC;
  return 0;
}

void usb_mode_request_reboot(void)
{
  g_reboot_pending = 1;
  g_reboot_at_ms = HAL_GetTick() + 150u;
}

void usb_mode_poll_reboot(void)
{
  if (g_reboot_pending && (int32_t)(HAL_GetTick() - g_reboot_at_ms) >= 0)
    NVIC_SystemReset();
}

const char *usb_mode_name(usb_mode_t mode)
{
  switch (mode) {
  case USB_MODE_J2534: return "J2534+ELM327";
  case USB_MODE_SLOT1: return "Empty slot 1";
  case USB_MODE_SLOT2: return "Empty slot 2";
  case USB_MODE_SLOT3: return "Empty slot 3";
  default:       return "?";
  }
}
