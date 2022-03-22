/*
 * handlers.c
 *
 * Created: 10/30/2013 12:47:56 PM
 *  Author: clarkson
 */

#include <asf.h>
#include <wallclock.h>
#include <extdac.h>
#include <display.h>
#include <ui_events.h>
#include <sequencer.h>
#include <event_handlers.h>
#include <voltages.h>
#include <clipboard.h>

// volatile bool clock_arrived = false;
volatile wallclock_t clock_arrival_time = 0;

bool insert_before = false;
uint8_t math_state = MATH_STATE_START;

static uint16_t volt0_saved;
static uint16_t volt1_saved;
static bool cycle_voltage_grid_on_release = false;
static uint8_t track_overlay_focus = SEGDISP_CVA;

void ext_reset_after(void)
{
  // rewind all play cursors back to beginning of step 1
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    position_init(&(seq.players[i]), &(seq.data), &(seq.data.tracks[i]));
    seq.players[i].ticks = 0;
    seq.ppqn_counters[i] = 1;
  }
  if (!seq.math_overlay_active)
  {
    select_left_display(focus_left_display);
  }
  seq_update_dac_outputs(NULL);
  seq_update_gate_outputs(wallclock(), NULL);
  seq.waiting_for_clock_after_reset = true;
  // replay one clock pulse to account for clearing the last clock pulse
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    seq.players[i].ticks = 1;
    seq.ppqn_counters[i] = 2;
  }
}

void ext_reset_before(void)
{
  seq.external_reset_armed = true;
  seq.waiting_for_clock_after_reset = true;
}

void on_clock_event()
{
  seq_on_clock_rising(clock_arrival_time, false, NULL);
}

void on_decrement_voltage_grid()
{
  track_t *pt = SEQ_FOCUS->pt;
  orange_clear(focus_right_display);
  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
    focus_right_display = SEGDISP_VOLT1;
    BIT_ASSIGN(true, pt->options, OPTION_COARSE);
    BIT_ASSIGN(false, pt->options, OPTION_SUPER_COARSE);
    break;
  case SEGDISP_VOLT1:
    focus_right_display = SEGDISP_VOLT01;
    BIT_ASSIGN(false, pt->options, OPTION_COARSE);
    BIT_ASSIGN(true, pt->options, OPTION_SUPER_COARSE);
    break;
  case SEGDISP_VOLT01:
    focus_right_display = SEGDISP_VOLT0;
    BIT_ASSIGN(false, pt->options, OPTION_COARSE);
    BIT_ASSIGN(false, pt->options, OPTION_SUPER_COARSE);
    // wrapping around so toggle pitch/voltage display
    // BIT_ASSIGN(!BIT_TEST(pt->options,pitched_option[seq.active_voltage_table]),pt->options,pitched_option[seq.active_voltage_table]);
    // seq_display_voltage();
    // volt0_saved = segdisp_display[SEGDISP_VOLT0];
    // volt1_saved = segdisp_display[SEGDISP_VOLT1];
    break;
  default:
    break;
  }
  orange_set(focus_right_display);
  cycle_voltage_grid_on_release = false;
}

void on_increment_voltage_grid()
{
  track_t *pt = SEQ_FOCUS->pt;
  orange_clear(focus_right_display);
  switch (focus_right_display)
  {
  case SEGDISP_VOLT01:
    focus_right_display = SEGDISP_VOLT1;
    BIT_ASSIGN(true, pt->options, OPTION_COARSE);
    BIT_ASSIGN(false, pt->options, OPTION_SUPER_COARSE);
    break;
  case SEGDISP_VOLT0:
    focus_right_display = SEGDISP_VOLT01;
    BIT_ASSIGN(false, pt->options, OPTION_COARSE);
    BIT_ASSIGN(true, pt->options, OPTION_SUPER_COARSE);
    // wrapping around so toggle pitch/voltage display
    // BIT_ASSIGN(!BIT_TEST(pt->options,pitched_option[seq.active_voltage_table]),pt->options,pitched_option[seq.active_voltage_table]);
    // seq_display_voltage();
    // volt0_saved = segdisp_display[SEGDISP_VOLT0];
    // volt1_saved = segdisp_display[SEGDISP_VOLT1];
    break;
  case SEGDISP_VOLT1:
    focus_right_display = SEGDISP_VOLT0;
    BIT_ASSIGN(false, pt->options, OPTION_COARSE);
    BIT_ASSIGN(false, pt->options, OPTION_SUPER_COARSE);
    break;
  default:
    break;
  }
  orange_set(focus_right_display);
  cycle_voltage_grid_on_release = false;
}

bool start_confirm(uint8_t mode)
{
  if (seq.confirm_mode != CONFIRM_NOTHING && seq.confirm_mode != mode)
  {
    // we have another confirm pending
    end_confirm();
    return false;
  }

  if (seq.confirm_mode == mode)
  {
    // confirm has succeeded
    end_confirm();
    return true;
  }

  seq.confirm_mode = mode;
  return false;
}

void end_confirm(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    switch (seq.confirm_mode)
    {
    case CONFIRM_SAVE:
    case CONFIRM_LOAD:
      seq_set_slot(seq.focus_slot);
      break;
    case CONFIRM_DELETE:
      highlight_left_display(focus_left_display);
      break;
    }
    seq_display_voltage();
    seq.confirm_mode = CONFIRM_NOTHING;
  }
}

static void increment_transform_mode(uint8_t *mode)
{
  if (*mode < NUM_TRANSFORM - 1)
  {
    *mode += 1;
  }
}

static void decrement_transform_mode(uint8_t *mode)
{
  if (*mode > 0)
  {
    *mode -= 1;
  }
}

static void toggle_transform_mode(uint8_t *mode)
{
  *mode += 1;
  if (*mode == NUM_TRANSFORM)
    *mode = 0;
}

static void display_transform(uint8_t mode, int operator, int left_display, int right_display)
{
  switch (mode)
  {
  case TRANSFORM_ARITHMETIC:
    if (operator<0)
    {
      segdisp_display[right_display] = segdisp_both_digits[-operator];
      segdisp_display[left_display] = SEGDISP_LEFT_A | SEGDISP_RIGHT_HYPHEN;
    }
    else
    {
      segdisp_display[right_display] = segdisp_both_digits[operator];
      segdisp_display[left_display] = SEGDISP_LEFT_A | SEGDISP_RIGHT_BLANK;
    }
    break;
  case TRANSFORM_GEOMETRIC:
    if (operator<0)
    {
      segdisp_display[right_display] = segdisp_both_digits[-operator];
      segdisp_display[left_display] = SEGDISP_LEFT_G | SEGDISP_RIGHT_u;
    }
    else
    {
      segdisp_display[right_display] = segdisp_both_digits[operator];
      segdisp_display[left_display] = SEGDISP_LEFT_G | SEGDISP_RIGHT_CARET;
    }
    break;
  case TRANSFORM_SET:
    if (operator<0)
    {
      segdisp_display[right_display] = segdisp_both_digits[-operator];
      segdisp_display[left_display] = SEGDISP_LEFT_S | SEGDISP_RIGHT_HYPHEN;
    }
    else
    {
      segdisp_display[right_display] = segdisp_both_digits[operator];
      segdisp_display[left_display] = SEGDISP_LEFT_S | SEGDISP_RIGHT_BLANK;
    }
    break;
  case TRANSFORM_RANDOM:
    if (operator<0)
    {
      segdisp_display[right_display] = segdisp_both_digits[-operator];
      segdisp_display[left_display] = SEGDISP_LEFT_r | SEGDISP_RIGHT_d;
    }
    else
    {
      segdisp_display[right_display] = segdisp_both_digits[operator];
      segdisp_display[left_display] = SEGDISP_LEFT_r | SEGDISP_RIGHT_d;
    }
    break;
  case TRANSFORM_JITTER:
    if (operator<0)
    {
      segdisp_display[right_display] = segdisp_both_digits[-operator];
      segdisp_display[left_display] = SEGDISP_LEFT_J | SEGDISP_RIGHT_t;
    }
    else
    {
      segdisp_display[right_display] = segdisp_both_digits[operator];
      segdisp_display[left_display] = SEGDISP_LEFT_J | SEGDISP_RIGHT_t;
    }
    break;
  case TRANSFORM_QUANTIZE:
    if (operator<0)
    {
      segdisp_display[right_display] = segdisp_both_digits[-operator];
      segdisp_display[left_display] = SEGDISP_LEFT_q | SEGDISP_RIGHT_t;
    }
    else
    {
      segdisp_display[right_display] = segdisp_both_digits[operator];
      segdisp_display[left_display] = SEGDISP_LEFT_q | SEGDISP_RIGHT_t;
    }
    break;
  }
}

