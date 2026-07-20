
#include "stm32h7xx_hal.h"
#include "drivers.h"
#include "mcp2518fd.h"
#include "../board/board.h"
#include "../board/matrix_map.h"


static uint16_t s_pca1;  
static uint16_t s_pca2;  


static int pca_write_outputs(uint8_t addr, uint16_t val)
{
  uint8_t buf[3] = { 0x02, (uint8_t)(val & 0xFF), (uint8_t)(val >> 8) };
  return board_i2c_write(addr, buf, sizeof buf);
}


static int pca_write_config(uint8_t addr, uint16_t in_mask)
{
  uint8_t buf[3] = { 0x06, (uint8_t)(in_mask & 0xFF), (uint8_t)(in_mask >> 8) };
  return board_i2c_write(addr, buf, sizeof buf);
}

void matrix_init(void)
{
  s_pca1 = 0;
  s_pca2 = 0;
  
  pca_write_config(PCA9555_1_ADDR, 0x0000);
  pca_write_outputs(PCA9555_1_ADDR, s_pca1);
  
  pca_write_config(PCA9555_2_ADDR, M2_INPUT_MASK);
  pca_write_outputs(PCA9555_2_ADDR, s_pca2);
}

int matrix_clear(void)
{
  s_pca1 = 0;
  s_pca2 &= M2_INPUT_MASK;  
  int e = pca_write_outputs(PCA9555_1_ADDR, s_pca1);
  return e ? e : pca_write_outputs(PCA9555_2_ADDR, s_pca2);
}

int matrix_set_bits(uint8_t pca, uint16_t set_mask, uint16_t clr_mask)
{
  if (pca == PCA9555_1_ADDR) {
    s_pca1 = (uint16_t)((s_pca1 & ~clr_mask) | set_mask);
    return pca_write_outputs(PCA9555_1_ADDR, s_pca1);
  } else if (pca == PCA9555_2_ADDR) {
    s_pca2 = (uint16_t)((s_pca2 & ~clr_mask) | set_mask);
    
    return pca_write_outputs(PCA9555_2_ADDR, (uint16_t)(s_pca2 & ~M2_INPUT_MASK));
  }
  return -1;
}

int matrix_read_fault_flags(uint8_t *ff)
{
  uint8_t reg = 0x00;  
  uint8_t in[2] = {0, 0};
  int e = board_i2c_read(PCA9555_2_ADDR, &reg, 1, in, 2);
  if (e) return e;
  uint16_t v = (uint16_t)(in[0] | (in[1] << 8));
  
  uint8_t f = 0;
  if (!(v & M2_FF_SW1)) f |= 1u << 0;
  if (!(v & M2_FF_SW2)) f |= 1u << 1;
  if (!(v & M2_FF_SW3)) f |= 1u << 2;
  if (ff) *ff = f;
  return 0;
}


static void pin_conflict_masks(uint8_t pin, uint16_t *m1, uint16_t *m2)
{
  uint16_t a = 0, b = 0;
  switch (pin) {
  case 1: b |= M2_K2_PIN1 | M2_SWCAN_PIN1; a |= M1_CAN4_CONNECT; break;
  case 3: b |= M2_K2_PIN3 | M2_K3_PIN3;  a |= M1_CAN2_CONNECT; break;
  case 8: b |= M2_K2_PIN8 | M2_K3_PIN8;             break;
  case 9: b |= M2_K2_PIN9; a |= M1_CAN4_CONNECT | M1_FEPS_PIN9; break;
  case 11: b |= M2_K2_PIN11; a |= M1_CAN2_CONNECT | M1_FEPS_PIN11; break;
  case 12: b |= M2_K2_PIN12; a |= M1_CAN1_SWAP | M1_CAN3_CONNECT | M1_FEPS_PIN12; break;
  case 13: b |= M2_K2_PIN13; a |= M1_CAN1_SWAP | M1_CAN3_CONNECT | M1_FEPS_PIN13; break;
  case 15: b |= M2_K2_PIN15; a |= M1_SHORT_GND;          break;
  default: break;
  }
  if (m1) *m1 = a;
  if (m2) *m2 = b;
}

int matrix_can_connect(uint8_t can_ch)
{
  
  switch (can_ch) {
  case 2: return matrix_set_bits(PCA9555_1_ADDR, M1_CAN2_CONNECT, 0);
  case 3: return matrix_set_bits(PCA9555_1_ADDR, M1_CAN3_CONNECT, 0);
  case 4: return matrix_set_bits(PCA9555_1_ADDR, M1_CAN4_CONNECT, 0);
  default: return -1;
  }
}

