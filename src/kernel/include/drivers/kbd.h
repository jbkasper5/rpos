#ifndef __KBD_H__
#define __KBD_H__

struct file_s;

void handle_keyboard_event(char c);
int keyboard_close(struct file_s* file);
int keyboard_open(struct file_s* file);
void keyboard_init();

#endif