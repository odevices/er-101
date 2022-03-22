/*
 * clipboard.c
 *
 * Created: 26/04/2016 17:15:36
 *  Author: clarkson
 */

#include <asf.h>
#include <meta.h>
#include <wallclock.h>
#include <extdac.h>
#include <display.h>
#include <sequencer.h>
#include <clipboard.h>

static void walk_and_copy(position_t *from, position_t *to, step_id_t last, bool include_patterns, bool include_loop_points, bool before)
{
  pattern_id_t prev_m = INVALID_PATTERN;

  while (true)
  {
    if (include_patterns && from->m != prev_m)
    {
      if (before)
      {
        if (!seq_insert_pattern_before(to))
          return;
      }
      else
      {
        if (!seq_insert_pattern_after(to))
          return;
      }
      if (from->m != INVALID_PATTERN && to->m != INVALID_PATTERN)
      {
        to->pm->options = from->pm->options;
      }
    }
    prev_m = from->m;
    if (from->s != INVALID_STEP)
    {
      if (before)
      {
        seq_insert_step_before(to);
      }
      else
      {
        seq_insert_step_after(to);
      }
      if (to->s != INVALID_STEP)
      {
        seq_copy_step(to->ps, from->ps);
        if (include_loop_points)
        {
          // copy loop points too
          if (from->s == from->pt->lstart)
          {
            to->pt->lstart = to->s;
          }
          if (from->s == from->pt->lend)
          {
            to->pt->lend = to->s;
          }
        }
      }
      if (from->s == last)
        return;
    }
    if (!seq_next_step(from))
      return;
    // all steps and patterns after the first are inserted after
    before = false;
    // all patterns after the first are inserted
    include_patterns = true;
  }
}

void copy_to_clipboard(position_t *p0, position_t *p1, uint8_t mode)
{
  position_t src;

  init_clipboard();

  if (position_is_before(p1, p0))
  {
    position_t *tmp = p1;
    p1 = p0;
    p0 = tmp;
  }

  switch (mode)
  {
  case CLIPBOARD_MODE_TRACK:
    position_init(&src, p0->data, p0->pt);
    walk_and_copy(&src, &seq.clipboard, INVALID_STEP, true, true, false);
    seq.clipboard_track.options = p1->pt->options;
    seq.clipboard_track.ppqn = p1->pt->ppqn;
    memcpy(&(seq.clipboard_track.voltages[0][0]), &(p1->pt->voltages[0][0]), NUM_VOLTAGES_PER_TABLE * sizeof(uint16_t));
    memcpy(&(seq.clipboard_track.voltages[1][0]), &(p1->pt->voltages[1][0]), NUM_VOLTAGES_PER_TABLE * sizeof(uint16_t));
    break;
  case CLIPBOARD_MODE_PATTERN:
    position_copy(&src, p0);
    walk_and_copy(&src, &seq.clipboard, p1->s, true, false, false);
    break;
  case CLIPBOARD_MODE_STEP:
    position_copy(&src, p0);
    walk_and_copy(&src, &seq.clipboard, p1->s, false, false, false);
    break;
  case CLIPBOARD_MODE_TABLE:
    if (seq.ref_table_active)
    {
      memcpy(&(seq.clipboard_track.voltages[0][0]), &(ref_tables[seq.focus_ref_table][0]), REFTABLE_SIZE);
      BIT_ASSIGN(ref_table_is_pitched[seq.focus_ref_table], seq.clipboard_track.options, pitched_option[0]);
    }
    else
    {
      memcpy(&(seq.clipboard_track.voltages[0][0]), &(p1->pt->voltages[seq.focus_voltage_table][0]), REFTABLE_SIZE);
      BIT_ASSIGN(BIT_TEST(p1->pt->options, pitched_option[seq.focus_voltage_table]), seq.clipboard_track.options, pitched_option[0]);
    }
    break;
  }

  seq.clipboard_mode = mode;
}

void paste_from_clipboard_after(position_t *p)
{
  step_id_t last_step;
  position_init(&seq.clipboard, seq.clipboard.data, &(seq.clipboard_track));
  if (seq.clipboard.m == INVALID_PATTERN)
  {
    // empty clipboard
    return;
  }
  last_step = PATTERN(seq.clipboard.data, seq.clipboard.pt->last).last;
  switch (seq.clipboard_mode)
  {
  case CLIPBOARD_MODE_TRACK:
    p->pt->options = seq.clipboard_track.options;
    p->pt->ppqn = seq.clipboard_track.ppqn;
    walk_and_copy(&seq.clipboard, p, last_step, true, true, false);
    memcpy(&(p->pt->voltages[0][0]), &(seq.clipboard_track.voltages[0][0]), NUM_VOLTAGES_PER_TABLE * sizeof(uint16_t));
    memcpy(&(p->pt->voltages[1][0]), &(seq.clipboard_track.voltages[1][0]), NUM_VOLTAGES_PER_TABLE * sizeof(uint16_t));
    break;
  case CLIPBOARD_MODE_PATTERN:
    walk_and_copy(&seq.clipboard, p, last_step, true, false, false);
    break;
  case CLIPBOARD_MODE_STEP:
    walk_and_copy(&seq.clipboard, p, last_step, false, false, false);
    break;
  }
}