int matrix_can1_polarity_swap(int swapped)
{
  return matrix_set_bits(PCA9555_1_ADDR,
              swapped ? M1_CAN1_SWAP : 0,
              swapped ? 0 : M1_CAN1_SWAP);
}

int matrix_can1_termination(int on)
{
  return matrix_set_bits(PCA9555_1_ADDR, on ? M1_CAN1_TERM : 0,
              on ? 0 : M1_CAN1_TERM);
}


int matrix_test_relay(uint8_t pca_idx, uint8_t bit, uint8_t on)
{
  uint8_t addr = (pca_idx == 2u) ? PCA9555_2_ADDR : PCA9555_1_ADDR;
  uint16_t mask = (uint16_t)(1u << (bit & 0x0Fu));
  return matrix_set_bits(addr, on ? mask : 0u, on ? 0u : mask);
}

int matrix_kline_route(uint8_t kch, uint8_t j1962_pin)
{
  
  uint16_t bit = 0;
  if (kch == 2) {
    switch (j1962_pin) {
    case 1: bit = M2_K2_PIN1; break; case 3: bit = M2_K2_PIN3; break;
    case 8: bit = M2_K2_PIN8; break; case 9: bit = M2_K2_PIN9; break;
    case 11: bit = M2_K2_PIN11; break; case 12: bit = M2_K2_PIN12; break;
    case 13: bit = M2_K2_PIN13; break; case 15: bit = M2_K2_PIN15; break;
    default: return -1;
    }
  } else if (kch == 3) {
    switch (j1962_pin) {
    case 3: bit = M2_K3_PIN3; break; case 8: bit = M2_K3_PIN8; break;
    default: return -1;
    }
  } else {
    return -1;
  }
  
  uint16_t c1, c2;
  pin_conflict_masks(j1962_pin, &c1, &c2);
  matrix_set_bits(PCA9555_1_ADDR, 0, c1);
  return matrix_set_bits(PCA9555_2_ADDR, bit, (uint16_t)(c2 & ~bit));
}


int feps_route_to_pin(uint8_t j1962_pin)
{
  uint16_t bit;
  switch (j1962_pin) {
  case 9: bit = M1_FEPS_PIN9; break;
  case 11: bit = M1_FEPS_PIN11; break;
  case 12: bit = M1_FEPS_PIN12; break;
  case 13: bit = M1_FEPS_PIN13; break;
  default: return -1;  
  }
  
  uint16_t c1, c2;
  pin_conflict_masks(j1962_pin, &c1, &c2);
  matrix_set_bits(PCA9555_2_ADDR, 0, c2);      
  matrix_set_bits(PCA9555_1_ADDR, 0, (uint16_t)(c1 & ~bit)); 
  return matrix_set_bits(PCA9555_1_ADDR, bit, 0);  
}

int feps_short_to_ground(uint8_t j1962_pin)
{
  if (j1962_pin != 15) return -1;  
  uint16_t c1, c2;
  pin_conflict_masks(15, &c1, &c2);
  matrix_set_bits(PCA9555_2_ADDR, 0, c2);
  return matrix_set_bits(PCA9555_1_ADDR, M1_SHORT_GND, 0);
}

void feps_off(void)
{
  
  matrix_set_bits(PCA9555_2_ADDR, 0, M2_FEPS_EN);
  matrix_set_bits(PCA9555_1_ADDR, 0,
    M1_FEPS_PIN9 | M1_FEPS_PIN11 | M1_FEPS_PIN12 | M1_FEPS_PIN13 |
    M1_SHORT_GND | M1_GPT1_ON | M1_GPT2_ON);
}

int gpt_set(uint8_t gpt_idx, int on)
{
  uint16_t bit = (gpt_idx == 1) ? M1_GPT1_ON : (gpt_idx == 2) ? M1_GPT2_ON : 0;
  if (!bit) return -1;
  return matrix_set_bits(PCA9555_1_ADDR, on ? bit : 0, on ? 0 : bit);
}


#define VPW_SHORT_US 64u
#define VPW_LONG_US 128u
#define VPW_SOF_US  200u
#define J1850_TX_PORT GPIOE
#define J1850_TX_PIN GPIO_PIN_5
#define J1850_RX_PIN GPIO_PIN_6
#define J1850_MAX_FRAME 12u   


