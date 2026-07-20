
#include "stm32h7xx_hal.h"

void usb_device_isr(void);  


void NMI_Handler(void)    { }
void HardFault_Handler(void) { while (1) { } }  
void MemManage_Handler(void) { while (1) { } }
void BusFault_Handler(void)  { while (1) { } }
void UsageFault_Handler(void) { while (1) { } }
void SVC_Handler(void)    { }
void DebugMon_Handler(void)  { }
void PendSV_Handler(void)   { }
void SysTick_Handler(void)  { HAL_IncTick(); }



extern void dcd_int_handler(uint8_t rhport);
void OTG_HS_IRQHandler(void) { dcd_int_handler(0); }


extern void can_irq(uint8_t ch);
void FDCAN1_IT0_IRQHandler(void) { can_irq(0); }
void FDCAN2_IT0_IRQHandler(void) { can_irq(1); }
void FDCAN3_IT0_IRQHandler(void) { can_irq(2); }


extern void kline_irq(uint8_t ch);
void USART2_IRQHandler(void) { kline_irq(0); }
void USART3_IRQHandler(void) { kline_irq(1); }
void UART4_IRQHandler(void) { kline_irq(2); }


void EXTI9_5_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6); }


