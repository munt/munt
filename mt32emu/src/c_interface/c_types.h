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
#include <stddef.h>

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
	/* Operation completed normally. */
	MT32EMU_RC_OK = 0,
	MT32EMU_RC_ADDED_CONTROL_ROM = 1,
	MT32EMU_RC_ADDED_PCM_ROM = 2,

	/* Definite error occurred. */
	MT32EMU_RC_ROM_NOT_IDENTIFIED = -1,
	MT32EMU_RC_FILE_NOT_FOUND = -2,
	MT32EMU_RC_FILE_NOT_LOADED = -3,
	MT32EMU_RC_MISSING_ROMS = -4,
	MT32EMU_RC_NOT_OPENED = -5,
	MT32EMU_RC_QUEUE_FULL = -6,

	/* Undefined error occurred. */
	MT32EMU_RC_FAILED = -100
} mt32emu_return_code;

/** Emulation context */
typedef union mt32emu_context mt32emu_context;
typedef union mt32emu_const_context mt32emu_const_context;

/* Convenience aliases */
#ifndef __cplusplus
typedef enum mt32emu_analog_output_mode mt32emu_analog_output_mode;
typedef enum mt32emu_dac_input_mode mt32emu_dac_input_mode;
typedef enum mt32emu_midi_delay_mode mt32emu_midi_delay_mode;
typedef enum mt32emu_partial_state mt32emu_partial_state;
#endif

/** Set of multiplexed output streams appeared at the DAC entrance. */
typedef struct {
	mt32emu_sample *nonReverbLeft;
	mt32emu_sample *nonReverbRight;
	mt32emu_sample *reverbDryLeft;
	mt32emu_sample *reverbDryRight;
	mt32emu_sample *reverbWetLeft;
	mt32emu_sample *reverbWetRight;
} mt32emu_dac_output_streams;

/* === Interface handling === */

/** Report handler interface versions */
typedef enum {
	MT32EMU_REPORT_HANDLER_VERSION_0 = 0,
	MT32EMU_REPORT_HANDLER_VERSION_CURRENT = MT32EMU_REPORT_HANDLER_VERSION_0
} mt32emu_report_handler_version;

/** MIDI receiver interface versions */
typedef enum {
	MT32EMU_MIDI_RECEIVER_VERSION_0 = 0,
	MT32EMU_MIDI_RECEIVER_VERSION_CURRENT = MT32EMU_MIDI_RECEIVER_VERSION_0
} mt32emu_midi_receiver_version;

/** Synth interface versions */
typedef enum {
	MT32EMU_SYNTH_VERSION_0 = 0,
	MT32EMU_SYNTH_VERSION_CURRENT = MT32EMU_SYNTH_VERSION_0
} mt32emu_synth_version;

/** Intended for run-time interface type identification */
typedef union {
	mt32emu_report_handler_version rh;
	mt32emu_midi_receiver_version mr;
	mt32emu_synth_version s;
} mt32emu_interface_version;

/** Basic interface definition */
typedef struct {
	/** Returns run-time interface type identifier */
	mt32emu_interface_version (*getVersionID)();
} mt32emu_interface;

/* === Report Handler Interface === */

typedef union mt32emu_report_handler_i mt32emu_report_handler_i;

/** Interface for handling reported events (initial version) */
typedef struct {
	mt32emu_interface base;

	/** Callback for debug messages, in vprintf() format */
	void (*printDebug)(const mt32emu_report_handler_i *instance, const char *fmt, va_list list);
	/** Callbacks for reporting errors */
	void (*onErrorControlROM)(const mt32emu_report_handler_i *instance);
	void (*onErrorPCMROM)(const mt32emu_report_handler_i *instance);
	/** Callback for reporting about displaying a new custom message on LCD */
	void (*showLCDMessage)(const mt32emu_report_handler_i *instance, const char *message);
	/** Callback for reporting actual processing of a MIDI message */
	void (*onMIDIMessagePlayed)(const mt32emu_report_handler_i *instance);
	/** Callback for reporting an overflow of the input MIDI queue */
	void (*onMIDIQueueOverflow)(const mt32emu_report_handler_i *instance);
	/**
	 * Callback invoked when a System Realtime MIDI message is detected in functions
	 * mt32emu_parse_stream and mt32emu_play_short_message and the likes.
	 */
	void(*onMIDISystemRealtime)(const mt32emu_report_handler_i *instance, mt32emu_bit8u systemRealtime);
	/** Callbacks for reporting system events */
	void (*onDeviceReset)(const mt32emu_report_handler_i *instance);
	void (*onDeviceReconfig)(const mt32emu_report_handler_i *instance);
	/** Callbacks for reporting changes of reverb settings */
	void (*onNewReverbMode)(const mt32emu_report_handler_i *instance, mt32emu_bit8u mode);
	void (*onNewReverbTime)(const mt32emu_report_handler_i *instance, mt32emu_bit8u time);
	void (*onNewReverbLevel)(const mt32emu_report_handler_i *instance, mt32emu_bit8u level);
	/** Callbacks for reporting various information */
	void (*onPolyStateChanged)(const mt32emu_report_handler_i *instance, int partNum);
	void (*onProgramChanged)(const mt32emu_report_handler_i *instance, int partNum, const char *soundGroupName, const char *patchName);
} mt32emu_report_handler_i_v0;

