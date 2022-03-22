/*
 * interrupt_handlers.c
 *
 * Created: 8/3/2014 8:48:35 PM
 *  Author: clarkson
 */
#include <asf.h>
#include <wallclock.h>
#include <interrupt_handlers.h>
#include <sequencer.h>
#include <ui_events.h>
#include <event_handlers.h>
#include <display.h>
#include <bridge.h>

#define DEBUG_COMMANDS_ENABLED false

__attribute__((__interrupt__)) static void left_qdec_handler(void)
{
  // uint16_t rc = qdec_read_rc(&LEFT_ENCODER);
  // segdisp_put_number(SEGDISP_PATTERN,rc%100);
  uint16_t pc = qdec_read_pc(&LEFT_ENCODER);
  // segdisp_put_number(SEGDISP_POSITION,pc%100);

  LEFT_ENCODER.scr = BIT(AVR32_QDEC_SCR_PCRO_OFFSET);
  LEFT_ENCODER.scr = BIT(AVR32_QDEC_SCR_RCRO_OFFSET);

  if (pc == 0)
  {
    ui_q_push(ui_left_increment);
  }
  else
  {
    ui_q_push(ui_left_decrement);
  }
}

__attribute__((__interrupt__)) static void right_qdec_handler(void)
{
  // static int counter = 0;

  // uint16_t rc = qdec_read_rc(&RIGHT_ENCODER);
  // segdisp_put_number(SEGDISP_CVA,rc%100);
  uint16_t pc = qdec_read_pc(&RIGHT_ENCODER);
  // segdisp_put_number(SEGDISP_CVB,pc%100);

  RIGHT_ENCODER.scr = BIT(AVR32_QDEC_SCR_PCRO_OFFSET);
  RIGHT_ENCODER.scr = BIT(AVR32_QDEC_SCR_RCRO_OFFSET);

  if (pc == 0)
  {
    ui_q_push(ui_right_decrement);
  }
  else
  {
    ui_q_push(ui_right_increment);
  }

  // for(int i=0;i<8;i++) extdac_set_value(i,counter);
}

__attribute__((__interrupt__)) static void gpio_handler0(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_LSTART_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_LSTART_BUTTON);
    if (gpio_pin_is_low(PIN_LSTART_BUTTON))
    {
      ui_q_debounce(ui_lstart_button_push, EVENT_TIMESTAMP_LSTART);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_LEND_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_LEND_BUTTON);
    if (gpio_pin_is_low(PIN_LEND_BUTTON))
    {
      ui_q_debounce(ui_lend_button_push, EVENT_TIMESTAMP_LEND);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_SNAPSHOT_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_SNAPSHOT_BUTTON);
    if (gpio_pin_is_low(PIN_SNAPSHOT_BUTTON))
    {
      on_select_left_focus(SEGDISP_SNAPSHOT);
    }
  }
}

__attribute__((__interrupt__)) static void gpio_handler1(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_RESET_INPUT))
  {
    gpio_clear_pin_interrupt_flag(PIN_RESET_INPUT);
    if (wallclock() - clock_arrival_time < seq.soft_reset_interval)
    {
      ui_q_push(ext_reset_after);
    }
    else
    {
      ui_q_push(ext_reset_before);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_COMMIT_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_COMMIT_BUTTON);
    if (gpio_pin_is_low(PIN_COMMIT_BUTTON))
    {
      ui_q_debounce(ui_commit_button_push, EVENT_TIMESTAMP_COMMIT);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_MODE_A))
  {
    gpio_clear_pin_interrupt_flag(PIN_MODE_A);
    ui_q_push(ui_mode_switch);
  }

  if (gpio_get_pin_interrupt_flag(PIN_MODE_B))
  {
    gpio_clear_pin_interrupt_flag(PIN_MODE_B);
    ui_q_push(ui_mode_switch);
  }
}

