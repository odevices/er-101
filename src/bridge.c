#include <asf.h>
#include "bridge.h"
#include "static_assert.h"
#include "wallclock.h"
#include "sequencer.h"
#include "display.h"
#include "extdac.h"
#include "interrupt_handlers.h"

#define BRIDGE_CRC_MAGIC 0b10000011

typedef struct
{
  uint8_t destination;
  uint8_t data[BRIDGE_BLOCK_SIZE];
  uint8_t crc;
} bridge_packet_t;

STATIC_ASSERT(sizeof(bridge_packet_t) == 50, bridge_packet_t_is_wrong_size);

bridge_control_t bridge_control;
bool bridge_control_has_changed = false;

bridge_state_t bridge_state;
static uint8_t event_index = 0;

fifo_desc_t event_q_desc;

typedef union
{
  bridge_event_t as_struct;
  uint32_t as_int;
} bridge_q_elt_t;

void bridge_init(void)
{
  fifo_init(&event_q_desc, reserved_q_buffer, RESERVED_Q_SIZE);
}

#define BRIDGE_TIMEOUT 50000
//#define BRIDGE_SYNC_MESSAGES

bool bridge_feed(void)
{
  static uint8_t state = 0;
  static bridge_packet_t rx_packet, tx_packet;
#ifdef BRIDGE_TIMEOUT
  static uint32_t timeout_count = 0;
#endif
  bool retval = false;
  static volatile avr32_pdca_channel_t *rx_channel;
  static volatile avr32_pdca_channel_t *tx_channel;

  rx_channel = pdca_get_handler(BRIDGE_PDCA_RX_CHANNEL);
  tx_channel = pdca_get_handler(BRIDGE_PDCA_TX_CHANNEL);

  /*
  static int count = 0;
  if(rx_channel->cr)
  {
    count++;
  }

  if(tx_channel->cr)
  {
    count++;
  }
  */

  if (state == 0)
  {
    // rx_channel = pdca_get_handler(BRIDGE_PDCA_RX_CHANNEL);
    // tx_channel = pdca_get_handler(BRIDGE_PDCA_TX_CHANNEL);
    //  DEBUG
    //  gpio_toggle_pin(PIN_COMMIT_LED);
    // pdca_disable(BRIDGE_PDCA_RX_CHANNEL);
    // pdca_disable(BRIDGE_PDCA_TX_CHANNEL);
    //  enable expansion SPI module in slave mode
    // spi_disable(EXPANSION_SPI);
    spi_reset(EXPANSION_SPI);
    spi_set_slave_mode(EXPANSION_SPI);
    spi_initSlave(EXPANSION_SPI, 8, SPI_MODE_0);
    spi_enable(EXPANSION_SPI);
    // EXPANSION_SPI->cr = AVR32_SPI_CR_FLUSHFIFO_MASK;

    bit_set(bridge_state.system, BRIDGE_COMM_RESET);

    pdca_channel_options_t TX_OPTIONS = {
        /* Select peripheral */
        .pid = AVR32_PDCA_PID_SPI1_TX,
        /* Select size of the transfer */
        .transfer_size = PDCA_TRANSFER_SIZE_BYTE,

        /* Memory address */
        .addr = NULL,
        /* Transfer counter */
        .size = 0,

        /* Next memory address */
        .r_addr = NULL,
        /* Next transfer counter */
        .r_size = 0,
    };

    /* Initialize the PDCA channels with the requested options. */
    pdca_init_channel(BRIDGE_PDCA_TX_CHANNEL, &TX_OPTIONS);

    pdca_channel_options_t RX_OPTIONS = {
        /* Select peripheral */
        .pid = AVR32_PDCA_PID_SPI1_RX,
        /* Select size of the transfer */
        .transfer_size = PDCA_TRANSFER_SIZE_BYTE,

        /* Memory address */
        .addr = NULL,
        /* Transfer counter */
        .size = 0,

        /* Next memory address */
        .r_addr = NULL,
        /* Next transfer counter */
        .r_size = 0,
    };

    /* Initialize the PDCA channels with the requested options. */
    pdca_init_channel(BRIDGE_PDCA_RX_CHANNEL, &RX_OPTIONS);

    // EXPANSION_SPI->MR.rxfifoen = 0;

    pdca_enable(BRIDGE_PDCA_RX_CHANNEL);
    pdca_enable(BRIDGE_PDCA_TX_CHANNEL);

    bridge_clear_state();

    // tell the controller about the state of the switches
    if (gpio_pin_is_high(PIN_MODE_A))
    {
      bridge_event(ID_MODE_SWITCH, EVENT_POS_DOWN);
    }
    else if (gpio_pin_is_high(PIN_MODE_B))
    {
      bridge_event(ID_MODE_SWITCH, EVENT_POS_UP);
    }
    else
    {
      bridge_event(ID_MODE_SWITCH, EVENT_POS_MIDDLE);
    }

    if (gpio_pin_is_high(PIN_TABLE_A))
    {
      bridge_event(ID_TABLE_SWITCH, EVENT_POS_DOWN);
    }
    else if (gpio_pin_is_high(PIN_TABLE_B))
    {
      bridge_event(ID_TABLE_SWITCH, EVENT_POS_UP);
    }
    else
    {
      bridge_event(ID_TABLE_SWITCH, EVENT_POS_MIDDLE);
    }
    state = 1;
  }
  else if (state == 1)
  {
    rx_packet.destination = BRIDGE_NULL_DESTINATION;
    rx_packet.crc = 0;

    if (1)
    {
      tx_packet.destination = BRIDGE_STATE_DESTINATION;
      // irqflags_t flags = cpu_irq_save();
      bridge_copy_system_state();
      memcpy((void *)tx_packet.data, (void *)&bridge_state, sizeof(bridge_state_t));
      bridge_clear_state();
      // cpu_irq_restore(flags);
      tx_packet.crc = BRIDGE_CRC_MAGIC;
    }

    if (0)
    {
      int i;
      tx_packet.destination = 0b11000001;
      for (i = 0; i < BRIDGE_BLOCK_SIZE; i++)
        tx_packet.data[i] = 0b10100001;
      tx_packet.crc = 0b10000011;
    }

    pdca_reload_channel(BRIDGE_PDCA_RX_CHANNEL, &rx_packet, sizeof(bridge_packet_t));
    pdca_reload_channel(BRIDGE_PDCA_TX_CHANNEL, &tx_packet, sizeof(bridge_packet_t));

    bridge_rx_ready = false;
    bridge_tx_ready = false;
    bridge_error = false;
    pdca_enable_interrupt_transfer_complete(BRIDGE_PDCA_RX_CHANNEL);
    pdca_enable_interrupt_transfer_complete(BRIDGE_PDCA_TX_CHANNEL);
    pdca_enable_interrupt_transfer_error(BRIDGE_PDCA_RX_CHANNEL);
    pdca_enable_interrupt_transfer_error(BRIDGE_PDCA_TX_CHANNEL);

    gpio_set_pin_high(PIN_EXPANSION_SYNC);

#ifdef BRIDGE_TIMEOUT
    timeout_count = 0;
#endif

    state = 2;
  }
  else if (state == 2)
  {

    // if(bridge_tx_ready && bridge_rx_ready)
    if (tx_channel->tcr == 0 && rx_channel->tcr == 0)
    {

      // pdca_disable(BRIDGE_PDCA_RX_CHANNEL);
      // pdca_disable(BRIDGE_PDCA_TX_CHANNEL);
      gpio_set_pin_low(PIN_EXPANSION_SYNC);

      // process packet
      if (rx_packet.crc != BRIDGE_CRC_MAGIC)
      {
        // bad packet
        state = 0;
#ifdef BRIDGE_SYNC_MESSAGES
        bridge_message(SEGDISP_LEFT_E | SEGDISP_RIGHT_1, SEGDISP_LEFT_S | SEGDISP_RIGHT_Y, SEGDISP_LEFT_N | SEGDISP_RIGHT_C);
#endif
      }
      else if (rx_packet.destination == BRIDGE_CONTROL_DESTINATION)
      {
        int i;
        memcpy(&bridge_control, rx_packet.data, sizeof(bridge_control_t));
        bridge_control.system = Swap16(bridge_control.system);
        bridge_control.leds = Swap16(bridge_control.leds);
        for (i = 0; i < NUM_CV; i++)
          bridge_control.cv[i] = Swap16(bridge_control.cv[i]);
        for (i = 0; i < SEGDISP_NUM; i++)
          bridge_control.segdisp[i] = Swap16(bridge_control.segdisp[i]);
        bridge_control.orange = Swap16(bridge_control.orange);
        bridge_control_has_changed = true;
        bit_set(bridge_state.system, BRIDGE_COMM_GOOD_PACKET);
        if (bit_get(bridge_control.system, BRIDGE_COMM_RESET))
        {
          state = 0;
#ifdef BRIDGE_SYNC_MESSAGES
          bridge_message(SEGDISP_LEFT_E | SEGDISP_RIGHT_2, SEGDISP_LEFT_S | SEGDISP_RIGHT_Y, SEGDISP_LEFT_N | SEGDISP_RIGHT_C);
#endif
        }
        else
        {
          state = 1;
        }
      }
      else
      {
        state = 0;
#ifdef BRIDGE_SYNC_MESSAGES
        bridge_message(SEGDISP_LEFT_E | SEGDISP_RIGHT_3, SEGDISP_LEFT_S | SEGDISP_RIGHT_Y, SEGDISP_LEFT_N | SEGDISP_RIGHT_C);
#endif
        bit_set(bridge_state.system, BRIDGE_COMM_UNKNOWN_DEST);
      }
    }
    else if (bridge_error || (EXPANSION_SPI->sr & (AVR32_SPI_SR_OVRES_MASK | AVR32_SPI_SR_UNDES_MASK)))
    {
      gpio_set_pin_low(PIN_EXPANSION_SYNC);
#ifdef BRIDGE_SYNC_MESSAGES
      bridge_message(SEGDISP_LEFT_E | SEGDISP_RIGHT_4, SEGDISP_LEFT_S | SEGDISP_RIGHT_Y, SEGDISP_LEFT_N | SEGDISP_RIGHT_C);
#endif
      state = 0;
    }
#ifdef BRIDGE_TIMEOUT
    else if (timeout_count >= BRIDGE_TIMEOUT)
    {
      gpio_set_pin_low(PIN_EXPANSION_SYNC);
      timeout_count = 0;
#ifdef BRIDGE_SYNC_MESSAGES
      bridge_message(SEGDISP_LEFT_E | SEGDISP_RIGHT_5, SEGDISP_LEFT_S | SEGDISP_RIGHT_Y, SEGDISP_LEFT_N | SEGDISP_RIGHT_C);
#endif
      state = 0;
    }
    else
    {
      timeout_count++;
      retval = true;
    }
#endif
  }

  return retval;
}

