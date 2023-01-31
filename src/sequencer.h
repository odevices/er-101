/*
 * sequencer.h
 *
 * Created: 5/2/2013 10:40:18 PM
 *  Author: clarkson
 */

#ifndef SEQUENCER_H_
#define SEQUENCER_H_

#include <voltages.h>
#include "static_assert.h"

#define bit_get(p, m) ((p) & (m))
#define bit_set(p, m) ((p) |= (m))
#define bit_clear(p, m) ((p) &= ~(m))
#define bit_flip(p, m) ((p) ^= (m))
#define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))

#define SEQ_MAGIC 0xABC01CBA

#define BIT_ASSIGN(value, dst, mask) ((value) ? (dst |= mask) : (dst &= ~(mask)))
#define BIT_TEST(opt, mask) (((opt) & (mask)) != 0)

#define MAX_STEPS 2000
#define MAX_PATTERNS 400
#define NUM_TRACKS 4

// to calculate ref_table section address: hex(0x2000+176*512-32-16*2*100)
#define NUM_REF_TABLES 16
#define REFTABLE_SIZE (sizeof(uint16_t) * NUM_VOLTAGES_PER_TABLE)
#define REFTABLE_BASE_ADDRESS (PROGRAM_START_ADDRESS + MAX_PROGRAM_SIZE - 32 - NUM_REF_TABLES * REFTABLE_SIZE)

// 16 snapshots + blank snapshot
#define NUM_SNAPSHOTS 17
#define SNAPSHOT_BASE_ADDRESS (PROGRAM_START_ADDRESS + MAX_PROGRAM_SIZE)
#define SNAPSHOT_SIZE (52 * AVR32_FLASH_PAGE_SIZE)
#define BLANK_SNAPSHOT 0

#define INVALID_PATTERN 0xFFFF
#define INVALID_STEP 0xFFFF
#define INVALID_INDEX 0xFF

#define INVALID_TIMESTAMP 0

#define MODE_EDIT 0
#define MODE_LIVE 1
#define MODE_COMMIT 2

#define CONFIRM_NOTHING 0
#define CONFIRM_DELETE 1
#define CONFIRM_SAVE 2
#define CONFIRM_LOAD 3

#define NO_QUANTIZATION 0
#define QUANTIZE_TO_STEP 1
#define QUANTIZE_TO_PATTERN 2
#define QUANTIZE_TO_TRACK 3

#define SEQ_TABLE_A 0
#define SEQ_TABLE_B 1
#define SEQ_TABLE_REF 2

typedef struct
{
  uint8_t cvA_mode;
  uint8_t cvB_mode;
  uint8_t duration_mode;
  uint8_t gate_mode;
  int cvA;
  int cvB;
  int duration;
  int gate;
} transform_t;

STATIC_ASSERT(sizeof(transform_t) == 20, transform_t_is_wrong_size);

#define TRANSFORM_ARITHMETIC 0
#define TRANSFORM_GEOMETRIC 1
#define TRANSFORM_SET 2
#define TRANSFORM_RANDOM 3
#define TRANSFORM_JITTER 4
#define TRANSFORM_QUANTIZE 5
#define NUM_TRANSFORM 6

typedef uint16_t step_id_t;
typedef uint16_t pattern_id_t;

typedef struct
{
  step_id_t prev;
  step_id_t next;
  uint8_t cvA;
  uint8_t cvB;
  uint8_t duration;
  uint8_t gate;
} step_t;

#define SMOOTH_MASK 0b10000000
#define CV_MASK 0b01111111
#define GET_STEP_SMOOTH(cv) BIT_TEST(cv, SMOOTH_MASK)
#define SET_STEP_SMOOTH(cv, value) BIT_ASSIGN(value, cv, SMOOTH_MASK)
#define GET_STEP_INDEX(cv) (CV_MASK & (cv))
#define SET_STEP_INDEX(cv, value) ((cv) = (SMOOTH_MASK & (cv)) | (value))

#define RATCHET_MASK 0b10000000
#define GATE_MASK 0b01111111
#define GET_STEP_RATCHET(x) BIT_TEST(x, RATCHET_MASK)
#define SET_STEP_RATCHET(x, value) BIT_ASSIGN(value, x, RATCHET_MASK)
#define GET_STEP_GATE(x) (GATE_MASK & (x))
#define SET_STEP_GATE(x, value) ((x) = (RATCHET_MASK & (x)) | (value))

