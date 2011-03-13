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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <mt32emu/mt32emu.h>

#include "widgets.h"
#include "keypad.h"
#include "lcd.h"
#include "drvreport.h"
#include "alsadrv.h"
		   
#define LEFT_BUTTON    1
#define MIDDLE_BUTTON  2
#define RIGHT_BUTTON   3

#define LXPM(dst, name, ncb)   (dst).p = create_button(name); (dst).cb = ncb; \
	(dst).w = 38; (dst).h = 16; 

#define SPIN(name, button, upper, lower, inc) if (button == LEFT_BUTTON) (name) += inc; \
	if ((button == RIGHT_BUTTON) || (button == MIDDLE_BUTTON)) (name) -= inc; \
	if (name > upper) name = lower; \
	if (name < lower) name = upper; 

void blank_button_set(button_set_t *bs);
void redraw_keypad();


/* button console */
button_set_t *cset;
button_set_t mainmenu, setupmenu, reverbmenu, recordmenu;


/* internal reverb info */
int newrv_type  = 0;
int newrv_time  = 0;
int newrv_level = 0;


/* setup callbacks */
#define MAX_BUFFER_SIZE 4000
#define MIN_BUFFER_SIZE 100
int choose_bufsize = 500;

void setup_update_display()
{
//	LXPM(setupmenu.buttons[5], pix_ms, cb_bufsize);

	redraw_keypad();
}

void cb_bufsize(int button)
{
	SPIN(choose_bufsize, button, MAX_BUFFER_SIZE, MIN_BUFFER_SIZE, 20);
	setup_update_display();
}

void cb_setup(int button)
{
	cset = &setupmenu;
	redraw_keypad();
}

void cb_setup_ok(int button)
{
	cset = &mainmenu;
	redraw_keypad();
}


/* Reverb callbacks */
void rv_update_display(int rvtype, int rvtime, int rvlevel);

void cb_rvdefault(int button)
{
	newrv_type  = rv_type;
	newrv_time  = rv_time;
	newrv_level = rv_level;
	rv_update_display(newrv_type, newrv_time, newrv_level);
	redraw_keypad();
}

void cb_rvtype(int button)
{
	SPIN(newrv_type, button, 3, 0, 1);	
	rv_update_display(newrv_type, newrv_time, newrv_level);
	redraw_keypad();
}
void cb_rvtime(int button)
{
	SPIN(newrv_time, button, 7, 0, 1);	
	rv_update_display(newrv_type, newrv_time, newrv_level);
	redraw_keypad();
}
void cb_rvlevel(int button)
{
	SPIN(newrv_level, button, 7, 0, 1);	
	rv_update_display(newrv_type, newrv_time, newrv_level);
	redraw_keypad();
}

void rv_update_display(int rvtype, int rvtime, int rvlevel)
{	
	char number[5];
	
	switch(rvtype)
	{
	    case 0: LXPM(reverbmenu.buttons[3], "room", cb_rvtype);	break;
	    case 1: LXPM(reverbmenu.buttons[3], "hall", cb_rvtype);	break;
	    case 2: LXPM(reverbmenu.buttons[3], "plate", cb_rvtype);	break;
	    default: 
		LXPM(reverbmenu.buttons[3], "tap", cb_rvtype);
	}
	
	sprintf(number, "%d", rvtime);
	LXPM(reverbmenu.buttons[4], number, cb_rvtime);	

	sprintf(number, "%d", rvlevel);
	LXPM(reverbmenu.buttons[5], number, cb_rvlevel);	
}

void cb_rvapply(int button)
{
	if (newrv_type != rv_type)
		send_rvmode_sysex(newrv_type);

	if (newrv_time != rv_time)
		send_rvtime_sysex(newrv_time);
	
	if (newrv_level != rv_level)
		send_rvlevel_sysex(newrv_level);

	cset = &mainmenu;
	redraw_keypad();	
}

void cb_reverb(int button)
{
	cb_rvdefault(0);	

	//	rv_update_display(0, 0, 0);
	cset = &reverbmenu;
	redraw_keypad();
}



/* Record callbacks */
void cb_sysexdump(int button);
void cb_wavdump(int button);

