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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include "widgets.h"

/* delay in seconds */
#define LCD_CLEAR_DELAY   3
time_t lcd_last_age;

const char *lcd_font1[] = 	
{		
	"-*-times-medium-r-*--20-*",
		"-*-fixed-medium-r-*--20-*",
		"-*-*-*-r-*--20-*",
		"*", NULL
};

const char *lcd_flagfont_info[] = 	
{		
	"-*-helvetica-medium-r-*--8-*",
		"-*-fixed-medium-r-*--8-*",
		"-*-*-*-r-*--8-*",
		"*", NULL
};

const char *lcd_font2[] = 
{      
	"-*-helvetica-medium-r-*--10-*",
		"-*-fixed-medium-r-*--10-*",
		"-*-*-*-r-*--10-*",
		"*", NULL
};

#define LCD_CHARS   20
#define MSG_CHARS   40

/* LCD sections */
char lcd_client[LCD_CHARS + 1] = "";
char lcd_message[MSG_CHARS + 1] = "";
char lcd_flags[] = {'W', 'P', 'X', 0};

int lcd_flags_w, lcd_flags_h;
int lcd_msec = 0;

XFontStruct *lcd_main;
XFontStruct *lcd_flagfont;
XFontStruct *lcd_sub;

void lcd_flag_draw(int n)
{
	int x, y;
	char c;
	
	c = lcd_flags[n];
	if (c == 0) return;
	
	if (c & (1 << 7))
		XSetForeground(dpy, mainGC, LCDHardColour.pixel);
	else
		XSetForeground(dpy, mainGC, LCDSoftColour.pixel); 	
	c &= ~(1 << 7);
	
	x = DISPLAY_X + DISPLAY_W - (lcd_flags_w * (n + 1) + 6 + (n << 3) );
	y = DISPLAY_Y + 4;
	
	XDrawRectangle(dpy, mainWindow, mainGC, x - 2, y - 2, lcd_flags_w + 4, lcd_flags_h + 4);
	
	y += lcd_flags_h;
	XDrawString(dpy, mainWindow, mainGC, x, y, &c, 1);
}

void lcd_flags_bounds(char *str, int &w, int &h)
{
	int i, nw, nh;
	char c;
	
	for (i = 0, w = 0, h = 0; ; i++)
	{
		c = str[i];		
		if (!c) 
			break;
		/* From xfontstruct man page: "If the per_char pointer is NULL, all glyphs between the
		first and last character indexes inclusive have the same information, as given by both
		min_bounds and max_bounds." */
		if (lcd_flagfont->per_char == NULL)
		{
			nw = lcd_flagfont->max_bounds.width;
			nh = lcd_flagfont->max_bounds.ascent + lcd_flagfont->max_bounds.descent;
		} else {
			nw = lcd_flagfont->per_char[c].width;
			nh = lcd_flagfont->per_char[c].ascent + lcd_flagfont->per_char[c].descent;
		}
		
		if (nw > w) w = nw;
		if (nh > h) h = nh;
	}
}

void lcd_display_channels()
{
	int i, j, x, y, h, w, v;

	XSetForeground(dpy, mainGC, LCDHardColour.pixel);
	
	w = 8;
	h = 1;
	x = DISPLAY_X + 2;
	y = DISPLAY_Y + DISPLAY_H - (h + 2);
	
	for (i = 1; i < 16; i++)
	{		
		v = i % 6;
		
		for (j = 0; j < v; j++)		
			XFillRectangle(dpy, mainWindow, mainGC, x, y - j * (h + 1), w, h);
		
		x += w + 2;
	}
}

void lcd_redraw()
{
	char str[32];
	int i, w, h;
	
	if (dpy == NULL)
		return;
	
	/* Blank it */
	XSetForeground(dpy, mainGC, LCDColour.pixel);
	XFillRectangle(dpy, mainWindow, mainGC,
		       DISPLAY_X, DISPLAY_Y, DISPLAY_W, DISPLAY_H);	
	
	/* Set text colours */
	XSetForeground(dpy, mainGC, LCDTextColour.pixel);

	/* Client area */
	XSetFont(dpy, mainGC, lcd_main->fid);
	XDrawString(dpy, mainWindow, mainGC, 
		    DISPLAY_X + 6, DISPLAY_Y + 20 + lcd_main->max_bounds.ascent, 
		    lcd_client, strlen(lcd_client));
	
	/* Message area */
	XSetFont(dpy, mainGC, lcd_sub->fid);
	XDrawString(dpy, mainWindow, mainGC, 
		    DISPLAY_X + 6, DISPLAY_Y + 44 + lcd_sub->max_bounds.ascent, 
		    lcd_message, strlen(lcd_message));
	
	/* latency info */
	XSetForeground(dpy, mainGC, LCDHardColour.pixel);
	sprintf(str, "%d msec", lcd_msec);
	XDrawString(dpy, mainWindow, mainGC, DISPLAY_X + 6, DISPLAY_Y + 12, str, strlen(str));
		
	/* draw flags */
	XSetFont(dpy, mainGC, lcd_flagfont->fid);
	for (i = 0; i < 4; i++)
		lcd_flag_draw(i);

//	lcd_display_channels();
}

int lcd_strnlen(const char *str, int n)
{
	int i;
	
	for (i = 0; i < n; i++)
		if (!str[i])
			break;

	return i;
}

void sysex_lcd_message(const char *buf)
{
	int length;
	
	length = lcd_strnlen(buf, LCD_CHARS);
	
	memcpy(lcd_client, buf, length);
	lcd_client[length] = 0;
	
	lcd_redraw();
}

void general_lcd_message(const char *buf)
{
	int length;
			
	length = strlen(buf);
	if (length > MSG_CHARS) 
		length = MSG_CHARS;
	
	memcpy(lcd_message, buf, length);
	lcd_message[length] = 0;
		
	lcd_redraw();

	/* schedule clearing of message area */
	lcd_last_age = time(NULL);
}

int lcd_age()
{		
	time_t current_time;
		
	/* return time remaining */
	if (lcd_last_age != 0)
	{
		current_time = time(NULL);
		
		/* return remaining time */
		if (current_time < lcd_last_age + LCD_CLEAR_DELAY)
			return (int)(lcd_last_age + LCD_CLEAR_DELAY - current_time); 
	}
	
	/* clear LCD */
	if (*lcd_message != 0)
	{
		*lcd_message = 0;	
		lcd_redraw();
	
		/* reset lcd aging */
		lcd_last_age = 0;
		
		return -1;
	}
	
	return -1;
}

void lcd_setup()
{	
	
	lcd_main = load_font(lcd_font1);
	lcd_sub  = load_font(lcd_font2);
	
	lcd_flagfont = load_font(lcd_flagfont_info);
	lcd_flags_bounds(lcd_flags, lcd_flags_w, lcd_flags_h);

	lcd_last_age = 0;
}

