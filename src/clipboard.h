/*
 * clipboard.h
 *
 * Created: 26/04/2016 17:15:48
 *  Author: clarkson
 */

#ifndef CLIPBOARD_H_
#define CLIPBOARD_H_

void init_clipboard(void);
void clear_clipboard(void);

void copy_to_clipboard(position_t *p0, position_t *p1, uint8_t mode);
void paste_from_clipboard_after(position_t *p);
void paste_from_clipboard_before(position_t *p);
void paste_table(void);
bool can_copy_from_clipboard(void);

#endif /* CLIPBOARD_H_ */