/*
 * display.h
 *
 * Created: 4/30/2013 4:16:30 PM
 *  Author: clarkson
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#define SEGDISP_VOLT0 0
#define SEGDISP_VOLT1 1
#define SEGDISP_CVB 2
#define SEGDISP_CVA 3
#define SEGDISP_DURATION 4
#define SEGDISP_GATE 5
#define SEGDISP_SNAPSHOT 6
#define SEGDISP_STEP 7
#define SEGDISP_PATTERN 8
#define SEGDISP_TRACK 9
#define SEGDISP_INDEX 10
#define SEGDISP_VOLT01 11

#define ORANGE_VOLT0 (1 << 5)
#define ORANGE_VOLT1 (1 << 6)
#define ORANGE_CVB (1 << 8)
#define ORANGE_CVA (1 << 7)
#define ORANGE_GATE (1 << 9)
#define ORANGE_DURATION (1 << 10)
#define ORANGE_SNAPSHOT (1 << 0)
#define ORANGE_PATTERN (1 << 1)
#define ORANGE_STEP (1 << 2)
#define ORANGE_TRACK (1 << 3)
#define ORANGE_INDEX (1 << 4)

extern const uint16_t display_to_led[];
extern const uint16_t segdisp_both_digits[100];
extern const uint16_t segdisp_left_digits[10];
extern const uint16_t segdisp_right_digits[10];

#define SEGDISP_BLANK 0x0000
#define SEGDISP_ALL 0xFFFF
#define SEGDISP_RIGHT_ALL 0xFF00
#define SEGDISP_LEFT_ALL 0x00FF

#define SEGDISP_RIGHT_0 0x7700
#define SEGDISP_RIGHT_1 0x4100
#define SEGDISP_RIGHT_2 0x3B00
#define SEGDISP_RIGHT_3 0x6B00
#define SEGDISP_RIGHT_4 0x4D00
#define SEGDISP_RIGHT_5 0x6E00
#define SEGDISP_RIGHT_6 0x7E00
#define SEGDISP_RIGHT_7 0x4300
#define SEGDISP_RIGHT_8 0x7F00
#define SEGDISP_RIGHT_9 0x6F00
#define SEGDISP_RIGHT_A 0x5F00
#define SEGDISP_RIGHT_BLANK 0x0000
#define SEGDISP_RIGHT_C 0x3600
#define SEGDISP_RIGHT_CARET 0x0700
#define SEGDISP_RIGHT_CLOCK0 0x0600
#define SEGDISP_RIGHT_CLOCK1 0x0300
#define SEGDISP_RIGHT_CLOCK2 0x4100
#define SEGDISP_RIGHT_CLOCK3 0x6000
#define SEGDISP_RIGHT_CLOCK4 0x3000
#define SEGDISP_RIGHT_CLOCK5 0x1400
#define SEGDISP_RIGHT_DP 0x8000
#define SEGDISP_RIGHT_E 0x3E00
#define SEGDISP_RIGHT_F 0x1E00
#define SEGDISP_RIGHT_G 0x7600
#define SEGDISP_RIGHT_H 0x5D00
#define SEGDISP_RIGHT_HYPHEN 0x0800
#define SEGDISP_RIGHT_J 0x7100
#define SEGDISP_RIGHT_L 0x3400
#define SEGDISP_RIGHT_M1 0x5700
#define SEGDISP_RIGHT_M2 0x4300
#define SEGDISP_RIGHT_N 0x5700
#define SEGDISP_RIGHT_O 0x7700
#define SEGDISP_RIGHT_P 0x1F00
#define SEGDISP_RIGHT_R 0x1700
#define SEGDISP_RIGHT_S 0x6E00
#define SEGDISP_RIGHT_SCLOCK0 0x0200
#define SEGDISP_RIGHT_SCLOCK1 0x0100
#define SEGDISP_RIGHT_SCLOCK2 0x4000
#define SEGDISP_RIGHT_SCLOCK3 0x2000
#define SEGDISP_RIGHT_SCLOCK4 0x1000
#define SEGDISP_RIGHT_SCLOCK5 0x0400
#define SEGDISP_RIGHT_U 0x7500
#define SEGDISP_RIGHT_XCLOCK0 0x2200
#define SEGDISP_RIGHT_XCLOCK1 0x1100
#define SEGDISP_RIGHT_XCLOCK2 0x4400
#define SEGDISP_RIGHT_XCLOCK3 0x2200
#define SEGDISP_RIGHT_XCLOCK4 0x1100
#define SEGDISP_RIGHT_XCLOCK5 0x4400
#define SEGDISP_RIGHT_Y 0x6D00
#define SEGDISP_RIGHT_b 0x7C00
#define SEGDISP_RIGHT_c 0x3800
#define SEGDISP_RIGHT_d 0x7900
#define SEGDISP_RIGHT_h 0x5C00
#define SEGDISP_RIGHT_i 0x4000
#define SEGDISP_RIGHT_l 0x4100
#define SEGDISP_RIGHT_n 0x5800
#define SEGDISP_RIGHT_o 0x7800
#define SEGDISP_RIGHT_q 0x4F00
#define SEGDISP_RIGHT_r 0x1800
#define SEGDISP_RIGHT_seg_dn 0x2000
#define SEGDISP_RIGHT_seg_ld 0x1000
#define SEGDISP_RIGHT_seg_lu 0x0400
#define SEGDISP_RIGHT_seg_mid 0x0800
#define SEGDISP_RIGHT_seg_rd 0x4000
#define SEGDISP_RIGHT_seg_ru 0x0100
#define SEGDISP_RIGHT_seg_up 0x0200
#define SEGDISP_RIGHT_t 0x3C00
#define SEGDISP_RIGHT_u 0x7000

#define SEGDISP_LEFT_0 0x0077
#define SEGDISP_LEFT_1 0x0014
#define SEGDISP_LEFT_2 0x00B3
#define SEGDISP_LEFT_3 0x00B6
#define SEGDISP_LEFT_4 0x00D4
#define SEGDISP_LEFT_5 0x00E6
#define SEGDISP_LEFT_6 0x00E7
#define SEGDISP_LEFT_7 0x0034
#define SEGDISP_LEFT_8 0x00F7
#define SEGDISP_LEFT_9 0x00F6
#define SEGDISP_LEFT_A 0x00F5
#define SEGDISP_LEFT_BLANK 0x0000
#define SEGDISP_LEFT_C 0x0063
#define SEGDISP_LEFT_CARET 0x0070
#define SEGDISP_LEFT_CLOCK0 0x0060
#define SEGDISP_LEFT_CLOCK1 0x0030
#define SEGDISP_LEFT_CLOCK2 0x0014
#define SEGDISP_LEFT_CLOCK3 0x0006
#define SEGDISP_LEFT_CLOCK4 0x0003
#define SEGDISP_LEFT_CLOCK5 0x0041
#define SEGDISP_LEFT_DP 0x0008
#define SEGDISP_LEFT_E 0x00E3
#define SEGDISP_LEFT_F 0x00E1
#define SEGDISP_LEFT_G 0x0067
#define SEGDISP_LEFT_H 0x00D5
#define SEGDISP_LEFT_HYPHEN 0x0080
#define SEGDISP_LEFT_J 0x0017
#define SEGDISP_LEFT_L 0x0043
#define SEGDISP_LEFT_M1 0x0075
#define SEGDISP_LEFT_M2 0x0034
#define SEGDISP_LEFT_N 0x0075
#define SEGDISP_LEFT_O 0x0077
#define SEGDISP_LEFT_P 0x00F1
#define SEGDISP_LEFT_R 0x0071
#define SEGDISP_LEFT_S 0x00E6
#define SEGDISP_LEFT_SCLOCK0 0x0020
#define SEGDISP_LEFT_SCLOCK1 0x0010
#define SEGDISP_LEFT_SCLOCK2 0x0004
#define SEGDISP_LEFT_SCLOCK3 0x0002
#define SEGDISP_LEFT_SCLOCK4 0x0001
#define SEGDISP_LEFT_SCLOCK5 0x0040
#define SEGDISP_LEFT_U 0x0057
#define SEGDISP_LEFT_XCLOCK0 0x0022
#define SEGDISP_LEFT_XCLOCK1 0x0011
#define SEGDISP_LEFT_XCLOCK2 0x0044
#define SEGDISP_LEFT_XCLOCK3 0x0022
#define SEGDISP_LEFT_XCLOCK4 0x0011
#define SEGDISP_LEFT_XCLOCK5 0x0044
#define SEGDISP_LEFT_Y 0x00D6
#define SEGDISP_LEFT_b 0x00C7
#define SEGDISP_LEFT_c 0x0083
#define SEGDISP_LEFT_d 0x0097
#define SEGDISP_LEFT_h 0x00C5
#define SEGDISP_LEFT_i 0x0004
#define SEGDISP_LEFT_l 0x0014
#define SEGDISP_LEFT_n 0x0085
#define SEGDISP_LEFT_o 0x0087
#define SEGDISP_LEFT_q 0x00F4
#define SEGDISP_LEFT_r 0x0081
#define SEGDISP_LEFT_seg_dn 0x0002
#define SEGDISP_LEFT_seg_ld 0x0001
#define SEGDISP_LEFT_seg_lu 0x0040
#define SEGDISP_LEFT_seg_mid 0x0080
#define SEGDISP_LEFT_seg_rd 0x0004
#define SEGDISP_LEFT_seg_ru 0x0010
#define SEGDISP_LEFT_seg_up 0x0020
#define SEGDISP_LEFT_t 0x00C3
#define SEGDISP_LEFT_u 0x0007

extern const uint16_t segdisp_left_clock[];
extern const uint16_t segdisp_right_clock[];
extern const uint16_t segdisp_infinite_clock[];

#define SEGDISP_NUM 11
extern uint8_t segdisp_buffer[SEGDISP_NUM * 2];
extern uint16_t segdisp_display[SEGDISP_NUM];
extern uint8_t orange_buffer[2];
extern uint16_t orange_display;
extern uint8_t focus_left_display;
extern uint8_t focus_right_display;
extern uint8_t saved_left_display;
extern uint8_t saved_right_display;

extern uint8_t flash_display;
extern uint8_t flash_state;
extern uint16_t flash_value;
extern uint16_t flash_led;

void wait_for_display_to_refresh(void);
void display_dma_start(void);
bool display_feed(void);

void flash(uint8_t orange, uint8_t display, uint16_t value);

void orange_clear(int display);
void orange_set(int display);
void display_init(void);
void select_left_display(int display);
void select_right_display(int display);
void highlight_left_display(int display);
void highlight_right_display(int display);
void restore_left_display(void);
void restore_right_display(void);
void segdisp_put_dualdigit(int display, uint8_t number);
void segdisp_put_number(int display, uint8_t number);
void segdisp_put_signed(int display, int number);
void segdisp_put_right_digit(int display, uint8_t digit);
void segdisp_put_note(uint16_t number);
void segdisp_put_voltage(uint16_t number);
void segdisp_put_version(uint16_t major, uint16_t minor, uint16_t patch);
void segdisp_clear(int display, uint16_t mask);
void segdisp_toggle(int display, uint16_t mask);
void segdisp_set(int display, uint16_t mask);
void segdisp_sharp_on(void);
void segdisp_sharp_off(void);
void segdisp_sharp_toggle(void);
void segdisp_flat_on(void);
void segdisp_flat_off(void);
void segdisp_flat_toggle(void);

//////////////////////////////
#define ERR_LOAD 10
#define ERR_SAVE 11
#define ERR_MEMTEST 12
void segdisp_on_error(uint8_t code);

#endif /* DISPLAY_H_ */