void display_transform_cvA(transform_t *t)
{
  display_transform(t->cvA_mode, t->cvA, SEGDISP_TRACK, SEGDISP_CVA);
}

void display_transform_cvB(transform_t *t)
{
  display_transform(t->cvB_mode, t->cvB, SEGDISP_PATTERN, SEGDISP_CVB);
}

void display_transform_duration(transform_t *t)
{
  display_transform(t->duration_mode, t->duration, SEGDISP_STEP, SEGDISP_DURATION);
}

void display_transform_gate(transform_t *t)
{
  display_transform(t->gate_mode, t->gate, SEGDISP_SNAPSHOT, SEGDISP_GATE);
}

void start_math_overlay(void)
{
  seq.math_overlay_active = true;

  segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_o | SEGDISP_RIGHT_P;
  segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_P;
  segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_1 | SEGDISP_RIGHT_N;
  segdisp_display[SEGDISP_TRACK] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_PATTERN] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_STEP] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_SNAPSHOT] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  display_transform_cvA(&(SEQ_EDIT_FOCUS->pt->transform));
  display_transform_cvB(&(SEQ_EDIT_FOCUS->pt->transform));
  display_transform_duration(&(SEQ_EDIT_FOCUS->pt->transform));
  display_transform_gate(&(SEQ_EDIT_FOCUS->pt->transform));
  on_select_right_focus(focus_right_display);
}

void end_math_overlay(void)
{
  seq.math_overlay_active = false;
  math_state = MATH_STATE_START;

  segdisp_display[SEGDISP_INDEX] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
  segdisp_display[SEGDISP_VOLT0] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
  segdisp_display[SEGDISP_VOLT1] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
  segdisp_display[SEGDISP_TRACK] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_PATTERN] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_STEP] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  segdisp_display[SEGDISP_SNAPSHOT] = SEGDISP_RIGHT_BLANK | SEGDISP_LEFT_BLANK;
  seq_display_position(SEQ_FOCUS);
  seq_set_slot(seq.focus_slot);

  // turn off blinking
  switch (focus_left_display)
  {
  case SEGDISP_INDEX:
  case SEGDISP_SNAPSHOT:
  case SEGDISP_TRACK:
    orange_clear(SEGDISP_TRACK);
    break;
  case SEGDISP_PATTERN:
    orange_clear(SEGDISP_PATTERN);
    break;
  case SEGDISP_STEP:
    orange_clear(SEGDISP_STEP);
    break;
    break;
  }

  highlight_left_display(focus_left_display);
}

void update_insert_overlay(void)
{
  switch (seq.insert_mode)
  {
  case INSERT_AFTER:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_A | SEGDISP_RIGHT_F;
    segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_t | SEGDISP_RIGHT_r;
    break;
  case INSERT_BEFORE:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_b | SEGDISP_RIGHT_E;
    segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_F | SEGDISP_RIGHT_r;
    break;
  case INSERT_MIDDLE:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_S | SEGDISP_RIGHT_P;
    segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_L | SEGDISP_RIGHT_t;
    break;
  }
}

void start_insert_overlay(uint8_t mode)
{
  seq.insert_overlay_active = true;
  seq.insert_mode = mode;

  segdisp_display[SEGDISP_CVA] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
  segdisp_display[SEGDISP_CVB] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
  segdisp_display[SEGDISP_DURATION] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
  segdisp_display[SEGDISP_GATE] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;

  update_insert_overlay();
}

void end_insert_overlay(void)
{
  seq.insert_overlay_active = false;
  seq_display_position(SEQ_FOCUS);
  seq_set_slot(seq.focus_slot);
}

void on_select_right_focus(int display)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  if (seq.math_overlay_active)
  {
    select_right_display(display);
    switch (display)
    {
    case SEGDISP_CVA:
      highlight_left_display(SEGDISP_TRACK);
      break;
    case SEGDISP_CVB:
      highlight_left_display(SEGDISP_PATTERN);
      break;
    case SEGDISP_DURATION:
      highlight_left_display(SEGDISP_STEP);
      break;
    case SEGDISP_GATE:
      highlight_left_display(SEGDISP_SNAPSHOT);
      break;
    }
  }
  else if (seq.track_overlay_active)
  {
    switch (display)
    {
    case SEGDISP_CVA:
      if (focus_right_display == SEGDISP_CVA)
      {
        if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TABLE_A_PITCHED))
        {
          BIT_ASSIGN(false, SEQ_FOCUS->pt->options, OPTION_TABLE_A_PITCHED);
          // segdisp_display[SEGDISP_CVA] = SEGDISP_LEFT_N | SEGDISP_RIGHT_r;
        }
        else
        {
          BIT_ASSIGN(true, SEQ_FOCUS->pt->options, OPTION_TABLE_A_PITCHED);
          // segdisp_display[SEGDISP_CVA] = SEGDISP_LEFT_N | SEGDISP_RIGHT_t;
        }
      }
      else
      {
        select_right_display(SEGDISP_CVA);
      }

      break;
    case SEGDISP_CVB:
      if (focus_right_display == SEGDISP_CVB)
      {
        if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TABLE_B_PITCHED))
        {
          BIT_ASSIGN(false, SEQ_FOCUS->pt->options, OPTION_TABLE_B_PITCHED);
          // segdisp_display[SEGDISP_CVB] = SEGDISP_LEFT_N | SEGDISP_RIGHT_r;
        }
        else
        {
          BIT_ASSIGN(true, SEQ_FOCUS->pt->options, OPTION_TABLE_B_PITCHED);
          // segdisp_display[SEGDISP_CVB] = SEGDISP_LEFT_N | SEGDISP_RIGHT_t;
        }
      }
      else
      {
        select_right_display(SEGDISP_CVB);
      }
      break;
    case SEGDISP_DURATION:
      select_right_display(SEGDISP_DURATION);
      break;
    case SEGDISP_GATE:
      if (focus_right_display == SEGDISP_GATE)
      {
        if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TRIGGER))
        {
          BIT_ASSIGN(false, SEQ_FOCUS->pt->options, OPTION_TRIGGER);
          // segdisp_display[SEGDISP_GATE] = SEGDISP_LEFT_G | SEGDISP_RIGHT_t;
        }
        else
        {
          BIT_ASSIGN(true, SEQ_FOCUS->pt->options, OPTION_TRIGGER);
          // segdisp_display[SEGDISP_GATE] = SEGDISP_LEFT_t | SEGDISP_RIGHT_r;
        }
      }
      else
      {
        select_right_display(SEGDISP_GATE);
      }
      break;
    }
    track_overlay_focus = display;
    update_track_overlay();
  }
  else
  {
    switch (display)
    {
    case SEGDISP_CVA:
      break;
    case SEGDISP_CVB:
      break;
    case SEGDISP_DURATION:
      break;
    case SEGDISP_GATE:
      if (focus_right_display == SEGDISP_GATE)
      {
        if (seq.mode == MODE_LIVE && stopped())
        {
          seq_toggle_ratchet(SEQ_LIVE_FOCUS);
        }
        else if (seq.mode != MODE_LIVE)
        {
          seq_toggle_ratchet(SEQ_EDIT_FOCUS);
        }
        else
        {
          tilt_trigger();
        }
      }
      break;
    case SEGDISP_VOLT0:
    case SEGDISP_VOLT1:
    case SEGDISP_VOLT01:
      break;
    }
    select_right_display(display);
  }
}