__attribute__((__interrupt__)) static void gpio_handler2(void)
{
  static int counter = 0;

  if (gpio_get_pin_interrupt_flag(PIN_CLOCK_INPUT))
  {
    gpio_clear_pin_interrupt_flag(PIN_CLOCK_INPUT);
    // estimate the current clock period
    clock_arrival_time = wallclock();

    if (clock_arrival_time > seq.last_clock_timestamp)
    {
      unsigned long this_period = clock_arrival_time - seq.last_clock_timestamp;
      if (this_period > seq.last_period / 2 && this_period < 2 * seq.last_period)
      {
        seq.latched_period = this_period;
      }
      else if (seq.latched_period == 0)
      {
        seq.latched_period = this_period;
      }
      seq.last_period = this_period;
    }
    else
    {
      tilt_trigger();
    }
    seq.last_clock_timestamp = clock_arrival_time;

    // queue this handler so that it doesn't fight with the extdac buffers
    ui_q_push(on_clock_event);

    if (!seq.math_overlay_active)
    {
      counter++;
      if (counter == 6)
        counter = 0;
      segdisp_display[SEGDISP_TRACK] &= 0xFF00;
      segdisp_display[SEGDISP_TRACK] |= segdisp_left_clock[counter];
    }

    seq.waiting_for_clock_after_reset = false;
  }
}

__attribute__((__interrupt__)) static void gpio_handler4(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_INDEX_BUTTON))
  {
    if (gpio_pin_is_low(PIN_INDEX_BUTTON))
    {
      on_select_left_focus(SEGDISP_INDEX);
    }
    gpio_clear_pin_interrupt_flag(PIN_INDEX_BUTTON);
  }

  if (gpio_get_pin_interrupt_flag(PIN_VOLTAGE_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_VOLTAGE_BUTTON);
    if (gpio_pin_is_low(PIN_VOLTAGE_BUTTON))
    {
      // button pushed
      ui_q_debounce(ui_voltage_button_push, EVENT_TIMESTAMP_VOLTAGE);
    }
    else
    {
      // button released
      ui_q_debounce(ui_voltage_button_release, EVENT_TIMESTAMP_VOLTAGE_RELEASE);
    }
  }
}

__attribute__((__interrupt__)) static void gpio_handler6(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_DURATION_BUTTON))
  {
    if (gpio_pin_is_low(PIN_DURATION_BUTTON))
    {
      on_select_right_focus(SEGDISP_DURATION);
    }
    else
    {
      // seq_display_voltage();
    }
    gpio_clear_pin_interrupt_flag(PIN_DURATION_BUTTON);
  }

  if (gpio_get_pin_interrupt_flag(PIN_GATE_BUTTON))
  {
    if (gpio_pin_is_low(PIN_GATE_BUTTON))
    {
      on_select_right_focus(SEGDISP_GATE);
    }
    gpio_clear_pin_interrupt_flag(PIN_GATE_BUTTON);
  }
}

__attribute__((__interrupt__)) static void gpio_handler8(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_STEP_BUTTON))
  {
    if (gpio_pin_is_low(PIN_STEP_BUTTON))
    {
      on_select_left_focus(SEGDISP_STEP);
    }
    gpio_clear_pin_interrupt_flag(PIN_STEP_BUTTON);
  }
}

__attribute__((__interrupt__)) static void gpio_handler10(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_SAVE_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_SAVE_BUTTON);
    if (gpio_pin_is_low(PIN_SAVE_BUTTON))
    {
      ui_q_debounce(ui_save_button_push, EVENT_TIMESTAMP_SAVE);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_COPY_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_COPY_BUTTON);
    if (gpio_pin_is_high(PIN_COPY_BUTTON))
    {
      ui_q_push_no_repeat(ui_copy_button_release);
    }
    else
    {
      ui_q_push_no_repeat(ui_copy_button_push);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_CVB_BUTTON))
  {
    if (gpio_pin_is_low(PIN_CVB_BUTTON))
    {
      on_select_right_focus(SEGDISP_CVB);
    }
    gpio_clear_pin_interrupt_flag(PIN_CVB_BUTTON);
  }

  if (gpio_get_pin_interrupt_flag(PIN_LOAD_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_LOAD_BUTTON);
    if (gpio_pin_is_low(PIN_LOAD_BUTTON))
    {
      ui_q_debounce(ui_load_button_push, EVENT_TIMESTAMP_LOAD);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_PAUSE_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_PAUSE_BUTTON);
    if (gpio_pin_is_low(PIN_PAUSE_BUTTON))
    {
      ui_q_debounce(ui_pause_button_push, EVENT_TIMESTAMP_PAUSE);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_RESET_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_RESET_BUTTON);
    if (gpio_pin_is_low(PIN_RESET_BUTTON))
    {
      if (gpio_pin_is_low(PIN_TRACK_BUTTON))
      {
        seq.reset_quantization = QUANTIZE_TO_TRACK;
      }
      else if (gpio_pin_is_low(PIN_PATTERN_BUTTON))
      {
        seq.reset_quantization = QUANTIZE_TO_PATTERN;
      }
      else if (gpio_pin_is_low(PIN_STEP_BUTTON))
      {
        seq.reset_quantization = QUANTIZE_TO_STEP;
      }
      else
      {
        seq.reset_quantization = NO_QUANTIZATION;
      }
      ui_q_debounce(ui_reset_button_push, EVENT_TIMESTAMP_RESET);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_MATH_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_MATH_BUTTON);
    if (gpio_pin_is_low(PIN_MATH_BUTTON))
    {
      // button pushed
      ui_q_debounce(ui_math_button_push, EVENT_TIMESTAMP_MATH);
    }
    else
    {
      // button released
      ui_q_debounce(ui_math_button_release, EVENT_TIMESTAMP_MATH_RELEASE);
    }
  }
}

