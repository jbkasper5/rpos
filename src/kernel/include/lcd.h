#ifndef __LCD_H__
#define __LCD_H__

#include "macros.h"

void lcd_init(uint8_t address);
void lcd_backlight(bool on);
void lcd_print(char* s);

#endif