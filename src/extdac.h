/*
 * extdac.h
 *
 * Created: 5/1/2013 3:17:59 PM
 *  Author: clarkson
 */

#ifndef EXTDAC_H_
#define EXTDAC_H_

#define EXTDAC_MAX_OUTPUT 0xFFFF
#define FLOAT_EPSILON 1.2e-7

void extdac_zero_all(void);
void extdac_feed_configuration(void);
void extdac_init(void);
void extdac_set_value(int i, int value);
void extdac_set_smoother_timing(uint8_t track, wallclock_t t0, unsigned long period);
void extdac_set_smoother_output(int i, int v0, int v1);
void extdac_disable_smoother(int i);
void extdac_calibration_off(void);
void extdac_calibration_on(void);
bool extdac_is_calibration_enabled(void);

void extdac_dma_start(void);
void extdac_feed(void);
void extdac_feed_slavemode(void);

#endif /* EXTDAC_H_ */