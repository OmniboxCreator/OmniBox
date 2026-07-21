
#ifndef VDEVICE_H
#define VDEVICE_H

#include "../shared/pt_transport.h"

#include <stddef.h>
#include <stdint.h>


pt_transport_t *vdevice_init(void);


void vdevice_reset(void);


int vdevice_handle_frame(const uint8_t *req, size_t n, uint8_t *resp, size_t cap, size_t *resp_len);


int mock_bus_load_dump(const char *path, uint32_t base);
void mock_bus_queue_can(uint32_t id, const uint8_t *data, uint8_t len);

#endif 
