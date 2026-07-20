
#ifndef VLOG_H
#define VLOG_H

#include <stdint.h>
#include <stddef.h>

void vlog_set(int on);                 
int vlog_enabled(void);
void vlog_request(const uint8_t *frame, size_t n);   
void vlog_response(const uint8_t *frame, size_t n);   
void vlog_event(const char *msg);            

#endif 
