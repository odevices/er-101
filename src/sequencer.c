/*
 * sequencer.c
 *
 * Created: 5/7/2013 10:37:33 AM
 *  Author: clarkson
 */

#include <asf.h>
#include <meta.h>
#include <wallclock.h>
#include <extdac.h>
#include <display.h>
#include <sequencer.h>
#include <string.h>
#include <voltages.h>
#include <clipboard.h>

sequencer_t seq;

const uint8_t smooth_option[2] = {OPTION_SMOOTH_A, OPTION_SMOOTH_B};
const uint8_t pitched_option[2] = {OPTION_TABLE_A_PITCHED, OPTION_TABLE_B_PITCHED};

#define PPQN_JUMP_TABLE_SIZE 12
const uint8_t ppqn_jump_table[PPQN_JUMP_TABLE_SIZE] = {2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96};

// reserve shared space for ui and event queues
uint32_t reserved_q_buffer[RESERVED_Q_SIZE];

static uint16_t find_next_semitone(uint16_t voltage)
{
  const uint16_t *semitones = &(ref_tables[0][0]);
  // binary search through the first ref table (semitones)
  uint16_t a = 0, b = 99, i = 49, previ = 100;
  while (i != previ)
  {
    if (voltage < semitones[i])
    {
      b = i;
    }
    else
    {
      a = i;
    }
    previ = i;
    i = (a + b) >> 1;
  }
  // now voltage is either in [semitones[a],semitones[b]) or is semitones[99]
  // but the next semitone is always semitone[b]
  return semitones[b];
}

static uint16_t find_prev_semitone(uint16_t voltage)
{
  const uint16_t *semitones = &(ref_tables[0][0]);
  // binary search through the first ref table (semitones)
  uint16_t a = 0, b = 99, i = 49, previ = 100;
  while (i != previ)
  {
    if (voltage > semitones[i])
    {
      a = i;
    }
    else
    {
      b = i;
    }
    previ = i;
    i = (a + b) >> 1;
  }
  // now voltage is either in (semitones[a],semitones[b]] or is semitones[0]
  // but the prev semitone is always semitone[a]
  return semitones[a];
}

void ref_display_table_name(void)
{
  segdisp_display[SEGDISP_VOLT1] = ref_table_name_left[seq.focus_ref_table];
  segdisp_display[SEGDISP_VOLT0] = ref_table_name_right[seq.focus_ref_table];
}

void ref_select(int table)
{
  seq.focus_ref_table = table;
  seq.focus_ref_index = INVALID_INDEX;
  segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_HYPHEN | SEGDISP_RIGHT_HYPHEN;
  segdisp_display[SEGDISP_VOLT1] = ref_table_name_left[table];
  segdisp_display[SEGDISP_VOLT0] = ref_table_name_right[table];
}

void ref_start(void)
{
  if (!seq.ref_table_active)
  {
    seq.ref_table_active = true;
    ref_select(0);
    uint8_t left_display = focus_left_display;
    uint8_t right_display = focus_right_display;
    select_left_display(SEGDISP_INDEX);
    select_right_display(SEGDISP_VOLT01);
    saved_left_display = left_display;
    saved_right_display = right_display;
  }
}

void ref_end(void)
{
  if (seq.ref_table_active)
  {
    seq.ref_table_active = false;
    if (saved_right_display == SEGDISP_VOLT01)
      seq_select_voltage_grid();
    else
      restore_right_display();
    restore_left_display();
  }
}

void ref_display(void)
{
  if (seq.focus_ref_index == INVALID_INDEX)
  {
    ref_select(seq.focus_ref_table);
  }
  else
  {
    segdisp_put_number(SEGDISP_INDEX, seq.focus_ref_index);
    if (ref_table_is_pitched[seq.focus_ref_table])
    {
      segdisp_put_note(ref_tables[seq.focus_ref_table][seq.focus_ref_index]);
    }
    else
    {
      segdisp_put_voltage(CODE_TO_VOLTAGE(ref_tables[seq.focus_ref_table][seq.focus_ref_index]));
      segdisp_set(SEGDISP_VOLT1, SEGDISP_LEFT_DP);
    }
  }
}

void ref_decrement_index(void)
{
  if (seq.focus_ref_index == INVALID_INDEX)
    seq.focus_ref_index = 0;
  if (seq.focus_ref_index > 0)
    seq.focus_ref_index--;
  ref_display();
}

void ref_increment_index(void)
{
  if (seq.focus_ref_index == INVALID_INDEX)
    seq.focus_ref_index = 0;
  if (seq.focus_ref_index < NUM_VOLTAGES_PER_TABLE - 1)
    seq.focus_ref_index++;
  ref_display();
}

void seq_write_to_dac(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;

  int A = p->pt->id * 2;
  int B = A + 1;
  int gate;
  int duration = p->ps->duration * p->pt->divide;

  if (BIT_TEST(p->pt->options, OPTION_TRIGGER))
  {
    // when trigger mode is enabled just assume that gate is half the step duration
    gate = duration >> 1;
  }
  else
  {
    gate = GET_STEP_GATE(p->ps->gate) * p->pt->divide;
  }

  if (seq_is_smooth(p, SEQ_TABLE_A) && duration > gate)
  {
    int v0 = p->pt->voltages[SEQ_TABLE_A][GET_STEP_INDEX(p->ps->cvA)];
    if (p->ticks >= gate)
    {
      position_t next;
      memcpy(&next, p, sizeof(position_t));
      seq_advance_position(&next);
      if (next.s == INVALID_STEP)
      {
        extdac_set_smoother_output(A, v0, v0);
      }
      else
      {
        int V;
        float Z, w0, w1;
        V = next.pt->voltages[SEQ_TABLE_A][GET_STEP_INDEX(next.ps->cvA)] - v0;

        Z = 1.0 / (duration - gate);

        w0 = (p->ticks - gate) * Z;
        // if(w0>1.0+FLOAT_EPSILON) w0 = 1.0;
        w1 = w0 + Z;
        // if(w1>1.0+FLOAT_EPSILON) w1 = 1.0;

        extdac_set_smoother_output(A, v0 + V * w0, v0 + V * w1);
      }
    }
    else
    {
      extdac_set_smoother_output(A, v0, v0);
    }
  }
  else
  {
    extdac_disable_smoother(A);
    extdac_set_value(A, p->pt->voltages[SEQ_TABLE_A][GET_STEP_INDEX(p->ps->cvA)]);
  }

  if (seq_is_smooth(p, SEQ_TABLE_B) && duration > gate)
  {
    int v0 = p->pt->voltages[SEQ_TABLE_B][GET_STEP_INDEX(p->ps->cvB)];
    if (p->ticks >= gate)
    {
      position_t next;
      memcpy(&next, p, sizeof(position_t));
      seq_advance_position(&next);
      if (next.s == INVALID_STEP)
      {
        extdac_set_smoother_output(B, v0, v0);
      }
      else
      {
        int V;
        float Z, w0, w1;
        V = next.pt->voltages[SEQ_TABLE_B][GET_STEP_INDEX(next.ps->cvB)] - v0;

        Z = 1.0 / (duration - gate);
        w0 = (p->ticks - gate) * Z;
        // if(w0>1.0+FLOAT_EPSILON) w0 = 1.0;
        w1 = w0 + Z;
        // if(w1>1.0+FLOAT_EPSILON) w1 = 1.0;

        extdac_set_smoother_output(B, v0 + V * w0, v0 + V * w1);
      }
    }
    else
    {
      extdac_set_smoother_output(B, v0, v0);
    }
  }
  else
  {
    extdac_disable_smoother(B);
    extdac_set_value(B, p->pt->voltages[SEQ_TABLE_B][GET_STEP_INDEX(p->ps->cvB)]);
  }
}

void seq_update_dac_outputs(bool *include)
{
  if (include == NULL || include[0])
  {
    seq_write_to_dac(&(seq.players[0]));
  }

  if (include == NULL || include[1])
  {
    seq_write_to_dac(&(seq.players[1]));
  }

  if (include == NULL || include[2])
  {
    seq_write_to_dac(&(seq.players[2]));
  }

  if (include == NULL || include[3])
  {
    seq_write_to_dac(&(seq.players[3]));
  }

  bool update_display = (!seq.math_overlay_active) && (seq.mode == MODE_LIVE) && (include == NULL || include[seq.focus_track]);
  if (update_display)
    seq_display_position(&(seq.players[seq.focus_track]));
}

void seq_update_gate_outputs(wallclock_t now, bool *include)
{
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    if (include != NULL && !include[i])
      continue;
    position_t *p = &(seq.players[i]);

    int gate = GET_STEP_GATE(p->ps->gate);
    int duration = p->ps->duration * p->pt->divide;

    if (BIT_TEST(p->pt->options, OPTION_TRIGGER))
    {
      if (!seq.pause && p->s != INVALID_STEP && duration > 0 && gate > 0)
      {
        if (GET_STEP_RATCHET(p->ps->gate))
        {
          gate *= p->pt->divide;
          if (gate > duration)
            gate = duration;
          if (p->ticks % gate == 0)
          {
            walltimer_schedule(now + min(seq.trigger_max_duration[i], 5 * seq.trigger_unit), &(seq.trigger_timers[i]));
            gpio_set_pin_high(seq.gate_output_pins[i]);
          }
        }
        else
        {
          if (p->ticks == 0)
          {
            walltimer_schedule(now + min(seq.trigger_max_duration[i], gate * seq.trigger_unit), &(seq.trigger_timers[i]));
            gpio_set_pin_high(seq.gate_output_pins[i]);
          }
        }
      }
    }
    else
    {
      if (!seq.pause && p->s != INVALID_STEP)
      {
        gate *= p->pt->divide;
        if (GET_STEP_RATCHET(p->ps->gate))
        {
          if (gate > duration)
            gate = duration;
          if (p->ticks >= duration - (duration % gate))
          {
            gpio_set_pin_low(seq.gate_output_pins[i]);
          }
          else
          {
            if (p->ticks % (2 * gate) < gate)
            {
              gpio_set_pin_high(seq.gate_output_pins[i]);
            }
            else
            {
              gpio_set_pin_low(seq.gate_output_pins[i]);
            }
          }
        }
        else
        {
          if (gate == duration && p->ticks + 1 == gate)
          {
            uint32_t period;
            if (p->pt->ppqn > 1)
            {
              period = PPQN_DIVIDE(seq.latched_period, p->pt->ppqn);
            }
            else
            {
              period = seq.latched_period;
            }
            walltimer_schedule(now + (period >> 1), &(seq.trigger_timers[i]));
            gpio_set_pin_high(seq.gate_output_pins[i]);
          }
          else if (p->ticks < gate)
          {
            gpio_set_pin_high(seq.gate_output_pins[i]);
          }
          else
          {
            gpio_set_pin_low(seq.gate_output_pins[i]);
          }
        }
      }
    }
  }
}

