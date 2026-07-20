
#include "stm32h7xx_hal.h"
#include "mcp2518fd.h"
#include "../board/board.h"


volatile uint32_t g_mcp_osc_dbg[3] = { 0, 0, 0 };


#define MCP_INSTR_RESET 0x0u
#define MCP_INSTR_READ 0x3u
#define MCP_INSTR_WRITE 0x2u


#define MCP_OSC    0xE00u
#define MCP_C1CON   0x000u
#define MCP_C1NBTCFG  0x004u
#define MCP_C1DBTCFG  0x008u
#define MCP_C1TXQCON  0x050u
#define MCP_C1TXQUA  0x058u
#define MCP_C1FIFOCON1 0x05Cu   
#define MCP_C1FIFOSTA1 0x060u
#define MCP_C1FIFOUA1 0x064u
#define MCP_C1FLTCON0 0x1D0u
#define MCP_C1FLTOBJ0 0x1F0u
#define MCP_C1MASK0  0x1F4u
#define MCP_RAM_START 0x400u


#define OSC_OSCRDY   (1u << 10)
#define CON_TXQEN   (1u << 19)
#define CON_ISOCRCEN  (1u << 5)
#define CON_REQOP_POS 24u
#define CON_OPMOD_POS 21u
#define REQOP_CLASSIC 6u      
#define FIFO_UINC   (1u << 8)
#define FIFO_TXREQ   (1u << 9)
#define FIFOSTA_NOTEMPTY (1u << 0)  

typedef struct { GPIO_TypeDef *port; uint16_t pin; } mcp_cs_t;
static const mcp_cs_t MCP_CS[3] = {
  { GPIOC, GPIO_PIN_4 },  
  { GPIOC, GPIO_PIN_5 },  
  { GPIOB, GPIO_PIN_1 },  
};

static inline void cs_lo(uint8_t i) { HAL_GPIO_WritePin(MCP_CS[i].port, MCP_CS[i].pin, GPIO_PIN_RESET); }
static inline void cs_hi(uint8_t i) { HAL_GPIO_WritePin(MCP_CS[i].port, MCP_CS[i].pin, GPIO_PIN_SET); }

static void mcp_reset(uint8_t i)
{
  uint8_t tx[2] = { 0x00u, 0x00u };     
  cs_lo(i); board_spi_transfer(tx, 0, 2); cs_hi(i);
}