STATIC_ASSERT(sizeof(step_t) == 8, step_t_is_wrong_size);

typedef struct
{
  step_id_t first;
  step_id_t last;
  pattern_id_t prev;
  pattern_id_t next;
  uint8_t ns; // num of steps
  uint8_t options;
} pattern_t;

STATIC_ASSERT(sizeof(pattern_t) == 10, pattern_t_is_wrong_size);

typedef struct
{
  transform_t transform;
  uint8_t id;
  uint8_t nm; // num of patterns
  pattern_id_t first;
  pattern_id_t last;
  // loop start
  step_id_t lstart;
  // loop end
  step_id_t lend;
  uint16_t voltages[2][NUM_VOLTAGES_PER_TABLE];
  uint8_t options;
  uint8_t ppqn;
  uint8_t divide;
  uint8_t reserved1;
  uint8_t reserved2;
  uint8_t reserved3;
} track_t;

STATIC_ASSERT(sizeof(track_t) == 436, track_t_is_wrong_size);

#define OPTION_SMOOTH_A (1 << 0)
#define OPTION_SMOOTH_B (1 << 1)
#define OPTION_TABLE_A_PITCHED (1 << 2)
#define OPTION_TABLE_B_PITCHED (1 << 3)
#define OPTION_COARSE (1 << 4)
#define OPTION_SUPER_COARSE (1 << 5)
#define OPTION_TRIGGER (1 << 6)

#define PPQN_DIVIDE(period, ppqn) (period / ppqn)

typedef struct
{
  step_t pool[MAX_STEPS];
  step_id_t available[MAX_STEPS];
  step_id_t head;
} step_pool_t;

STATIC_ASSERT(sizeof(step_pool_t) == MAX_STEPS * sizeof(step_t) + MAX_STEPS * sizeof(step_id_t) + sizeof(uint16_t), step_pool_t_is_wrong_size);

typedef struct
{
  pattern_t pool[MAX_PATTERNS];
  pattern_id_t available[MAX_PATTERNS];
  pattern_id_t head;
  // uint8_t reserved;
} pattern_pool_t;

STATIC_ASSERT(sizeof(pattern_pool_t) == MAX_PATTERNS * sizeof(pattern_t) + MAX_PATTERNS * sizeof(pattern_id_t) + 2, pattern_pool_t_is_wrong_size);

typedef struct
{
  uint32_t magic;
  transform_t transform; // not used
  track_t tracks[NUM_TRACKS];
  pattern_pool_t patterns;
  step_pool_t steps;
  // uint16_t reserved;
} sequencer_data_t;

STATIC_ASSERT(sizeof(sequencer_data_t) == 4 + sizeof(transform_t) + NUM_TRACKS * sizeof(track_t) + sizeof(pattern_pool_t) + sizeof(step_pool_t), sequencer_data_t_is_wrong_size);
STATIC_ASSERT(sizeof(sequencer_data_t) < SNAPSHOT_SIZE, sequencer_data_bigger_than_snapshot);
STATIC_ASSERT(SNAPSHOT_SIZE * NUM_SNAPSHOTS < AVR32_FLASH_SIZE, snapshots_bigger_than_flash);
STATIC_ASSERT(sizeof(sequencer_data_t) == 26572, sequencer_data_is_wrong_size);

#define STEP(data, id) (data->steps.pool[id])
#define PATTERN(data, id) (data->patterns.pool[id])
#define TRACK(data, id) (data->tracks[id])

typedef struct
{
  sequencer_data_t *data;
  track_t *pt;
  step_id_t s;
  step_t *ps;
  pattern_id_t m;
  pattern_t *pm;
  uint8_t sname;
  uint8_t mname;
  int16_t ticks;
} position_t;

#define CLIPBOARD_MODE_NONE 0
#define CLIPBOARD_MODE_STEP 1
#define CLIPBOARD_MODE_PATTERN 2
#define CLIPBOARD_MODE_TRACK 3
#define CLIPBOARD_MODE_TABLE 4

#define INSERT_AFTER 0
#define INSERT_MIDDLE 1
#define INSERT_BEFORE 2