void bridge_clear_state(void)
{
  bridge_state.system = 0;
  bridge_state.pins = 0;
}

#define DEBOUNCE_THRESHOLD (WALLCLOCK_TICKS_PER_SEC / 10)
void bridge_event(uint8_t id, uint8_t e)
{
  if (e == EVENT_PUSH)
  {
    static wallclock_t lastPush = 0;
    wallclock_t now = wallclock();
    if (now - lastPush > DEBOUNCE_THRESHOLD)
    {
      bridge_q_elt_t tmp = {.as_struct.id = id, .as_struct.event = e};
      fifo_push_uint32(&event_q_desc, tmp.as_int);
    }
    lastPush = now;
  }
  else if (e == EVENT_RELEASE)
  {
    static wallclock_t lastRelease = 0;
    wallclock_t now = wallclock();
    if (now - lastRelease > DEBOUNCE_THRESHOLD)
    {
      bridge_q_elt_t tmp = {.as_struct.id = id, .as_struct.event = e};
      fifo_push_uint32(&event_q_desc, tmp.as_int);
    }
    lastRelease = now;
  }
  else
  {
    bridge_q_elt_t tmp = {.as_struct.id = id, .as_struct.event = e};
    fifo_push_uint32(&event_q_desc, tmp.as_int);
  }
}