static void mcp_write32(uint8_t i, uint16_t addr, uint32_t v)
{
  uint16_t cmd = (uint16_t)((MCP_INSTR_WRITE << 12) | (addr & 0x0FFFu));
  uint8_t tx[6] = { (uint8_t)(cmd >> 8), (uint8_t)cmd,
           (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
  cs_lo(i); board_spi_transfer(tx, 0, 6); cs_hi(i);
}

static uint32_t mcp_read32(uint8_t i, uint16_t addr)
{
  uint16_t cmd = (uint16_t)((MCP_INSTR_READ << 12) | (addr & 0x0FFFu));
  uint8_t tx[6] = { (uint8_t)(cmd >> 8), (uint8_t)cmd, 0, 0, 0, 0 };
  uint8_t rx[6];
  cs_lo(i); board_spi_transfer(tx, rx, 6); cs_hi(i);
  return (uint32_t)rx[2] | ((uint32_t)rx[3] << 8) | ((uint32_t)rx[4] << 16) | ((uint32_t)rx[5] << 24);
}

static void mcp_write_ram(uint8_t i, uint16_t addr, const uint8_t *d, uint8_t n)
{
  uint8_t tx[2 + 16];
  if (n > 16) n = 16;
  uint16_t cmd = (uint16_t)((MCP_INSTR_WRITE << 12) | (addr & 0x0FFFu));
  tx[0] = (uint8_t)(cmd >> 8); tx[1] = (uint8_t)cmd;
  for (uint8_t k = 0; k < n; k++) tx[2 + k] = d[k];
  cs_lo(i); board_spi_transfer(tx, 0, (size_t)(2 + n)); cs_hi(i);
}

static void mcp_read_ram(uint8_t i, uint16_t addr, uint8_t *d, uint8_t n)
{
  uint8_t tx[2 + 16], rx[2 + 16];
  if (n > 16) n = 16;
  uint16_t cmd = (uint16_t)((MCP_INSTR_READ << 12) | (addr & 0x0FFFu));
  tx[0] = (uint8_t)(cmd >> 8); tx[1] = (uint8_t)cmd;
  for (uint8_t k = 0; k < n; k++) tx[2 + k] = 0;
  cs_lo(i); board_spi_transfer(tx, rx, (size_t)(2 + n)); cs_hi(i);
  for (uint8_t k = 0; k < n; k++) d[k] = rx[2 + k];
}

int mcp_init(uint8_t i, uint32_t nominal_baud)
{
  if (i > 2) return -1;
  if (!nominal_baud) nominal_baud = 500000u;
  cs_hi(i);
  mcp_reset(i);
  for (volatile uint32_t d = 0; d < 200000u; d++) { }    

  mcp_write32(i, MCP_OSC, 0x00000000u);          
  
  uint32_t osc = 0, t0 = board_micros();
  do { osc = mcp_read32(i, MCP_OSC); } while (!(osc & OSC_OSCRDY) && (board_micros() - t0) < 5000u);
  g_mcp_osc_dbg[i] = osc;                  
  if (!(osc & OSC_OSCRDY)) return -1;            

  
  uint32_t div = 40000000u / (80u * nominal_baud);
  uint32_t brp = (div > 0u) ? (div - 1u) : 0u;
  mcp_write32(i, MCP_C1NBTCFG, (brp << 24) | (62u << 16) | (15u << 8) | 15u);
  mcp_write32(i, MCP_C1DBTCFG, (brp << 24) | (14u << 16) | (3u << 8) | 3u);  

  mcp_write32(i, MCP_C1TXQCON,  (1u << 7) | (7u << 19));  
  mcp_write32(i, MCP_C1FIFOCON1, (15u << 19));       
  mcp_write32(i, MCP_C1FLTOBJ0, 0u);
  mcp_write32(i, MCP_C1MASK0,  0u);            
  mcp_write32(i, MCP_C1FLTCON0, 0x00000081u);       

  mcp_write32(i, MCP_C1CON, CON_TXQEN | CON_ISOCRCEN | (REQOP_CLASSIC << CON_REQOP_POS));
  t0 = board_micros();
  while ((((mcp_read32(i, MCP_C1CON) >> CON_OPMOD_POS) & 0x7u) != REQOP_CLASSIC)
      && (board_micros() - t0) < 5000u) { }
  return 0;
}

int mcp_tx(uint8_t i, uint32_t id, const uint8_t *data, uint8_t len, uint8_t ext)
{
  if (i > 2) return -1;
  if (len > 8) len = 8;
  uint32_t ua = mcp_read32(i, MCP_C1TXQUA);
  uint8_t obj[8 + 8] = {0};
  uint32_t t0 = ext ? (id & 0x1FFFFFFFu) : (id & 0x7FFu);
  uint32_t t1 = (uint32_t)len | (ext ? (1u << 4) : 0u);  
  obj[0] = (uint8_t)t0; obj[1] = (uint8_t)(t0 >> 8); obj[2] = (uint8_t)(t0 >> 16); obj[3] = (uint8_t)(t0 >> 24);
  obj[4] = (uint8_t)t1; obj[5] = (uint8_t)(t1 >> 8); obj[6] = (uint8_t)(t1 >> 16); obj[7] = (uint8_t)(t1 >> 24);
  for (uint8_t k = 0; k < len; k++) obj[8 + k] = data[k];
  uint8_t total = (uint8_t)(8u + ((len + 3u) & ~3u));   
  mcp_write_ram(i, (uint16_t)(MCP_RAM_START + ua), obj, total);
  mcp_write32(i, MCP_C1TXQCON, mcp_read32(i, MCP_C1TXQCON) | FIFO_UINC | FIFO_TXREQ);
  return 0;
}

int mcp_read(uint8_t i, uint32_t *id, uint8_t *data, uint8_t *len, uint8_t *ext)
{
  if (i > 2) return 0;
  if (!(mcp_read32(i, MCP_C1FIFOSTA1) & FIFOSTA_NOTEMPTY)) return 0;
  uint32_t ua = mcp_read32(i, MCP_C1FIFOUA1);
  uint8_t obj[8 + 8];
  mcp_read_ram(i, (uint16_t)(MCP_RAM_START + ua), obj, sizeof obj);
  uint32_t r0 = (uint32_t)obj[0] | ((uint32_t)obj[1] << 8) | ((uint32_t)obj[2] << 16) | ((uint32_t)obj[3] << 24);
  uint32_t r1 = (uint32_t)obj[4] | ((uint32_t)obj[5] << 8) | ((uint32_t)obj[6] << 16) | ((uint32_t)obj[7] << 24);
  uint8_t e = (r1 & (1u << 4)) ? 1u : 0u;
  if (ext) *ext = e;
  if (id) *id = e ? (r0 & 0x1FFFFFFFu) : (r0 & 0x7FFu);
  uint8_t l = (uint8_t)(r1 & 0x0Fu); if (l > 8u) l = 8u;
  if (len) *len = l;
  if (data) for (uint8_t k = 0; k < l; k++) data[k] = obj[8 + k];
  mcp_write32(i, MCP_C1FIFOCON1, mcp_read32(i, MCP_C1FIFOCON1) | FIFO_UINC);  
  return 1;
}