void on_select_left_focus(int display)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  if (seq.track_overlay_active)
  {
    switch (display)
    {
    case SEGDISP_INDEX:
      break;
    case SEGDISP_TRACK:
      if (focus_left_display == SEGDISP_TRACK)
      {
        end_track_overlay();
        return;
      }
      else
      {
        select_left_display(SEGDISP_TRACK);
        track_overlay_focus = SEGDISP_TRACK;
      }
      break;
    case SEGDISP_PATTERN:
      break;
    case SEGDISP_STEP:
      select_left_display(SEGDISP_STEP);
      track_overlay_focus = SEGDISP_STEP;
      break;
    case SEGDISP_SNAPSHOT:
      break;
    }
    update_track_overlay();
  }
  else if (seq.math_overlay_active)
  {
    highlight_left_display(display);
    switch (display)
    {
    case SEGDISP_TRACK:
      if (focus_right_display == SEGDISP_CVA)
      {
        toggle_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.cvA_mode));
        display_transform_cvA(&(SEQ_EDIT_FOCUS->pt->transform));
      }
      else
      {
        select_right_display(SEGDISP_CVA);
      }
      break;
    case SEGDISP_PATTERN:
      if (focus_right_display == SEGDISP_CVB)
      {
        toggle_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.cvB_mode));
        display_transform_cvB(&(SEQ_EDIT_FOCUS->pt->transform));
      }
      else
      {
        select_right_display(SEGDISP_CVB);
      }
      break;
    case SEGDISP_STEP:
      if (focus_right_display == SEGDISP_DURATION)
      {
        toggle_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.duration_mode));
        display_transform_duration(&(SEQ_EDIT_FOCUS->pt->transform));
      }
      else
      {
        select_right_display(SEGDISP_DURATION);
      }
      break;
    case SEGDISP_SNAPSHOT:
      if (focus_right_display == SEGDISP_GATE)
      {
        toggle_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.gate_mode));
        display_transform_gate(&(SEQ_EDIT_FOCUS->pt->transform));
      }
      else
      {
        select_right_display(SEGDISP_GATE);
      }
      break;
    default:
      end_math_overlay();
      on_select_left_focus(display);
      break;
    }
  }
  else if (seq.selecting && display == SEGDISP_TRACK)
  {
    // can't go to another track while selecting
    tilt_trigger();
  }
  else
  {
    int prev_display = focus_left_display;
    select_left_display(display);
    seq_display_voltage();
    seq_display_loop(SEQ_FOCUS);
    switch (display)
    {
    case SEGDISP_INDEX:
      seq_display_voltage();
      break;
    case SEGDISP_TRACK:
      if (prev_display == SEGDISP_TRACK)
        start_track_overlay();
      break;
    case SEGDISP_PATTERN:
      // inspect
      if (prev_display == SEGDISP_PATTERN)
      {
        position_t *focus = SEQ_FOCUS;
        int nP = 0;
        position_t cursor;
        position_copy(&cursor, focus);
        seq_pattern_begin(&cursor);
        while (cursor.m == focus->m)
        {
          if (cursor.s == INVALID_STEP)
            break;
          nP += cursor.ps->duration;
          if (!seq_next_step(&cursor))
            break;
        }
        segdisp_put_dualdigit(SEGDISP_VOLT1, (nP / 100) % 100);
        segdisp_put_dualdigit(SEGDISP_VOLT0, nP % 100);
        segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_n | SEGDISP_RIGHT_P;
      }
      break;
    case SEGDISP_STEP:
      // inspect
      if (prev_display == SEGDISP_STEP)
      {
        segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_r | SEGDISP_RIGHT_E;
        segdisp_put_voltage(MAX_STEPS - seq_get_focused_data()->steps.head);
      }
      break;
    case SEGDISP_SNAPSHOT:
      break;
    }
  }
}

void ui_load_button_push()
{
  if (start_confirm(CONFIRM_LOAD))
  {
    if (seq.focus_slot == BLANK_SNAPSHOT)
    {
      // clear sequencer state
      sequencer_init();
      seq_display_position(SEQ_FOCUS);
      if (seq.mode == MODE_EDIT || seq.mode == MODE_LIVE)
      {
        seq_write_to_dac(SEQ_FOCUS);
      }
    }
    else
    {
      seq_load_snapshot(seq.focus_slot - 1);
    }
  }
}

void ui_save_button_push()
{
  if (seq.focus_slot == BLANK_SNAPSHOT)
  {
    tilt_trigger();
    return;
  }
  if (start_confirm(CONFIRM_SAVE))
  {
    seq_save_snapshot(seq.focus_slot - 1);
  }
}

void ui_mode_switch(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
    end_confirm();
  if (gpio_pin_is_high(PIN_MODE_A))
  {
    // down
    if (seq.mode == MODE_COMMIT)
    {
      clear_clipboard();
      seq_release_hold();
    }
    gpio_set_pin_high(PIN_COMMIT_LED);
    seq.mode = MODE_LIVE;
    seq_initiate_live();
  }
  else if (gpio_pin_is_high(PIN_MODE_B))
  {
    // up
    if (seq.mode != MODE_COMMIT)
    {
      seq.mode = MODE_COMMIT;
      seq.commit_armed = false;
      seq.commit_quantization = NO_QUANTIZATION;
      clear_clipboard();
      seq_initiate_hold();
      gpio_set_pin_low(PIN_COMMIT_LED);
    }
  }
  else
  {
    // middle
    if (seq.mode == MODE_COMMIT)
    {
      clear_clipboard();
      seq_release_hold();
    }
    gpio_set_pin_high(PIN_COMMIT_LED);
    seq.mode = MODE_EDIT;
    seq_initiate_edit();
  }
}

void ui_table_switch(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
    end_confirm();
  if (gpio_pin_is_high(PIN_TABLE_A))
  {
    seq_set_table(SEQ_TABLE_B);
    ref_end();
    seq_display_position(SEQ_FOCUS);
  }
  else if (gpio_pin_is_high(PIN_TABLE_B))
  {
    seq_set_table(SEQ_TABLE_A);
    ref_end();
    seq_display_position(SEQ_FOCUS);
  }
  else
  {
    // enter reference table mode
    if (seq.math_overlay_active)
    {
      math_state = MATH_STATE_START;
      end_math_overlay();
    }
    ref_start();
  }
}

void ui_smooth_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  if (seq.math_overlay_active)
  {
    tilt_trigger();
    return;
  }
  position_t *p;
  if (seq.mode == MODE_LIVE)
  {
    p = SEQ_LIVE_FOCUS;
  }
  else
  {
    p = SEQ_EDIT_FOCUS;
  }
  switch (focus_left_display)
  {
  case SEGDISP_INDEX:
    tilt_trigger();
    break;
  case SEGDISP_TRACK:
    seq_set_track_smooth(p, seq.focus_voltage_table, !seq_is_track_smooth(p, seq.focus_voltage_table));
    break;
  case SEGDISP_PATTERN:
    seq_set_pattern_smooth(p, seq.focus_voltage_table, !seq_is_pattern_smooth(p, seq.focus_voltage_table));
    break;
  case SEGDISP_STEP:
    seq_set_step_smooth(p, seq.focus_voltage_table, !seq_is_step_smooth(p, seq.focus_voltage_table));
    break;
  case SEGDISP_SNAPSHOT:
    tilt_trigger();
    break;
  }
}

void ui_copy_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    return;
  }
  if (seq.math_overlay_active)
  {
    tilt_trigger();
    return;
  }
  if (seq.clipboard_mode == CLIPBOARD_MODE_NONE && (focus_left_display == SEGDISP_PATTERN || focus_left_display == SEGDISP_STEP))
  {
    seq.selecting = true;
    seq.nselected = 1;
    position_set_start(&seq.select_start, SEQ_FOCUS, focus_left_display);
    seq_display_nselected();
  }
}