typedef struct { uint16_t us; uint8_t active; } vpw_pulse_t;
#define VPW_PULSE_DEPTH 64
static volatile vpw_pulse_t s_vpw_pulse[VPW_PULSE_DEPTH];
static volatile uint8_t s_vpw_ph, s_vpw_pt;
static uint32_t s_vpw_last_us;

static uint8_t s_vpw_frame[J1850_MAX_FRAME];
static uint8_t s_vpw_frame_len;
static volatile uint8_t s_vpw_ready;

static uint8_t s_vpw_st, s_vpw_byte, s_vpw_bits, s_vpw_blen, s_vpw_buf[J1850_MAX_FRAME];

static void vpw_wait_us(uint32_t us) { uint32_t t0 = board_micros(); while ((board_micros() - t0) < us) { } }

void j1850_init(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  (void)matrix_set_bits(PCA9555_1_ADDR, 0, M1_J1850_PWM);  
  
  HAL_GPIO_WritePin(J1850_TX_PORT, J1850_TX_PIN, GPIO_PIN_RESET);
  GPIO_InitTypeDef g = {0};
  g.Mode = GPIO_MODE_OUTPUT_PP; g.Pull = GPIO_NOPULL; g.Speed = GPIO_SPEED_FREQ_HIGH;
  g.Pin = J1850_TX_PIN; HAL_GPIO_Init(J1850_TX_PORT, &g);
  
  g.Mode = GPIO_MODE_IT_RISING_FALLING; g.Pull = GPIO_NOPULL; g.Pin = J1850_RX_PIN;
  HAL_GPIO_Init(GPIOE, &g);
  s_vpw_last_us = board_micros();
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 4, 0); HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

int j1850_set_mode_vpw(int vpw)
{
  
  return matrix_set_bits(PCA9555_1_ADDR, vpw ? 0 : M1_J1850_PWM, vpw ? M1_J1850_PWM : 0);
}


int j1850_tx(const uint8_t *d, uint16_t n)
{
  if (!d || n == 0 || n > J1850_MAX_FRAME) return -1;
  __disable_irq();             
  HAL_GPIO_WritePin(J1850_TX_PORT, J1850_TX_PIN, GPIO_PIN_SET);  
  vpw_wait_us(VPW_SOF_US);
  uint8_t active = 0;           
  for (uint16_t i = 0; i < n; i++) {
    for (int b = 7; b >= 0; b--) {
      uint8_t bit = (d[i] >> b) & 1u;
      
      uint32_t w = active ? (bit ? VPW_LONG_US : VPW_SHORT_US)
                : (bit ? VPW_SHORT_US : VPW_LONG_US);
      HAL_GPIO_WritePin(J1850_TX_PORT, J1850_TX_PIN, active ? GPIO_PIN_SET : GPIO_PIN_RESET);
      vpw_wait_us(w);
      active = !active;
    }
  }
  HAL_GPIO_WritePin(J1850_TX_PORT, J1850_TX_PIN, GPIO_PIN_RESET); 
  __enable_irq();
  vpw_wait_us(VPW_SOF_US);         
  return 0;
}


void j1850_capture(void)
{
  uint32_t now = board_micros();
  uint16_t w = (uint16_t)(now - s_vpw_last_us);
  s_vpw_last_us = now;
  uint8_t newlvl = (HAL_GPIO_ReadPin(GPIOE, J1850_RX_PIN) == GPIO_PIN_SET) ? 1u : 0u;
  uint8_t completed_active = newlvl ? 0u : 1u;   
  uint8_t next = (uint8_t)((s_vpw_ph + 1) % VPW_PULSE_DEPTH);
  if (next != s_vpw_pt) {
    s_vpw_pulse[s_vpw_ph].us = w;
    s_vpw_pulse[s_vpw_ph].active = completed_active;
    s_vpw_ph = next;
  }
}


