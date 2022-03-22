/*
 * ui_events.h
 *
 * Created: 4/30/2013 9:17:21 PM
 *  Author: clarkson
 */

#ifndef UI_EVENTS_H_
#define UI_EVENTS_H_

#define DEBOUNCE_THRESHOLD (WALLCLOCK_TICKS_PER_SEC / 10)
#define EVENT_TIMESTAMP_INSERT 0
#define EVENT_TIMESTAMP_DELETE 1
#define EVENT_TIMESTAMP_MATH 2
#define EVENT_TIMESTAMP_COPY 3
#define EVENT_TIMESTAMP_LOAD 4
#define EVENT_TIMESTAMP_SAVE 5
#define EVENT_TIMESTAMP_PAUSE 6
#define EVENT_TIMESTAMP_RESET 7
#define EVENT_TIMESTAMP_LSTART 8
#define EVENT_TIMESTAMP_LEND 9
#define EVENT_TIMESTAMP_COMMIT 10
#define EVENT_TIMESTAMP_SMOOTH 11
#define EVENT_TIMESTAMP_MATH_RELEASE 12
#define EVENT_TIMESTAMP_INSERT_RELEASE 13
#define EVENT_TIMESTAMP_VOLTAGE 14
#define EVENT_TIMESTAMP_VOLTAGE_RELEASE 15
#define EVENT_TIMESTAMP_TRACK 16
#define EVENT_TIMESTAMP_TRACK_RELEASE 17
#define NUM_EVENT_TIMESTAMPS 18

// typedef uint16_t ui_event_t;
typedef void (*ui_event_t)(void);
#define UI_NONE 0

void ui_q_init(void);
void ui_q_push_no_repeat(ui_event_t evt);
void ui_q_push(ui_event_t evt);
void ui_q_debounce(ui_event_t evt, int timestamp_id);
ui_event_t ui_q_pop(void);
bool ui_q_empty(void);

#endif /* UI_EVENTS_H_ */