void ui_copy_button_release(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }

  if (seq.math_overlay_active)
    return;

  if (seq.clipboard_mode == CLIPBOARD_MODE_NONE)
  {
    if (seq.selecting)
    {
      seq.selecting = false;
      seq_display_voltage();
      position_set_end(&seq.select_end, SEQ_FOCUS, focus_left_display);
    }
    switch (focus_left_display)
    {
    case SEGDISP_INDEX:
      copy_to_clipboard(SEQ_FOCUS, SEQ_FOCUS, CLIPBOARD_MODE_TABLE);
      gpio_set_pin_low(PIN_COPY_LED);
      break;
    case SEGDISP_TRACK:
      copy_to_clipboard(SEQ_FOCUS, SEQ_FOCUS, CLIPBOARD_MODE_TRACK);
      gpio_set_pin_low(PIN_COPY_LED);
      break;
    case SEGDISP_PATTERN:
      if (seq.select_start.m == seq.select_end.m)
      {
        // include the whole pattern
        seq_pattern_begin(&seq.select_start);
        seq_pattern_end(&seq.select_end);
      }
      copy_to_clipboard(&seq.select_start, &seq.select_end, CLIPBOARD_MODE_PATTERN);
      gpio_set_pin_low(PIN_COPY_LED);
      break;
    case SEGDISP_STEP:
      copy_to_clipboard(&seq.select_start, &seq.select_end, CLIPBOARD_MODE_STEP);
      gpio_set_pin_low(PIN_COPY_LED);
      break;
    case SEGDISP_SNAPSHOT:
      tilt_trigger();
      break;
    }
  }
  else
  {
    switch (seq.clipboard_mode)
    {
    case CLIPBOARD_MODE_TRACK:
      orange_clear(SEGDISP_TRACK);
      break;
    case CLIPBOARD_MODE_PATTERN:
      orange_clear(SEGDISP_PATTERN);
      break;
    case CLIPBOARD_MODE_STEP:
      orange_clear(SEGDISP_STEP);
      break;
    case CLIPBOARD_MODE_TABLE:
      orange_clear(SEGDISP_INDEX);
      break;
    }
    clear_clipboard();
    orange_set(focus_left_display);
  }
}

void ui_commit_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  if (seq.mode == MODE_COMMIT)
  {
    if (seq.commit_armed)
    {
      seq_commit();
    }
    else
    {
      switch (focus_left_display)
      {
      case SEGDISP_TRACK:
      case SEGDISP_SNAPSHOT:
        seq.commit_quantization = QUANTIZE_TO_TRACK;
        break;
      case SEGDISP_PATTERN:
        seq.commit_quantization = QUANTIZE_TO_PATTERN;
        break;
      case SEGDISP_STEP:
        seq.commit_quantization = QUANTIZE_TO_STEP;
        break;
      default:
        seq.commit_quantization = QUANTIZE_TO_STEP;
        break;
      }
      switch (seq.commit_quantization)
      {
      case NO_QUANTIZATION:
        seq_commit();
        break;
      case QUANTIZE_TO_STEP:
      case QUANTIZE_TO_TRACK:
      case QUANTIZE_TO_PATTERN:
        seq.commit_armed = true;
        seq.trigger_track = seq.focus_track;
        break;
      }
    }
  }
}

void ui_lstart_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  if (seq.math_overlay_active)
  {
    tilt_trigger();
    return;
  }

  seq_set_lstart(SEQ_FOCUS);
  seq_display_position(SEQ_FOCUS);
}

void ui_lend_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  if (seq.math_overlay_active)
  {
    tilt_trigger();
    return;
  }
  seq_set_lend(SEQ_FOCUS);
  seq_display_position(SEQ_FOCUS);
}

void ui_pause_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  if (seq.pause)
  {
    seq.pause = false;
    gpio_set_pin_high(PIN_PAUSE_LED);
  }
  else
  {
    seq.pause = true;
    for (int i = 0; i < NUM_TRACKS; i++)
    {
      gpio_set_pin_low(seq.gate_output_pins[i]);
      seq.players[i].ticks = 0;
      seq.current_tick_adjustment[i] = 0;
      seq.next_tick_adjustment[i] = 0;
    }
    gpio_set_pin_low(PIN_PAUSE_LED);
  }
}

void ui_reset_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  switch (seq.reset_quantization)
  {
  case NO_QUANTIZATION:
    if (seq.commit_armed)
    {
      seq_commit();
    }

    seq_hard_reset();
    seq_update_dac_outputs(NULL);
    break;
  case QUANTIZE_TO_STEP:
  case QUANTIZE_TO_TRACK:
  case QUANTIZE_TO_PATTERN:
    seq.internal_reset_armed = true;
    seq.trigger_track = seq.focus_track;
    break;
  }
}

void flash_voltage(bool toggle)
{
  if (toggle)
  {
    segdisp_display[SEGDISP_VOLT0] = volt0_saved;
    segdisp_display[SEGDISP_VOLT1] = volt1_saved;
  }
  else
  {
    volt0_saved = segdisp_display[SEGDISP_VOLT0];
    volt1_saved = segdisp_display[SEGDISP_VOLT1];
    switch (focus_right_display)
    {
    case SEGDISP_VOLT0:
      segdisp_display[SEGDISP_VOLT0] = (volt0_saved & SEGDISP_LEFT_ALL);
      break;
    case SEGDISP_VOLT1:
      segdisp_display[SEGDISP_VOLT0] = (volt0_saved & SEGDISP_RIGHT_ALL);
      segdisp_display[SEGDISP_VOLT1] = (volt1_saved & SEGDISP_LEFT_ALL);
      break;
    case SEGDISP_VOLT01:
      segdisp_display[SEGDISP_VOLT1] = (volt1_saved & SEGDISP_RIGHT_ALL);
      break;
    default:
      break;
    }
  }
}

void ui_voltage_button_push()
{
  if (seq.track_overlay_active)
    return;
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
  }
  else if (seq.math_overlay_active)
  {
    if (math_state == MATH_STATE_PINNED)
    {
      end_math_overlay();
    }
    else
    {
      math_state = MATH_STATE_PINNED;
      seq.math_button_release_ignore = true;
      segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_d | SEGDISP_RIGHT_O;
      segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_N | SEGDISP_RIGHT_E;
    }
  }
  else if (seq.ref_table_active)
  {
    uint8_t save = seq.focus_ref_index;
    ref_select(seq.focus_ref_table);
    seq.focus_ref_index = save;
    if (seq.focus_ref_index != INVALID_INDEX)
      segdisp_put_number(SEGDISP_INDEX, seq.focus_ref_index);
    orange_clear(focus_right_display);
    focus_right_display = SEGDISP_VOLT01;
    orange_set(focus_right_display);
  }
  else
  {
    seq.voltage_overlay_active = true;
    cycle_voltage_grid_on_release = focus_right_display == SEGDISP_VOLT0 || focus_right_display == SEGDISP_VOLT1 || focus_right_display == SEGDISP_VOLT01;
    volt0_saved = segdisp_display[SEGDISP_VOLT0];
    volt1_saved = segdisp_display[SEGDISP_VOLT1];
    seq_select_voltage_grid();
  }
}

void ui_voltage_button_release()
{
  if (seq.ref_table_active || seq.math_overlay_active || seq.track_overlay_active)
    return;
  if (seq.voltage_overlay_active)
  {
    seq.voltage_overlay_active = false;
    segdisp_display[SEGDISP_VOLT0] = volt0_saved;
    segdisp_display[SEGDISP_VOLT1] = volt1_saved;
    if (cycle_voltage_grid_on_release)
    {
      on_decrement_voltage_grid();
    }
  }
}

void start_track_overlay(void)
{
  seq.track_overlay_active = true;

  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
  case SEGDISP_VOLT1:
  case SEGDISP_VOLT01:
    select_right_display(SEGDISP_DURATION);
    break;
  }

  track_overlay_focus = SEGDISP_TRACK;

  update_track_overlay();
}