static bool skip_unplayable_steps(position_t *p)
{
  bool retval = false;

  // skip over empty patterns
  if (p->m != INVALID_PATTERN && p->s == INVALID_STEP)
  {
    pattern_id_t breadcrumb = p->m;
    while (p->s == INVALID_STEP)
    {
      pattern_id_t prev_m = p->m;
      retval = retval || seq_next_step_with_loop(p);

      if (p->m == breadcrumb || p->m == prev_m)
      {
        break;
      }
    }
  }

  // skip zero duration steps
  if (p->s != INVALID_STEP)
  {
    // skip over steps with zero duration
    step_id_t breadcrumb = p->s;
    while (p->ps->duration == 0)
    {
      step_id_t prev_s = p->s;
      retval = retval || seq_advance_position(p);
      p->ticks = 0;
      // stop advancing if we return to the original step or there is no change
      if (p->s == prev_s || p->s == breadcrumb || p->s == INVALID_STEP)
        break;
    }
  }

  return retval;
}

void synthetic_clock_maintenance(void)
{
  bool include[4] = {false, false, false, false};
  bool have_clock = false;
  if (seq.players[0].pt->ppqn > 1 && walltimer_elapsed(&(seq.ppqn_timers[0])))
  {
    include[0] = true;
    have_clock = true;
  }
  if (seq.players[1].pt->ppqn > 1 && walltimer_elapsed(&(seq.ppqn_timers[1])))
  {
    include[1] = true;
    have_clock = true;
  }
  if (seq.players[2].pt->ppqn > 1 && walltimer_elapsed(&(seq.ppqn_timers[2])))
  {
    include[2] = true;
    have_clock = true;
  }
  if (seq.players[3].pt->ppqn > 1 && walltimer_elapsed(&(seq.ppqn_timers[3])))
  {
    include[3] = true;
    have_clock = true;
  }
  if (have_clock)
    seq_on_clock_rising(wallclock(), true, include);
}

void seq_on_real_clock_rising(void)
{
  seq_on_clock_rising(wallclock(), false, NULL);
}

void seq_on_clock_rising(wallclock_t now, bool synthetic, bool *include)
{
  bool track_rewound[NUM_TRACKS] = {false, false, false, false};

  if (seq.pause)
    return;

  // advance the play position on each track
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    if (synthetic && !include[i])
      continue;
    position_t *p = &(seq.players[i]);

    if (p->pt->ppqn > 1)
    {
      unsigned long synthetic_clock_period = PPQN_DIVIDE(seq.latched_period, p->pt->ppqn);
      seq.trigger_max_duration[i] = synthetic_clock_period - seq.trigger_unit;
      if (synthetic)
      {
        if (seq.ppqn_counters[i] < p->pt->ppqn)
        {
          walltimer_restart(synthetic_clock_period, &(seq.ppqn_timers[i]));
          seq.ppqn_counters[i]++;
        }
      }
      else
      {
        walltimer_schedule(now + synthetic_clock_period, &(seq.ppqn_timers[i]));
        seq.ppqn_counters[i] = 2;
      }
      extdac_set_smoother_timing(i, now, synthetic_clock_period);
    }
    else
    {
      extdac_set_smoother_timing(i, now, seq.latched_period);
      seq.trigger_max_duration[i] = seq.latched_period - seq.trigger_unit;
    }

    if (p->s == INVALID_STEP)
    {
      // try to reinitialize the play position
      position_init(p, p->data, p->pt);
      seq.current_tick_adjustment[i] = 0;
      seq.next_tick_adjustment[i] = 0;
      track_rewound[i] = true;
    }
    else
    {
      int16_t duration = (p->ps->duration + seq.current_tick_adjustment[i]) * p->pt->divide;
      if (p->ticks >= duration)
      {
        track_rewound[i] = seq_advance_position(p);
        p->ticks = 0;
        seq.current_tick_adjustment[i] = seq.next_tick_adjustment[i];
        seq.next_tick_adjustment[i] = 0;
      }
    }

    track_rewound[i] = track_rewound[i] || skip_unplayable_steps(p);
  }

  if (seq.commit_armed)
  {
    // If a quantized commit is armed then watch for the appropriate event
    position_t *p = &(seq.players[seq.trigger_track]);
    switch (seq.commit_quantization)
    {
    case NO_QUANTIZATION:
      seq_commit();
      break;
    case QUANTIZE_TO_STEP:
      if (p->m != INVALID_PATTERN && p->ticks == 0)
      {
        seq_commit();
      }
      break;
    case QUANTIZE_TO_PATTERN:
      if (p->m != INVALID_PATTERN && p->ticks == 0)
      {
        if (p->s == p->pm->first)
        {
          seq_commit();
        }
      }
      break;
    case QUANTIZE_TO_TRACK:
      if (track_rewound[seq.trigger_track])
      {
        seq_commit();
      }
      break;
    }
  }

  if (seq.external_reset_armed && !synthetic)
  {
    seq_hard_reset();
  }
  else if (seq.internal_reset_armed)
  {
    // If a quantized reset is armed then watch for the appropriate event
    position_t *p = &(seq.players[seq.trigger_track]);
    switch (seq.reset_quantization)
    {
    case NO_QUANTIZATION:
      seq_hard_reset();
      break;
    case QUANTIZE_TO_STEP:
      if (p->m != INVALID_PATTERN && p->ticks == 0)
      {
        seq_hard_reset();
      }
      break;
    case QUANTIZE_TO_PATTERN:
      if (p->m != INVALID_PATTERN && p->ticks == 0)
      {
        if (p->s == p->pm->first)
        {
          seq_hard_reset();
        }
      }
      break;
    case QUANTIZE_TO_TRACK:
      if (track_rewound[seq.trigger_track])
      {
        seq_hard_reset();
      }
      break;
    }
  }

  // Now set the outputs
  seq_update_dac_outputs(include);
  seq_update_gate_outputs(now, include);

  // increment the clock count for the appropriate players
  if (synthetic)
  {
    if (include[0])
    {
      seq.players[0].ticks++;
    }
    if (include[1])
    {
      seq.players[1].ticks++;
    }
    if (include[2])
    {
      seq.players[2].ticks++;
    }
    if (include[3])
    {
      seq.players[3].ticks++;
    }
  }
  else
  {
    seq.players[0].ticks++;
    seq.players[1].ticks++;
    seq.players[2].ticks++;
    seq.players[3].ticks++;
  }
}

pattern_id_t seq_allocate_pattern(sequencer_data_t *data)
{
  if (data->patterns.head == MAX_PATTERNS)
  {
    return INVALID_PATTERN;
  }
  pattern_id_t m = data->patterns.available[data->patterns.head];
  data->patterns.head++;
  pattern_init(&PATTERN(data, m));
  return m;
}

void seq_free_pattern(sequencer_data_t *data, pattern_id_t m)
{
  if (data->patterns.head == 0)
    return;
  data->patterns.head--;
  data->patterns.available[data->patterns.head] = m;
}

step_id_t seq_allocate_step(sequencer_data_t *data)
{
  if (data->steps.head == MAX_STEPS)
  {
    return INVALID_STEP;
  }
  step_id_t s = data->steps.available[data->steps.head];
  data->steps.head++;
  step_init(&STEP(data, s));
  return s;
}

void seq_free_step(sequencer_data_t *data, step_id_t s)
{
  if (data->steps.head == 0)
    return;
  data->steps.head--;
  data->steps.available[data->steps.head] = s;
}

void step_init(step_t *step)
{
  step->cvA = 0;
  step->cvB = 0;
  step->duration = 0;
  step->gate = 0;
  step->prev = INVALID_STEP;
  step->next = INVALID_STEP;
}

void seq_set_slot(uint8_t i)
{
  seq.focus_slot = i;
  if (seq.focus_slot == BLANK_SNAPSHOT)
  {
    segdisp_display[SEGDISP_SNAPSHOT] = SEGDISP_LEFT_HYPHEN | SEGDISP_RIGHT_HYPHEN;
  }
  else
  {
    segdisp_put_number(SEGDISP_SNAPSHOT, i);
  }
}

void seq_set_table(uint8_t i)
{
  if (i < 2)
  {
    seq.focus_voltage_table = i;
    if (focus_left_display != SEGDISP_INDEX && SEQ_FOCUS->s != INVALID_STEP)
    {
      if (seq.focus_voltage_table == SEQ_TABLE_A)
      {
        seq.focus_voltage_index = GET_STEP_INDEX(SEQ_FOCUS->ps->cvA);
      }
      else
      {
        seq.focus_voltage_index = GET_STEP_INDEX(SEQ_FOCUS->ps->cvB);
      }
    }
  }
  seq_display_voltage();
}

void seq_set_track(uint8_t i)
{
  seq.focus_track = i;
  if (focus_right_display == SEGDISP_VOLT0 || focus_right_display == SEGDISP_VOLT1 || focus_right_display == SEGDISP_VOLT01)
  {
    orange_clear(focus_right_display);
    if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_COARSE))
    {
      focus_right_display = SEGDISP_VOLT1;
    }
    else if (BIT_TEST(SEQ_FOCUS->pt->options, OPTION_SUPER_COARSE))
    {
      focus_right_display = SEGDISP_VOLT01;
    }
    else
    {
      // if none are set, then fine
      focus_right_display = SEGDISP_VOLT0;
    }
    orange_set(focus_right_display);
  }
  seq_display_position(SEQ_FOCUS);
}

