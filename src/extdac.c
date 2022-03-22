/*
 * extdac.c
 *
 * Created: 5/7/2013 5:27:53 PM
 *  Author: clarkson
 */

#include <asf.h>
#include <wallclock.h>
#include <extdac.h>
#include <sequencer.h>
#include <bridge.h>
#include <event_handlers.h>

#define EXTDAC_CONFIG_ADDRESS 0
#define EXTDAC_BROADCAST_ADDRESS 0b00111

// SCE=1
#define EXTDAC_CONFIG_DATA_SCE_ON 0b1000010111000000
// SCE=0
#define EXTDAC_CONFIG_DATA_SCE_OFF 0b1000000111000000

#define EXTDAC_MAGIC 0xBAAB
#define EXTDAC_CALIBRATION_DATA_ADDRESS AVR32_FLASHC_USER_PAGE_ADDRESS

typedef struct
{
  uint8_t address;
  uint8_t data0;
  uint8_t data1;
} extdac_packet_t;

typedef struct
{
  uint8_t data0;
  uint8_t data1;
} extdac_channel_t;

typedef struct
{
  int v0;
  int V;
  bool enabled;
} extdac_smooth_t;

wallclock_t extdac_smoother_t0[4] = {0};
float extdac_smoother_invT[4] = {0};

static const int channel_map[] = {6, 4, 1, 3, 7, 5, 0, 2};
static const uint8_t extdac_addresses[8] = {0b01000, 0b01001, 0b01010, 0b01011, 0b01100, 0b01101, 0b01110, 0b01111};
static const uint8_t extdac_zero_addresses[8] = {0b10000, 0b10001, 0b10010, 0b10011, 0b10100, 0b10101, 0b10110, 0b10111};
static const uint8_t extdac_gain_addresses[8] = {0b11000, 0b11001, 0b11010, 0b11011, 0b11100, 0b11101, 0b11110, 0b11111};
extdac_packet_t extdac_packets[8];
extdac_packet_t send_packet;
extdac_smooth_t smoothers[8];
bool sce = false;

static bool extdac_calibration_data_exists(void)
{
  uint16_t *pdata = (uint16_t *)EXTDAC_CALIBRATION_DATA_ADDRESS;
  return (*pdata) == EXTDAC_MAGIC;
}

static uint16_t extdac_get_calibration_gain(int ch)
{
  uint16_t *pdata = (uint16_t *)EXTDAC_CALIBRATION_DATA_ADDRESS;
  return pdata[ch * 2 + 1];
}

static uint16_t extdac_get_calibration_zero(int ch)
{
  uint16_t *pdata = (uint16_t *)EXTDAC_CALIBRATION_DATA_ADDRESS;
  return pdata[ch * 2 + 2];
}

void extdac_set_smoother_timing(uint8_t track, wallclock_t t0, unsigned long period)
{
  extdac_smoother_t0[track] = t0;
  if (period > 0)
  {
    extdac_smoother_invT[track] = 1.0 / period;
  }
}

static void extdac_init_smoothers(void)
{
  for (int i = 0; i < 8; i++)
  {
    smoothers[i].enabled = false;
  }
}

static void extdac_update_smoother(int i)
{
  if (smoothers[i].enabled)
  {
    wallclock_t now = wallclock();
    wallclock_t t0 = extdac_smoother_t0[i >> 1];
    float invT = extdac_smoother_invT[i >> 1];
    float w = ((unsigned long)(now - t0)) * invT;

    if (w > 1.0 + FLOAT_EPSILON)
    {
      extdac_set_value(i, smoothers[i].v0 + smoothers[i].V);
    }
    else
    {
      extdac_set_value(i, smoothers[i].v0 + smoothers[i].V * w);
    }
  }
}

void extdac_set_smoother_output(int i, int v0, int v1)
{
  smoothers[i].v0 = v0;
  smoothers[i].V = v1 - v0;
  smoothers[i].enabled = true;
}

void extdac_disable_smoother(int i)
{
  smoothers[i].enabled = false;
}

