#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#include <wallclock.h>
#include <static_assert.h>
#include <display.h>
#include <sequencer.h>
#include "string.h"

#define BRIDGE_BLOCK_SIZE 48
#define BRIDGE_NULL_DESTINATION 0xFF
#define BRIDGE_STATE_DESTINATION 0b11000001
#define BRIDGE_CONTROL_DESTINATION 0b11000001

// leds
#define BRIDGE_SMOOTH_LED (1 << 0)
#define BRIDGE_COPY_LED (1 << 1)
#define BRIDGE_PAUSE_LED (1 << 2)
#define BRIDGE_COMMIT_LED (1 << 3)
#define BRIDGE_LEND_LED (1 << 4)
#define BRIDGE_LSTART_LED (1 << 5)

#define NUM_CV (NUM_TRACKS * 2)

typedef struct
{
  uint16_t system;
  uint16_t leds;
  uint16_t cv[NUM_CV];
  uint16_t segdisp[SEGDISP_NUM];
  uint16_t orange;
  uint8_t gate[NUM_TRACKS];
} bridge_control_t;

STATIC_ASSERT(sizeof(bridge_control_t) == BRIDGE_BLOCK_SIZE, bridge_control_t_is_wrong_size);

typedef struct
{
  uint8_t id;
  uint8_t event;
} bridge_event_t;

STATIC_ASSERT(sizeof(bridge_event_t) == 2, bridge_event_t_is_wrong_size);

#define BRIDGE_EVENT_QUEUE_SIZE 22
typedef struct
{
  uint16_t system;
  uint16_t pins;
  bridge_event_t events[BRIDGE_EVENT_QUEUE_SIZE];
} bridge_state_t;

STATIC_ASSERT(sizeof(bridge_state_t) == BRIDGE_BLOCK_SIZE, er101_state_t_is_wrong_size);

// Bit mask for system flag in bridge_state

// buttons and switches
#define BRIDGE_DURATION_BUTTON (1 << 0)
#define BRIDGE_MODE_A (1 << 1)
#define BRIDGE_MODE_B (1 << 2)
#define BRIDGE_TABLE_A (1 << 3)
#define BRIDGE_TABLE_B (1 << 4)
#define BRIDGE_TRACK_BUTTON (1 << 5)
#define BRIDGE_PATTERN_BUTTON (1 << 6)
#define BRIDGE_STEP_BUTTON (1 << 7)
#define BRIDGE_CVA_BUTTON (1 << 8)
#define BRIDGE_CVB_BUTTON (1 << 9)
#define BRIDGE_GATE_BUTTON (1 << 10)

// bridge communication
#define BRIDGE_COMM_TIME_OUT (1 << 0)
#define BRIDGE_COMM_BAD_PACKET (1 << 1)
#define BRIDGE_COMM_GOOD_PACKET (1 << 2)
#define BRIDGE_COMM_UNKNOWN_DEST (1 << 3)
#define BRIDGE_COMM_RESET (1 << 4)

// IDs for UI elements
#define BRIDGE_NULL_ID 0
#define ID_BASE 1

#define ID_LEFT_ENCODER (ID_BASE)
#define ID_RIGHT_ENCODER (ID_BASE + 1)
#define ID_LSTART_BUTTON (ID_BASE + 2)
#define ID_LEND_BUTTON (ID_BASE + 3)
#define ID_SNAPSHOT_BUTTON (ID_BASE + 4)
#define ID_RESET_INPUT (ID_BASE + 5)
#define ID_COMMIT_BUTTON (ID_BASE + 6)
#define ID_MODE_SWITCH (ID_BASE + 7)
#define ID_CLOCK_INPUT (ID_BASE + 8)
#define ID_INDEX_BUTTON (ID_BASE + 9)
#define ID_VOLTAGE_BUTTON (ID_BASE + 10)
#define ID_DURATION_BUTTON (ID_BASE + 11)
#define ID_GATE_BUTTON (ID_BASE + 12)
#define ID_STEP_BUTTON (ID_BASE + 13)
#define ID_SAVE_BUTTON (ID_BASE + 14)
#define ID_COPY_BUTTON (ID_BASE + 15)
#define ID_CVB_BUTTON (ID_BASE + 16)
#define ID_LOAD_BUTTON (ID_BASE + 17)
#define ID_PAUSE_BUTTON (ID_BASE + 18)
#define ID_RESET_BUTTON (ID_BASE + 19)
#define ID_MATH_BUTTON (ID_BASE + 20)
#define ID_PATTERN_BUTTON (ID_BASE + 21)
#define ID_DELETE_BUTTON (ID_BASE + 22)
#define ID_INSERT_BUTTON (ID_BASE + 23)
#define ID_TABLE_SWITCH (ID_BASE + 24)
#define ID_TRACK_BUTTON (ID_BASE + 25)
#define ID_SMOOTH_BUTTON (ID_BASE + 26)
#define ID_CVA_BUTTON (ID_BASE + 27)

// events for UI elements
#define EVENT_BASE 0

#define EVENT_INCREMENT (EVENT_BASE)
#define EVENT_DECREMENT (EVENT_BASE + 1)
#define EVENT_PUSH (EVENT_BASE + 2)
#define EVENT_RELEASE (EVENT_BASE + 3)
#define EVENT_RISING_EDGE (EVENT_BASE + 4)
#define EVENT_FALLING_EDGE (EVENT_BASE + 5)
#define EVENT_POS_UP (EVENT_BASE + 6)
#define EVENT_POS_MIDDLE (EVENT_BASE + 7)
#define EVENT_POS_DOWN (EVENT_BASE + 8)

#define BRIDGE_GATE_OFF 0
#define BRIDGE_GATE_ON 0xFF
#define BRIDGE_GATE_NO_CHANGE 0xFE

void bridge_init(void);
bool bridge_feed(void);
void bridge_event(uint8_t id, uint8_t e);
void bridge_clear_state(void);
void bridge_commit_control_changes(void);
void bridge_copy_system_state(void);
void bridge_message(uint16_t modifier, uint16_t left, uint16_t right);

extern bridge_control_t bridge_control;
extern bool bridge_control_has_changed;
extern bridge_state_t bridge_state;

#endif // __BRIDGE_H__