void j1850_service(void)
{
  while (s_vpw_pt != s_vpw_ph) {
    uint16_t w = s_vpw_pulse[s_vpw_pt].us;
    uint8_t act = s_vpw_pulse[s_vpw_pt].active;
    s_vpw_pt = (uint8_t)((s_vpw_pt + 1) % VPW_PULSE_DEPTH);

    if (s_vpw_st == 0u) {             
      if (act && w >= 164u && w <= 239u) { s_vpw_st = 1u; s_vpw_bits = 0; s_vpw_byte = 0; s_vpw_blen = 0; }
      continue;
    }
    
    if (!act && w >= 164u) {            
      if (s_vpw_blen > 0u) {
        for (uint8_t k = 0; k < s_vpw_blen; k++) s_vpw_frame[k] = s_vpw_buf[k];
        s_vpw_frame_len = s_vpw_blen; s_vpw_ready = 1u;
      }
      s_vpw_st = 0u; continue;
    }
    uint8_t bit;
    if (w >= 34u && w <= 96u)    bit = act ? 0u : 1u;  
    else if (w >= 97u && w <= 163u) bit = act ? 1u : 0u;  
    else { s_vpw_st = 0u; continue; }            
    s_vpw_byte = (uint8_t)((s_vpw_byte << 1) | bit);
    if (++s_vpw_bits == 8u) {
      if (s_vpw_blen < J1850_MAX_FRAME) s_vpw_buf[s_vpw_blen++] = s_vpw_byte;
      s_vpw_bits = 0; s_vpw_byte = 0;
    }
  }
}


int j1850_read(uint8_t *buf, uint16_t *len)
{
  if (!s_vpw_ready) return 0;
  uint8_t n = s_vpw_frame_len;
  if (buf) for (uint8_t k = 0; k < n; k++) buf[k] = s_vpw_frame[k];
  if (len) *len = n;
  s_vpw_ready = 0u;
  return 1;
}


void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
  if (pin == J1850_RX_PIN) j1850_capture();
}


void swcan_init(void) { (void)matrix_set_bits(PCA9555_1_ADDR, 0, M1_SWCAN_MODE0 | M1_SWCAN_MODE1); }
int swcan_set_mode(uint8_t mode)
{
  
  uint16_t bits = 0;
  if (mode & 1u) bits |= M1_SWCAN_MODE0;
  if (mode & 2u) bits |= M1_SWCAN_MODE1;
  return matrix_set_bits(PCA9555_1_ADDR, bits,
              (uint16_t)((M1_SWCAN_MODE0 | M1_SWCAN_MODE1) & ~bits));
}



static FDCAN_HandleTypeDef s_fdcan[3];
static const uint32_t S_FDCAN_RAMOFF[3] = { 0u, 97u, 194u };  

typedef struct { uint32_t id; uint8_t data[8]; uint8_t len; uint8_t ext; } can_frame_t;
#define CAN_RX_DEPTH 32
static volatile can_frame_t s_can_rx[3][CAN_RX_DEPTH];
static volatile uint16_t s_can_rx_head[3], s_can_rx_tail[3];

static FDCAN_GlobalTypeDef *can_instance(uint8_t ch)
{
  switch (ch) {
  case CAN_CH1: return FDCAN1; case CAN_CH2: return FDCAN2; case CAN_CH3: return FDCAN3;
  default: return 0;
  }
}