typedef struct
{
  sequencer_data_t data;
  sequencer_data_t shadow;
  uint32_t gate_output_pins[NUM_TRACKS];
  position_t players[NUM_TRACKS];
  position_t editors[NUM_TRACKS];
  position_t restore[NUM_TRACKS];
  position_t clipboard;
  track_t clipboard_track;
  position_t select_start;
  position_t select_end;

  wallclock_t last_clock_timestamp;
  unsigned long latched_period;
  unsigned long last_period;
  int nselected;

  uint8_t clipboard_mode;
  uint8_t focus_voltage_table;
  uint8_t focus_voltage_index;
  uint8_t focus_track;
  uint8_t trigger_track;
  uint8_t focus_slot;
  uint8_t mode;
  uint8_t focus_ref_table;
  uint8_t focus_ref_index;
  uint8_t insert_mode;
  uint8_t confirm_mode;
  uint8_t reset_quantization;
  uint8_t commit_quantization;

  bool initialized;
  bool pause;
  bool voltage_overlay_active;
  bool track_overlay_active;
  bool math_overlay_active;
  bool insert_overlay_active;
  bool ref_table_active;
  bool blink_active;
  bool internal_reset_armed;
  bool external_reset_armed;
  bool commit_armed;
  bool selecting;
  bool waiting_for_clock_after_reset;
  bool clock_is_late;
  bool flash_lstart;
  bool flash_lend;
  bool math_button_release_ignore;

  walltimer_t trigger_timers[NUM_TRACKS];
  unsigned long trigger_unit;                     // in cpu cycles
  unsigned long trigger_max_duration[NUM_TRACKS]; // max before sequential triggers would overlap

  walltimer_t ppqn_timers[NUM_TRACKS];
  uint8_t ppqn_counters[NUM_TRACKS];

  int16_t current_tick_adjustment[NUM_TRACKS];
  int16_t next_tick_adjustment[NUM_TRACKS];

  unsigned long soft_reset_interval; // in cpu cycles

  uint16_t *pasted_table;

  uint8_t euclid_n;
  uint8_t euclid_k;
} sequencer_t;

#define SEQ_EDIT_FOCUS (&(seq.editors[seq.focus_track]))
#define SEQ_LIVE_FOCUS (&(seq.players[seq.focus_track]))
#define SEQ_FOCUS ((seq.mode == MODE_LIVE) ? SEQ_LIVE_FOCUS : SEQ_EDIT_FOCUS)
#define SEQ_ANTI_FOCUS ((seq.mode == MODE_LIVE) ? SEQ_EDIT_FOCUS : SEQ_LIVE_FOCUS)

extern const uint8_t pitched_option[];
extern const uint8_t smooth_option[];
extern const bool ref_table_can_edit[];

#define RESERVED_Q_SIZE 128
extern uint32_t reserved_q_buffer[RESERVED_Q_SIZE];

////////////////////////////

extern sequencer_t seq;

////////////////////////////

void ref_start(void);
void ref_end(void);
void ref_select(int i);
void ref_decrement_index(void);
void ref_increment_index(void);
void ref_display(void);
void ref_display_table_name(void);

////////////////////////////

void position_init(position_t *p, sequencer_data_t *data, track_t *track);
void position_copy(position_t *dst, const position_t *src);
void position_set_start(position_t *dst, const position_t *src, uint8_t focus);
void position_set_end(position_t *dst, const position_t *src, uint8_t focus);
bool position_is_before(position_t *a, position_t *b);
int position_count_steps(position_t *a, position_t *b);
void pattern_init(pattern_t *pattern);

void track_init(track_t *track, uint8_t id);
void step_init(step_t *step);
void step_init_with_1V(position_t *p);
void transform_init(transform_t *t);
void sequencer_data_init(sequencer_data_t *data);
void sequencer_init(void);

sequencer_data_t *seq_get_focused_data(void);

////////////////////////////

pattern_id_t seq_allocate_pattern(sequencer_data_t *data);
void seq_free_pattern(sequencer_data_t *data, pattern_id_t m);
step_id_t seq_allocate_step(sequencer_data_t *data);
void seq_free_step(sequencer_data_t *data, step_id_t s);

////////////////////////////

void seq_display_step(step_t *step);
void seq_display_position(position_t *p);
void seq_display_loop(position_t *p);
void seq_display_voltage(void);
void seq_display_nselected(void);

////////////////////////////

void seq_on_real_clock_rising(void);
void seq_on_clock_rising(wallclock_t now, bool synthetic, bool *include);
void seq_on_clock_falling(void);
void synthetic_clock_maintenance(void);
void trigger_maintenance(void);

