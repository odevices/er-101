/*
 * ui_events.c
 *
 * Created: 10/30/2013 12:57:20 PM
 *  Author: clarkson
 */

#include <asf.h>
#include <ui_events.h>
#include <wallclock.h>
#include <sequencer.h>

wallclock_t event_timestamps[NUM_EVENT_TIMESTAMPS] = {0};
fifo_desc_t ui_q_desc;

void ui_q_init(void)
{
  fifo_init(&ui_q_desc, reserved_q_buffer, RESERVED_Q_SIZE);
}

void ui_q_push_no_repeat(ui_event_t evt)
{
  if (fifo_is_empty(&ui_q_desc) || fifo_peek_uint32(&ui_q_desc) != (uint32_t)evt)
  {
    fifo_push_uint32(&ui_q_desc, (uint32_t)evt);
  }
}

void ui_q_push(ui_event_t evt)
{
  if (fifo_push_uint32(&ui_q_desc, (uint32_t)evt) == FIFO_ERROR_OVERFLOW)
  {
    tilt_trigger();
  }
}

void ui_q_debounce(ui_event_t evt, int timestamp_id)
{
  wallclock_t now = wallclock();
  if (now - event_timestamps[timestamp_id] > DEBOUNCE_THRESHOLD)
  {
    ui_q_push_no_repeat(evt);
    event_timestamps[timestamp_id] = now;
  }
}

ui_event_t ui_q_pop(void)
{
  uint32_t evt;
  if (fifo_pull_uint32(&ui_q_desc, &evt) == FIFO_OK)
  {
    return (ui_event_t)evt;
  }
  else
  {
    return UI_NONE;
  }
}

bool ui_q_empty(void)
{
  return fifo_is_empty(&ui_q_desc);
}