static inline void apply_gate(wallclock_t now, uint8_t i, uint8_t gate)
{
  switch (gate)
  {
  case BRIDGE_GATE_NO_CHANGE:
    break;
  case BRIDGE_GATE_OFF:
    gpio_set_pin_low(seq.gate_output_pins[i]);
    break;
  case BRIDGE_GATE_ON:
    gpio_set_pin_high(seq.gate_output_pins[i]);
    break;
  default:
    // trigger
    gpio_set_pin_high(seq.gate_output_pins[i]);
    walltimer_schedule(now + gate * seq.trigger_unit, &(seq.trigger_timers[i]));
    break;
  }
}

static inline void apply_led(uint32_t led, uint32_t mask, uint32_t pin)
{
  if (bit_get(led, mask))
  {
    gpio_set_pin_low(pin);
  }
  else
  {
    gpio_set_pin_high(pin);
  }
}

void bridge_commit_control_changes(void)
{
  wallclock_t now = wallclock();

  extdac_set_value(0, bridge_control.cv[0]);
  extdac_set_value(1, bridge_control.cv[1]);
  extdac_set_value(2, bridge_control.cv[2]);
  extdac_set_value(3, bridge_control.cv[3]);
  extdac_set_value(4, bridge_control.cv[4]);
  extdac_set_value(5, bridge_control.cv[5]);
  extdac_set_value(6, bridge_control.cv[6]);
  extdac_set_value(7, bridge_control.cv[7]);

  apply_gate(now, 0, bridge_control.gate[0]);
  apply_gate(now, 1, bridge_control.gate[1]);
  apply_gate(now, 2, bridge_control.gate[2]);
  apply_gate(now, 3, bridge_control.gate[3]);

  memcpy(segdisp_display, bridge_control.segdisp, sizeof(bridge_control.segdisp));
  memcpy(&orange_display, &bridge_control.orange, sizeof(uint16_t));

  apply_led(bridge_control.leds, BRIDGE_SMOOTH_LED, PIN_SMOOTH_LED);
  apply_led(bridge_control.leds, BRIDGE_COPY_LED, PIN_COPY_LED);
  apply_led(bridge_control.leds, BRIDGE_PAUSE_LED, PIN_PAUSE_LED);
  apply_led(bridge_control.leds, BRIDGE_COMMIT_LED, PIN_COMMIT_LED);
  apply_led(bridge_control.leds, BRIDGE_LEND_LED, PIN_LEND_LED);
  apply_led(bridge_control.leds, BRIDGE_LSTART_LED, PIN_LSTART_LED);

  bridge_control_has_changed = false;
}