void seq_set_track_smooth(position_t *p, uint8_t table, bool value)
{
  BIT_ASSIGN(value, p->pt->options, smooth_option[table]);
  seq_display_position(SEQ_FOCUS);
}

void seq_set_pattern_smooth(position_t *p, uint8_t table, bool value)
{
  if (p->m != INVALID_PATTERN)
  {
    BIT_ASSIGN(value, p->pm->options, smooth_option[table]);
  }
  seq_display_position(SEQ_FOCUS);
}

void seq_set_step_smooth(position_t *p, uint8_t table, bool value)
{
  if (p->s != INVALID_STEP)
  {
    if (table == SEQ_TABLE_A)
    {
      SET_STEP_SMOOTH(p->ps->cvA, value);
    }
    else if (table == SEQ_TABLE_B)
    {
      SET_STEP_SMOOTH(p->ps->cvB, value);
    }
  }
  seq_display_position(SEQ_FOCUS);
}

void seq_decrement_track(void)
{
  if (seq.focus_track == 0)
  {
    seq_set_track(0);
  }
  else
  {
    seq_set_track(seq.focus_track - 1);
  }
}

void seq_increment_track(void)
{
  if (seq.focus_track == NUM_TRACKS - 1)
  {
    seq_set_track(NUM_TRACKS - 1);
  }
  else
  {
    seq_set_track(seq.focus_track + 1);
  }
}

void seq_hard_reset()
{
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    gpio_set_pin_low(seq.gate_output_pins[i]);
    position_init(&(seq.players[i]), &(seq.data), &(seq.data.tracks[i]));
    seq.current_tick_adjustment[i] = 0;
    seq.next_tick_adjustment[i] = 0;
  }
  seq.internal_reset_armed = false;
  seq.external_reset_armed = false;
  if (!seq.math_overlay_active)
  {
    select_left_display(focus_left_display);
  }
}

bool seq_advance_position(position_t *p)
{
  bool retval = false;

  retval = retval || seq_next_step_with_loop(p);

  // skip over empty patterns
  if (p->m != INVALID_PATTERN && p->s == INVALID_STEP)
  {
    pattern_id_t breadcrumb = p->m;
    while (p->s == INVALID_STEP)
    {
      pattern_id_t prev_m = p->m;
      retval = retval || seq_next_step_with_loop(p);

      if (p->m == breadcrumb || p->m == prev_m)
      {
        break;
      }
    }
  }

  return retval;
}

bool seq_next_step_with_loop(position_t *p)
{
  if (p->m == p->pt->last && p->s == p->pm->last)
  {
    // return to the beginning
    seq_rewind(p);
    return true;
  }
  else
  {
    if (p->pt->lend != INVALID_STEP && p->s == p->pt->lend)
    {
      // loop end
      seq_rewind(p);
      return true;
    }
    else
    {
      seq_next_step(p);
      return false;
    }
  }
}

bool seq_next_pattern(position_t *p, bool fail_to_last_step)
{
  if (p->m == INVALID_PATTERN)
    return false;
  if (p->pm->next == INVALID_PATTERN)
  {
    if (fail_to_last_step)
    {
      // current step will be the last step of this pattern
      p->s = p->pm->last;
      if (p->s == INVALID_STEP)
      {
        p->ps = NULL;
        p->sname = 0;
      }
      else
      {
        p->ps = &STEP(p->data, p->s);
        p->sname = p->pm->ns;
      }
    }
    return false;
  }
  p->m = p->pm->next;
  p->pm = &PATTERN(p->data, p->m);
  p->mname++;
  // current step will be the first step of the next pattern
  p->s = p->pm->first;
  if (p->s == INVALID_STEP)
  {
    p->ps = NULL;
    p->sname = 0;
  }
  else
  {
    p->ps = &STEP(p->data, p->s);
    p->sname = 1;
  }
  return true;
}

bool seq_prev_pattern(position_t *p, bool fail_to_first_step)
{
  if (p->m == INVALID_PATTERN)
    return false;
  if (p->pm->prev == INVALID_PATTERN)
  {
    if (fail_to_first_step)
    {
      // current step will be the first step of this pattern
      p->s = p->pm->first;
      if (p->s == INVALID_STEP)
      {
        p->ps = NULL;
        p->sname = 0;
      }
      else
      {
        p->ps = &STEP(p->data, p->s);
        p->sname = 1;
      }
    }
    return false;
  }
  p->m = p->pm->prev;
  p->pm = &PATTERN(p->data, p->m);
  p->mname--;
  // current step will be the last step of the prev pattern
  p->s = p->pm->last;
  if (p->s == INVALID_STEP)
  {
    p->ps = NULL;
    p->sname = 0;
  }
  else
  {
    p->ps = &STEP(p->data, p->s);
    p->sname = p->pm->ns;
  }
  return true;
}

bool seq_next_step(position_t *p)
{
  if (p->m == INVALID_PATTERN)
    return false;
  if (p->s == p->pm->last)
  {
    // try to go to the next pattern
    return seq_next_pattern(p, false);
  }
  p->s = p->ps->next;
  p->ps = &STEP(p->data, p->s);
  p->sname++;
  return true;
}

bool seq_prev_step(position_t *p)
{
  if (p->m == INVALID_PATTERN)
    return false;
  if (p->s == p->pm->first)
  {
    // try to go to the prev pattern
    return seq_prev_pattern(p, false);
  }
  p->s = p->ps->prev;
  p->ps = &STEP(p->data, p->s);
  p->sname--;
  return true;
}

void seq_display_nselected(void)
{
  segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_n | SEGDISP_RIGHT_S;
  segdisp_put_dualdigit(SEGDISP_VOLT0, (seq.nselected) % 100);
  segdisp_put_dualdigit(SEGDISP_VOLT1, ((seq.nselected) / 100) % 100);
}

void seq_display_voltage(void)
{
  if (seq.ref_table_active)
  {
    ref_display();
  }
  else if (seq.selecting)
  {
    seq_display_nselected();
  }
  else
  {
    track_t *pt = &(seq_get_focused_data()->tracks[seq.focus_track]);
    uint16_t V = pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index];
    segdisp_put_number(SEGDISP_INDEX, seq.focus_voltage_index);
    if (BIT_TEST(pt->options, pitched_option[seq.focus_voltage_table]))
    {
      segdisp_put_note(V);
    }
    else
    {
      segdisp_put_voltage(CODE_TO_VOLTAGE(V));
      segdisp_set(SEGDISP_VOLT1, SEGDISP_LEFT_DP);
    }
  }
}

void seq_display_step(step_t *step)
{
  if (step == NULL)
  {
    segdisp_display[SEGDISP_CVA] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_CVB] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_DURATION] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_GATE] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    if (seq.insert_overlay_active || seq.confirm_mode != CONFIRM_NOTHING || seq.blink_active || seq.clipboard_mode == CLIPBOARD_MODE_TABLE)
    {
      // do nothing
    }
    else if (seq.ref_table_active)
    {
      ref_display();
    }
    else
    {
      seq_display_voltage();
    }
    seq.focus_voltage_index = 0;
  }
  else
  {
    segdisp_put_number(SEGDISP_CVA, GET_STEP_INDEX(step->cvA));
    segdisp_put_number(SEGDISP_CVB, GET_STEP_INDEX(step->cvB));
    segdisp_put_number(SEGDISP_DURATION, step->duration);
    segdisp_put_number(SEGDISP_GATE, GET_STEP_GATE(step->gate));
    if (GET_STEP_RATCHET(step->gate))
    {
      segdisp_display[SEGDISP_GATE] |= SEGDISP_LEFT_DP | SEGDISP_RIGHT_DP;
    }

    // also update voltage display
    if (seq.ref_table_active)
    {
      ref_display();
    }
    else
    {
      if (focus_left_display != SEGDISP_INDEX)
      {
        if (seq.focus_voltage_table == SEQ_TABLE_A)
        {
          seq.focus_voltage_index = GET_STEP_INDEX(step->cvA);
        }
        else
        {
          seq.focus_voltage_index = GET_STEP_INDEX(step->cvB);
        }
        if (!(seq.insert_overlay_active || seq.confirm_mode != CONFIRM_NOTHING || seq.blink_active || seq.clipboard_mode == CLIPBOARD_MODE_TABLE))
        {
          seq_display_voltage();
        }
      }
    }
  }
}
bool seq_is_smooth(position_t *p, uint8_t table)
{
  if (BIT_TEST(p->pt->options, smooth_option[table]))
  {
    return true;
  }

  if (p->m != INVALID_PATTERN && BIT_TEST(p->pm->options, smooth_option[table]))
  {
    return true;
  }

  if (p->s != INVALID_STEP)
  {
    if (table == SEQ_TABLE_A)
    {
      return GET_STEP_SMOOTH(p->ps->cvA);
    }
    else if (table == SEQ_TABLE_B)
    {
      return GET_STEP_SMOOTH(p->ps->cvB);
    }
  }

  return false;
}

bool seq_is_track_smooth(position_t *p, uint8_t table)
{
  return BIT_TEST(p->pt->options, smooth_option[table]);
}

bool seq_is_pattern_smooth(position_t *p, uint8_t table)
{
  if (p->m != INVALID_PATTERN && BIT_TEST(p->pm->options, smooth_option[table]))
  {
    return true;
  }
  return false;
}

bool seq_is_step_smooth(position_t *p, uint8_t table)
{
  if (p->s != INVALID_STEP)
  {
    if (table == SEQ_TABLE_A)
    {
      return GET_STEP_SMOOTH(p->ps->cvA);
    }
    else if (table == SEQ_TABLE_B)
    {
      return GET_STEP_SMOOTH(p->ps->cvB);
    }
  }

  return false;
}

