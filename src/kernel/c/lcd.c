#include "lcd.h"
#include "timer.h"
#include "i2c.h"

typedef enum Flags {
    FLAG_RS = 1,
    FLAG_RW = 2,
    FLAG_EN = 4
} flags;

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

#define LCD_BACKLIGHT 8
#define LCD_NOBACKLIGHT 0

// static uint8_t _backlight = LCD_BACKLIGHT;
// static uint8_t _lcd_address = 0;


// static void write_i2c(uint8_t data){
//     uint8_t value = data | _backlight;
//     i2c_send(_lcd_address, &value, 1);
// }

// static void pulse(uint8_t data){
//     write_i2c(data | FLAG_EN);
//     timer_sleep(5);
//     write_i2c(data & ~FLAG_EN);
//     timer_sleep(1);
// }

// static void write_4_bits(uint8_t data){
//     write_i2c(data);
//     pulse(data); 
// }

// void lcd_init(uint8_t address){

// }

// void lcd_backlight(bool on){

// }

// void lcd_print(char* s){

// }