
#ifndef ELM327_GLUE_H
#define ELM327_GLUE_H

#include <stdint.h>

void elm327_usb_init(void);            
void elm327_usb_service(void);          
void elm327_usb_rx(const uint8_t *d, uint32_t n); 

#endif 
