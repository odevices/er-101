/*
 * display.c
 *
 * Created: 5/7/2013 12:13:01 PM
 *  Author: clarkson
 */

#include <asf.h>
#include <display.h>
#include <voltages.h>

extern volatile bool segdisp_ready;

const uint16_t display_to_led[] = {
    ORANGE_VOLT0,
    ORANGE_VOLT1,
    ORANGE_CVB,
    ORANGE_CVA,
    ORANGE_GATE,
    ORANGE_DURATION,
    ORANGE_SNAPSHOT,
    ORANGE_PATTERN,
    ORANGE_STEP,
    ORANGE_TRACK,
    ORANGE_INDEX};

const uint16_t segdisp_both_digits[100] = {
    0x7777, 0x4177, 0x3B77, 0x6B77, 0x4D77, 0x6E77, 0x7E77, 0x4377, 0x7F77, 0x6F77,
    0x7714, 0x4114, 0x3B14, 0x6B14, 0x4D14, 0x6E14, 0x7E14, 0x4314, 0x7F14, 0x6F14,
    0x77B3, 0x41B3, 0x3BB3, 0x6BB3, 0x4DB3, 0x6EB3, 0x7EB3, 0x43B3, 0x7FB3, 0x6FB3,
    0x77B6, 0x41B6, 0x3BB6, 0x6BB6, 0x4DB6, 0x6EB6, 0x7EB6, 0x43B6, 0x7FB6, 0x6FB6,
    0x77D4, 0x41D4, 0x3BD4, 0x6BD4, 0x4DD4, 0x6ED4, 0x7ED4, 0x43D4, 0x7FD4, 0x6FD4,
    0x77E6, 0x41E6, 0x3BE6, 0x6BE6, 0x4DE6, 0x6EE6, 0x7EE6, 0x43E6, 0x7FE6, 0x6FE6,
    0x77E7, 0x41E7, 0x3BE7, 0x6BE7, 0x4DE7, 0x6EE7, 0x7EE7, 0x43E7, 0x7FE7, 0x6FE7,
    0x7734, 0x4134, 0x3B34, 0x6B34, 0x4D34, 0x6E34, 0x7E34, 0x4334, 0x7F34, 0x6F34,
    0x77F7, 0x41F7, 0x3BF7, 0x6BF7, 0x4DF7, 0x6EF7, 0x7EF7, 0x43F7, 0x7FF7, 0x6FF7,
    0x77F6, 0x41F6, 0x3BF6, 0x6BF6, 0x4DF6, 0x6EF6, 0x7EF6, 0x43F6, 0x7FF6, 0x6FF6};

const uint16_t segdisp_left_digits[10] = {
    0x0077, 0x0014, 0x00B3, 0x00B6, 0x00D4, 0x00E6, 0x00E7, 0x0034, 0x00F7, 0x00F6};

const uint16_t segdisp_right_digits[10] = {
    0x7700, 0x4100, 0x3B00, 0x6B00, 0x4D00, 0x6E00, 0x7E00, 0x4300, 0x7F00, 0x6F00};

const uint16_t segdisp_left_clock[] = {
    SEGDISP_LEFT_CLOCK0,
    SEGDISP_LEFT_CLOCK1,
    SEGDISP_LEFT_CLOCK2,
    SEGDISP_LEFT_CLOCK3,
    SEGDISP_LEFT_CLOCK4,
    SEGDISP_LEFT_CLOCK5};

const uint16_t segdisp_right_clock[] = {
    SEGDISP_RIGHT_CLOCK0,
    SEGDISP_RIGHT_CLOCK1,
    SEGDISP_RIGHT_CLOCK2,
    SEGDISP_RIGHT_CLOCK3,
    SEGDISP_RIGHT_CLOCK4,
    SEGDISP_RIGHT_CLOCK5};

