
#ifndef TRANSPORT_SELECT_H
#define TRANSPORT_SELECT_H

#include <stddef.h>
#include "../shared/pt_transport.h"

pt_transport_t *pt_open_transport(char *err, size_t err_cap);
void      pt_close_transport(pt_transport_t *t);

#endif 