static int fdcan_setup(uint8_t ch, uint32_t nominal_baud)
{
  FDCAN_GlobalTypeDef *inst = can_instance(ch);
  if (!inst) return -1;
  if (!nominal_baud) nominal_baud = 500000u;
  uint32_t presc = 80000000u / (16u * nominal_baud);  
  if (presc < 1u) presc = 1u;
  if (presc > 512u) presc = 512u;

  FDCAN_HandleTypeDef *h = &s_fdcan[ch];
  h->Instance = inst;
  h->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  h->Init.Mode = FDCAN_MODE_NORMAL;
  h->Init.AutoRetransmission = ENABLE;
  h->Init.TransmitPause = DISABLE;
  h->Init.ProtocolException = DISABLE;
  h->Init.NominalPrescaler = presc;
  h->Init.NominalSyncJumpWidth = 1;
  h->Init.NominalTimeSeg1 = 13;    
  h->Init.NominalTimeSeg2 = 2;
  h->Init.DataPrescaler = presc;    
  h->Init.DataSyncJumpWidth = 1;
  h->Init.DataTimeSeg1 = 13;
  h->Init.DataTimeSeg2 = 2;
  h->Init.MessageRAMOffset = S_FDCAN_RAMOFF[ch];
  h->Init.StdFiltersNbr = 1;
  h->Init.ExtFiltersNbr = 0;
  h->Init.RxFifo0ElmtsNbr = 16;
  h->Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
  h->Init.RxFifo1ElmtsNbr = 0;
  h->Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
  h->Init.RxBuffersNbr = 0;
  h->Init.RxBufferSize = FDCAN_DATA_BYTES_8;
  h->Init.TxEventsNbr = 0;
  h->Init.TxBuffersNbr = 0;
  h->Init.TxFifoQueueElmtsNbr = 8;
  h->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  h->Init.TxElmtSize = FDCAN_DATA_BYTES_8;
  if (HAL_FDCAN_Init(h) != HAL_OK) return -1;

  
  FDCAN_FilterTypeDef f = {0};
  f.IdType = FDCAN_STANDARD_ID;
  f.FilterIndex = 0;
  f.FilterType = FDCAN_FILTER_MASK;
  f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  f.FilterID1 = 0; f.FilterID2 = 0;
  if (HAL_FDCAN_ConfigFilter(h, &f) != HAL_OK) return -1;
  if (HAL_FDCAN_ConfigGlobalFilter(h, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0,
                   FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) return -1;
  if (HAL_FDCAN_ActivateNotification(h, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) return -1;
  if (HAL_FDCAN_Start(h) != HAL_OK) return -1;
  return 0;
}

void can_init(void)
{
  __HAL_RCC_FDCAN_CLK_ENABLE();  
  for (uint8_t ch = CAN_CH1; ch <= CAN_CH3; ch++) {
    s_can_rx_head[ch] = s_can_rx_tail[ch] = 0;
    fdcan_setup(ch, 500000u);  
  }
  HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5, 0); HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
  HAL_NVIC_SetPriority(FDCAN2_IT0_IRQn, 5, 0); HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
  HAL_NVIC_SetPriority(FDCAN3_IT0_IRQn, 5, 0); HAL_NVIC_EnableIRQ(FDCAN3_IT0_IRQn);
  
  for (uint8_t m = 0; m < 3u; m++) mcp_init(m, 500000u);
}

void can_service(void) { }

int can_open(uint8_t ch, uint32_t nominal_baud, uint32_t data_baud, uint32_t flags)
{
  (void)data_baud; (void)flags;    
  if (ch > CAN_CH3) return mcp_init((uint8_t)(ch - CAN_CH4_MCP1), nominal_baud); 
  HAL_FDCAN_Stop(&s_fdcan[ch]);
  return fdcan_setup(ch, nominal_baud);
}

int can_close(uint8_t ch)
{
  if (ch > CAN_CH3) return 0;
  return (HAL_FDCAN_Stop(&s_fdcan[ch]) == HAL_OK) ? 0 : -1;
}

int can_tx(uint8_t ch, uint32_t id, const uint8_t *data, uint8_t len, uint32_t flags)
{
  if (ch > CAN_CH3)          
    return mcp_tx((uint8_t)(ch - CAN_CH4_MCP1), id, data, (len > 8) ? 8 : len,
           (flags & J2534_CAN_29BIT_ID) ? 1u : 0u);
  if (len > 8) len = 8;
  FDCAN_TxHeaderTypeDef t = {0};
  t.Identifier = id;
  t.IdType = (flags & J2534_CAN_29BIT_ID) ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
  t.TxFrameType = FDCAN_DATA_FRAME;
  t.DataLength = len;         
  t.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  t.BitRateSwitch = FDCAN_BRS_OFF;
  t.FDFormat = FDCAN_CLASSIC_CAN;
  t.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  t.MessageMarker = 0;
  return (HAL_FDCAN_AddMessageToTxFifoQ(&s_fdcan[ch], &t, (uint8_t *)data) == HAL_OK) ? 0 : -1;
}


int can_read(uint8_t ch, uint32_t *id, uint8_t *data, uint8_t *len, uint8_t *ext)
{
  if (ch > CAN_CH3)          
    return mcp_read((uint8_t)(ch - CAN_CH4_MCP1), id, data, len, ext);
  if (s_can_rx_tail[ch] == s_can_rx_head[ch]) return 0;
  const volatile can_frame_t *fr = &s_can_rx[ch][s_can_rx_tail[ch]];
  if (id) *id = fr->id;
  if (len) *len = fr->len;
  if (ext) *ext = fr->ext;
  if (data) for (uint8_t i = 0; i < fr->len; i++) data[i] = fr->data[i];
  s_can_rx_tail[ch] = (uint16_t)((s_can_rx_tail[ch] + 1) % CAN_RX_DEPTH);
  return 1;
}


void can_irq(uint8_t ch) { if (ch <= CAN_CH3) HAL_FDCAN_IRQHandler(&s_fdcan[ch]); }


void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
  if (!(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)) return;
  uint8_t ch = (hfdcan->Instance == FDCAN1) ? CAN_CH1 :
         (hfdcan->Instance == FDCAN2) ? CAN_CH2 :
         (hfdcan->Instance == FDCAN3) ? CAN_CH3 : 0xFFu;
  if (ch == 0xFFu) return;
  FDCAN_RxHeaderTypeDef rh;
  uint8_t buf[8];
  while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0u) {
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rh, buf) != HAL_OK) break;
    uint16_t next = (uint16_t)((s_can_rx_head[ch] + 1) % CAN_RX_DEPTH);
    if (next == s_can_rx_tail[ch]) break;  
    volatile can_frame_t *fr = &s_can_rx[ch][s_can_rx_head[ch]];
    fr->id = rh.Identifier;
    fr->ext = (rh.IdType == FDCAN_EXTENDED_ID) ? 1u : 0u;
    fr->len = (uint8_t)((rh.DataLength <= 8u) ? rh.DataLength : 8u);
    for (uint8_t i = 0; i < fr->len; i++) fr->data[i] = buf[i];
    s_can_rx_head[ch] = next;
  }
}


