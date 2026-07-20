
#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define CFG_TUSB_MCU       OPT_MCU_STM32H7
#define CFG_TUSB_OS        OPT_OS_NONE
#define BOARD_TUD_RHPORT     0    
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN    __attribute__ ((aligned(4)))
#endif

#define CFG_TUD_ENDPOINT0_SIZE  64


#define CFG_TUD_CDC        1
#define CFG_TUD_MSC        0
#define CFG_TUD_HID        0
#define CFG_TUD_HID_EP_BUFSIZE  64
#define CFG_TUD_VENDOR      1

#define CFG_TUD_VENDOR_RX_BUFSIZE 512
#define CFG_TUD_VENDOR_TX_BUFSIZE 512
#define CFG_TUD_VENDOR_EPSIZE   64

#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  256
#define CFG_TUD_CDC_EP_BUFSIZE  64

#endif 