bool seq_step_in_pattern(position_t *p, step_id_t s)
{
  position_t iter;

  if (p->m == INVALID_PATTERN || s == INVALID_STEP)
    return false;

  position_copy(&iter, p);
  seq_pattern_begin(&iter);
  if (iter.s == s)
    return true;

  while (iter.s != iter.pm->last)
  {
    if (!seq_next_step(&iter))
      break;
    if (iter.s == s)
      return true;
  }

  return false;
}

void seq_display_loop(position_t *p)
{
  seq.flash_lstart = false;
  seq.flash_lend = false;
  if (p->m == INVALID_PATTERN || p->s == INVALID_STEP)
  {
    gpio_set_pin_high(PIN_LSTART_LED);
    gpio_set_pin_high(PIN_LEND_LED);
  }
  else if (focus_left_display == SEGDISP_PATTERN)
  {
    if (p->pm->first == p->pt->lstart)
    {
      gpio_set_pin_low(PIN_LSTART_LED);
    }
    else
    {
      gpio_set_pin_high(PIN_LSTART_LED);
      seq.flash_lstart = seq_step_in_pattern(p, p->pt->lstart);
    }
    if (p->pm->last == p->pt->lend)
    {
      gpio_set_pin_low(PIN_LEND_LED);
    }
    else
    {
      gpio_set_pin_high(PIN_LEND_LED);
      seq.flash_lend = seq_step_in_pattern(p, p->pt->lend);
    }
  }
  else
  {
    if (p->s == p->pt->lstart)
    {
      gpio_set_pin_low(PIN_LSTART_LED);
    }
    else
    {
      gpio_set_pin_high(PIN_LSTART_LED);
    }
    if (p->s == p->pt->lend)
    {
      gpio_set_pin_low(PIN_LEND_LED);
    }
    else
    {
      gpio_set_pin_high(PIN_LEND_LED);
    }
  }
}

void seq_display_position(position_t *p)
{
  segdisp_put_right_digit(SEGDISP_TRACK, seq.focus_track + 1);

  if (seq.voltage_overlay_active || seq.track_overlay_active)
    return;

  if (p->m == INVALID_PATTERN)
  {
    segdisp_display[SEGDISP_PATTERN] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_STEP] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_CVA] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_CVB] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_DURATION] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    segdisp_display[SEGDISP_GATE] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
    if (seq.insert_overlay_active || seq.confirm_mode != CONFIRM_NOTHING || seq.blink_active || seq.clipboard_mode == CLIPBOARD_MODE_TABLE)
    {
      // do nothing
    }
    else if (seq.ref_table_active)
    {
      ref_display();
    }
    else
    {
      seq_display_voltage();
    }
  }
  else
  {
    segdisp_put_number(SEGDISP_PATTERN, p->mname);
    if (p->s == INVALID_STEP)
    {
      segdisp_display[SEGDISP_STEP] = SEGDISP_RIGHT_HYPHEN | SEGDISP_LEFT_HYPHEN;
      gpio_set_pin_high(PIN_LSTART_LED);
      gpio_set_pin_high(PIN_LEND_LED);
      seq_display_step(NULL);
    }
    else
    {
      segdisp_put_number(SEGDISP_STEP, p->sname);
      seq_display_step(p->ps);
    }
  }

  bool smooth = false;
  if (seq_is_step_smooth(p, seq.focus_voltage_table))
  {
    segdisp_set(SEGDISP_STEP, SEGDISP_RIGHT_DP);
    smooth = true;
  }

  if (seq_is_pattern_smooth(p, seq.focus_voltage_table))
  {
    segdisp_set(SEGDISP_PATTERN, SEGDISP_RIGHT_DP);
    smooth = true;
  }

  if (seq_is_track_smooth(p, seq.focus_voltage_table))
  {
    segdisp_set(SEGDISP_TRACK, SEGDISP_RIGHT_DP);
    smooth = true;
  }

  if (smooth)
  {
    gpio_set_pin_low(PIN_SMOOTH_LED);
  }
  else
  {
    gpio_set_pin_high(PIN_SMOOTH_LED);
  }

  seq_display_loop(p);
}

void seq_increment_voltage(uint16_t amt)
{
  track_t *pt = &(seq_get_focused_data()->tracks[seq.focus_track]);
  uint16_t v = pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index];
  if (EXTDAC_MAX_OUTPUT - v >= amt)
  {
    v += amt;
  }
  pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index] = v;
  if (BIT_TEST(pt->options, pitched_option[seq.focus_voltage_table]))
  {
    segdisp_put_note(v);
  }
  else
  {
    segdisp_put_voltage(CODE_TO_VOLTAGE(v));
    segdisp_set(SEGDISP_VOLT1, SEGDISP_LEFT_DP);
  }
}

void seq_decrement_voltage(uint16_t amt)
{
  track_t *pt = &(seq_get_focused_data()->tracks[seq.focus_track]);
  uint16_t v = pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index];
  if (v >= amt)
  {
    v -= amt;
  }
  pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index] = v;
  if (BIT_TEST(pt->options, pitched_option[seq.focus_voltage_table]))
  {
    segdisp_put_note(v);
  }
  else
  {
    segdisp_put_voltage(CODE_TO_VOLTAGE(v));
    segdisp_set(SEGDISP_VOLT1, SEGDISP_LEFT_DP);
  }
}

void seq_increment_voltage_coarse()
{
  track_t *pt = &(seq_get_focused_data()->tracks[seq.focus_track]);
  if (BIT_TEST(pt->options, pitched_option[seq.focus_voltage_table]))
  {
    uint16_t v = pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index];
    v = find_next_semitone(v);
    pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index] = v;
    segdisp_put_note(v);
  }
  else
  {
    seq_increment_voltage(VOLTAGE_TO_CODE(100));
  }
}

void seq_decrement_voltage_coarse()
{
  track_t *pt = &(seq_get_focused_data()->tracks[seq.focus_track]);
  if (BIT_TEST(pt->options, pitched_option[seq.focus_voltage_table]))
  {
    uint16_t v = pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index];
    v = find_prev_semitone(v);
    pt->voltages[seq.focus_voltage_table][seq.focus_voltage_index] = v;
    segdisp_put_note(v);
  }
  else
  {
    seq_decrement_voltage(VOLTAGE_TO_CODE(100));
  }
}

void seq_increment_voltage_super_coarse(void)
{
  seq_increment_voltage(VOLTAGE_TO_CODE(1000));
}

void seq_decrement_voltage_super_coarse(void)
{
  seq_decrement_voltage(VOLTAGE_TO_CODE(1000));
}

void seq_increment_cva(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (GET_STEP_INDEX(step->cvA) < NUM_VOLTAGES_PER_TABLE - 1)
  {
    step->cvA++;
  }

  seq_display_step(step);
}

void seq_decrement_cva(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (GET_STEP_INDEX(step->cvA) > 0)
  {
    step->cvA--;
  }

  seq_display_step(step);
}

void seq_increment_cvb(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (GET_STEP_INDEX(step->cvB) < NUM_VOLTAGES_PER_TABLE - 1)
  {
    step->cvB++;
  }

  seq_display_step(step);
}

void seq_decrement_cvb(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (GET_STEP_INDEX(step->cvB) > 0)
  {
    step->cvB--;
  }

  seq_display_step(step);
}

void seq_toggle_ratchet(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (GET_STEP_RATCHET(step->gate))
  {
    SET_STEP_RATCHET(step->gate, false);
    blink_double_message(SEGDISP_LEFT_n | SEGDISP_RIGHT_t | SEGDISP_RIGHT_DP, SEGDISP_LEFT_r | SEGDISP_RIGHT_P, SEGDISP_RIGHT_o, SEGDISP_LEFT_F | SEGDISP_RIGHT_F);
  }
  else
  {
    SET_STEP_RATCHET(step->gate, true);
    blink_double_message(SEGDISP_LEFT_n | SEGDISP_RIGHT_t | SEGDISP_RIGHT_DP, SEGDISP_LEFT_r | SEGDISP_RIGHT_P, SEGDISP_BLANK, SEGDISP_LEFT_o | SEGDISP_RIGHT_n);
    // For user convenience, if gate length is longer then duration then clamp it at the duration
    if (GET_STEP_GATE(step->gate) > step->duration)
    {
      SET_STEP_GATE(step->gate, step->duration);
    }
  }

  segdisp_put_number(SEGDISP_GATE, GET_STEP_GATE(step->gate));
  if (GET_STEP_RATCHET(step->gate))
  {
    segdisp_display[SEGDISP_GATE] |= SEGDISP_LEFT_DP | SEGDISP_RIGHT_DP;
  }
}

void seq_increment_gate(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (GET_STEP_GATE(step->gate) < 99)
  {
    SET_STEP_GATE(step->gate, step->gate + 1);
  }

  segdisp_put_number(SEGDISP_GATE, GET_STEP_GATE(step->gate));
  if (GET_STEP_RATCHET(step->gate))
  {
    segdisp_display[SEGDISP_GATE] |= SEGDISP_LEFT_DP | SEGDISP_RIGHT_DP;
  }
}

void seq_decrement_gate(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (GET_STEP_GATE(step->gate) > 0)
  {
    SET_STEP_GATE(step->gate, step->gate - 1);
  }

  segdisp_put_number(SEGDISP_GATE, GET_STEP_GATE(step->gate));
  if (GET_STEP_RATCHET(step->gate))
  {
    segdisp_display[SEGDISP_GATE] |= SEGDISP_LEFT_DP | SEGDISP_RIGHT_DP;
  }
}

void seq_increment_duration(position_t *p, bool steal)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (step->duration < 99)
  {
    if (steal)
    {
      position_t next;
      position_copy(&next, p);
      seq_next_step_with_loop(&next);
      if (next.s == INVALID_STEP)
      {
        step->duration++;
      }
      else if (next.ps->duration > 0)
      {
        step->duration++;
        next.ps->duration--;
        if (step == seq.players[seq.focus_track].ps)
        {
          seq.current_tick_adjustment[seq.focus_track]--;
          seq.next_tick_adjustment[seq.focus_track]++;
        }
        else if (next.ps == seq.players[seq.focus_track].ps)
        {
          seq.current_tick_adjustment[seq.focus_track]++;
        }
        segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_HYPHEN | SEGDISP_RIGHT_HYPHEN;
        segdisp_put_dualdigit(SEGDISP_VOLT1, step->duration);
        segdisp_put_dualdigit(SEGDISP_VOLT0, next.ps->duration);
        segdisp_set(SEGDISP_VOLT1, SEGDISP_RIGHT_DP);
      }
    }
    else
    {
      step->duration++;
    }
  }

  segdisp_put_number(SEGDISP_DURATION, step->duration);
}