static inline void apply_pin_state(uint32_t pin, uint32_t mask)
{
  if (gpio_pin_is_high(pin))
  {
    bit_set(bridge_state.pins, mask);
  }
  else
  {
    bit_clear(bridge_state.pins, mask);
  }
}

void bridge_copy_system_state(void)
{
  bridge_q_elt_t item;

  event_index = 0;
  while (fifo_pull_uint32(&event_q_desc, &(item.as_int)) == FIFO_OK)
  {
    if (event_index < BRIDGE_EVENT_QUEUE_SIZE)
    {
      bridge_state.events[event_index] = item.as_struct;
      event_index++;
    }
  }

  // mark the end of the queue
  if (event_index < BRIDGE_EVENT_QUEUE_SIZE)
  {
    bridge_state.events[event_index].id = BRIDGE_NULL_ID;
  }

  apply_pin_state(PIN_DURATION_BUTTON, BRIDGE_DURATION_BUTTON);
  apply_pin_state(PIN_GATE_BUTTON, BRIDGE_GATE_BUTTON);
  apply_pin_state(PIN_CVA_BUTTON, BRIDGE_CVA_BUTTON);
  apply_pin_state(PIN_CVB_BUTTON, BRIDGE_CVB_BUTTON);
  apply_pin_state(PIN_MODE_A, BRIDGE_MODE_A);
  apply_pin_state(PIN_MODE_B, BRIDGE_MODE_B);
  apply_pin_state(PIN_TABLE_A, BRIDGE_TABLE_A);
  apply_pin_state(PIN_TABLE_B, BRIDGE_TABLE_B);
  apply_pin_state(PIN_TRACK_BUTTON, BRIDGE_TRACK_BUTTON);
  apply_pin_state(PIN_PATTERN_BUTTON, BRIDGE_PATTERN_BUTTON);
  apply_pin_state(PIN_STEP_BUTTON, BRIDGE_STEP_BUTTON);
}

extern volatile wallclock_t display_freeze;
void bridge_message(uint16_t modifier, uint16_t left, uint16_t right)
{
  segdisp_display[SEGDISP_VOLT1] = left;
  segdisp_display[SEGDISP_VOLT0] = right;
  segdisp_display[SEGDISP_INDEX] = modifier;
  wait_for_display_to_refresh();
  // wallclock_delay(WALLCLOCK_TICKS_PER_SEC/2);
  display_freeze = wallclock() + WALLCLOCK_TICKS_PER_SEC / 2;
}