void update_track_overlay(void)
{
  segdisp_display[SEGDISP_PATTERN] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
  segdisp_display[SEGDISP_STEP] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
  segdisp_display[SEGDISP_SNAPSHOT] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;

  segdisp_put_number(SEGDISP_STEP, SEQ_FOCUS->pt->ppqn);
  segdisp_put_number(SEGDISP_DURATION, SEQ_FOCUS->pt->divide);

  if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TRIGGER))
  {
    segdisp_display[SEGDISP_GATE] = SEGDISP_LEFT_t | SEGDISP_RIGHT_r;
  }
  else
  {
    segdisp_display[SEGDISP_GATE] = SEGDISP_LEFT_G | SEGDISP_RIGHT_t;
  }

  if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TABLE_A_PITCHED))
  {
    segdisp_display[SEGDISP_CVA] = SEGDISP_LEFT_N | SEGDISP_RIGHT_t;
  }
  else
  {
    segdisp_display[SEGDISP_CVA] = SEGDISP_LEFT_N | SEGDISP_RIGHT_r;
  }

  if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TABLE_B_PITCHED))
  {
    segdisp_display[SEGDISP_CVB] = SEGDISP_LEFT_N | SEGDISP_RIGHT_t;
  }
  else
  {
    segdisp_display[SEGDISP_CVB] = SEGDISP_LEFT_N | SEGDISP_RIGHT_r;
  }

  // help display in VOLTAGE
  switch (track_overlay_focus)
  {
  case SEGDISP_TRACK:
  {
    position_t *focus = SEQ_FOCUS;
    int nP = 0;
    position_t cursor;
    position_init(&cursor, focus->data, focus->pt);
    while (cursor.s != INVALID_STEP)
    {
      nP += cursor.ps->duration;
      if (!seq_next_step(&cursor))
        break;
    }
    segdisp_put_dualdigit(SEGDISP_VOLT1, (nP / 100) % 100);
    segdisp_put_dualdigit(SEGDISP_VOLT0, nP % 100);
    segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_n | SEGDISP_RIGHT_P;
  }
  break;
  case SEGDISP_STEP:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
    segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_n | SEGDISP_LEFT_seg_up | SEGDISP_RIGHT_U;
    segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_L | SEGDISP_RIGHT_t;
    break;
  case SEGDISP_CVA:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
    if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TABLE_A_PITCHED))
    {
      segdisp_put_note(8000);
    }
    else
    {
      segdisp_put_voltage(1000);
    }
    break;
  case SEGDISP_CVB:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
    if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TABLE_B_PITCHED))
    {
      segdisp_put_note(8000);
    }
    else
    {
      segdisp_put_voltage(1000);
    }
    break;
  case SEGDISP_DURATION:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
    segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_d | SEGDISP_RIGHT_i;
    segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_u | SEGDISP_LEFT_seg_up | SEGDISP_RIGHT_i;
    break;
  case SEGDISP_GATE:
    segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
    if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_TRIGGER))
    {
      segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_t | SEGDISP_RIGHT_r;
      segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_i | SEGDISP_RIGHT_G;
    }
    else
    {
      segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_G | SEGDISP_RIGHT_A;
      segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_t | SEGDISP_RIGHT_E;
    }
    break;
  }
}

void end_track_overlay(void)
{
  seq.track_overlay_active = false;
  seq_display_position(SEQ_FOCUS);
  seq_set_slot(seq.focus_slot);
}

void ui_track_button_push()
{
  on_select_left_focus(SEGDISP_TRACK);
}

void ui_track_button_release()
{
}

void ui_insert_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    return;
  }

  if (seq.math_overlay_active || focus_left_display == SEGDISP_SNAPSHOT)
  {
    tilt_trigger();
    return;
  }

  if (seq.clipboard_mode == CLIPBOARD_MODE_TABLE)
  {
    paste_table();
    if (seq.mode == MODE_LIVE)
      seq_write_to_dac(SEQ_FOCUS);
    return;
  }

  if (focus_left_display == SEGDISP_PATTERN && seq.mode == MODE_LIVE && !stopped())
  {
    // empty patterns will get skipped during playback
    tilt_trigger();
    return;
  }

  if (focus_left_display == SEGDISP_TRACK)
  {
    if (seq.clipboard_mode == CLIPBOARD_MODE_TRACK)
    {
      // no insert overlay when copying to a track
      position_t *focus = SEQ_FOCUS;
      position_t *anti_focus = SEQ_ANTI_FOCUS;
      paste_from_clipboard_after(focus);
      position_init(anti_focus, anti_focus->data, anti_focus->pt);
      seq_display_position(focus);
    }
    else
    {
      if (SEQ_FOCUS->pt->nm == 0)
      {
        // let the user insert the first step of an empty track
        select_left_display(SEGDISP_STEP);
        ui_insert_button_push();
      }
      else
      {
        tilt_trigger();
      }
    }
    return;
  }

  if (seq.clipboard_mode == CLIPBOARD_MODE_TRACK && focus_left_display == SEGDISP_INDEX)
  {
    tilt_trigger();
    return;
  }

  start_insert_overlay(INSERT_AFTER);
}

void ui_insert_button_release(void)
{
  end_insert_overlay();

  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }

  if (seq.math_overlay_active || focus_left_display == SEGDISP_SNAPSHOT || seq.clipboard_mode == CLIPBOARD_MODE_TRACK || seq.clipboard_mode == CLIPBOARD_MODE_TABLE)
    return;
  if (focus_left_display == SEGDISP_PATTERN && seq.mode == MODE_LIVE && !stopped())
    return;

  position_t *focus = SEQ_FOCUS;
  position_t *anti_focus = SEQ_ANTI_FOCUS;

  if (!can_copy_from_clipboard())
  {
    switch (focus_left_display)
    {
    case SEGDISP_INDEX:
      break;
    case SEGDISP_TRACK:
      break;
    case SEGDISP_PATTERN:
      switch (seq.insert_mode)
      {
      case INSERT_AFTER:
        seq_insert_pattern_after(focus);
        break;
      case INSERT_BEFORE:
        seq_insert_pattern_before(focus);
        break;
      case INSERT_MIDDLE:
        seq_split_pattern(focus);
        break;
      }
      seq_rename(anti_focus);
      seq_display_position(focus);
      if (seq.mode == MODE_LIVE)
        seq_write_to_dac(focus);
      break;
    case SEGDISP_STEP:
      switch (seq.insert_mode)
      {
      case INSERT_AFTER:
        seq_smart_insert_step_after(focus);
        break;
      case INSERT_BEFORE:
        seq_smart_insert_step_before(focus);
        break;
      case INSERT_MIDDLE:
        seq_split_step(focus);
        break;
      }
      seq_rename(anti_focus);
      seq_display_position(focus);
      if (seq.mode == MODE_LIVE)
        seq_write_to_dac(focus);
      break;
    case SEGDISP_SNAPSHOT:
      break;
    }
  }
  else
  {
    switch (seq.insert_mode)
    {
    case INSERT_AFTER:
      paste_from_clipboard_after(focus);
      break;
    case INSERT_BEFORE:
      paste_from_clipboard_before(focus);
      break;
    case INSERT_MIDDLE:
      tilt_trigger();
      break;
    }
    seq_rename(anti_focus);
    seq_display_position(focus);
    if (seq.mode == MODE_LIVE)
      seq_write_to_dac(focus);
  }

  if ((seq.mode == MODE_LIVE || seq.mode == MODE_EDIT) && focus->m != INVALID_PATTERN && anti_focus->m == INVALID_PATTERN)
  {
    // A pattern was inserted at the focus but the anti-focus is not pointing to anything valid yet.
    position_init(anti_focus, anti_focus->data, anti_focus->pt);
  }
}

void ui_delete_button_push(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING && seq.confirm_mode != CONFIRM_DELETE)
  {
    end_confirm();
    return;
  }
  position_t *focus = SEQ_FOCUS;
  position_t *anti_focus = SEQ_ANTI_FOCUS;
  position_t *edit_focus = SEQ_EDIT_FOCUS;
  position_t *live_focus = SEQ_LIVE_FOCUS;

  if (seq.math_overlay_active)
  {
    transform_init(&(focus->pt->transform));
    display_transform_cvA(&(focus->pt->transform));
    display_transform_cvB(&(focus->pt->transform));
    display_transform_duration(&(focus->pt->transform));
    display_transform_gate(&(focus->pt->transform));
    return;
  }
  // if(seq.mode==MODE_LIVE && !stopped()) {
  //	tilt_trigger();
  //	return;
  // }
  switch (focus_left_display)
  {
  case SEGDISP_INDEX:
    tilt_trigger();
    break;
  case SEGDISP_TRACK:
    if (start_confirm(CONFIRM_DELETE))
    {
      seq_delete_track(edit_focus);
      position_init(live_focus, live_focus->data, live_focus->pt);
      seq_display_position(focus);
    }
    break;
  case SEGDISP_PATTERN:
    if (focus->m != INVALID_PATTERN && focus->m == anti_focus->m)
    {
      seq_delete_pattern(focus);
      if (focus->data == anti_focus->data)
      {
        position_copy(anti_focus, focus);
        live_focus->ticks = 0;
      }
    }
    else
    {
      seq_delete_pattern(focus);
    }
    seq_rename(anti_focus);
    seq_display_position(focus);
    if (seq.mode == MODE_LIVE)
    {
      live_focus->ticks = 0;
      seq_write_to_dac(live_focus);
    }
    break;
  case SEGDISP_STEP:
    if (focus->s != INVALID_STEP && focus->s == anti_focus->s)
    {
      seq_delete_step(focus);
      if (focus->data == anti_focus->data)
      {
        position_copy(anti_focus, focus);
        live_focus->ticks = 0;
      }
    }
    else
    {
      seq_delete_step(focus);
    }
    seq_rename(anti_focus);
    seq_display_position(focus);
    if (seq.mode == MODE_LIVE)
    {
      live_focus->ticks = 0;
      seq_write_to_dac(live_focus);
    }
    break;
  case SEGDISP_SNAPSHOT:
    tilt_trigger();
    break;
  }
}