void seq_decrement_duration(position_t *p, bool steal)
{
  if (p->s == INVALID_STEP)
    return;
  step_t *step = p->ps;

  if (step->duration > 0)
  {
    if (steal)
    {
      position_t next;
      position_copy(&next, p);
      seq_next_step_with_loop(&next);
      if (next.s == INVALID_STEP)
      {
        step->duration--;
      }
      else if (next.ps->duration < 99)
      {
        step->duration--;
        next.ps->duration++;
        if (step == seq.players[seq.focus_track].ps)
        {
          seq.current_tick_adjustment[seq.focus_track]++;
          seq.next_tick_adjustment[seq.focus_track]--;
        }
        else if (next.ps == seq.players[seq.focus_track].ps)
        {
          seq.current_tick_adjustment[seq.focus_track]--;
        }
        segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_HYPHEN | SEGDISP_RIGHT_HYPHEN;
        segdisp_put_dualdigit(SEGDISP_VOLT1, step->duration);
        segdisp_put_dualdigit(SEGDISP_VOLT0, next.ps->duration);
        segdisp_set(SEGDISP_VOLT1, SEGDISP_RIGHT_DP);
      }
    }
    else
    {
      step->duration--;
    }
  }

  segdisp_put_number(SEGDISP_DURATION, step->duration);
}

void pattern_init(pattern_t *pattern)
{
  pattern->first = INVALID_STEP;
  pattern->last = INVALID_STEP;
  pattern->prev = INVALID_PATTERN;
  pattern->next = INVALID_PATTERN;
  pattern->ns = 0;
  pattern->options = 0;
}

void track_init(track_t *track, uint8_t id)
{
  transform_init(&(track->transform));
  track->id = id;
  track->nm = 0;
  track->first = INVALID_PATTERN;
  track->last = INVALID_PATTERN;
  track->lstart = INVALID_STEP;
  track->lend = INVALID_STEP;
  track->options = OPTION_TABLE_A_PITCHED;
  track->ppqn = 1;
  track->divide = 1;

  for (int i = 0; i < NUM_VOLTAGES_PER_TABLE; i++)
  {
    track->voltages[0][i] = ref_tables[REF_TABLE_SEMITONES][i];
    track->voltages[1][i] = ref_tables[REF_TABLE_LINEAR][i];
  }
}

void seq_set_lstart(position_t *p)
{
  if (focus_left_display == SEGDISP_PATTERN)
  {
    if (p->m != INVALID_PATTERN)
    {
      if (p->pt->lstart == p->pm->first)
      {
        p->pt->lstart = INVALID_STEP;
      }
      else
      {
        p->pt->lstart = p->pm->first;
        flash(SEGDISP_PATTERN, SEGDISP_STEP, SEGDISP_RIGHT_1);
      }
    }
  }
  else
  {
    if (p->s != INVALID_STEP)
    {
      if (p->pt->lstart == p->s)
      {
        p->pt->lstart = INVALID_STEP;
      }
      else
      {
        p->pt->lstart = p->s;
      }
    }
  }
}

void seq_set_lend(position_t *p)
{
  if (focus_left_display == SEGDISP_PATTERN)
  {
    if (p->m != INVALID_PATTERN)
    {
      if (p->pt->lend == p->pm->last)
      {
        p->pt->lend = INVALID_STEP;
      }
      else
      {
        p->pt->lend = p->pm->last;
        if (p->pm->ns > 9)
        {
          flash(SEGDISP_PATTERN, SEGDISP_STEP, segdisp_both_digits[p->pm->ns]);
        }
        else
        {
          flash(SEGDISP_PATTERN, SEGDISP_STEP, segdisp_right_digits[p->pm->ns]);
        }
      }
    }
  }
  else
  {
    if (p->s != INVALID_STEP)
    {
      if (p->pt->lend == p->s)
      {
        p->pt->lend = INVALID_STEP;
      }
      else
      {
        p->pt->lend = p->s;
      }
    }
  }
}

void seq_rewind(position_t *p)
{
  if (p->pt->lstart != INVALID_STEP)
  {
    while (p->s != p->pt->lstart)
    {
      if (p->m == p->pt->first && p->s == p->pm->first)
        break;
      step_id_t prev_s = p->s;
      pattern_id_t prev_m = p->m;
      seq_prev_step(p);
      // protection against infinite loops
      if (p->m == prev_m && p->s == prev_s)
      {
        position_init(p, p->data, p->pt);
        break;
      }
    }
  }
  else
  {
    position_init(p, p->data, p->pt);
  }
}

void position_init(position_t *p, sequencer_data_t *data, track_t *track)
{
  p->data = data;
  p->pt = track;

  p->m = track->first;
  if (p->m == INVALID_PATTERN)
  {
    p->pm = NULL;
    p->s = INVALID_STEP;
    p->ps = NULL;
    p->sname = 0;
    p->mname = 0;
  }
  else
  {
    p->pm = &PATTERN(data, p->m);
    p->s = p->pm->first;
    p->mname = 1;
    if (p->s == INVALID_STEP)
    {
      p->ps = NULL;
      p->sname = 0;
    }
    else
    {
      p->ps = &STEP(data, p->s);
      p->sname = 1;
    }
  }
  p->ticks = 0;
}

void position_copy(position_t *dst, const position_t *src)
{
  memcpy(dst, src, sizeof(position_t));
}

void step_init_with_1V(position_t *p)
{
  if (p->s == INVALID_STEP)
    return;

  int i, diff;
  int Amini = 0, Amindiff = 100000, Bmini = 0, Bmindiff = 100000;

  for (i = 0; i < NUM_VOLTAGES_PER_TABLE; i++)
  {
    diff = p->pt->voltages[SEQ_TABLE_A][i] - CODE_PER_OCTAVE;
    if (diff < 0)
      diff = -diff;
    if (diff < Amindiff)
    {
      Amindiff = diff;
      Amini = i;
    }

    diff = p->pt->voltages[SEQ_TABLE_B][i] - CODE_PER_OCTAVE;
    if (diff < 0)
      diff = -diff;
    if (diff < Bmindiff)
    {
      Bmindiff = diff;
      Bmini = i;
    }
  }

  SET_STEP_INDEX(p->ps->cvA, Amini);
  SET_STEP_INDEX(p->ps->cvB, Bmini);
}

bool seq_smart_insert_step_after(position_t *p)
{
  step_id_t save = p->s;
  if (seq_insert_step_after(p))
  {
    if (save == INVALID_STEP)
    {
      step_init_with_1V(p);
    }
    else
    {
      seq_copy_step(p->ps, &STEP(p->data, save));
    }
    return true;
  }

  return false;
}

bool seq_smart_insert_step_before(position_t *p)
{
  step_id_t save = p->s;
  if (seq_insert_step_before(p))
  {
    if (save == INVALID_STEP)
    {
      step_init_with_1V(p);
    }
    else
    {
      seq_copy_step(p->ps, &STEP(p->data, save));
    }
    return true;
  }

  return false;
}

bool seq_split_step(position_t *p)
{
  if (p->s == INVALID_STEP || p->ps->duration < 2)
  {
    tilt_trigger();
    return false;
  }

  step_t *ps = p->ps;
  if (p->pt->lstart == p->s)
  {
    // The focused step is the beginning of a loop.
    // Thus place the new step after this step to keep the loop duration the same.
    if (seq_insert_step_after(p))
    {
      seq_copy_step(p->ps, ps);
      // half the duration and gate but keep the sum the same
      uint8_t tmp = ps->duration;
      p->ps->duration = tmp / 2;
      ps->duration = tmp - p->ps->duration;
      if (ps->gate > 1)
      {
        tmp = ps->gate;
        p->ps->gate = tmp / 2;
        ps->gate = tmp - p->ps->gate;
      }
      return true;
    }
  }
  else
  {
    if (seq_insert_step_before(p))
    {
      seq_copy_step(p->ps, ps);
      // half the duration and gate but keep the sum the same
      uint8_t tmp = ps->duration;
      ps->duration = tmp / 2;
      p->ps->duration = tmp - ps->duration;
      if (ps->gate > 1)
      {
        tmp = ps->gate;
        ps->gate = tmp / 2;
        p->ps->gate = tmp - ps->gate;
      }
      return true;
    }
  }

  return false;
}

bool seq_insert_step_after(position_t *p)
{

  if (p->m == INVALID_PATTERN)
  {
    // automatically insert a pattern if there is none
    seq_insert_pattern_after(p);
  }

  // position must point to a valid pattern in order to insert a step
  if (p->m == INVALID_PATTERN)
    return false;

  if (p->pm->ns == 99)
  {
    tilt_trigger();
    return false;
  }

  step_id_t s = seq_allocate_step(p->data);
  if (s == INVALID_STEP)
  {
    tilt_trigger();
    return false;
  }

  step_t *ps = &STEP(p->data, s);

  if (p->ps != NULL)
  {
    ps->next = p->ps->next;
    ps->prev = p->s;
    if (ps->next != INVALID_STEP)
    {
      STEP(p->data, ps->next).prev = s;
    }
    p->ps->next = s;
  }

  // update the pattern containing this step
  p->pm->ns++;

  if (ps->prev == INVALID_STEP)
  {
    p->pm->first = s;
  }

  if (ps->next == INVALID_STEP)
  {
    p->pm->last = s;
  }

  // update the position struct
  p->sname++;
  p->s = s;
  p->ps = ps;

  return true;
}

