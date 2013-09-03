/* Copyright (C) 2003 Tristan
 * Copyright (C) 2004, 2005 Tristan, Jerome Fisher
 * Copyright (C) 2008, 2011 Tristan, Jerome Fisher, Jörg Walter
 * Copyright (C) 2013 Tristan, Jerome Fisher, Jörg Walter, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32_LCD_H
#define MT32_LCD_H


void lcd_setup();

int lcd_age();   /* returns seconds remaining or -1 for none */
void lcd_redraw();
void sysex_lcd_message(const char *buf);
void general_lcd_message(const char *buf);


extern XFontStruct *flcd;

extern int lcd_msec;
extern char lcd_flags[];

#endif

