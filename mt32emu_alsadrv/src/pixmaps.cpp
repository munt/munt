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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>

#include "img/logo.xpm"
#include "img/stripe.xpm"
#include "img/button.xpm"

#include "widgets.h"

XFontStruct *button_font = NULL;

Pixmap pix_stripe = None;
int stripe_width, stripe_height;

Pixmap pix_logo = None;
int logo_width, logo_height;

Pixmap pix_button = None;
int button_width, button_height;

void create_pixmaps()
{
	XpmAttributes atr;
	
	atr.valuemask = 0;
	XpmCreatePixmapFromData(dpy, rootWindow, xpm_logo, &pix_logo, NULL, &atr);
	logo_width = atr.width; logo_height = atr.height;

	atr.valuemask = 0;
	XpmCreatePixmapFromData(dpy, rootWindow, xpm_stripe, &pix_stripe, NULL, &atr);
	stripe_width = atr.width; stripe_height = atr.height;
	
	atr.valuemask = 0;
	XpmCreatePixmapFromData(dpy, rootWindow, xpm_button, &pix_button, NULL, &atr);
	button_width = atr.width; button_height = atr.height;	
}

XFontStruct * load_font(const char **fontlist)
{
	XFontStruct *fs;
	int i;
	
	for (i = 0; ; i++)
	{
		if (fontlist[i] == NULL)
			return NULL;
		
		fs = XLoadQueryFont(dpy, fontlist[i]);
		if (fs != NULL)
			break;
	}
	
	return fs;
}

void get_string_dims(XFontStruct *fs, const char *txt, int *_w, int *_h)
{
	unsigned char c;
	int tl, w, h, nh, i;
	
	w = 0; h = 0;
	
	for (i = 0; ; i++)
	{
		if (txt[i] == 0)
			break;
	
		c = txt[i];
		
		/* From xfontstruct man page: "If the per_char pointer is NULL, all glyphs between the
		first and last character indexes inclusive have the same information, as given by both
		min_bounds and max_bounds." */
		if (fs->per_char == NULL)
		{
			w += fs->max_bounds.width;
			nh = fs->max_bounds.ascent; // + fs->max_bounds.descent;
		} else {
			w += fs->per_char[c].width;			
			nh = fs->per_char[c].ascent; // + fs->per_char[c].descent;
		}
		
		if (nh > h)
			h = nh;
	}
	
	*_w = w;
	*_h = h;
}

const char *button_fonts[] =
{
	"-*-helvetica-medium-r-*--10-*",
		"-*-fixed-medium-r-*--10-*",
		"-*-*-*-r-*--10-*", 
		"*",
		NULL
};


Pixmap create_button(const char *txt)
{
	Pixmap p;
	int tl, x, y, w, h;
	
	tl = strlen(txt);
	
	/* first time */
	if (button_font == NULL)
		button_font = load_font(button_fonts);
	
	
	/* make a copy of the button */
	p = XCreatePixmap(dpy, mainWindow, button_width, button_height, rootDepth);
	XCopyArea(dpy, pix_button, p, mainGC, 0, 0, button_width, button_height, 0, 0);
			
	/* put the text on to it */
	get_string_dims(button_font, txt, &w, &h);
	x = (button_width - w) >> 1;
	y = h + ((button_height - h) >> 1);
		
	XSetFont(dpy, mainGC, button_font->fid);	
	XSetForeground(dpy, mainGC, buttonColour.pixel);
	
	XDrawString(dpy, p, mainGC, x, y, txt, tl);
	
	return p;
}