__attribute__((__interrupt__)) static void gpio_handler11(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_PATTERN_BUTTON))
  {
    if (gpio_pin_is_low(PIN_PATTERN_BUTTON))
    {
      on_select_left_focus(SEGDISP_PATTERN);
    }
    gpio_clear_pin_interrupt_flag(PIN_PATTERN_BUTTON);
  }
}

__attribute__((__interrupt__)) static void gpio_handler12(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_DELETE_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_DELETE_BUTTON);
    if (gpio_pin_is_low(PIN_DELETE_BUTTON))
    {
      ui_q_debounce(ui_delete_button_push, EVENT_TIMESTAMP_DELETE);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_INSERT_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_INSERT_BUTTON);
    if (gpio_pin_is_high(PIN_INSERT_BUTTON))
    {
      ui_q_debounce(ui_insert_button_release, EVENT_TIMESTAMP_INSERT_RELEASE);
    }
    else
    {
      ui_q_debounce(ui_insert_button_push, EVENT_TIMESTAMP_INSERT);
    }
  }
}

__attribute__((__interrupt__)) static void gpio_handler13(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_TABLE_A))
  {
    ui_q_push(ui_table_switch);
    gpio_clear_pin_interrupt_flag(PIN_TABLE_A);
  }

  if (gpio_get_pin_interrupt_flag(PIN_TABLE_B))
  {
    ui_q_push(ui_table_switch);
    gpio_clear_pin_interrupt_flag(PIN_TABLE_B);
  }
}

__attribute__((__interrupt__)) static void gpio_handler14(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_TRACK_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_TRACK_BUTTON);
    if (gpio_pin_is_high(PIN_TRACK_BUTTON))
    {
      ui_q_debounce(ui_track_button_release, EVENT_TIMESTAMP_TRACK_RELEASE);
    }
    else
    {
      ui_q_debounce(ui_track_button_push, EVENT_TIMESTAMP_TRACK);
    }
  }

  if (gpio_get_pin_interrupt_flag(PIN_SMOOTH_BUTTON))
  {
    gpio_clear_pin_interrupt_flag(PIN_SMOOTH_BUTTON);
    if (gpio_pin_is_low(PIN_SMOOTH_BUTTON))
    {
      ui_q_debounce(ui_smooth_button_push, EVENT_TIMESTAMP_SMOOTH);
    }
  }
}

__attribute__((__interrupt__)) static void gpio_handler15(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_CVA_BUTTON))
  {
    if (gpio_pin_is_low(PIN_CVA_BUTTON))
    {
      on_select_right_focus(SEGDISP_CVA);
    }
    gpio_clear_pin_interrupt_flag(PIN_CVA_BUTTON);
  }
}

////////////////////////////////////////////////////////////////////////////////////////
// slave mode handlers

