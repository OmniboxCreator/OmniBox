
#ifndef MCP2518FD_H
#define MCP2518FD_H

#include <stdint.h>

int mcp_init(uint8_t idx, uint32_t nominal_baud);  
int mcp_tx(uint8_t idx, uint32_t id, const uint8_t *data, uint8_t len, uint8_t ext);
int mcp_read(uint8_t idx, uint32_t *id, uint8_t *data, uint8_t *len, uint8_t *ext); 

#endif 
