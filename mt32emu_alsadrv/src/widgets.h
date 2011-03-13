/* Copyright (C) 2003 Tristan
 * Copyright (C) 2004, 2005 Tristan, Jerome Fisher
 * Copyright (C) 2008, 2011 Tristan, Jerome Fisher, Jörg Walter
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

#ifndef XGUI_WIDGETS_H
#define XGUI_WIDGETS_H

#define WINDOW_W     480
#define WINDOW_H     100 

/* geometry */
#define BUTTONPAD_X  253
#define BUTTONPAD_Y   20
#define BUTTONPAD_W  210
#define BUTTONPAD_H   60
//#define BUTTON_W      38
#define BUTTON_W      70
#define BUTTON_H      20

#define DISPLAY_X     22
#define DISPLAY_Y     10
#define DISPLAY_W     215
#define DISPLAY_H     79


/* General X variables */
extern Display  *dpy;
extern Window   rootWindow, mainWindow;
extern GC       mainGC;
extern Colormap mainCM;
extern int      mainScreen;
extern int      rootDepth;

/* Colours */
extern XColor LCDColour;
extern XColor LCDTextColour;
extern XColor LCDSoftColour, LCDHardColour;
extern XColor edgeColour;
extern XColor buttonColour;

/* Some settings */
extern int xreverb_switch;

XFontStruct * load_font(const char **fontlist);
void get_string_dims(XFontStruct *fs, const char *txt, int *_w, int *_h);
Pixmap create_button(const char *txt);

void create_pixmaps();
void quit();

extern Pixmap pix_stripe, pix_logo, pix_button;
extern int stipe_width, stripe_height;
extern int logo_width, logo_height;
extern int button_width, button_height;


#endif


