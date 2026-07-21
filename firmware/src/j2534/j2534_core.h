
#ifndef J2534_CORE_H
#define J2534_CORE_H

#include <stdint.h>
#include "j2534_defs.h"
#include "j2534_filter.h"

#define J2534_MAX_CHANNELS 8
#define J2534_RXQ_DEPTH   8
#define J2534_MAX_FILTERS  8
#define J2534_MAX_PERIODIC 4
#define J2534_PERIODIC_MAX_DATA 32  


#define J2534_RX_INLINE   64
#define J2534_RX_LARGE_POOL 8

typedef struct {
  uint32_t protocol_id, rx_status, tx_flags, timestamp, extra_data_index, data_size;
  int16_t large;     
  uint8_t inl[J2534_RX_INLINE];
} j2534_rx_slot_t;


typedef struct {
  uint8_t in_use;
  uint32_t interval_ms;
  uint32_t last_ms;    
  uint32_t tx_flags;
  uint16_t len;
  uint8_t data[J2534_PERIODIC_MAX_DATA];
} j2534_periodic_t;

typedef struct {
  uint8_t in_use;
  uint16_t protocol_id;   
  uint32_t flags;
  uint32_t baudrate;
  uint8_t can_phys;    
  uint8_t kline_phys;   
  uint8_t cfg_bs;     
  uint8_t cfg_stmin;    
  uint8_t loopback;    
  uint8_t obd_pin;
  uint8_t termination;
  uint8_t can_swap;
  uint8_t can_fd;
  uint8_t swcan_mode;
  uint32_t data_baudrate;
  
  j2534_rx_slot_t rxq[J2534_RXQ_DEPTH];
  uint16_t rx_head, rx_tail;
  uint8_t rx_overflow;   
  
  j2534_filter_t filters[J2534_MAX_FILTERS];
  
  j2534_periodic_t periodics[J2534_MAX_PERIODIC];
} j2534_channel_t;

void j2534_init(void);
void j2534_service(void);   


uint8_t j2534_open(uint32_t *device_id);
uint8_t j2534_close(void);
uint8_t j2534_connect(uint16_t protocol_id, uint32_t flags, uint32_t baud, uint32_t *channel_id);
uint8_t j2534_disconnect(uint32_t channel_id);
uint8_t j2534_read_msgs(uint32_t channel_id, j2534_msg_t *out, uint32_t *count, uint32_t timeout_ms);
uint8_t j2534_write_msgs(uint32_t channel_id, const j2534_msg_t *in, uint32_t count, uint32_t timeout_ms);


uint8_t j2534_start_filter(uint32_t channel_id, uint8_t type,
	              uint32_t mask_id, uint32_t pattern_id, uint32_t fc_id,
	              uint32_t *filter_id);
uint8_t j2534_start_filter_msg(uint32_t channel_id, uint8_t type,
                  const j2534_msg_t *mask, const j2534_msg_t *pattern,
                  const j2534_msg_t *flow, uint32_t *filter_id);
uint8_t j2534_stop_filter(uint32_t channel_id, uint32_t filter_id);


uint8_t j2534_start_periodic(uint32_t channel_id, const uint8_t *data, uint16_t len,
               uint32_t tx_flags, uint32_t interval_ms, uint32_t *msg_id);
uint8_t j2534_stop_periodic(uint32_t channel_id, uint32_t msg_id);
uint8_t j2534_ioctl(uint32_t channel_id, uint32_t ioctl_id,
          const uint8_t *in, uint16_t in_len, uint8_t *out, uint16_t *out_len);
uint8_t j2534_set_prog_voltage(uint32_t pin, uint32_t millivolts);
uint8_t j2534_read_vbatt(uint32_t *millivolts);


void j2534_rx_push(uint32_t channel_id, const j2534_msg_t *msg);

#endif 