const uint16_t segdisp_infinite_clock[] = {
    SEGDISP_LEFT_SCLOCK2 | SEGDISP_RIGHT_SCLOCK5,
    SEGDISP_LEFT_SCLOCK3 | SEGDISP_LEFT_SCLOCK2,
    SEGDISP_LEFT_SCLOCK4 | SEGDISP_LEFT_SCLOCK3,
    SEGDISP_LEFT_SCLOCK5 | SEGDISP_LEFT_SCLOCK4,
    SEGDISP_LEFT_SCLOCK0 | SEGDISP_LEFT_SCLOCK5,
    SEGDISP_LEFT_SCLOCK1 | SEGDISP_LEFT_SCLOCK0,
    SEGDISP_RIGHT_SCLOCK4 | SEGDISP_LEFT_SCLOCK1,
    SEGDISP_RIGHT_SCLOCK3 | SEGDISP_RIGHT_SCLOCK4,
    SEGDISP_RIGHT_SCLOCK2 | SEGDISP_RIGHT_SCLOCK3,
    SEGDISP_RIGHT_SCLOCK1 | SEGDISP_RIGHT_SCLOCK2,
    SEGDISP_RIGHT_SCLOCK0 | SEGDISP_RIGHT_SCLOCK1,
    SEGDISP_RIGHT_SCLOCK5 | SEGDISP_RIGHT_SCLOCK0};

uint8_t segdisp_buffer[22] = {0};
uint16_t segdisp_display[11] = {0};
uint8_t orange_buffer[2] = {0};
uint16_t orange_display;

uint8_t focus_left_display = 0;
uint8_t focus_right_display = 0;
uint8_t saved_left_display = 0;
uint8_t saved_right_display = 0;

uint8_t flash_display = 0;
uint8_t flash_state = 0;
uint16_t flash_value = 0;
uint16_t flash_led = 0;

void flash(uint8_t orange, uint8_t display, uint16_t value)
{
  flash_led = display_to_led[orange];
  flash_display = display;
  flash_value = value;
  flash_state = 6;
}

bool display_feed(void)
{
  static uint8_t state = 0;

  if (state == 0)
  {
    if (usart_tx_empty(&USART_DISPLAY))
    {
      gpio_set_pin_high(PIN_SELECT_SEGDISP);
      cpu_delay_us(1, CPU_HZ);
      gpio_set_pin_low(PIN_SELECT_SEGDISP);

      memcpy(orange_buffer, &orange_display, sizeof(orange_buffer));

      // overlay
      if (flash_state > 0)
      {
        bool toggle = flash_state % 2 == 0;
        uint16_t *tmp = (uint16_t *)orange_buffer;
        if (toggle)
        {
          tmp[0] |= flash_led;
        }
        else
        {
          tmp[0] &= ~flash_led;
        }
      }

      pdca_reload_channel(DISPLAY_PDCA_CHANNEL, (void *)orange_buffer, sizeof(orange_buffer));
      state = 1;
      segdisp_ready = false;
      pdca_enable_interrupt_transfer_complete(DISPLAY_PDCA_CHANNEL);

      return true;
    }

    return false;
  }
  else if (state == 1)
  {
    if (usart_tx_empty(&USART_DISPLAY))
    {
      gpio_set_pin_high(PIN_SELECT_ORANGE);
      cpu_delay_us(1, CPU_HZ);
      gpio_set_pin_low(PIN_SELECT_ORANGE);

      memcpy(segdisp_buffer, segdisp_display, sizeof(segdisp_buffer));
      // overlay
      if (flash_state > 0)
      {
        bool toggle = flash_state % 2 == 0;
        uint16_t *tmp = (uint16_t *)segdisp_buffer;
        if (toggle)
        {
          tmp[flash_display] = flash_value;
        }
        else
        {
          tmp[flash_display] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
        }
      }
      pdca_reload_channel(DISPLAY_PDCA_CHANNEL, (void *)segdisp_buffer, sizeof(segdisp_buffer));
      state = 0;
      segdisp_ready = false;
      pdca_enable_interrupt_transfer_complete(DISPLAY_PDCA_CHANNEL);
    }

    return false;
  }

  return false;
}

