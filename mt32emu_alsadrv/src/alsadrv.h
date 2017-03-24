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

#ifndef MT32_ALSA_DRV_H
#define MT32_ALSA_DRV_H

#include <mt32emu/mt32emu.h>

typedef struct {
	unsigned char type;	
	
	unsigned int msg;
	struct timeval stamp;

	void *sysex;
	unsigned short sysex_len;	
} midiev_t;


/* Buffering info */
#define BUFFER_AUTO      0
#define BUFFER_MANUAL    1

/* Consumer info */
#define CONSUME_PLAYING      1
#define CONSUME_WAVOUT       2
#define CONSUME_SYSEX        4

/* Event packages */
#define EVENT_MIDI           0
#define EVENT_NONE           1
#define EVENT_SYSEX          2
#define EVENT_UNSUBSCRIBED   3
#define EVENT_SET_RVMODE     4
#define EVENT_SET_RVTIME     5
#define EVENT_SET_RVLEVEL    6
#define EVENT_RESET          7
#define EVENT_WAVREC_ON      8
#define EVENT_WAVREC_OFF     9
#define EVENT_SYXREC_ON      10
#define EVENT_SYXREC_OFF     11
#define EVENT_MIDI_TRIPLET   12

extern int minimum_msec;
extern int maximum_msec;
extern int buffer_mode;
extern int buffermsec;

extern int consumer_types;

/* Reverb info */
extern int rv_type;
extern int rv_time;
extern int rv_level;

/* aux-output info */
extern char *recsyx_filename;
extern FILE *recsyx_file;
extern char *recwav_filename;
extern FILE *recwav_file;


void send_rvmode_sysex(int newmode);
void send_rvtime_sysex(int newtime);
void send_rvlevel_sysex(int newlevel);

extern int alsa_buffer_size;
extern int eventpipe[];
extern char *pcm_name;

extern double gain_multiplier;
extern MT32Emu::AnalogOutputMode analog_output_mode;
extern unsigned int sample_rate;

extern char *rom_dir;
extern enum rom_search_type_t {
	ROM_SEARCH_TYPE_DEFAULT,
	ROM_SEARCH_TYPE_CM32L_ONLY,
	ROM_SEARCH_TYPE_MT32_ONLY
} rom_search_type;

int init_alsadrv();
int process_loop(int rv);

extern MT32Emu::Synth *mt32;



#endif
