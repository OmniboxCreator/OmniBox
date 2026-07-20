
#include "stm32h7xx_hal.h"
#include "board/board.h"
#include "j2534/j2534_core.h"
#include "elm327/elm327_glue.h"
#include "transport/protocol.h"
#include "usb/usb_mode.h"

void usb_device_init(void);  
void usb_device_task(void);

int main(void)
{
  HAL_Init();       
  board_clock_init();   
  board_gpio_init();    
  board_periph_init();   

  j2534_init();      
  usb_mode_init();     
  if (board_recovery_strapped())   
    usb_mode_force(USB_MODE_J2534);
  usb_device_init();    
  elm327_usb_init();    

  for (;;) {
    usb_device_task();  
    transport_poll();  
    j2534_service();   
    elm327_usb_service();
    usb_mode_poll_reboot();
  }
}