/**
 * Extensible interface for handling reported events.
 * Union intended to view an interface of any subsequent version as any parent interface not requiring a cast.
 * Elements are to be addressed using the tag of the interface version when they were introduced.
 */
union mt32emu_report_handler_i {
	const mt32emu_interface *interface;
	const mt32emu_report_handler_i_v0 *v0;
};

/* === MIDI Receiver Interface === */

typedef union mt32emu_midi_receiver_i mt32emu_midi_receiver_i;

/** Interface for receiving MIDI messages generated by MIDI stream parser (initial version) */
typedef struct {
	mt32emu_interface base;

	/** Invoked when a complete short MIDI message is parsed in the input MIDI stream. */
	void (*handleShortMessage)(const mt32emu_midi_receiver_i *instance, const mt32emu_bit32u message);

	/** Invoked when a complete well-formed System Exclusive MIDI message is parsed in the input MIDI stream. */
	void (*handleSysex)(const mt32emu_midi_receiver_i *instance, const mt32emu_bit8u stream[], const mt32emu_bit32u length);

	/** Invoked when a System Realtime MIDI message is parsed in the input MIDI stream. */
	void (*handleSystemRealtimeMessage)(const mt32emu_midi_receiver_i *instance, const mt32emu_bit8u realtime);
} mt32emu_midi_receiver_i_v0;

/**
* Extensible interface for receiving MIDI messages.
* Union intended to view an interface of any subsequent version as any parent interface not requiring a cast.
* Elements are to be addressed using the tag of the interface version when they were introduced.
*/
union mt32emu_midi_receiver_i {
	const mt32emu_interface *interface;
	const mt32emu_midi_receiver_i_v0 *v0;
};

/* === Synth Interface === */

/**
 * Basic interface that defines all the library services (initial version).
 * The members closely resemble C functions declared in c_interface.h, and the intention is to provide for easier
 * access when the library is dynamically loaded in run-time, e.g. as a plugin. This way the client only needs
 * to bind to mt32emu_create_synth() factory function instead of binding to each function it needs to use.
 * See c_interface.h for parameter description.
 */