void record_update_display()
{
	if (consumer_types & CONSUME_SYSEX) {
		LXPM(recordmenu.buttons[3], "yes", cb_sysexdump);	
	} else { 
		LXPM(recordmenu.buttons[3], "no", cb_sysexdump);	
	}
	
	if (consumer_types & CONSUME_WAVOUT) {
		LXPM(recordmenu.buttons[4], "yes", cb_wavdump);		
	} else {
		LXPM(recordmenu.buttons[4], "no", cb_wavdump);		
	}	

	redraw_keypad();
}

void cb_sysexdump(int button)	  
{
	midiev_t newev;	
	if (consumer_types & CONSUME_SYSEX)
	{
		newev.type = EVENT_SYXREC_OFF;
		LXPM(recordmenu.buttons[3], "no", cb_sysexdump);
	} else {
		newev.type = EVENT_SYXREC_ON;
		LXPM(recordmenu.buttons[3], "yes", cb_sysexdump);		
	}
	newev.msg = 0;
	write(eventpipe[1], &newev, sizeof(newev));	

	redraw_keypad();
}
void cb_wavdump(int button)	  
{
	midiev_t newev;
	if (consumer_types & CONSUME_WAVOUT)
	{
		newev.type = EVENT_WAVREC_OFF;
		LXPM(recordmenu.buttons[4], "no", cb_wavdump);
	} else {
		newev.type = EVENT_WAVREC_ON;
		LXPM(recordmenu.buttons[4], "yes", cb_wavdump);		
	}
	newev.msg = 0;
	write(eventpipe[1], &newev, sizeof(newev));	

	redraw_keypad();
}

void cb_record_ok(int button)
{
	cset = &mainmenu;
	redraw_keypad();
}

void cb_record(int button)
{
	cset = &recordmenu;
	record_update_display();
}


/* Misc callbacks */
void cb_exit(int button)
{
	quit();
}

void cb_clear(int button)
{
	int cmd;
	
	cmd = DRVCMD_CLEAR;
	write(uicmd_pipe[1], &cmd, sizeof(int));

	general_lcd_message("Notes cleared");
}

void cb_reset(int button)
{
	midiev_t newev;
	newev.type = EVENT_RESET; newev.msg = 0;
	write(eventpipe[1], &newev, sizeof(newev));	
}

void cb_back(int button)
{
	cset = &mainmenu;
	redraw_keypad();
}

void create_buttonsets()
{		
	
	
	/* main menu */
	blank_button_set(&mainmenu);
	//LXPM(mainmenu.buttons[0], "Setup",  cb_setup);
	
	if (xreverb_switch)
		LXPM(mainmenu.buttons[1], "Reverb", cb_reverb);
	
	LXPM(mainmenu.buttons[2], "Record", cb_record);
	LXPM(mainmenu.buttons[6], "Clear",  cb_clear);
	LXPM(mainmenu.buttons[7], "Reset",  cb_reset);
	LXPM(mainmenu.buttons[8], "Exit",   cb_exit);
	
	/* setup menu */
	blank_button_set(&setupmenu);
//	LXPM(setupmenu.buttons[2], "Buffer",  cb_bufsize);	
//	LXPM(setupmenu.buttons[7], "Save",    cb_setup_ok);
	LXPM(setupmenu.buttons[8], "Back",    cb_back);	
	
	/* reverb menu */
	blank_button_set(&reverbmenu);
	LXPM(reverbmenu.buttons[0], "Type",    cb_rvtype);
	LXPM(reverbmenu.buttons[1], "Time",    cb_rvtime);
	LXPM(reverbmenu.buttons[2], "Level",   cb_rvlevel);

	LXPM(reverbmenu.buttons[6], "Default", cb_rvdefault);
	LXPM(reverbmenu.buttons[7], "Apply",   cb_rvapply);
	LXPM(reverbmenu.buttons[8], "Cancel",  cb_back);

	/* record menu */
	blank_button_set(&recordmenu);
	LXPM(recordmenu.buttons[0], "Sysex",  cb_sysexdump);
	LXPM(recordmenu.buttons[1], "Wave",   cb_wavdump);
	LXPM(recordmenu.buttons[8], "Back",   cb_back);

}