void display_dma_start(void)
{
  int i;

  for (i = 0; i < 22; i++)
    segdisp_buffer[i] = 0;
  orange_buffer[0] = 0;
  orange_buffer[1] = 0;

  pdca_channel_options_t TX_OPTIONS = {
      /* Select peripheral - data is transmitted on USART TX line */
      .pid = AVR32_PDCA_PID_USART1_TX,
      /* Select size of the transfer */
      .transfer_size = PDCA_TRANSFER_SIZE_BYTE,

      /* Memory address */
      .addr = NULL,
      /* Transfer counter */
      .size = 0,

      /* Next memory address */
      .r_addr = segdisp_buffer,
      /* Next transfer counter */
      .r_size = sizeof(segdisp_buffer),
  };

  /* Initialize the PDCA channels with the requested options. */
  pdca_init_channel(DISPLAY_PDCA_CHANNEL, &TX_OPTIONS);
  segdisp_ready = false;
  pdca_enable_interrupt_transfer_complete(DISPLAY_PDCA_CHANNEL);
  pdca_enable(DISPLAY_PDCA_CHANNEL);
}

void wait_for_display_to_refresh(void)
{
  while (!display_feed())
    ;
  while (!display_feed())
    ;
}

void orange_clear(int display)
{
  if (display == SEGDISP_VOLT01)
  {
    orange_clear(SEGDISP_VOLT0);
    orange_clear(SEGDISP_VOLT1);
  }
  else
  {
    uint16_t mask = display_to_led[display];
    orange_display &= ~mask;
  }
}

void orange_set(int display)
{
  if (display == SEGDISP_VOLT01)
  {
    orange_set(SEGDISP_VOLT0);
    orange_set(SEGDISP_VOLT1);
  }
  else
  {
    uint16_t mask = display_to_led[display];
    orange_display |= mask;
  }
}

void display_init(void)
{
  segdisp_display[SEGDISP_TRACK] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_PATTERN] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_STEP] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_SNAPSHOT] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_INDEX] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_CVA] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_CVB] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_DURATION] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_GATE] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
}

void select_left_display(int display)
{
  orange_clear(SEGDISP_INDEX);
  orange_clear(SEGDISP_TRACK);
  orange_clear(SEGDISP_PATTERN);
  orange_clear(SEGDISP_STEP);
  orange_clear(SEGDISP_SNAPSHOT);
  saved_left_display = display;
  focus_left_display = display;
  orange_set(focus_left_display);
}

void select_right_display(int display)
{
  orange_clear(focus_right_display);
  saved_right_display = display;
  focus_right_display = display;
  orange_set(focus_right_display);
}

void highlight_left_display(int display)
{
  orange_clear(SEGDISP_INDEX);
  orange_clear(SEGDISP_TRACK);
  orange_clear(SEGDISP_PATTERN);
  orange_clear(SEGDISP_STEP);
  orange_clear(SEGDISP_SNAPSHOT);
  orange_set(display);
}

void highlight_right_display(int display)
{
  orange_clear(SEGDISP_CVA);
  orange_clear(SEGDISP_CVB);
  orange_clear(SEGDISP_DURATION);
  orange_clear(SEGDISP_GATE);
  orange_set(display);
}

void restore_left_display()
{
  select_left_display(saved_left_display);
}

void restore_right_display()
{
  select_right_display(saved_right_display);
}

void segdisp_put_signed(int display, int number)
{
  if (number < 0)
  {
    segdisp_display[display] = segdisp_both_digits[-number] | SEGDISP_LEFT_DP | SEGDISP_RIGHT_DP;
  }
  else
  {
    segdisp_display[display] = segdisp_both_digits[number];
  }
}

void segdisp_put_dualdigit(int display, uint8_t number)
{
  segdisp_display[display] = segdisp_both_digits[number];
}

void segdisp_put_number(int display, uint8_t number)
{
  if (number < 10)
  {
    segdisp_display[display] = segdisp_right_digits[number];
  }
  else
  {
    segdisp_display[display] = segdisp_both_digits[number];
  }
}

void segdisp_put_right_digit(int display, uint8_t digit)
{
  segdisp_display[display] &= 0x00FF;
  segdisp_display[display] |= segdisp_right_digits[digit];
}