static UART_HandleTypeDef s_uart[KLINE_CH_COUNT];
#define KLINE_RX_DEPTH 64
static volatile uint8_t s_kl_rx[KLINE_CH_COUNT][KLINE_RX_DEPTH];
static volatile uint16_t s_kl_head[KLINE_CH_COUNT], s_kl_tail[KLINE_CH_COUNT];
static uint8_t s_kl_byte[KLINE_CH_COUNT];   

static USART_TypeDef *kline_instance(uint8_t ch)
{
  switch (ch) {
  case KLINE_CH1: return USART2; case KLINE_CH2: return USART3; case KLINE_CH3: return UART4;
  default: return 0;
  }
}

static int kline_setup(uint8_t ch, uint32_t baud)
{
  USART_TypeDef *inst = kline_instance(ch);
  if (!inst) return -1;
  if (!baud) baud = 10400u;
  UART_HandleTypeDef *u = &s_uart[ch];
  u->Instance = inst;
  u->Init.BaudRate = baud;
  u->Init.WordLength = UART_WORDLENGTH_8B;
  u->Init.StopBits = UART_STOPBITS_1;
  u->Init.Parity = UART_PARITY_NONE;
  u->Init.Mode = UART_MODE_TX_RX;
  u->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  u->Init.OverSampling = UART_OVERSAMPLING_16;
  u->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  u->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  u->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_TXINVERT_INIT;  
  u->AdvancedInit.TxPinLevelInvert = UART_ADVFEATURE_TXINV_ENABLE;
  if (HAL_UART_Init(u) != HAL_OK) return -1;
  s_kl_head[ch] = s_kl_tail[ch] = 0;
  HAL_UART_Receive_IT(u, &s_kl_byte[ch], 1);  
  return 0;
}

void kline_init(void)
{
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_UART4_CLK_ENABLE();
  for (uint8_t ch = KLINE_CH1; ch < KLINE_CH_COUNT; ch++) kline_setup(ch, 10400u);
  HAL_NVIC_SetPriority(USART2_IRQn, 6, 0); HAL_NVIC_EnableIRQ(USART2_IRQn);
  HAL_NVIC_SetPriority(USART3_IRQn, 6, 0); HAL_NVIC_EnableIRQ(USART3_IRQn);
  HAL_NVIC_SetPriority(UART4_IRQn, 6, 0); HAL_NVIC_EnableIRQ(UART4_IRQn);
}

void kline_service(void) { }

int kline_open(uint8_t ch, uint32_t baud)
{
  if (ch >= KLINE_CH_COUNT) return -1;
  HAL_UART_DeInit(&s_uart[ch]);
  return kline_setup(ch, baud);
}

int kline_tx(uint8_t ch, const uint8_t *d, uint16_t n)
{
  if (ch >= KLINE_CH_COUNT || !d) return -1;
  
  return (HAL_UART_Transmit(&s_uart[ch], (uint8_t *)d, n, 1000u) == HAL_OK) ? 0 : -1;
}