void extdac_dma_start(void)
{
  gpio_set_pin_high(PIN_SELECT_DAC);
  gpio_set_pin_low(PIN_LATCH_DAC);

  for (int i = 0; i < 8; i++)
  {
    extdac_packets[i].address = extdac_addresses[channel_map[i]];
    extdac_update_smoother(i);
  }

  memcpy(&send_packet, &(extdac_packets[0]), sizeof(extdac_packet_t));

  pdca_channel_options_t TX_OPTIONS = {
      /* Select peripheral - data is transmitted on USART TX line */
      .pid = AVR32_PDCA_PID_USART0_TX,
      /* Select size of the transfer */
      .transfer_size = PDCA_TRANSFER_SIZE_BYTE,

      /* Memory address */
      .addr = NULL,
      /* Transfer counter */
      .size = 0,

      /* Next memory address */
      .r_addr = &send_packet,
      /* Next transfer counter */
      .r_size = sizeof(extdac_packet_t),
  };

  gpio_set_pin_high(PIN_LATCH_DAC);
  gpio_set_pin_low(PIN_SELECT_DAC);

  /* Initialize the PDCA channels with the requested options. */
  pdca_init_channel(EXTDAC_PDCA_CHANNEL, &TX_OPTIONS);
  extdac_ready = false;
  pdca_enable_interrupt_transfer_complete(EXTDAC_PDCA_CHANNEL);
  pdca_enable(EXTDAC_PDCA_CHANNEL);
}

void extdac_feed(void)
{
  static uint8_t cur = 0;
  pdca_disable(EXTDAC_PDCA_CHANNEL);

  cur++;
  if (cur == 8)
  {
    cur = 0;
  }

  extdac_update_smoother(cur);

  gpio_set_pin_high(PIN_SELECT_DAC);
  gpio_set_pin_low(PIN_LATCH_DAC);

  memcpy(&send_packet, &(extdac_packets[cur]), sizeof(extdac_packet_t));

  gpio_set_pin_high(PIN_LATCH_DAC);
  gpio_set_pin_low(PIN_SELECT_DAC);

  pdca_reload_channel(EXTDAC_PDCA_CHANNEL, (void *)&send_packet, sizeof(extdac_packet_t));
  extdac_ready = false;
  pdca_enable_interrupt_transfer_complete(EXTDAC_PDCA_CHANNEL);
  pdca_enable(EXTDAC_PDCA_CHANNEL);
}

void extdac_feed_slavemode(void)
{
  static uint8_t cur = 0;
  pdca_disable(EXTDAC_PDCA_CHANNEL);

  cur++;
  if (cur == 8)
  {
    cur = 0;
  }

  gpio_set_pin_high(PIN_SELECT_DAC);
  gpio_set_pin_low(PIN_LATCH_DAC);

  memcpy(&send_packet, &(extdac_packets[cur]), sizeof(extdac_packet_t));

  gpio_set_pin_high(PIN_LATCH_DAC);
  gpio_set_pin_low(PIN_SELECT_DAC);

  pdca_reload_channel(EXTDAC_PDCA_CHANNEL, (void *)&send_packet, sizeof(extdac_packet_t));
  extdac_ready = false;
  pdca_enable_interrupt_transfer_complete(EXTDAC_PDCA_CHANNEL);
  pdca_enable(EXTDAC_PDCA_CHANNEL);
}

static void delay_cycles(int n)
{
  while (--n > 0)
    ;
}

void extdac_zero_all()
{
  gpio_set_pin_high(PIN_LATCH_DAC);
  delay_cycles(100);
  gpio_set_pin_low(PIN_SELECT_DAC);
  delay_cycles(100);
  usart_putchar(&USART_DAC, EXTDAC_BROADCAST_ADDRESS);
  usart_putchar(&USART_DAC, 0);
  usart_putchar(&USART_DAC, 0);
  while (!usart_tx_empty(&USART_DAC))
    ;
  gpio_set_pin_high(PIN_SELECT_DAC);
  delay_cycles(100);
  gpio_set_pin_low(PIN_LATCH_DAC);
  delay_cycles(100);
}