void seq_hard_reset(void);
void seq_advance_all_tracks(void);
bool seq_advance_position(position_t *p);
void seq_write_to_dac(position_t *p);
void seq_update_dac_outputs(bool *include);
void seq_update_gate_outputs(wallclock_t now, bool *include);

void seq_set_slot(uint8_t i);
void seq_set_table(uint8_t i);
void seq_set_track(uint8_t i);
void seq_increment_track(void);
void seq_decrement_track(void);

void seq_set_track_smooth(position_t *p, uint8_t table, bool value);
void seq_set_pattern_smooth(position_t *p, uint8_t table, bool value);
void seq_set_step_smooth(position_t *p, uint8_t table, bool value);
bool seq_is_smooth(position_t *p, uint8_t table);
bool seq_is_track_smooth(position_t *p, uint8_t table);
bool seq_is_pattern_smooth(position_t *p, uint8_t table);
bool seq_is_step_smooth(position_t *p, uint8_t table);

void seq_set_lstart(position_t *p);
void seq_set_lend(position_t *p);

bool seq_next_pattern(position_t *p, bool fail_to_last_step);
bool seq_prev_pattern(position_t *p, bool fail_to_first_step);

void seq_pattern_begin(position_t *p);
void seq_pattern_end(position_t *p);
void seq_track_begin(position_t *p);

bool seq_next_step(position_t *p);
bool seq_prev_step(position_t *p);
bool seq_next_step_with_loop(position_t *p);

void seq_rewind(position_t *p);

void seq_increment_cva(position_t *p);
void seq_decrement_cva(position_t *p);
void seq_increment_cvb(position_t *p);
void seq_decrement_cvb(position_t *p);
void seq_increment_gate(position_t *p);
void seq_decrement_gate(position_t *p);
void seq_increment_duration(position_t *p, bool steal);
void seq_decrement_duration(position_t *p, bool steal);
void seq_increment_voltage(uint16_t amt);
void seq_decrement_voltage(uint16_t amt);
void seq_increment_voltage_coarse(void);
void seq_decrement_voltage_coarse(void);
void seq_increment_voltage_super_coarse(void);
void seq_decrement_voltage_super_coarse(void);
void seq_select_voltage_grid(void);

void seq_toggle_ratchet(position_t *p);

bool seq_insert_pattern_after(position_t *p);
bool seq_insert_step_after(position_t *p);
bool seq_insert_pattern_before(position_t *p);
bool seq_split_pattern(position_t *p);
bool seq_insert_step_before(position_t *p);
bool seq_split_step(position_t *p);

bool seq_smart_insert_step_after(position_t *p);
bool seq_smart_insert_step_before(position_t *p);

void seq_delete_track(position_t *p);
void seq_delete_pattern(position_t *p);
void seq_delete_step(position_t *p);

bool seq_step_in_pattern(position_t *p, step_id_t s);

////////////////////////////

void seq_save_snapshot(uint8_t slot);
void seq_load_snapshot(uint8_t slot);
bool seq_snapshot_exists(uint8_t slot);

////////////////////////////

void seq_initiate_hold(void);
void seq_commit(void);
void seq_release_hold(void);
void seq_initiate_edit(void);
void seq_initiate_live(void);

////////////////////////////

void seq_copy_step(step_t *dst, step_t *src);

////////////////////////////

void seq_apply_transform_to_track(position_t *p, transform_t *t);
void seq_apply_transform_to_pattern(position_t *p, transform_t *t);
void seq_apply_transform_to_step(position_t *p, transform_t *t);

void transform_increment_cva(transform_t *t);
void transform_increment_cvb(transform_t *t);
void transform_increment_duration(transform_t *t);
void transform_increment_gate(transform_t *t);
void transform_decrement_cva(transform_t *t);
void transform_decrement_cvb(transform_t *t);
void transform_decrement_duration(transform_t *t);
void transform_decrement_gate(transform_t *t);

////////////////////////////

void tilt_trigger(void);
void blink_single_message(uint16_t left, uint16_t right);
void blink_double_message(uint16_t left, uint16_t right, uint16_t left2, uint16_t right2);
void blink_message_update(void);

void seq_rename(position_t *p);

void seq_jump_to_next_ppqn(position_t *p);
void seq_notify_ppqn_changed(position_t *p);

void seq_copy_to_reftable(uint8_t table, uint16_t *data);

static inline bool stopped(void)
{
  return seq.pause || seq.clock_is_late;
}

#endif /* SEQUENCER_H_ */