void ui_math_button_push(void)
{
  seq.math_button_release_ignore = false;
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    return;
  }
  switch (math_state)
  {
  case MATH_STATE_START:
    math_state = MATH_STATE_HELD;
    start_math_overlay();
    break;
  case MATH_STATE_HELD:
    break;
  case MATH_STATE_PINNED:
    break;
  }
}

void ui_math_button_release(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
    return;
  }
  position_t *focus = SEQ_FOCUS;

  if (seq.math_button_release_ignore)
  {
    seq.math_button_release_ignore = false;
    return;
  }

  switch (math_state)
  {
  case MATH_STATE_START:
    break;
  case MATH_STATE_HELD:
    end_math_overlay();
    switch (focus_left_display)
    {
    case SEGDISP_INDEX:

      break;
    case SEGDISP_TRACK:
      srand(Get_system_register(AVR32_COUNT));
      seq_apply_transform_to_track(focus, &(focus->pt->transform));
      seq_display_position(focus);
      break;
    case SEGDISP_PATTERN:
      srand(Get_system_register(AVR32_COUNT));
      seq_apply_transform_to_pattern(focus, &(focus->pt->transform));
      seq_display_position(focus);
      break;
    case SEGDISP_STEP:
      srand(Get_system_register(AVR32_COUNT));
      seq_apply_transform_to_step(focus, &(focus->pt->transform));
      seq_display_position(focus);
      break;
    case SEGDISP_SNAPSHOT:

      break;
    }
    break;
  case MATH_STATE_PINNED:
    switch (focus_left_display)
    {
    case SEGDISP_INDEX:

      break;
    case SEGDISP_TRACK:
      srand(Get_system_register(AVR32_COUNT));
      seq_apply_transform_to_track(focus, &(focus->pt->transform));
      break;
    case SEGDISP_PATTERN:
      srand(Get_system_register(AVR32_COUNT));
      seq_apply_transform_to_pattern(focus, &(focus->pt->transform));
      break;
    case SEGDISP_STEP:
      srand(Get_system_register(AVR32_COUNT));
      seq_apply_transform_to_step(focus, &(focus->pt->transform));
      break;
    case SEGDISP_SNAPSHOT:

      break;
    }
    break;
  }
}

void math_left_increment(void)
{
  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
  case SEGDISP_VOLT1:
  case SEGDISP_VOLT01:
    break;
  case SEGDISP_CVA:
    increment_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.cvA_mode));
    display_transform_cvA(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  case SEGDISP_CVB:
    increment_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.cvB_mode));
    display_transform_cvB(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  case SEGDISP_DURATION:
    increment_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.duration_mode));
    display_transform_duration(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  case SEGDISP_GATE:
    increment_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.gate_mode));
    display_transform_gate(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  }
}

void math_right_increment(void)
{
  position_t *focus = SEQ_FOCUS;
  transform_t *t = &(focus->pt->transform);
  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
  case SEGDISP_VOLT1:
  case SEGDISP_VOLT01:
    break;
  case SEGDISP_CVA:
    transform_increment_cva(t);
    display_transform_cvA(t);
    break;
  case SEGDISP_CVB:
    transform_increment_cvb(t);
    display_transform_cvB(t);
    break;
  case SEGDISP_DURATION:
    transform_increment_duration(t);
    display_transform_duration(t);
    break;
  case SEGDISP_GATE:
    transform_increment_gate(t);
    display_transform_gate(t);
    break;
  }
}

void math_left_decrement(void)
{
  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
  case SEGDISP_VOLT1:
  case SEGDISP_VOLT01:
    break;
  case SEGDISP_CVA:
    decrement_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.cvA_mode));
    display_transform_cvA(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  case SEGDISP_CVB:
    decrement_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.cvB_mode));
    display_transform_cvB(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  case SEGDISP_DURATION:
    decrement_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.duration_mode));
    display_transform_duration(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  case SEGDISP_GATE:
    decrement_transform_mode(&(SEQ_EDIT_FOCUS->pt->transform.gate_mode));
    display_transform_gate(&(SEQ_EDIT_FOCUS->pt->transform));
    break;
  }
}

void math_right_decrement(void)
{
  position_t *focus = SEQ_FOCUS;
  transform_t *t = &(focus->pt->transform);
  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
  case SEGDISP_VOLT1:
  case SEGDISP_VOLT01:
    break;
  case SEGDISP_CVA:
    transform_decrement_cva(t);
    display_transform_cvA(t);
    break;
  case SEGDISP_CVB:
    transform_decrement_cvb(t);
    display_transform_cvB(t);
    break;
  case SEGDISP_DURATION:
    transform_decrement_duration(t);
    display_transform_duration(t);
    break;
  case SEGDISP_GATE:
    transform_decrement_gate(t);
    display_transform_gate(t);
    break;
  }
}

void ui_left_decrement(void)
{
  position_t save_focus;
  position_t *focus = SEQ_FOCUS;
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
  }
  if (seq.math_overlay_active)
  {
    math_left_decrement();
    return;
  }

  if (seq.track_overlay_active)
  {
    switch (focus_left_display)
    {
    case SEGDISP_TRACK:
      seq_decrement_track();
      start_track_overlay();
      break;
    case SEGDISP_STEP:
      if (focus->pt->ppqn > 1)
      {
        focus->pt->ppqn--;
        // segdisp_put_number(SEGDISP_STEP, focus->pt->ppqn);
        if (seq.mode != MODE_COMMIT)
        {
          seq_notify_ppqn_changed(focus);
        }
        track_overlay_focus = focus_left_display;
        update_track_overlay();
      }
      break;
    }
    return;
  }

  if (seq.selecting)
  {
    position_copy(&save_focus, SEQ_FOCUS);
  }
  switch (focus_left_display)
  {
  case SEGDISP_INDEX:
    if (seq.ref_table_active)
    {
      ref_decrement_index();
      return;
    }
    else if (seq.focus_voltage_index > 0)
    {
      seq.focus_voltage_index--;
    }
    seq_display_voltage();
    break;
  case SEGDISP_TRACK:
    seq_decrement_track();
    if (seq.selecting)
    {
      position_set_start(&seq.select_start, focus, SEGDISP_TRACK);
    }
    break;
  case SEGDISP_PATTERN:
    if (focus->s == focus->pm->first)
    {
      seq_prev_pattern(focus, true);
    }
    else
    {
      seq_pattern_begin(focus);
    }
    seq_display_position(focus);
    if (seq.mode == MODE_LIVE)
    {
      focus->ticks = 0;
      seq_write_to_dac(focus);
    }
    break;
  case SEGDISP_STEP:
    seq_prev_step(focus);
    seq_display_position(focus);
    if (seq.mode == MODE_LIVE)
    {
      focus->ticks = 0;
      seq_write_to_dac(focus);
    }
    break;
  case SEGDISP_SNAPSHOT:
    if (seq.focus_slot > 0)
      seq_set_slot(seq.focus_slot - 1);
    break;
  }
  if (seq.selecting)
  {
    if (position_is_before(&seq.select_start, &save_focus))
    {
      seq.nselected -= position_count_steps(&save_focus, focus);
    }
    else
    {
      seq.nselected += position_count_steps(&save_focus, focus);
    }
    seq_display_nselected();
  }
}