void segdisp_put_note(uint16_t number)
{
  // determine octave
  uint16_t tmp;
  if (number < CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[0];
  }
  else if (number < 2 * CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[1];
    number -= CODE_PER_OCTAVE;
  }
  else if (number < 3 * CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[2];
    number -= 2 * CODE_PER_OCTAVE;
  }
  else if (number < 4 * CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[3];
    number -= 3 * CODE_PER_OCTAVE;
  }
  else if (number < 5 * CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[4];
    number -= 4 * CODE_PER_OCTAVE;
  }
  else if (number < 6 * CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[5];
    number -= 5 * CODE_PER_OCTAVE;
  }
  else if (number < 7 * CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[6];
    number -= 6 * CODE_PER_OCTAVE;
  }
  else if (number < 8 * CODE_PER_OCTAVE)
  {
    tmp = segdisp_left_digits[7];
    number -= 7 * CODE_PER_OCTAVE;
  }
  else
  {
    tmp = segdisp_left_digits[8];
    number -= 8 * CODE_PER_OCTAVE;
  }
  // determine note letter
  //
  if (number < CODE_FOR_D)
  {
    tmp |= SEGDISP_RIGHT_C;
  }
  else if (number < CODE_FOR_E)
  {
    tmp |= SEGDISP_RIGHT_d;
    number -= CODE_FOR_D;
  }
  else if (number < CODE_FOR_F)
  {
    tmp |= SEGDISP_RIGHT_E;
    number -= CODE_FOR_E;
  }
  else if (number < CODE_FOR_G)
  {
    tmp |= SEGDISP_RIGHT_F;
    number -= CODE_FOR_F;
  }
  else if (number < CODE_FOR_A)
  {
    tmp |= SEGDISP_RIGHT_G;
    number -= CODE_FOR_G;
  }
  else if (number < CODE_FOR_B)
  {
    tmp |= SEGDISP_RIGHT_A;
    number -= CODE_FOR_A;
  }
  else
  {
    tmp |= SEGDISP_RIGHT_b;
    number -= CODE_FOR_B;
  }
  segdisp_display[SEGDISP_VOLT1] = tmp | SEGDISP_LEFT_DP | SEGDISP_RIGHT_DP;
  // determine percent of whole tone
  // whole tone = 0.167V
  segdisp_put_dualdigit(SEGDISP_VOLT0, wholetone[CODE_TO_VOLTAGE(number)]);
}

void segdisp_put_voltage(uint16_t number)
{
  segdisp_put_dualdigit(SEGDISP_VOLT0, (number) % 100);
  segdisp_put_dualdigit(SEGDISP_VOLT1, ((number) / 100) % 100);
  segdisp_set(SEGDISP_VOLT1, SEGDISP_LEFT_DP);
}

void segdisp_put_version(uint16_t major, uint16_t minor, uint16_t patch)
{
  segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_F | segdisp_right_digits[major] | SEGDISP_RIGHT_DP;
  segdisp_put_dualdigit(SEGDISP_VOLT0, minor);
  if (patch > 0)
  {
    segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_P | segdisp_right_digits[patch];
  }
  else
  {
    segdisp_display[SEGDISP_INDEX] = SEGDISP_BLANK;
  }
}

void segdisp_clear(int display, uint16_t mask)
{
  segdisp_display[display] &= ~mask;
}

void segdisp_toggle(int display, uint16_t mask)
{
  segdisp_display[display] ^= mask;
}

void segdisp_set(int display, uint16_t mask)
{
  segdisp_display[display] |= mask;
}

void segdisp_sharp_on(void)
{
  segdisp_display[SEGDISP_CVA] |= SEGDISP_RIGHT_DP;
}

void segdisp_sharp_off(void)
{
  segdisp_display[SEGDISP_CVA] &= ~(SEGDISP_RIGHT_DP);
}

void segdisp_sharp_toggle(void)
{
  segdisp_display[SEGDISP_CVA] ^= SEGDISP_RIGHT_DP;
}

void segdisp_flat_on(void)
{
  segdisp_display[SEGDISP_CVA] |= SEGDISP_LEFT_DP;
}

void segdisp_flat_off(void)
{
  segdisp_display[SEGDISP_CVA] &= ~(SEGDISP_LEFT_DP);
}

void segdisp_flat_toggle(void)
{
  segdisp_display[SEGDISP_CVA] ^= SEGDISP_LEFT_DP;
}

void segdisp_on_error(uint8_t code)
{
  segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_HYPHEN | SEGDISP_RIGHT_HYPHEN;
  segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_E | SEGDISP_RIGHT_HYPHEN;
  segdisp_display[SEGDISP_VOLT0] = segdisp_both_digits[code];
}
