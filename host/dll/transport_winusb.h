
#ifndef TRANSPORT_WINUSB_H
#define TRANSPORT_WINUSB_H

#include <stddef.h>
#include "../shared/pt_transport.h"

pt_transport_t *transport_winusb_connect(char *err, size_t err_cap);
void      transport_winusb_close(pt_transport_t *t);

#endif 