bool seq_insert_pattern_after(position_t *p)
{

  if (p->pt->nm == 99)
  {
    tilt_trigger();
    return false;
  }

  pattern_id_t m = seq_allocate_pattern(p->data);
  if (m == INVALID_PATTERN)
  {
    tilt_trigger();
    return false;
  }
  pattern_t *pm = &PATTERN(p->data, m);
  pattern_init(pm);
  if (p->pm != NULL)
  {
    pm->next = p->pm->next;
    pm->prev = p->m;
    if (p->pm->next != INVALID_PATTERN)
    {
      PATTERN(p->data, p->pm->next).prev = m;
    }
    p->pm->next = m;
  }

  p->pt->nm++;
  p->mname++;
  p->m = m;
  p->pm = pm;
  p->sname = 0;
  p->s = INVALID_STEP;
  p->ps = NULL;

  if (pm->prev == INVALID_PATTERN)
  {
    p->pt->first = m;
  }

  if (pm->next == INVALID_PATTERN)
  {
    p->pt->last = m;
  }

  return true;
}

bool seq_insert_step_before(position_t *p)
{

  if (p->m == INVALID_PATTERN)
  {
    // automatically insert a pattern if there is none
    seq_insert_pattern_before(p);
  }

  // position must point to a valid pattern in order to insert a step
  if (p->m == INVALID_PATTERN)
    return false;

  if (p->pm->ns == 99)
  {
    tilt_trigger();
    return false;
  }

  step_id_t s = seq_allocate_step(p->data);
  if (s == INVALID_STEP)
  {
    tilt_trigger();
    return false;
  }

  step_t *ps = &STEP(p->data, s);

  if (p->ps != NULL)
  {
    ps->next = p->s;
    ps->prev = p->ps->prev;
    if (ps->prev != INVALID_STEP)
    {
      STEP(p->data, ps->prev).next = s;
    }
    p->ps->prev = s;
  }

  // update the pattern containing this step
  p->pm->ns++;

  if (ps->prev == INVALID_STEP)
  {
    p->pm->first = s;
    p->sname = 1;
  }

  if (ps->next == INVALID_STEP)
  {
    p->pm->last = s;
    p->sname = p->pm->ns;
  }

  // update the position struct
  p->s = s;
  p->ps = ps;

  return true;
}

bool seq_split_pattern(position_t *p)
{
  if (p->m == INVALID_PATTERN || p->pm->ns < 2)
  {
    tilt_trigger();
    return false;
  }
  position_t orig;
  position_copy(&orig, p);
  if (seq_insert_pattern_before(p))
  {
    // move the first half of the original pattern to the new focused pattern
    uint8_t ns = orig.sname;
    seq_pattern_begin(&orig);
    while (ns > 0)
    {
      seq_insert_step_after(p);
      seq_copy_step(p->ps, orig.ps);
      seq_delete_step(&orig);
      ns--;
    }

    return true;
  }
  return false;
}

bool seq_insert_pattern_before(position_t *p)
{
  if (p->pt->nm == 99)
  {
    tilt_trigger();
    return false;
  }
  pattern_id_t m = seq_allocate_pattern(p->data);
  if (m == INVALID_PATTERN)
  {
    tilt_trigger();
    return false;
  }
  pattern_t *pm = &PATTERN(p->data, m);
  pattern_init(pm);
  if (p->pm != NULL)
  {
    pm->prev = p->pm->prev;
    pm->next = p->m;
    if (pm->prev != INVALID_PATTERN)
    {
      PATTERN(p->data, pm->prev).next = m;
    }
    p->pm->prev = m;
  }

  p->pt->nm++;
  p->m = m;
  p->pm = pm;
  p->sname = 0;
  p->s = INVALID_STEP;
  p->ps = NULL;

  if (pm->prev == INVALID_PATTERN)
  {
    p->pt->first = m;
    p->mname = 1;
  }

  if (pm->next == INVALID_PATTERN)
  {
    p->pt->last = m;
    p->mname = p->pt->nm;
  }

  return true;
}

void seq_delete_track(position_t *p)
{
  gpio_set_pin_low(seq.gate_output_pins[p->pt->id]);
  while (p->pt->nm > 0)
  {
    seq_delete_pattern(p);
  }
  track_init(p->pt, p->pt->id);
}

void seq_delete_pattern(position_t *p)
{
  if (p->m == INVALID_PATTERN)
    return;

  // delete any steps in this pattern
  while (p->pm->ns > 0)
  {
    seq_delete_step(p);
  }

  pattern_id_t m;
  pattern_t *pm = NULL;

  if (p->pm->prev != INVALID_PATTERN)
  {
    m = p->pm->prev;
    pm = &PATTERN(p->data, m);
    pm->next = p->pm->next;
    p->s = pm->last;
    p->sname = pm->ns;
    p->mname--;
  }
  else if (p->pm->next != INVALID_PATTERN)
  {
    m = p->pm->next;
    pm = &PATTERN(p->data, m);
    p->s = pm->first;
    p->sname = 1;
    pm->prev = INVALID_PATTERN;
  }
  else
  {
    m = INVALID_PATTERN;
    p->s = INVALID_STEP;
    p->ps = NULL;
    p->sname = 0;
    p->mname--;
  }

  seq_free_pattern(p->data, p->m);

  p->m = m;
  p->pm = pm;
  if (p->s != INVALID_STEP)
    p->ps = &STEP(p->data, p->s);

  // update the track containing this pattern
  p->pt->nm--;

  if (p->m == INVALID_PATTERN || p->pt->nm == 0)
  {
    p->pt->first = INVALID_PATTERN;
    p->pt->last = INVALID_PATTERN;
    p->pt->lend = INVALID_STEP;
    p->pt->lstart = INVALID_STEP;
    p->pt->nm = 0;
    p->mname = 0;
    p->sname = 0;
    p->s = INVALID_STEP;
    p->m = INVALID_PATTERN;
  }
  else
  {
    if (p->pm->prev == INVALID_PATTERN)
    {
      p->pt->first = p->m;
    }
    else
    {
      PATTERN(p->data, p->pm->prev).next = p->m;
    }
    if (p->pm->next == INVALID_PATTERN)
    {
      p->pt->last = p->m;
    }
    else
    {
      PATTERN(p->data, p->pm->next).prev = p->m;
    }
  }
}

void seq_delete_step(position_t *p)
{
  if (p->m == INVALID_PATTERN)
    return;
  if (p->s == INVALID_STEP)
    return;

  step_id_t s;
  step_t *ps = NULL;

  if (p->ps->prev != INVALID_STEP)
  {
    s = p->ps->prev;
    ps = &STEP(p->data, s);
    ps->next = p->ps->next;
    p->sname--;
  }
  else if (p->ps->next != INVALID_STEP)
  {
    s = p->ps->next;
    ps = &STEP(p->data, s);
    ps->prev = INVALID_STEP;
  }
  else
  {
    s = INVALID_STEP;
    p->sname--;
  }

  if (p->pt->lstart == p->s)
  {
    p->pt->lstart = INVALID_STEP;
  }

  if (p->pt->lend == p->s)
  {
    p->pt->lend = INVALID_STEP;
  }

  seq_free_step(p->data, p->s);

  p->s = s;
  p->ps = ps;

  // update the pattern containing this step
  p->pm->ns--;

  if (p->s == INVALID_STEP || p->pm->ns == 0)
  {
    p->pm->first = INVALID_STEP;
    p->pm->last = INVALID_STEP;
    p->pm->ns = 0;
    p->s = INVALID_STEP;
  }
  else
  {
    // update the pattern containing this step
    if (p->ps->prev == INVALID_STEP)
    {
      p->pm->first = p->s;
    }
    else
    {
      STEP(p->data, p->ps->prev).next = p->s;
    }
    if (p->ps->next == INVALID_STEP)
    {
      p->pm->last = p->s;
    }
    else
    {
      STEP(p->data, p->ps->next).prev = p->s;
    }
  }
}

void sequencer_data_init(sequencer_data_t *data)
{
  int i;

  transform_init(&(data->transform));

  data->magic = SEQ_MAGIC;

  for (i = 0; i < MAX_STEPS; i++)
  {
    step_init(&(data->steps.pool[i]));
    data->steps.available[i] = i;
  }
  data->steps.head = 0;

  for (i = 0; i < MAX_PATTERNS; i++)
  {
    pattern_init(&(data->patterns.pool[i]));
    data->patterns.available[i] = i;
  }
  data->patterns.head = 0;

  for (i = 0; i < NUM_TRACKS; i++)
  {
    track_init(&(data->tracks[i]), i);
  }
}

void sequencer_init(void)
{
  int i;

  sequencer_data_init(&(seq.data));
  sequencer_data_init(&(seq.shadow));

  seq.gate_output_pins[0] = PIN_TRACK1_GATE;
  seq.gate_output_pins[1] = PIN_TRACK2_GATE;
  seq.gate_output_pins[2] = PIN_TRACK3_GATE;
  seq.gate_output_pins[3] = PIN_TRACK4_GATE;

  for (i = 0; i < NUM_TRACKS; i++)
  {
    position_init(&(seq.players[i]), &(seq.data), &(seq.data.tracks[i]));
    position_init(&(seq.editors[i]), &(seq.data), &(seq.data.tracks[i]));
    seq.current_tick_adjustment[i] = 0;
    seq.next_tick_adjustment[i] = 0;
    seq.trigger_timers[i].running = false;
    seq.trigger_max_duration[i] = CPU_HZ;
    seq.ppqn_timers[i].running = false;
    seq.ppqn_counters[i] = 255;
  }

  seq.trigger_unit = cpu_us_2_cy(100, CPU_HZ);
  seq.soft_reset_interval = cpu_us_2_cy(500, CPU_HZ);

  seq.focus_voltage_table = SEQ_TABLE_A;
  seq.focus_voltage_index = 0;

  // don't use seq_set_track here to prevent version from being overwritten
  seq.focus_track = 0;
  segdisp_put_right_digit(SEGDISP_TRACK, 1);
  seq.trigger_track = 0;

  seq_set_slot(BLANK_SNAPSHOT);

  seq.pause = false;
  seq.clock_is_late = false;
  seq.mode = MODE_EDIT;
  seq.confirm_mode = CONFIRM_NOTHING;
  seq.reset_quantization = NO_QUANTIZATION;
  seq.commit_quantization = NO_QUANTIZATION;
  seq.insert_mode = INSERT_AFTER;
  seq.voltage_overlay_active = false;
  seq.track_overlay_active = false;
  seq.math_overlay_active = false;
  seq.ref_table_active = false;
  seq.blink_active = false;
  seq.internal_reset_armed = false;
  seq.external_reset_armed = false;
  seq.commit_armed = false;
  seq.selecting = false;
  seq.waiting_for_clock_after_reset = false;
  seq.flash_lend = false;
  seq.flash_lstart = false;

  init_clipboard();

  seq.last_clock_timestamp = INVALID_TIMESTAMP;
  seq.latched_period = 0;
  seq.last_period = 0;

  seq.pasted_table = NULL;
  seq.initialized = true;
}