__attribute__((__interrupt__)) static void left_qdec_handler_slave(void)
{
  uint16_t pc = qdec_read_pc(&LEFT_ENCODER);

  LEFT_ENCODER.scr = BIT(AVR32_QDEC_SCR_PCRO_OFFSET);
  LEFT_ENCODER.scr = BIT(AVR32_QDEC_SCR_RCRO_OFFSET);

  if (pc == 0)
  {
    bridge_event(ID_LEFT_ENCODER, EVENT_INCREMENT);
  }
  else
  {
    bridge_event(ID_LEFT_ENCODER, EVENT_DECREMENT);
  }
}

__attribute__((__interrupt__)) static void right_qdec_handler_slave(void)
{
  uint16_t pc = qdec_read_pc(&RIGHT_ENCODER);

  if (pc == 0)
  {
    bridge_event(ID_RIGHT_ENCODER, EVENT_DECREMENT);
  }
  else
  {
    bridge_event(ID_RIGHT_ENCODER, EVENT_INCREMENT);
  }

  RIGHT_ENCODER.scr = BIT(AVR32_QDEC_SCR_PCRO_OFFSET);
  RIGHT_ENCODER.scr = BIT(AVR32_QDEC_SCR_RCRO_OFFSET);
}

static inline void handle_button_irq(uint32_t pin, uint8_t id)
{
  if (gpio_get_pin_interrupt_flag(pin))
  {
    gpio_clear_pin_interrupt_flag(pin);
    if (gpio_pin_is_low(pin))
    {
      bridge_event(id, EVENT_PUSH);
    }
    else
    {
      bridge_event(id, EVENT_RELEASE);
    }
  }
}

__attribute__((__interrupt__)) static void gpio_handler0_slave(void)
{
  handle_button_irq(PIN_LSTART_BUTTON, ID_LSTART_BUTTON);
  handle_button_irq(PIN_LEND_BUTTON, ID_LEND_BUTTON);
  handle_button_irq(PIN_SNAPSHOT_BUTTON, ID_SNAPSHOT_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler1_slave(void)
{

  if (gpio_get_pin_interrupt_flag(PIN_RESET_INPUT))
  {
    gpio_clear_pin_interrupt_flag(PIN_RESET_INPUT);
    bridge_event(ID_RESET_INPUT, EVENT_RISING_EDGE);
  }

  handle_button_irq(PIN_COMMIT_BUTTON, ID_COMMIT_BUTTON);

  if (gpio_get_pin_interrupt_flag(PIN_MODE_A) || gpio_get_pin_interrupt_flag(PIN_MODE_B))
  {
    gpio_clear_pin_interrupt_flag(PIN_MODE_A);
    gpio_clear_pin_interrupt_flag(PIN_MODE_B);

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
  }
}

__attribute__((__interrupt__)) static void gpio_handler2_slave(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_CLOCK_INPUT))
  {
    gpio_clear_pin_interrupt_flag(PIN_CLOCK_INPUT);
    bridge_event(ID_CLOCK_INPUT, EVENT_RISING_EDGE);
  }
}

