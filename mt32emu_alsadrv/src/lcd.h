#ifndef MT32_LCD_H
#define MT32_LCD_H


void lcd_setup();

int lcd_age();   /* returns seconds remaining or -1 for none */
void lcd_redraw();
void sysex_lcd_message(char *buf);
void general_lcd_message(char *buf);


extern XFontStruct *flcd;

extern int lcd_msec;
extern char lcd_flags[];

#endif