void paste_from_clipboard_before(position_t *p)
{
  step_id_t last_step;
  position_init(&seq.clipboard, seq.clipboard.data, &(seq.clipboard_track));
  if (seq.clipboard.m == INVALID_PATTERN)
  {
    // empty clipboard
    return;
  }
  last_step = PATTERN(seq.clipboard.data, seq.clipboard.pt->last).last;
  switch (seq.clipboard_mode)
  {
  case CLIPBOARD_MODE_TRACK:
    p->pt->options = seq.clipboard_track.options;
    p->pt->ppqn = seq.clipboard_track.ppqn;
    walk_and_copy(&seq.clipboard, p, last_step, true, true, true);
    memcpy(&(p->pt->voltages[0][0]), &(seq.clipboard_track.voltages[0][0]), NUM_VOLTAGES_PER_TABLE * sizeof(uint16_t));
    memcpy(&(p->pt->voltages[1][0]), &(seq.clipboard_track.voltages[1][0]), NUM_VOLTAGES_PER_TABLE * sizeof(uint16_t));
    break;
  case CLIPBOARD_MODE_PATTERN:
    walk_and_copy(&seq.clipboard, p, last_step, true, false, true);
    break;
  case CLIPBOARD_MODE_STEP:
    walk_and_copy(&seq.clipboard, p, last_step, false, false, true);
    break;
  }
}

void init_clipboard(void)
{
  clear_clipboard();

  // set clipboard data to the 'other' data section.
  if (seq.mode == MODE_COMMIT)
  {
    seq.clipboard.data = &seq.data;
  }
  else
  {
    seq.clipboard.data = &seq.shadow;
  }

  track_init(&seq.clipboard_track, 0);
  position_init(&seq.clipboard, seq.clipboard.data, &(seq.clipboard_track));
  seq.clipboard_mode = CLIPBOARD_MODE_NONE;
  seq.pasted_table = NULL;
}

void clear_clipboard(void)
{
  // first delete any clipboard contents
  if (seq.clipboard_mode != CLIPBOARD_MODE_NONE)
  {
    position_init(&seq.clipboard, seq.clipboard.data, &(seq.clipboard_track));

    while (seq.clipboard.m != INVALID_PATTERN)
    {
      seq_delete_pattern(&seq.clipboard);
    }

    seq.clipboard_mode = CLIPBOARD_MODE_NONE;
    gpio_set_pin_high(PIN_COPY_LED);
  }
}

void paste_table(void)
{
  if (seq.ref_table_active)
  {
    if (ref_table_can_edit[seq.focus_ref_table])
    {
      seq_copy_to_reftable(seq.focus_ref_table, &(seq.clipboard_track.voltages[0][0]));
      seq_display_position(SEQ_FOCUS);
      blink_single_message(SEGDISP_LEFT_C | SEGDISP_RIGHT_o, SEGDISP_LEFT_P | SEGDISP_RIGHT_Y);
    }
    else
    {
      tilt_trigger();
    }
  }
  else
  {
    memcpy(&(SEQ_EDIT_FOCUS->pt->voltages[seq.focus_voltage_table][0]), &(seq.clipboard_track.voltages[0][0]), sizeof(uint16_t) * NUM_VOLTAGES_PER_TABLE);
    BIT_ASSIGN(BIT_TEST(seq.clipboard_track.options, pitched_option[0]), SEQ_EDIT_FOCUS->pt->options, pitched_option[seq.focus_voltage_table]);
    seq_display_position(SEQ_FOCUS);
    blink_single_message(SEGDISP_LEFT_C | SEGDISP_RIGHT_o, SEGDISP_LEFT_P | SEGDISP_RIGHT_Y);
  }
}

bool can_copy_from_clipboard(void)
{
  switch (seq.clipboard_mode)
  {
  case CLIPBOARD_MODE_NONE:
    return false;
  case CLIPBOARD_MODE_TABLE:
    return true;
  case CLIPBOARD_MODE_TRACK:
    return focus_left_display == SEGDISP_TRACK;
  case CLIPBOARD_MODE_PATTERN:
    return focus_left_display == SEGDISP_PATTERN;
  case CLIPBOARD_MODE_STEP:
    return focus_left_display == SEGDISP_STEP;
  }

  return false;
}