void ui_left_increment(void)
{
  position_t save_focus;
  position_t *focus = SEQ_FOCUS;
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
  }
  if (seq.math_overlay_active)
  {
    math_left_increment();
    return;
  }
  if (seq.track_overlay_active)
  {
    switch (focus_left_display)
    {
    case SEGDISP_TRACK:
      seq_increment_track();
      start_track_overlay();
      break;
    case SEGDISP_STEP:
      if (focus->pt->ppqn < 99)
      {
        focus->pt->ppqn++;
        // segdisp_put_number(SEGDISP_STEP, focus->pt->ppqn);
        if (seq.mode != MODE_COMMIT)
        {
          seq_notify_ppqn_changed(focus);
        }
        track_overlay_focus = focus_left_display;
        update_track_overlay();
      }
      break;
    }
    return;
  }
  if (seq.selecting)
  {
    position_copy(&save_focus, SEQ_FOCUS);
  }
  switch (focus_left_display)
  {
  case SEGDISP_INDEX:
    if (seq.ref_table_active)
    {
      ref_increment_index();
      return;
    }
    else if (seq.focus_voltage_index < NUM_VOLTAGES_PER_TABLE - 1)
    {
      seq.focus_voltage_index++;
    }
    seq_display_voltage();
    break;
  case SEGDISP_TRACK:
    seq_increment_track();
    break;
  case SEGDISP_PATTERN:
    if (focus->s == focus->pm->last)
    {
      seq_next_pattern(focus, true);
    }
    else
    {
      seq_pattern_end(focus);
    }
    seq_display_position(focus);
    if (seq.mode == MODE_LIVE)
    {
      focus->ticks = 0;
      seq_write_to_dac(focus);
    }
    break;
  case SEGDISP_STEP:
    seq_next_step(focus);
    seq_display_position(focus);
    if (seq.mode == MODE_LIVE)
    {
      focus->ticks = 0;
      seq_write_to_dac(focus);
    }
    break;
  case SEGDISP_SNAPSHOT:
    if (seq.focus_slot + 1 < NUM_SNAPSHOTS)
      seq_set_slot(seq.focus_slot + 1);
    break;
  }
  if (seq.selecting)
  {
    if (position_is_before(&save_focus, &seq.select_start))
    {
      seq.nselected -= position_count_steps(&save_focus, focus);
    }
    else
    {
      seq.nselected += position_count_steps(&save_focus, focus);
    }
    seq_display_nselected();
  }
}

void ui_right_decrement(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
  }
  if (seq.math_overlay_active)
  {
    math_right_decrement();
    return;
  }
  if (seq.insert_overlay_active)
  {
    switch (seq.insert_mode)
    {
    case INSERT_MIDDLE:
      seq.insert_mode = INSERT_BEFORE;
      break;
    case INSERT_AFTER:
      if (can_copy_from_clipboard())
      {
        seq.insert_mode = INSERT_BEFORE;
      }
      else
      {
        seq.insert_mode = INSERT_MIDDLE;
      }
      break;
    }
    update_insert_overlay();
    return;
  }
  if (seq.voltage_overlay_active)
  {
    on_decrement_voltage_grid();
    return;
  }
  if (seq.track_overlay_active)
  {
    switch (focus_right_display)
    {
    case SEGDISP_CVA:
      BIT_ASSIGN(false, SEQ_FOCUS->pt->options, OPTION_TABLE_A_PITCHED);
      // segdisp_display[SEGDISP_CVA] = SEGDISP_LEFT_N | SEGDISP_RIGHT_r;
      break;
    case SEGDISP_CVB:
      BIT_ASSIGN(false, SEQ_FOCUS->pt->options, OPTION_TABLE_B_PITCHED);
      // segdisp_display[SEGDISP_CVB] = SEGDISP_LEFT_N | SEGDISP_RIGHT_r;
      break;
    case SEGDISP_DURATION:
      if (SEQ_FOCUS->pt->divide > 1)
      {
        SEQ_FOCUS->pt->divide--;
        // segdisp_put_number(SEGDISP_DURATION, SEQ_FOCUS->pt->divide);
      }
      break;
    case SEGDISP_GATE:
      BIT_ASSIGN(false, SEQ_FOCUS->pt->options, OPTION_TRIGGER);
      // segdisp_display[SEGDISP_GATE] = SEGDISP_LEFT_G | SEGDISP_RIGHT_t;
      break;
    }
    track_overlay_focus = focus_right_display;
    update_track_overlay();
    return;
  }
  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
    if (seq.mode == MODE_LIVE && (stopped() || focus_left_display == SEGDISP_INDEX))
    {
      seq_decrement_voltage(VOLTAGE_TO_CODE(1));
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_decrement_voltage(VOLTAGE_TO_CODE(1));
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_VOLT1:
    if (seq.mode == MODE_LIVE && (stopped() || focus_left_display == SEGDISP_INDEX))
    {
      seq_decrement_voltage_coarse();
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_decrement_voltage_coarse();
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_VOLT01:
    if (seq.ref_table_active)
    {
      if (seq.focus_ref_table > 0)
        ref_select(seq.focus_ref_table - 1);
      return;
    }
    else if (seq.mode == MODE_LIVE && (stopped() || focus_left_display == SEGDISP_INDEX))
    {
      seq_decrement_voltage_super_coarse();
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_decrement_voltage_super_coarse();
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_CVA:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_decrement_cva(SEQ_LIVE_FOCUS);
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_decrement_cva(SEQ_EDIT_FOCUS);
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_CVB:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_decrement_cvb(SEQ_LIVE_FOCUS);
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_decrement_cvb(SEQ_EDIT_FOCUS);
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_DURATION:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_decrement_duration(SEQ_LIVE_FOCUS, gpio_pin_is_low(PIN_DURATION_BUTTON));
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_decrement_duration(SEQ_EDIT_FOCUS, gpio_pin_is_low(PIN_DURATION_BUTTON));
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_GATE:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_decrement_gate(SEQ_LIVE_FOCUS);
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_decrement_gate(SEQ_EDIT_FOCUS);
    }
    else
    {
      tilt_trigger();
    }
    break;
  }
}