bool seq_snapshot_exists(uint8_t slot)
{
  uint32_t address = SNAPSHOT_BASE_ADDRESS + ((uint32_t)slot) * SNAPSHOT_SIZE;
  uint32_t magic;
  if (nvm_read(INT_FLASH, address, (void *)&magic, sizeof(uint32_t)) != STATUS_OK)
  {
    segdisp_on_error(ERR_LOAD);
  }
  return magic == SEQ_MAGIC;
}

void seq_save_snapshot(uint8_t slot)
{
  sequencer_data_t *data = &(seq.data);
  uint32_t address = SNAPSHOT_BASE_ADDRESS + ((uint32_t)slot) * SNAPSHOT_SIZE;
  uint16_t index = segdisp_display[SEGDISP_INDEX];
  uint16_t volt0 = segdisp_display[SEGDISP_VOLT0];
  uint16_t volt1 = segdisp_display[SEGDISP_VOLT1];
  segdisp_display[SEGDISP_INDEX] = SEGDISP_LEFT_HYPHEN | SEGDISP_RIGHT_HYPHEN;
  segdisp_display[SEGDISP_VOLT1] = SEGDISP_LEFT_S | SEGDISP_RIGHT_HYPHEN;
  segdisp_display[SEGDISP_VOLT0] = segdisp_both_digits[slot + 1];
  wait_for_display_to_refresh();
  // cpu_delay_ms(500,CPU_HZ);
  // seq_lock = true;
  if (nvm_write(INT_FLASH, address, (void *)data, sizeof(sequencer_data_t)) != STATUS_OK)
  {
    segdisp_on_error(ERR_SAVE);
  }
  else
  {
    segdisp_display[SEGDISP_INDEX] = index;
    segdisp_display[SEGDISP_VOLT1] = volt1;
    segdisp_display[SEGDISP_VOLT0] = volt0;
    set_last_saved_snapshot(slot);
  }
  // seq_lock = false;
  // wait_for_display_to_refresh();
}

void seq_load_snapshot(uint8_t slot)
{
  sequencer_data_t *data = seq_get_focused_data();
  uint32_t address = SNAPSHOT_BASE_ADDRESS + ((uint32_t)slot) * SNAPSHOT_SIZE;
  // bool save_pause = stopped();
  // stopped() = true;
  if (nvm_read(INT_FLASH, address, (void *)data, sizeof(sequencer_data_t)) != STATUS_OK)
  {
    segdisp_on_error(ERR_LOAD);
  }
  seq_set_track(seq.focus_track);
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    if (seq.mode == MODE_EDIT || seq.mode == MODE_LIVE)
    {
      position_init(&(seq.players[i]), data, &TRACK(data, i));
    }
    position_init(&(seq.editors[i]), data, &TRACK(data, i));
  }
  seq_display_position(SEQ_FOCUS);
  if (seq.mode == MODE_EDIT || seq.mode == MODE_LIVE)
  {
    seq_write_to_dac(SEQ_FOCUS);
  }
  set_last_saved_snapshot(slot);
  // stopped() = save_pause;
}

void seq_initiate_hold(void)
{
  sequencer_data_t *data = &(seq.shadow);
  // copy data to shadow buffer
  memcpy(data, &(seq.data), sizeof(sequencer_data_t));
  // point editors to shadow buffer
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    position_t *p = &(seq.editors[i]);
    memcpy(&(seq.restore[i]), p, sizeof(position_t));
    p->data = data;
    p->pt = &TRACK(data, p->pt->id);
    if (p->m != INVALID_PATTERN)
    {
      p->pm = &PATTERN(data, p->m);
      if (p->s != INVALID_STEP)
      {
        p->ps = &STEP(data, p->s);
      }
    }
  }
}

void seq_commit(void)
{
  clear_clipboard();
  sequencer_data_t *data = &(seq.data);
  memcpy(data, &(seq.shadow), sizeof(sequencer_data_t));
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    memcpy(&(seq.restore[i]), &(seq.editors[i]), sizeof(position_t));
    position_init(&(seq.players[i]), data, &TRACK(data, i));
    seq.current_tick_adjustment[i] = 0;
    seq.next_tick_adjustment[i] = 0;
    skip_unplayable_steps(&(seq.players[i]));
  }
  if (!seq.math_overlay_active)
  {
    seq_display_position(SEQ_EDIT_FOCUS);
  }
  seq.commit_armed = false;
  seq.commit_quantization = NO_QUANTIZATION;
  gpio_set_pin_low(PIN_COMMIT_LED);
  select_left_display(focus_left_display);
}

void seq_release_hold(void)
{
  if (seq.commit_armed)
  {
    seq_commit();
  }
  sequencer_data_t *data = &(seq.data);
  // point editors back to data 0
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    position_t *p = &(seq.editors[i]);
    memcpy(p, &(seq.restore[i]), sizeof(position_t));
    p->data = data;
    p->pt = &TRACK(data, p->pt->id);
    if (p->m != INVALID_PATTERN)
    {
      p->pm = &PATTERN(data, p->m);
      if (p->s != INVALID_STEP)
      {
        p->ps = &STEP(data, p->s);
      }
    }
  }
  if (!seq.math_overlay_active)
  {
    seq_display_position(SEQ_FOCUS);
  }
}

void seq_initiate_edit(void)
{
  if (!seq.math_overlay_active)
  {
    seq_display_position(SEQ_EDIT_FOCUS);
  }
}

void seq_initiate_live(void)
{
  // try to initialize the players for each track
  for (int i = 0; i < NUM_TRACKS; i++)
  {
    if (seq.players[i].s == INVALID_STEP)
    {
      position_init(&(seq.players[i]), &(seq.data), &(seq.data.tracks[i]));
      seq.current_tick_adjustment[i] = 0;
      seq.next_tick_adjustment[i] = 0;
    }
  }
  if (!seq.math_overlay_active)
  {
    seq_display_position(SEQ_LIVE_FOCUS);
  }
}

void seq_copy_step(step_t *dst, step_t *src)
{
  dst->cvA = src->cvA;
  dst->cvB = src->cvB;
  dst->duration = src->duration;
  dst->gate = src->gate;
}

bool position_is_before(position_t *p0, position_t *p1)
{
  // determine which position is earlier in the sequence
  int x0 = ((int)p0->mname) * 100 + p0->sname;
  int x1 = ((int)p1->mname) * 100 + p1->sname;
  return x0 < x1;
}

int position_count_steps(position_t *p0, position_t *p1)
{
  position_t iter;
  int n = 0;

  position_copy(&iter, p0);
  if (position_is_before(p0, p1))
  {
    while (iter.s != p1->s)
    {
      if (!seq_next_step(&iter))
        break;
      if (iter.s != INVALID_STEP)
      {
        n++;
      }
    }
  }
  else
  {
    while (iter.s != p1->s)
    {
      if (!seq_prev_step(&iter))
        break;
      if (iter.s != INVALID_STEP)
      {
        n++;
      }
    }
  }

  return n;
}

sequencer_data_t *seq_get_focused_data(void)
{
  if (seq.mode == MODE_COMMIT)
    return &(seq.shadow);
  return &(seq.data);
}

void transform_init(transform_t *t)
{
  t->cvA = 0;
  t->cvB = 0;
  t->duration = 1;
  t->gate = 1;
  t->cvA_mode = TRANSFORM_ARITHMETIC;
  t->cvB_mode = TRANSFORM_ARITHMETIC;
  t->duration_mode = TRANSFORM_GEOMETRIC;
  t->gate_mode = TRANSFORM_GEOMETRIC;
}

void seq_apply_transform_to_track(position_t *p, transform_t *t)
{
  position_t tmp;
  position_init(&tmp, p->data, p->pt);
  while (true)
  {
    seq_apply_transform_to_step(&tmp, t);
    if (!seq_next_step(&tmp))
    {
      return;
    }
  }
}

void seq_apply_transform_to_pattern(position_t *p, transform_t *t)
{
  if (p->m == INVALID_PATTERN)
    return;
  position_t tmp;
  position_init(&tmp, p->data, p->pt);
  tmp.m = p->m;
  tmp.pm = p->pm;
  tmp.s = p->pm->first;
  if (tmp.s == INVALID_STEP)
    return;
  tmp.ps = &STEP(tmp.data, tmp.s);
  while (true)
  {
    seq_apply_transform_to_step(&tmp, t);
    if (!seq_next_step(&tmp) || tmp.m != p->m)
    {
      return;
    }
  }
}

static uint8_t do_transform(uint8_t operand, int operator, uint8_t mode)
{
  int tmp = operand;

  switch (mode)
  {
  case TRANSFORM_ARITHMETIC:
    tmp += operator;
    break;
  case TRANSFORM_GEOMETRIC:
    if (operator<0)
    {
      tmp /= -operator;
    }
    else if (operator> 0)
    {
      tmp *= operator;
    }
    else
    {
      tmp = 0;
    }
    break;
  case TRANSFORM_SET:
    tmp = operator;
    break;
  case TRANSFORM_RANDOM:
    tmp = (rand() % (operator+ 1));
    break;
  case TRANSFORM_JITTER:
    tmp += (rand() % (2 * operator+ 1)) - operator;
    break;
  case TRANSFORM_QUANTIZE:
    if (operator> 1)
    {
      int remainder = tmp % operator;
      if (operator- remainder> remainder)
      {
        tmp -= remainder;
      }
      else
      {
        tmp += operator- remainder;
      }
    }
    break;
  }

  // If a transformation causes a value to over or underflow,
  // then that particular transformation will be canceled.
  if (tmp > -1 && tmp < 100)
  {
    return (uint8_t)tmp;
  }

  return operand;
}

