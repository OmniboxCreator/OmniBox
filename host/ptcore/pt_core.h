
#ifndef PT_CORE_H
#define PT_CORE_H

#include "../shared/j2534_abi.h"
#include "../shared/pt_transport.h"

typedef struct {
  pt_transport_t *t;
  uint8_t seq;
  char   last_error[96];
  uint8_t pbuf[16 + 24 + J2534_DATA_MAX];  
  uint8_t txbuf[16 + 24 + J2534_DATA_MAX]; 
  uint8_t rxbuf[16 + 24 + J2534_DATA_MAX]; 
} pt_handle_t;

void pt_init(pt_handle_t *h, pt_transport_t *t);
const char *pt_last_error(pt_handle_t *h);

int32_t pt_open(pt_handle_t *h, const char *name, uint32_t *device_id);
int32_t pt_close(pt_handle_t *h, uint32_t device_id);
int32_t pt_connect(pt_handle_t *h, uint32_t device_id, uint32_t protocol_id,
          uint32_t flags, uint32_t baud, uint32_t *channel_id);
int32_t pt_disconnect(pt_handle_t *h, uint32_t channel_id);
int32_t pt_read_msgs(pt_handle_t *h, uint32_t channel_id, PASSTHRU_MSG *msgs,
           uint32_t *num, uint32_t timeout_ms);
int32_t pt_write_msgs(pt_handle_t *h, uint32_t channel_id, const PASSTHRU_MSG *msgs,
           uint32_t *num, uint32_t timeout_ms);
int32_t pt_start_filter(pt_handle_t *h, uint32_t channel_id, uint32_t type,
            const PASSTHRU_MSG *mask, const PASSTHRU_MSG *pattern,
            const PASSTHRU_MSG *flow, uint32_t *filter_id);
int32_t pt_stop_filter(pt_handle_t *h, uint32_t channel_id, uint32_t filter_id);
int32_t pt_set_prog_voltage(pt_handle_t *h, uint32_t pin, uint32_t millivolts);
int32_t pt_read_vbatt(pt_handle_t *h, uint32_t *millivolts);

int32_t pt_read_version(pt_handle_t *h, char *fw, char *dll, char *api);


int32_t pt_set_config(pt_handle_t *h, uint32_t channel_id, const SCONFIG *cfg, uint32_t n);
int32_t pt_get_config(pt_handle_t *h, uint32_t channel_id, SCONFIG *cfg, uint32_t n);
int32_t pt_ioctl_clear(pt_handle_t *h, uint32_t channel_id, uint32_t ioctl_id);


int32_t pt_start_periodic(pt_handle_t *h, uint32_t channel_id, const PASSTHRU_MSG *msg,
             uint32_t interval_ms, uint32_t *msg_id);
int32_t pt_stop_periodic(pt_handle_t *h, uint32_t channel_id, uint32_t msg_id);

#endif 
