/*
 * interrupt_handlers.h
 *
 * Created: 8/3/2014 8:50:54 PM
 *  Author: clarkson
 */

#ifndef INTERRUPT_HANDLERS_H_
#define INTERRUPT_HANDLERS_H_

void configure_early_interrupts(bool slavemode);
void configure_late_interrupts(bool slavemode);

extern volatile bool bridge_tx_ready;
extern volatile bool bridge_rx_ready;
extern volatile bool bridge_error;
extern volatile bool extdac_ready;
extern volatile bool segdisp_ready;

#endif /* INTERRUPT_HANDLERS_H_ */