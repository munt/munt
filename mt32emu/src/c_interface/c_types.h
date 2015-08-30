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

#include "../globals.h"

#define MT32EMU_C_ENUMERATIONS
#include "../Enumerations.h"
#undef MT32EMU_C_ENUMERATIONS

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

typedef enum {
	MT32EMU_BOOL_FALSE, MT32EMU_BOOL_TRUE
} mt32emu_boolean;

typedef enum {
	// Operation completed normally.
	MT32EMU_RC_OK = 0,
	MT32EMU_RC_ADDED_CONTROL_ROM = 1,
	MT32EMU_RC_ADDED_PCM_ROM = 2,

	// Definite error occurred.
	MT32EMU_RC_ROM_NOT_IDENTIFIED = -1,
	MT32EMU_RC_FILE_NOT_FOUND = -2,
	MT32EMU_RC_FILE_NOT_LOADED = -3,
	MT32EMU_RC_MISSING_ROMS = -4,
	MT32EMU_RC_NOT_OPENED = -5,
	MT32EMU_RC_QUEUE_FULL = -5,

	// Undefined error occurred.
	MT32EMU_RC_FAILED = -100
} mt32emu_return_code;

// Emulation context
typedef struct mt32emu_data *mt32emu_context;
typedef const struct mt32emu_data *mt32emu_const_context;

// Convenience aliases
#ifndef __cplusplus
typedef enum mt32emu_boolean mt32emu_boolean;
typedef enum mt32emu_return_code mt32emu_return_code;
typedef enum mt32emu_analog_output_mode mt32emu_analog_output_mode;
typedef enum mt32emu_dac_input_mode mt32emu_dac_input_mode;
typedef enum mt32emu_midi_delay_mode mt32emu_midi_delay_mode;
typedef enum mt32emu_partial_state mt32emu_partial_state;
#endif

typedef struct mt32emu_report_handler_o mt32emu_report_handler_o;

// Interface for handling reported events
typedef struct {
	// Callback for debug messages, in vprintf() format
	void (*printDebug)(const mt32emu_report_handler_o *instance, const char *fmt, va_list list);
	// Callbacks for reporting errors
	void (*onErrorControlROM)(const mt32emu_report_handler_o *instance);
	void (*onErrorPCMROM)(const mt32emu_report_handler_o *instance);
	// Callback for reporting about displaying a new custom message on LCD
	void (*showLCDMessage)(const mt32emu_report_handler_o *instance, const char *message);
	// Callback for reporting actual processing of a MIDI message
	void (*onMIDIMessagePlayed)(const mt32emu_report_handler_o *instance);
	// Callback for reporting an overflow of the input MIDI queue
	void (*onMIDIQueueOverflow)(const mt32emu_report_handler_o *instance);
	// Callback invoked when a System Realtime MIDI message is detected in functions
	// mt32emu_parse_stream and mt32emu_play_short_message and the likes.
	void(*onMIDISystemRealtime)(const mt32emu_report_handler_o *instance, mt32emu_bit8u systemRealtime);
	// Callbacks for reporting system events
	void (*onDeviceReset)(const mt32emu_report_handler_o *instance);
	void (*onDeviceReconfig)(const mt32emu_report_handler_o *instance);
	// Callbacks for reporting changes of reverb settings
	void (*onNewReverbMode)(const mt32emu_report_handler_o *instance, mt32emu_bit8u mode);
	void (*onNewReverbTime)(const mt32emu_report_handler_o *instance, mt32emu_bit8u time);
	void (*onNewReverbLevel)(const mt32emu_report_handler_o *instance, mt32emu_bit8u level);
	// Callbacks for reporting various information
	void (*onPolyStateChanged)(const mt32emu_report_handler_o *instance, int partNum);
	void (*onProgramChanged)(const mt32emu_report_handler_o *instance, int partNum, const char *soundGroupName, const char *patchName);
} mt32emu_report_handler_i;

// Instance of report handler object
struct mt32emu_report_handler_o {
	// Interface
	const mt32emu_report_handler_i *i;
	// User data for this instance
	void *d;
};

// Set of multiplexed output streams appeared at the DAC entrance.
typedef struct {
	mt32emu_sample *nonReverbLeft;
	mt32emu_sample *nonReverbRight;
	mt32emu_sample *reverbDryLeft;
	mt32emu_sample *reverbDryRight;
	mt32emu_sample *reverbWetLeft;
	mt32emu_sample *reverbWetRight;
} mt32emu_dac_output_streams;

#endif // #ifndef MT32EMU_C_TYPES_H