static void extdac_write_configuration(uint16_t config_data)
{
  // gpio_set_pin_high(PIN_LATCH_DAC);
  // delay_cycles(100);
  gpio_set_pin_low(PIN_SELECT_DAC);
  delay_cycles(100);
  usart_putchar(&USART_DAC, EXTDAC_CONFIG_ADDRESS);
  usart_putchar(&USART_DAC, MSB(config_data));
  usart_putchar(&USART_DAC, LSB(config_data));
  while (!usart_tx_empty(&USART_DAC))
    ;
  gpio_set_pin_high(PIN_SELECT_DAC);
  delay_cycles(100);
  // gpio_set_pin_low(PIN_LATCH_DAC);
  // delay_cycles(100);
}

static void extdac_write_calibration(int ch, uint16_t gain_data, uint16_t zero_data)
{
  uint16_t tmp;
  ch = channel_map[ch];

  // gpio_set_pin_high(PIN_LATCH_DAC);
  // delay_cycles(100);
  gpio_set_pin_low(PIN_SELECT_DAC);
  delay_cycles(100);
  usart_putchar(&USART_DAC, extdac_gain_addresses[ch]);
  tmp = gain_data << 4;
  usart_putchar(&USART_DAC, MSB(tmp));
  usart_putchar(&USART_DAC, LSB(tmp));
  while (!usart_tx_empty(&USART_DAC))
    ;
  gpio_set_pin_high(PIN_SELECT_DAC);
  delay_cycles(100);
  // gpio_set_pin_low(PIN_LATCH_DAC);
  // delay_cycles(100);

  // gpio_set_pin_high(PIN_LATCH_DAC);
  // delay_cycles(100);
  gpio_set_pin_low(PIN_SELECT_DAC);
  delay_cycles(100);
  usart_putchar(&USART_DAC, extdac_zero_addresses[ch]);
  tmp = zero_data << 4;
  usart_putchar(&USART_DAC, MSB(tmp));
  usart_putchar(&USART_DAC, LSB(tmp));
  while (!usart_tx_empty(&USART_DAC))
    ;
  gpio_set_pin_high(PIN_SELECT_DAC);
  delay_cycles(100);
  // gpio_set_pin_low(PIN_LATCH_DAC);
  // delay_cycles(100);
}

void extdac_calibration_off()
{
  extdac_write_configuration(EXTDAC_CONFIG_DATA_SCE_OFF);
  sce = false;
}

void extdac_calibration_on()
{
  extdac_write_configuration(EXTDAC_CONFIG_DATA_SCE_ON);
  sce = true;
}

bool extdac_is_calibration_enabled(void)
{
  return sce;
}

void extdac_init(void)
{

  // turn on AVDD (+10V)
  gpio_set_pin_low(PIN_ENABLE_DACSUPPLY);
  cpu_delay_ms(100, CPU_HZ);

  // turn on VREF
  gpio_set_pin_high(PIN_ENABLE_VREF);
  cpu_delay_ms(100, CPU_HZ);

  // bring the DAC out of reset
  gpio_set_pin_high(PIN_RESET_DAC);
  cpu_delay_ms(100, CPU_HZ);

  // enable the outputs
  gpio_set_pin_high(PIN_CLEAR_DAC);

  extdac_zero_all();

  if (extdac_calibration_data_exists())
  {
    extdac_calibration_on();
    for (int i = 0; i < 8; i++)
    {
      uint16_t gain_data = extdac_get_calibration_gain(i);
      uint16_t zero_data = extdac_get_calibration_zero(i);
      extdac_write_calibration(i, gain_data, zero_data);
    }
  }
  else
  {
    extdac_calibration_off();
    for (int i = 0; i < 8; i++)
    {
      extdac_write_calibration(i, 0, 0);
    }
  }

  extdac_init_smoothers();
}

void extdac_set_value(int i, int value)
{
  extdac_packets[i].data0 = LSB1W(value);
  extdac_packets[i].data1 = LSB0W(value);
}