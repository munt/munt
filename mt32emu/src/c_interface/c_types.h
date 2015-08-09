/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
* Copyright (C) 2011-2015 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#ifndef MT32EMU_C_TYPES_H
#define MT32EMU_C_TYPES_H

#include <stdarg.h>

typedef unsigned int       mt32emu_bit32u;
typedef   signed int       mt32emu_bit32s;
typedef unsigned short int mt32emu_bit16u;
typedef   signed short int mt32emu_bit16s;
typedef unsigned char      mt32emu_bit8u;
typedef   signed char      mt32emu_bit8s;

#if MT32EMU_USE_FLOAT_SAMPLES
typedef float mt32emu_sample;
typedef float mt32emu_sample_ex;
#else
typedef mt32emu_bit16s mt32emu_sample;
typedef mt32emu_bit32s mt32emu_sample_ex;
#endif

typedef char mt32emu_sha1_digest[41];

enum mt32emu_return_code {
	mt32emu_rc_ok = 0,
	mt32emu_rc_added_control_rom = 1,
	mt32emu_rc_added_pcm_rom = 2,
	mt32emu_rc_rom_not_identified = -1,
	mt32emu_rc_file_not_found = -2,
	mt32emu_rc_file_not_loaded = -3
};

// Emulation context
typedef struct mt32emu_data *mt32emu_context;

// Interface for handling reported events
struct mt32emu_report_handler_i {
	// Callback for debug messages, in vprintf() format
	void (*printDebug)(const struct mt32emu_report_handler_o *instance, const char *fmt, va_list list);
	// Callbacks for reporting errors
	void (*onErrorControlROM)(const struct mt32emu_report_handler_o *instance);
	void (*onErrorPCMROM)(const struct mt32emu_report_handler_o *instance);
	// Callback for reporting about displaying a new custom message on LCD
	void (*showLCDMessage)(const struct mt32emu_report_handler_o *instance, const char *message);
	// Callback for reporting actual processing of a MIDI message
	void (*onMIDIMessagePlayed)(const struct mt32emu_report_handler_o *instance);
	// Callbacks for reporting system events
	void (*onDeviceReset)(const struct mt32emu_report_handler_o *instance);
	void (*onDeviceReconfig)(const struct mt32emu_report_handler_o *instance);
	// Callbacks for reporting changes of reverb settings
	void (*onNewReverbMode)(const struct mt32emu_report_handler_o *instance, mt32emu_bit8u mode);
	void (*onNewReverbTime)(const struct mt32emu_report_handler_o *instance, mt32emu_bit8u time);
	void (*onNewReverbLevel)(const struct mt32emu_report_handler_o *instance, mt32emu_bit8u level);
	// Callbacks for reporting various information
	void (*onPolyStateChanged)(const struct mt32emu_report_handler_o *instance, int partNum);
	void (*onProgramChanged)(const struct mt32emu_report_handler_o *instance, int partNum, const char *soundGroupName, const char *patchName);
};

// Instance of report handler object
struct mt32emu_report_handler_o {
	// Interface
	const struct mt32emu_report_handler_i *i;
	// User data for this instance
	void *d;
};

#endif // #ifndef MT32EMU_C_TYPES_H
