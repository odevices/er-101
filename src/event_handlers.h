/*
 * handlers.h
 *
 * Created: 5/2/2013 9:54:51 PM
 *  Author: clarkson
 */

#ifndef HANDLERS_H_
#define HANDLERS_H_

extern volatile bool bridge_tx_ready;
extern volatile bool bridge_rx_ready;
extern volatile bool extdac_ready;
extern volatile bool segdisp_ready;
// extern volatile bool clock_arrived;
extern volatile wallclock_t clock_arrival_time;

extern bool insert_before;

#define MATH_STATE_START 0
#define MATH_STATE_HELD 1
#define MATH_STATE_PINNED 2
extern uint8_t math_state;

#define INSERT_STATE_START 0
#define INSERT_STATE_HELD 1
extern uint8_t insert_state;

void on_clock_event(void);
void seq_select_voltage_grid(void);
void on_increment_voltage_grid(void);
void on_decrement_voltage_grid(void);
bool start_confirm(uint8_t mode);
void end_confirm(void);
void display_transform_cvA(transform_t *t);
void display_transform_cvB(transform_t *t);
void display_transform_duration(transform_t *t);
void display_transform_gate(transform_t *t);
void start_math_overlay(void);
void end_math_overlay(void);
void start_track_overlay(void);
void update_track_overlay(void);
void end_track_overlay(void);
void update_insert_overlay(void);
void start_insert_overlay(uint8_t mode);
void end_insert_overlay(void);
void on_select_right_focus(int display);
void on_select_left_focus(int display);

void ui_load_button_push(void);
void ui_save_button_push(void);
void ui_mode_switch(void);
void ui_table_switch(void);
void ui_smooth_button_push(void);
void ui_copy_button_push(void);
void ui_copy_button_release(void);
void ui_commit_button_push(void);
void ui_lstart_button_push(void);
void ui_lend_button_push(void);
void ui_pause_button_push(void);
void ui_reset_button_push(void);
void ui_insert_button_push(void);
void ui_insert_button_release(void);
void ui_voltage_button_push(void);
void ui_voltage_button_release(void);
void ui_track_button_push(void);
void ui_track_button_release(void);
void ui_delete_button_push(void);
void ui_math_button_push(void);
void ui_math_button_release(void);
void ui_left_decrement(void);
void ui_left_increment(void);
void ui_right_decrement(void);
void ui_right_increment(void);

void math_left_increment(void);
void math_right_increment(void);
void math_left_decrement(void);
void math_right_decrement(void);

void flash_voltage(bool toggle);

void ext_reset_before(void);
void ext_reset_after(void);

void ui_timers_init(void);
void ui_timers_maintenance(void);

#endif /* HANDLERS_H_ */