int kline_read(uint8_t ch, uint8_t *byte)
{
  if (ch >= KLINE_CH_COUNT) return 0;
  if (s_kl_tail[ch] == s_kl_head[ch]) return 0;
  if (byte) *byte = s_kl_rx[ch][s_kl_tail[ch]];
  s_kl_tail[ch] = (uint16_t)((s_kl_tail[ch] + 1) % KLINE_RX_DEPTH);
  return 1;
}


void kline_irq(uint8_t ch) { if (ch < KLINE_CH_COUNT) HAL_UART_IRQHandler(&s_uart[ch]); }


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t ch = (huart->Instance == USART2) ? KLINE_CH1 :
         (huart->Instance == USART3) ? KLINE_CH2 :
         (huart->Instance == UART4) ? KLINE_CH3 : 0xFFu;
  if (ch == 0xFFu) return;
  uint16_t next = (uint16_t)((s_kl_head[ch] + 1) % KLINE_RX_DEPTH);
  if (next != s_kl_tail[ch]) {         
    s_kl_rx[ch][s_kl_head[ch]] = s_kl_byte[ch];
    s_kl_head[ch] = next;
  }
  HAL_UART_Receive_IT(huart, &s_kl_byte[ch], 1);  
}


int kline_five_baud_init(uint8_t ch, uint8_t a, uint8_t *kb) { (void)ch;(void)a;(void)kb; return -1; }
int kline_fast_init(uint8_t ch, const uint8_t *tx, uint8_t n, uint8_t *rx, uint8_t *rn) { (void)ch;(void)tx;(void)n;(void)rx;(void)rn; return -1; }


#define DAC_CS_PORT GPIOB
#define DAC_CS_PIN  GPIO_PIN_0
#define MCP3426_ADDR 0x68u
#define ADC_DIV_NUM 113  
#define ADC_DIV_DEN 13

static void dac_write(uint16_t code12)
{
  uint16_t cmd = 0x7000u | (code12 & 0x0FFFu);  
  uint8_t tx[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFFu) };
  HAL_GPIO_WritePin(DAC_CS_PORT, DAC_CS_PIN, GPIO_PIN_RESET);
  board_spi_transfer(tx, 0, 2);
  HAL_GPIO_WritePin(DAC_CS_PORT, DAC_CS_PIN, GPIO_PIN_SET);
}

void feps_init(void)
{
  feps_off();  
}

int feps_set_voltage_mv(uint32_t mv)
{
  if (mv < 5000u) return -1;     
  if (mv > 24000u) mv = 24000u;
  int32_t vdac_mv = 1600 + (14800 - (int32_t)mv) * 1000 / 5769;
  if (vdac_mv < 0) vdac_mv = 0;
  if (vdac_mv > 3300) vdac_mv = 3300;
  uint32_t code = (uint32_t)vdac_mv * 4096u / 3300u;
  if (code > 4095u) code = 4095u;
  dac_write((uint16_t)code);
  (void)matrix_set_bits(PCA9555_2_ADDR, M2_FEPS_EN, 0);  
  return 0;
}


static int adc_read_mv(uint8_t ch, int32_t *vadc_mv)
{
  uint8_t cfg = 0x80u | ((ch == 2u) ? 0x20u : 0x00u);  
  if (board_i2c_write(MCP3426_ADDR, &cfg, 1) != 0) return -1;
  uint8_t buf[3];
  uint32_t t0 = board_millis();
  do {
    if (board_i2c_read(MCP3426_ADDR, 0, 0, buf, 3) != 0) return -1;
    if (!(buf[2] & 0x80u)) break;           
  } while ((board_millis() - t0) < 20u);
  if (buf[2] & 0x80u) return -1;             
  int16_t code = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]); 
  if (code < 0) code = 0;
  if (vadc_mv) *vadc_mv = code;
  return 0;
}

int feps_read_vbatt_mv(uint32_t *mv)    
{
  int32_t v;
  if (adc_read_mv(1u, &v) != 0) return -1;
  if (mv) *mv = (uint32_t)(v * ADC_DIV_NUM / ADC_DIV_DEN);
  return 0;
}

int feps_read_prog_mv(uint32_t *mv)    
{
  int32_t v;
  if (adc_read_mv(2u, &v) != 0) return -1;
  if (mv) *mv = (uint32_t)(v * ADC_DIV_NUM / ADC_DIV_DEN);
  return 0;
}
