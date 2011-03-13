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

#ifndef MT_KEYPAD_H
#define MT_KEYPAD_H

typedef struct
{
	Pixmap p;
	void (*cb)(int);
	int w, h;
} button_t;

typedef struct
{
	button_t buttons[9];
} button_set_t;


extern button_set_t *cset;
extern button_set_t mainmenu, setupmenu, reverbmenu, recordmenu;


void create_buttonsets();

#endif

