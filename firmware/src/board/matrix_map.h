
#ifndef MATRIX_MAP_H
#define MATRIX_MAP_H


#define PCA9555_1_ADDR  0x24  
#define PCA9555_2_ADDR  0x21  
#define ADG2128_DEPRECATED 



#define M1_SWCAN_MODE0   (1u << 0) 
#define M1_SWCAN_MODE1   (1u << 1) 
#define M1_CAN1_SWAP    (1u << 2) 
#define M1_CAN1_TERM    (1u << 3) 
#define M1_FEPS_PIN9    (1u << 4) 
#define M1_FEPS_PIN11   (1u << 5) 
#define M1_FEPS_PIN12   (1u << 6) 
#define M1_FEPS_PIN13   (1u << 7) 

#define M1_SHORT_GND    (1u << 8) 
#define M1_CAN2_CONNECT  (1u << 9) 
#define M1_CAN3_CONNECT  (1u << 10) 
#define M1_CAN4_CONNECT  (1u << 11) 
#define M1_J1850_PWM    (1u << 12) 
#define M1_GPT1_ON     (1u << 13) 
#define M1_GPT2_ON     (1u << 14) 




#define M2_K2_PIN1     (1u << 0) 
#define M2_K2_PIN3     (1u << 1) 
#define M2_K2_PIN8     (1u << 2) 
#define M2_K2_PIN9     (1u << 3) 
#define M2_K2_PIN11    (1u << 4) 
#define M2_K2_PIN12    (1u << 5) 
#define M2_K2_PIN13    (1u << 6) 
#define M2_K2_PIN15    (1u << 7) 

#define M2_SWCAN_PIN1   (1u << 8) 
#define M2_K3_PIN3     (1u << 9) 
#define M2_K3_PIN8     (1u << 10) 
#define M2_FEPS_EN     (1u << 11) 
#define M2_FF_SW1     (1u << 12) 
#define M2_FF_SW2     (1u << 13) 
#define M2_FF_SW3     (1u << 14) 



#define M2_INPUT_MASK   (M2_FF_SW1 | M2_FF_SW2 | M2_FF_SW3)



#endif 