__attribute__((__interrupt__)) static void gpio_handler4_slave(void)
{
  handle_button_irq(PIN_INDEX_BUTTON, ID_INDEX_BUTTON);
  handle_button_irq(PIN_VOLTAGE_BUTTON, ID_VOLTAGE_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler6_slave(void)
{
  handle_button_irq(PIN_DURATION_BUTTON, ID_DURATION_BUTTON);
  handle_button_irq(PIN_GATE_BUTTON, ID_GATE_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler8_slave(void)
{
  handle_button_irq(PIN_STEP_BUTTON, ID_STEP_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler10_slave(void)
{
  handle_button_irq(PIN_SAVE_BUTTON, ID_SAVE_BUTTON);
  handle_button_irq(PIN_COPY_BUTTON, ID_COPY_BUTTON);
  handle_button_irq(PIN_CVB_BUTTON, ID_CVB_BUTTON);
  handle_button_irq(PIN_LOAD_BUTTON, ID_LOAD_BUTTON);
  handle_button_irq(PIN_PAUSE_BUTTON, ID_PAUSE_BUTTON);
  handle_button_irq(PIN_RESET_BUTTON, ID_RESET_BUTTON);
  handle_button_irq(PIN_MATH_BUTTON, ID_MATH_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler11_slave(void)
{
  handle_button_irq(PIN_PATTERN_BUTTON, ID_PATTERN_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler12_slave(void)
{
  handle_button_irq(PIN_DELETE_BUTTON, ID_DELETE_BUTTON);
  handle_button_irq(PIN_INSERT_BUTTON, ID_INSERT_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler13_slave(void)
{
  if (gpio_get_pin_interrupt_flag(PIN_TABLE_A) || gpio_get_pin_interrupt_flag(PIN_TABLE_B))
  {
    gpio_clear_pin_interrupt_flag(PIN_TABLE_A);
    gpio_clear_pin_interrupt_flag(PIN_TABLE_B);

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
  }
}

__attribute__((__interrupt__)) static void gpio_handler14_slave(void)
{
  handle_button_irq(PIN_TRACK_BUTTON, ID_TRACK_BUTTON);
  handle_button_irq(PIN_SMOOTH_BUTTON, ID_SMOOTH_BUTTON);
}

__attribute__((__interrupt__)) static void gpio_handler15_slave(void)
{
  handle_button_irq(PIN_CVA_BUTTON, ID_CVA_BUTTON);
}

#define COMPARE_PERIOD (10 * CPU_HZ)

__attribute__((__interrupt__)) static void compare_irq_handler(void)
{
  Set_system_register(AVR32_COMPARE, COMPARE_PERIOD);
  wallclock_base += COMPARE_PERIOD;
  // gpio_toggle_pin(PIN_COMMIT_LED);
}

volatile bool extdac_ready = false;
__attribute__((__interrupt__)) static void pdca_extdac_handler(void)
{
  pdca_disable_interrupt_transfer_complete(EXTDAC_PDCA_CHANNEL);
  extdac_ready = true;
}

volatile bool segdisp_ready = false;
__attribute__((__interrupt__)) static void pdca_display_handler(void)
{
  pdca_disable_interrupt_transfer_complete(DISPLAY_PDCA_CHANNEL);
  segdisp_ready = true;
}

volatile bool bridge_error = false;
volatile bool bridge_tx_ready = false;
__attribute__((__interrupt__)) static void pdca_bridge_tx_handler(void)
{
  uint32_t status = pdca_get_transfer_status(BRIDGE_PDCA_TX_CHANNEL);

  if (status & PDCA_TRANSFER_ERROR)
  {
    pdca_disable_interrupt_transfer_error(BRIDGE_PDCA_TX_CHANNEL);
    bridge_error = true;
  }

  if (status & PDCA_TRANSFER_COMPLETE)
  {
    pdca_disable_interrupt_transfer_complete(BRIDGE_PDCA_TX_CHANNEL);
    bridge_tx_ready = true;
  }
}

volatile bool bridge_rx_ready = false;
__attribute__((__interrupt__)) static void pdca_bridge_rx_handler(void)
{
  uint32_t status = pdca_get_transfer_status(BRIDGE_PDCA_RX_CHANNEL);

  if (status & PDCA_TRANSFER_ERROR)
  {
    pdca_disable_interrupt_transfer_error(BRIDGE_PDCA_RX_CHANNEL);
    bridge_error = true;
  }

  if (status & PDCA_TRANSFER_COMPLETE)
  {
    pdca_disable_interrupt_transfer_complete(BRIDGE_PDCA_RX_CHANNEL);
    bridge_rx_ready = true;
  }
}

void configure_early_interrupts(bool slavemode)
{
  // Disable_global_interrupt();
  INTC_init_interrupts();
  INTC_register_interrupt(&pdca_display_handler, AVR32_PDCA_IRQ_3, AVR32_INTC_INT3);
  // Enable_global_interrupt();
}

void configure_late_interrupts(bool slavemode)
{
  Disable_global_interrupt();

  INTC_register_interrupt(&compare_irq_handler, AVR32_CORE_COMPARE_IRQ, AVR32_INTC_INT3);
  Set_system_register(AVR32_COMPARE, COMPARE_PERIOD);
  Set_system_register(AVR32_COUNT, 0);
  wallclock_base = 0;

  // gpio irq 0
  gpio_enable_pin_interrupt(PIN_LEND_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_LSTART_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 1
  gpio_enable_pin_interrupt(PIN_MODE_A, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_MODE_B, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_RESET_INPUT, GPIO_RISING_EDGE);
  gpio_enable_pin_interrupt(PIN_COMMIT_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 2
  gpio_enable_pin_interrupt(PIN_CLOCK_INPUT, GPIO_RISING_EDGE);

  // gpio irq 4
  gpio_enable_pin_interrupt(PIN_VOLTAGE_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_INDEX_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 6
  gpio_enable_pin_interrupt(PIN_DURATION_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_GATE_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 7
  gpio_enable_pin_interrupt(PIN_STEP_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_SNAPSHOT_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 10
  gpio_enable_pin_interrupt(PIN_CVB_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_MATH_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_COPY_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_LOAD_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_SAVE_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_PAUSE_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_RESET_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 11
  gpio_enable_pin_interrupt(PIN_PATTERN_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 12
  gpio_enable_pin_interrupt(PIN_INSERT_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_DELETE_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 13
  gpio_enable_pin_interrupt(PIN_TABLE_A, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_TABLE_B, GPIO_PIN_CHANGE);

  // gpio irq 14
  gpio_enable_pin_interrupt(PIN_TRACK_BUTTON, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt(PIN_SMOOTH_BUTTON, GPIO_PIN_CHANGE);

  // gpio irq 15
  gpio_enable_pin_interrupt(PIN_CVA_BUTTON, GPIO_PIN_CHANGE);

  if (slavemode)
  {
    INTC_register_interrupt(&gpio_handler0_slave, AVR32_GPIO_IRQ_0, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler1_slave, AVR32_GPIO_IRQ_1, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler2_slave, AVR32_GPIO_IRQ_2, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler4_slave, AVR32_GPIO_IRQ_4, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler6_slave, AVR32_GPIO_IRQ_6, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler8_slave, AVR32_GPIO_IRQ_8, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler10_slave, AVR32_GPIO_IRQ_10, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler11_slave, AVR32_GPIO_IRQ_11, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler12_slave, AVR32_GPIO_IRQ_12, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler13_slave, AVR32_GPIO_IRQ_13, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler14_slave, AVR32_GPIO_IRQ_14, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler15_slave, AVR32_GPIO_IRQ_15, AVR32_INTC_INT1);

    // Register the QDEC interrupt handler to the interrupt controller.
    INTC_register_interrupt(&left_qdec_handler_slave, AVR32_QDEC0_IRQ, AVR32_INTC_INT0);
    INTC_register_interrupt(&right_qdec_handler_slave, AVR32_QDEC1_IRQ, AVR32_INTC_INT0);

    INTC_register_interrupt(&pdca_bridge_tx_handler, BRIDGE_PDCA_TX_IRQ, AVR32_INTC_INT3);
    INTC_register_interrupt(&pdca_bridge_rx_handler, BRIDGE_PDCA_RX_IRQ, AVR32_INTC_INT3);
  }
  else
  {
    INTC_register_interrupt(&gpio_handler0, AVR32_GPIO_IRQ_0, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler1, AVR32_GPIO_IRQ_1, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler2, AVR32_GPIO_IRQ_2, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler4, AVR32_GPIO_IRQ_4, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler6, AVR32_GPIO_IRQ_6, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler8, AVR32_GPIO_IRQ_8, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler10, AVR32_GPIO_IRQ_10, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler11, AVR32_GPIO_IRQ_11, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler12, AVR32_GPIO_IRQ_12, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler13, AVR32_GPIO_IRQ_13, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler14, AVR32_GPIO_IRQ_14, AVR32_INTC_INT1);
    INTC_register_interrupt(&gpio_handler15, AVR32_GPIO_IRQ_15, AVR32_INTC_INT1);

    // Register the QDEC interrupt handler to the interrupt controller.
    INTC_register_interrupt(&left_qdec_handler, AVR32_QDEC0_IRQ, AVR32_INTC_INT0);
    INTC_register_interrupt(&right_qdec_handler, AVR32_QDEC1_IRQ, AVR32_INTC_INT0);
  }

  INTC_register_interrupt(&pdca_extdac_handler, AVR32_PDCA_IRQ_2, AVR32_INTC_INT3);
  Enable_global_interrupt();
}