typedef struct {
	mt32emu_interface base;

	void (*freeSynth)(mt32emu_context context);
	mt32emu_return_code (*addROMData)(mt32emu_context context, const mt32emu_bit8u *data, size_t data_size, const mt32emu_sha1_digest *sha1_digest);
	mt32emu_return_code (*addROMFile)(mt32emu_context context, const char *filename);
	mt32emu_return_code (*openSynth)(mt32emu_const_context context, const unsigned int *partial_count, const mt32emu_analog_output_mode *analog_output_mode);
	void (*closeSynth)(mt32emu_const_context context);
	mt32emu_boolean (*isOpen)(mt32emu_const_context context);
	unsigned int (*getStereoOutputSamplerate)(mt32emu_const_context context, const mt32emu_analog_output_mode analog_output_mode);
	unsigned int (*getActualStereoOutputSamplerate)(mt32emu_const_context context);
	void (*flushMIDIQueue)(mt32emu_const_context context);
	mt32emu_bit32u (*setMIDIEventQueueSize)(mt32emu_const_context context, const mt32emu_bit32u queue_size);
	mt32emu_midi_receiver_version (*setMIDIReceiver)(mt32emu_const_context context, const mt32emu_midi_receiver_i *midi_receiver);

	void (*parseStream)(mt32emu_const_context context, const mt32emu_bit8u *stream, mt32emu_bit32u length);
	void (*parseStream_At)(mt32emu_const_context context, const mt32emu_bit8u *stream, mt32emu_bit32u length, mt32emu_bit32u timestamp);
	void (*playShortMessage)(mt32emu_const_context context, mt32emu_bit32u message);
	void (*playShortMessageAt)(mt32emu_const_context context, mt32emu_bit32u message, mt32emu_bit32u timestamp);
	mt32emu_return_code (*playMsg)(mt32emu_const_context context, mt32emu_bit32u msg);
	mt32emu_return_code (*playSysex)(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len);
	mt32emu_return_code (*playMsgAt)(mt32emu_const_context context, mt32emu_bit32u msg, mt32emu_bit32u timestamp);
	mt32emu_return_code (*playSysexAt)(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len, mt32emu_bit32u timestamp);

	void (*playMsgNow)(mt32emu_const_context context, mt32emu_bit32u msg);
	void (*playMsgOnPart)(mt32emu_const_context context, unsigned char part, unsigned char code, unsigned char note, unsigned char velocity);
	void (*playSysexNow)(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len);
	void (*writeSysex)(mt32emu_const_context context, unsigned char channel, const mt32emu_bit8u *sysex, mt32emu_bit32u len);

	void (*setReverbEnabled)(mt32emu_const_context context, const mt32emu_boolean reverb_enabled);
	mt32emu_boolean (*isReverbEnabled)(mt32emu_const_context context);
	void (*setReverbOverridden)(mt32emu_const_context context, const mt32emu_boolean reverbOverridden);
	mt32emu_boolean (*isReverbOverridden)(mt32emu_const_context context);
	void (*setReverbCompatibilityMode)(mt32emu_const_context context, const mt32emu_boolean mt32_compatible_mode);
	mt32emu_boolean (*isMT32ReverbCompatibilityMode)(mt32emu_const_context context);
	mt32emu_boolean (*isDefaultReverbMT32Compatible)(mt32emu_const_context context);

	void (*setDACInputMode)(mt32emu_const_context context, const mt32emu_dac_input_mode mode);
	mt32emu_dac_input_mode (*getDACInputMode)(mt32emu_const_context context);

	void (*setMIDIDelayMode)(mt32emu_const_context context, const mt32emu_midi_delay_mode mode);
	mt32emu_midi_delay_mode (*getMIDIDelayMode)(mt32emu_const_context context);

	void (*setOutputGain)(mt32emu_const_context context, float gain);
	float (*getOutputGain)(mt32emu_const_context context);
	void (*setReverbOutputGain)(mt32emu_const_context context, float gain);
	float (*getReverbOutputGain)(mt32emu_const_context context);

	void (*setReversedStereoEnabled)(mt32emu_const_context context, const mt32emu_boolean enabled);
	mt32emu_boolean (*isReversedStereoEnabled)(mt32emu_const_context context);

	void (*render)(mt32emu_const_context context, mt32emu_sample *stream, mt32emu_bit32u len);
	void (*renderStreams)(mt32emu_const_context context, const mt32emu_dac_output_streams *streams, mt32emu_bit32u len);

	mt32emu_boolean (*hasActivePartials)(mt32emu_const_context context);
	mt32emu_boolean (*isActive)(mt32emu_const_context context);
	unsigned int (*getPartialCount)(mt32emu_const_context context);
	mt32emu_bit32u (*getPartStates)(mt32emu_const_context context);
	void (*getPartialStates)(mt32emu_const_context context, mt32emu_bit8u *partial_states);
	unsigned int (*getPlayingNotes)(mt32emu_const_context context, unsigned int part_number, mt32emu_bit8u *keys, mt32emu_bit8u *velocities);
	const char *(*getPatchName)(mt32emu_const_context context, unsigned int part_number);
	void (*readMemory)(mt32emu_const_context context, mt32emu_bit32u addr, mt32emu_bit32u len, mt32emu_bit8u *data);
	mt32emu_report_handler_version (*getSupportedReportHandlerVersionID)(mt32emu_const_context context);
} mt32emu_synth_i_v0;

/**
* Extensible interface for receiving MIDI messages.
* Union intended to view an interface of any subsequent version as any parent interface not requiring a cast.
* Elements are to be addressed using the tag of the interface version when they were introduced.
*/
typedef union {
	const mt32emu_interface *interface;
	const mt32emu_synth_i_v0 *v0;
} mt32emu_synth_i;

union mt32emu_context {
	const mt32emu_synth_i *i;
	struct mt32emu_data *d;
};

union mt32emu_const_context {
	const mt32emu_synth_i *i;
	const struct mt32emu_data *c;
};

#endif /* #ifndef MT32EMU_C_TYPES_H */