void seq_apply_transform_to_step(position_t *p, transform_t *t)
{
  if (p->s == INVALID_STEP)
    return;

  SET_STEP_INDEX(p->ps->cvA, do_transform(GET_STEP_INDEX(p->ps->cvA), t->cvA, t->cvA_mode));
  SET_STEP_INDEX(p->ps->cvB, do_transform(GET_STEP_INDEX(p->ps->cvB), t->cvB, t->cvB_mode));
  p->ps->duration = do_transform(p->ps->duration, t->duration, t->duration_mode);
  SET_STEP_GATE(p->ps->gate, do_transform(GET_STEP_GATE(p->ps->gate), t->gate, t->gate_mode));
}

void transform_increment_cva(transform_t *t)
{
  if (++t->cvA > 99)
    t->cvA = 99;
}

void transform_increment_cvb(transform_t *t)
{
  if (++t->cvB > 99)
    t->cvB = 99;
}

void transform_increment_duration(transform_t *t)
{
  if (++t->duration > 99)
    t->duration = 99;
}

void transform_increment_gate(transform_t *t)
{
  if (++t->gate > 99)
    t->gate = 99;
}

void transform_decrement_cva(transform_t *t)
{
  if (--t->cvA < -99)
    t->cvA = -99;
  if ((t->cvA_mode == TRANSFORM_RANDOM || t->cvA_mode == TRANSFORM_JITTER || t->cvA_mode == TRANSFORM_QUANTIZE) && t->cvA < 0)
  {
    t->cvA = 0;
  }
}

void transform_decrement_cvb(transform_t *t)
{
  if (--t->cvB < -99)
    t->cvB = -99;
  if ((t->cvB_mode == TRANSFORM_RANDOM || t->cvB_mode == TRANSFORM_JITTER || t->cvA_mode == TRANSFORM_QUANTIZE) && t->cvB < 0)
  {
    t->cvB = 0;
  }
}

void transform_decrement_duration(transform_t *t)
{
  if (--t->duration < -99)
    t->duration = -99;
  if ((t->duration_mode == TRANSFORM_RANDOM || t->duration_mode == TRANSFORM_JITTER || t->cvA_mode == TRANSFORM_QUANTIZE) && t->duration < 0)
  {
    t->duration = 0;
  }
}

void transform_decrement_gate(transform_t *t)
{
  if (--t->gate < -99)
    t->gate = -99;
  if ((t->gate_mode == TRANSFORM_RANDOM || t->gate_mode == TRANSFORM_JITTER || t->cvA_mode == TRANSFORM_QUANTIZE) && t->gate < 0)
  {
    t->gate = 0;
  }
}

static bool blink_double = false;
static uint8_t blink_state = 0;
static uint16_t blink_left, blink_right;
static uint16_t blink_left2, blink_right2;

void tilt_trigger(void)
{
  blink_single_message(SEGDISP_LEFT_t | SEGDISP_RIGHT_i, SEGDISP_LEFT_l | SEGDISP_RIGHT_t);
}

void blink_single_message(uint16_t left, uint16_t right)
{
  blink_double = false;
  blink_left = left;
  blink_right = right;
  segdisp_display[SEGDISP_VOLT1] = blink_left;
  segdisp_display[SEGDISP_VOLT0] = blink_right;
  segdisp_display[SEGDISP_INDEX] = SEGDISP_BLANK;
  seq.blink_active = true;
  blink_state = 3;
}

void blink_double_message(uint16_t left, uint16_t right, uint16_t left2, uint16_t right2)
{
  blink_double = true;
  blink_left = left;
  blink_right = right;
  blink_left2 = left2;
  blink_right2 = right2;
  segdisp_display[SEGDISP_VOLT1] = blink_left;
  segdisp_display[SEGDISP_VOLT0] = blink_right;
  segdisp_display[SEGDISP_INDEX] = SEGDISP_BLANK;
  seq.blink_active = true;
  blink_state = 5;
}

void blink_message_update(void)
{
  if (blink_state == 0)
    return;
  if (blink_double)
  {
    if (blink_state == 3)
    {
      segdisp_display[SEGDISP_VOLT1] = blink_left2;
      segdisp_display[SEGDISP_VOLT0] = blink_right2;
      segdisp_display[SEGDISP_INDEX] = SEGDISP_BLANK;
    }
    else if (blink_state == 1)
    {
      seq_display_voltage();
      seq.blink_active = false;
    }
  }
  else
  {
    if (blink_state == 1)
    {
      seq_display_voltage();
      seq.blink_active = false;
    }
  }
  blink_state--;
}

void position_set_start(position_t *dst, const position_t *src, uint8_t focus)
{
  switch (focus)
  {
  case SEGDISP_INDEX:
    tilt_trigger();
    break;
  case SEGDISP_TRACK:
    position_init(dst, src->data, src->pt);
    break;
  case SEGDISP_PATTERN:
    position_copy(dst, src);
    break;
  case SEGDISP_STEP:
    position_copy(dst, src);
    break;
  case SEGDISP_SNAPSHOT:
    tilt_trigger();
    break;
  }
}

void position_set_end(position_t *dst, const position_t *src, uint8_t focus)
{
  switch (focus)
  {
  case SEGDISP_INDEX:
    tilt_trigger();
    break;
  case SEGDISP_TRACK:
    position_init(dst, src->data, src->pt);
    break;
  case SEGDISP_PATTERN:
    position_copy(dst, src);
    break;
  case SEGDISP_STEP:
    position_copy(dst, src);
    break;
  case SEGDISP_SNAPSHOT:
    tilt_trigger();
    break;
  }
}

void seq_pattern_begin(position_t *p)
{
  if (p->m != INVALID_PATTERN)
  {
    p->s = p->pm->first;
    if (p->s != INVALID_STEP)
    {
      p->ps = &STEP(p->data, p->s);
      p->sname = 1;
    }
    else
    {
      p->ps = NULL;
      p->sname = 0;
    }
  }
}

void seq_pattern_end(position_t *p)
{
  if (p->m != INVALID_PATTERN)
  {
    p->s = p->pm->last;
    if (p->s != INVALID_STEP)
    {
      p->ps = &STEP(p->data, p->s);
      p->sname = p->pm->ns;
    }
    else
    {
      p->ps = NULL;
      p->sname = 0;
    }
  }
}

void seq_track_begin(position_t *p)
{
  p->m = p->pt->first;
  if (p->m != INVALID_PATTERN)
  {
    p->pm = &PATTERN(p->data, p->m);
    p->mname = 1;
    seq_pattern_begin(p);
  }
  else
  {
    p->pm = NULL;
    p->mname = 0;
    p->s = INVALID_STEP;
    p->ps = NULL;
  }
}

void seq_rename(position_t *p)
{
  position_t iter;
  position_init(&iter, p->data, p->pt);
  while (!(iter.m == p->m && iter.s == p->s))
  {
    if (!seq_next_step(&iter))
    {
      // could not find the position, something must be wrong
      position_init(p, p->data, p->pt);
      return;
    }
  }
  p->sname = iter.sname;
  p->mname = iter.mname;
}

void trigger_maintenance(void)
{
  if (walltimer_elapsed(&(seq.trigger_timers[0])))
  {
    gpio_set_pin_low(seq.gate_output_pins[0]);
  }
  if (walltimer_elapsed(&(seq.trigger_timers[1])))
  {
    gpio_set_pin_low(seq.gate_output_pins[1]);
  }
  if (walltimer_elapsed(&(seq.trigger_timers[2])))
  {
    gpio_set_pin_low(seq.gate_output_pins[2]);
  }
  if (walltimer_elapsed(&(seq.trigger_timers[3])))
  {
    gpio_set_pin_low(seq.gate_output_pins[3]);
  }
}

void seq_jump_to_next_ppqn(position_t *p)
{
  for (int i = 0; i < PPQN_JUMP_TABLE_SIZE; i++)
  {
    if (ppqn_jump_table[i] > p->pt->ppqn)
    {
      p->pt->ppqn = ppqn_jump_table[i];
      return;
    }
  }
  // Did not find a larger ppqn so wrap back to 1.
  p->pt->ppqn = 1;
}

void seq_notify_ppqn_changed(position_t *p)
{
  // cancel clock multiplication until the next real clock pulse
  seq.ppqn_counters[p->pt->id] = 255;
  seq.ppqn_timers[p->pt->id].running = false;
  // reset to the beginning of the current step
  p->ticks = 0;
}

void seq_copy_to_reftable(uint8_t table, uint16_t *data)
{
  // memcpy((void *)&(ref_tables[table][0]),(void*)data,sizeof(uint16_t)*(MAX_VOLTAGES_PER_TABLE));
  uint32_t address = REFTABLE_BASE_ADDRESS + ((uint32_t)table) * REFTABLE_SIZE;
  if (nvm_write(INT_FLASH, address, (void *)data, REFTABLE_SIZE) != STATUS_OK)
  {
    segdisp_on_error(ERR_SAVE);
  }
  else
  {
  }
}

void seq_select_voltage_grid()
{
  if (seq.ref_table_active)
  {
    orange_clear(focus_right_display);
    focus_right_display = SEGDISP_VOLT01;
    orange_set(focus_right_display);
  }
  else
  {
    track_t *pt = SEQ_FOCUS->pt;
    orange_clear(focus_right_display);
    if (BIT_TEST(pt->options, OPTION_COARSE))
    {
      focus_right_display = SEGDISP_VOLT1;
    }
    else if (BIT_TEST(pt->options, OPTION_SUPER_COARSE))
    {
      focus_right_display = SEGDISP_VOLT01;
    }
    else
    {
      // if none are set, then fine
      focus_right_display = SEGDISP_VOLT0;
    }
    orange_set(focus_right_display);
  }
}