void ui_right_increment(void)
{
  if (seq.confirm_mode != CONFIRM_NOTHING)
  {
    end_confirm();
  }
  if (seq.math_overlay_active)
  {
    math_right_increment();
    return;
  }
  if (seq.insert_overlay_active)
  {
    switch (seq.insert_mode)
    {
    case INSERT_MIDDLE:
      seq.insert_mode = INSERT_AFTER;
      break;
    case INSERT_BEFORE:
      if (can_copy_from_clipboard())
      {
        seq.insert_mode = INSERT_AFTER;
      }
      else
      {
        seq.insert_mode = INSERT_MIDDLE;
      }
      break;
    }
    update_insert_overlay();
    return;
  }
  if (seq.voltage_overlay_active)
  {
    on_increment_voltage_grid();
    return;
  }
  if (seq.track_overlay_active)
  {
    switch (focus_right_display)
    {
    case SEGDISP_CVA:
      BIT_ASSIGN(true, SEQ_FOCUS->pt->options, OPTION_TABLE_A_PITCHED);
      // segdisp_display[SEGDISP_CVA] = SEGDISP_LEFT_N | SEGDISP_RIGHT_t;
      break;
    case SEGDISP_CVB:
      BIT_ASSIGN(true, SEQ_FOCUS->pt->options, OPTION_TABLE_B_PITCHED);
      // segdisp_display[SEGDISP_CVB] = SEGDISP_LEFT_N | SEGDISP_RIGHT_t;
      break;
    case SEGDISP_DURATION:
      if (SEQ_FOCUS->pt->divide < 99)
      {
        SEQ_FOCUS->pt->divide++;
        // segdisp_put_number(SEGDISP_DURATION, SEQ_FOCUS->pt->divide);
      }
      break;
    case SEGDISP_GATE:
      BIT_ASSIGN(true, SEQ_FOCUS->pt->options, OPTION_TRIGGER);
      // segdisp_display[SEGDISP_GATE] = SEGDISP_LEFT_t | SEGDISP_RIGHT_r;
      break;
    }
    track_overlay_focus = focus_right_display;
    update_track_overlay();
    return;
  }
  switch (focus_right_display)
  {
  case SEGDISP_VOLT0:
    if (seq.mode == MODE_LIVE && (stopped() || focus_left_display == SEGDISP_INDEX))
    {
      seq_increment_voltage(VOLTAGE_TO_CODE(1));
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_increment_voltage(VOLTAGE_TO_CODE(1));
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_VOLT1:
    if (seq.mode == MODE_LIVE && (stopped() || focus_left_display == SEGDISP_INDEX))
    {
      seq_increment_voltage_coarse();
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_increment_voltage_coarse();
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_VOLT01:
    if (seq.ref_table_active)
    {
      if (seq.focus_ref_table < NUM_REF_TABLES - 1)
        ref_select(seq.focus_ref_table + 1);
      return;
    }
    else if (seq.mode == MODE_LIVE && (stopped() || focus_left_display == SEGDISP_INDEX))
    {
      seq_increment_voltage_super_coarse();
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_increment_voltage_super_coarse();
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_CVA:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_increment_cva(SEQ_LIVE_FOCUS);
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_increment_cva(SEQ_EDIT_FOCUS);
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_CVB:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_increment_cvb(SEQ_LIVE_FOCUS);
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_increment_cvb(SEQ_EDIT_FOCUS);
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_DURATION:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_increment_duration(SEQ_LIVE_FOCUS, gpio_pin_is_low(PIN_DURATION_BUTTON));
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_increment_duration(SEQ_EDIT_FOCUS, gpio_pin_is_low(PIN_DURATION_BUTTON));
    }
    else
    {
      tilt_trigger();
    }
    break;
  case SEGDISP_GATE:
    if (seq.mode == MODE_LIVE && stopped())
    {
      seq_increment_gate(SEQ_LIVE_FOCUS);
      seq_write_to_dac(SEQ_LIVE_FOCUS);
    }
    else if (seq.mode != MODE_LIVE)
    {
      seq_increment_gate(SEQ_EDIT_FOCUS);
    }
    else
    {
      tilt_trigger();
    }
    break;
  }
}

static walltimer_t slow_timer;
static walltimer_t fast_timer;
static wallclock_t fast_period;
static wallclock_t slow_period;

void ui_timers_init(void)
{
  fast_period = cpu_ms_2_cy(125, CPU_HZ);
  slow_period = cpu_ms_2_cy(333, CPU_HZ);
  walltimer_start(slow_period, &slow_timer);
  walltimer_start(fast_period, &fast_timer);
}

void ui_timers_maintenance(void)
{
  static bool slow_toggle = true;
  static bool fast_toggle = true;
  static uint8_t count = 0;

  if (walltimer_elapsed(&slow_timer))
  {
    walltimer_restart(slow_period, &slow_timer);

    slow_toggle = !slow_toggle;

    blink_message_update();

    // clear clock rotating display if no clock has arrived in awhile
    if (!seq.math_overlay_active)
    {
      if (wallclock() - clock_arrival_time > 2 * seq.latched_period)
      {
        segdisp_display[SEGDISP_TRACK] &= 0xFF00;
        seq.clock_is_late = true;
      }
      else
      {
        seq.clock_is_late = false;
      }
    }
  }

  if (walltimer_elapsed(&fast_timer))
  {
    walltimer_restart(fast_period, &fast_timer);

    fast_toggle = !fast_toggle;

    if (count == 0)
    {
      count = 4;
    }
    else
    {
      count--;
    }

    if (seq.waiting_for_clock_after_reset)
    {
      if (wallclock() - clock_arrival_time > seq.last_period * 2)
      {
        // it appears the clock has stopped so lets shut down all high gates
        seq_hard_reset();
        seq_update_dac_outputs(NULL);
        seq.waiting_for_clock_after_reset = false;
      }
    }

    if (seq.selecting)
    {
      gpio_tgl_gpio_pin(PIN_COPY_LED);
    }

    if (seq.commit_armed)
    {
      gpio_tgl_gpio_pin(PIN_COMMIT_LED);
    }

    if (seq.clipboard_mode == CLIPBOARD_MODE_TRACK || (seq.internal_reset_armed && seq.reset_quantization == QUANTIZE_TO_TRACK) || (seq.commit_armed && seq.commit_quantization == QUANTIZE_TO_TRACK))
    {
      if (fast_toggle)
      {
        orange_clear(SEGDISP_TRACK);
      }
      else
      {
        orange_set(SEGDISP_TRACK);
      }
    }

    if (seq.clipboard_mode == CLIPBOARD_MODE_STEP || (seq.internal_reset_armed && seq.reset_quantization == QUANTIZE_TO_STEP) || (seq.commit_armed && seq.commit_quantization == QUANTIZE_TO_STEP))
    {
      if (fast_toggle)
      {
        orange_clear(SEGDISP_STEP);
      }
      else
      {
        orange_set(SEGDISP_STEP);
      }
    }

    if (seq.clipboard_mode == CLIPBOARD_MODE_PATTERN || (seq.internal_reset_armed && seq.reset_quantization == QUANTIZE_TO_PATTERN) || (seq.commit_armed && seq.commit_quantization == QUANTIZE_TO_PATTERN))
    {
      if (fast_toggle)
      {
        orange_clear(SEGDISP_PATTERN);
      }
      else
      {
        orange_set(SEGDISP_PATTERN);
      }
    }

    if (seq.clipboard_mode == CLIPBOARD_MODE_TABLE)
    {
      if (fast_toggle)
      {
        orange_clear(SEGDISP_INDEX);
      }
      else
      {
        orange_set(SEGDISP_INDEX);
      }
    }

    if (math_state == MATH_STATE_HELD)
    {
      if (count == 0)
      {
        segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
        segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
      }
      else
      {
        segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_P;
        segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_1 | SEGDISP_RIGHT_N;
      }
    }
    else if (math_state == MATH_STATE_PINNED)
    {
      if (count == 0)
      {
        segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
        segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
      }
      else
      {
        segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_d | SEGDISP_RIGHT_O;
        segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_N | SEGDISP_RIGHT_E;
      }

      if (fast_toggle)
      {
        switch (focus_left_display)
        {
        case SEGDISP_INDEX:
        case SEGDISP_SNAPSHOT:
        case SEGDISP_TRACK:
          orange_clear(SEGDISP_TRACK);
          break;
        case SEGDISP_PATTERN:
          orange_clear(SEGDISP_PATTERN);
          break;
        case SEGDISP_STEP:
          orange_clear(SEGDISP_STEP);
          break;
        }
      }
      else
      {
        switch (focus_left_display)
        {
        case SEGDISP_INDEX:
        case SEGDISP_SNAPSHOT:
        case SEGDISP_TRACK:
          orange_set(SEGDISP_TRACK);
          break;
        case SEGDISP_PATTERN:
          orange_set(SEGDISP_PATTERN);
          break;
        case SEGDISP_STEP:
          orange_set(SEGDISP_STEP);
          break;
        }
      }
    }

    switch (seq.confirm_mode)
    {
    case CONFIRM_SAVE:
    case CONFIRM_LOAD:
      if (fast_toggle)
      {
        segdisp_display[SEGDISP_SNAPSHOT] = SEGDISP_BLANK;
      }
      else
      {
        seq_set_slot(seq.focus_slot);
      }
      break;
    case CONFIRM_DELETE:
      if (fast_toggle)
      {
        orange_clear(SEGDISP_TRACK);
      }
      else
      {
        orange_set(SEGDISP_TRACK);
      }
      break;
    }

    if (seq.confirm_mode != CONFIRM_NOTHING)
    {
      if (fast_toggle)
      {
        segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_A | SEGDISP_RIGHT_b;
        segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_r | SEGDISP_RIGHT_t;
      }
      else
      {
        segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
        segdisp_display[SEGDISP_VOLT0] = SEGDISP_LEFT_BLANK | SEGDISP_RIGHT_BLANK;
      }
    }

    if (seq.insert_overlay_active)
    {
      update_insert_overlay();
    }

    if (seq.voltage_overlay_active)
    {
      flash_voltage(fast_toggle);
    }

    if (seq.flash_lend)
    {
      if (fast_toggle)
        gpio_set_pin_low(PIN_LEND_LED);
      else
        gpio_set_pin_high(PIN_LEND_LED);
    }

    if (seq.flash_lstart)
    {
      if (fast_toggle)
        gpio_set_pin_low(PIN_LSTART_LED);
      else
        gpio_set_pin_high(PIN_LSTART_LED);
    }

    if (flash_state > 0)
    {
      flash_state--;
    }
  }
}
