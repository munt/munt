/*
 * Copyright (C) 2009, 2011 Jerome Fisher
 * Copyright (C) 2012-2022 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <cerrno>
#include <cfloat>
#include <climits>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <glib.h>

#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>

#if GLIB_MAJOR_VERSION != 2 || GLIB_MINOR_VERSION < 32
#error Incompatible glib2 library version
#endif

#if !MT32EMU_IS_COMPATIBLE(2, 7)
#error Incompatible mt32emu library version
#endif

#if defined FLT_RADIX && FLT_RADIX != 2
#error Binary floating point support required
#endif

#include "smf.h"

static const int DEFAULT_BUFFER_SIZE = 128 * 1024;

// Maximum number of frames to render in each pass while waiting for reverb to become inactive.
static const unsigned int MAX_REVERB_END_FRAMES = 8192;

static const int HEADEROFFS_RIFFLEN = 4;
static const int HEADEROFFS_FORMAT_TAG = 20;
static const int HEADEROFFS_SAMPLERATE = 24;
static const int HEADEROFFS_BYTERATE = 28;
static const int HEADEROFFS_BLOCK_ALIGN = 32;
static const int HEADEROFFS_BIT_DEPTH = 34;
static const int HEADEROFFS_DATALEN = 40;

enum OUTPUT_SAMPLE_FORMAT {
	OUTPUT_SAMPLE_FORMAT_SINT16 = 0,
	OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32 = 1
};

static const MT32Emu::DACInputMode DAC_INPUT_MODES[] = {
	MT32Emu::DACInputMode_NICE,
	MT32Emu::DACInputMode_PURE,
	MT32Emu::DACInputMode_GENERATION1,
	MT32Emu::DACInputMode_GENERATION2
};

static const MT32Emu::AnalogOutputMode ANALOG_OUTPUT_MODES[] = {
	MT32Emu::AnalogOutputMode_DIGITAL_ONLY,
	MT32Emu::AnalogOutputMode_COARSE,
	MT32Emu::AnalogOutputMode_ACCURATE,
	MT32Emu::AnalogOutputMode_OVERSAMPLED
};

static const MT32Emu::RendererType RENDERER_TYPES[] = {
	MT32Emu::RendererType_BIT16S,
	MT32Emu::RendererType_FLOAT
};

static const MT32Emu::SamplerateConversionQuality SRC_QUALITIES[] = {
	MT32Emu::SamplerateConversionQuality_FASTEST,
	MT32Emu::SamplerateConversionQuality_FAST,
	MT32Emu::SamplerateConversionQuality_GOOD,
	MT32Emu::SamplerateConversionQuality_BEST
};

struct Options {
	gchar **inputFilenames;
	gchar *outputFilename;
	gboolean force;
	gboolean quiet;

	gchar *romDir;
	gchar *machineID;
	unsigned int bufferFrameCount;
	gint sampleRate;
	OUTPUT_SAMPLE_FORMAT outputSampleFormat;

	MT32Emu::DACInputMode dacInputMode;
	MT32Emu::AnalogOutputMode analogOutputMode;
	MT32Emu::RendererType rendererType;
	MT32Emu::SamplerateConversionQuality srcQuality;
	int partialCount;
	int rawChannelMap[8];
	int rawChannelCount;

	unsigned int renderMinFrames;
	unsigned int renderMaxFrames;
	gint recordMaxStartSilentFrames;
	gint recordMaxEndSilentFrames;
	gint recordMaxLA32EndSilentFrames;
	gboolean waitForLA32;
	gboolean waitForReverb;
	gboolean sendAllNotesOff;
	gboolean niceAmpRamp;
	gboolean nicePanning;
	gboolean nicePartialMixing;
};

struct State {
	void *stereoSampleBuffer;
	void *rawSampleBuffer[6];
	MT32Emu::Service &service;
	FILE *outputFile;
	bool lastInputFile;
	bool firstNoiseEncountered;
	unsigned long unwrittenSilentFrames;
	unsigned long renderedFrames;
	unsigned long writtenFrames;
};

static void freeOptions(Options *options) {
	g_strfreev(options->inputFilenames);
	options->inputFilenames = NULL;
	g_free(options->outputFilename);
	options->outputFilename = NULL;
	g_free(options->machineID);
	options->machineID = NULL;
	g_free(options->romDir);
	options->romDir = NULL;
}

static bool parseOptions(int argc, char *argv[], Options *options) {
	gint dacInputModeIx = 0;
	gint analogOutputModeIx = 0;
	gint rendererTypeIx = 0;
	gint srcQualityIx = 2;
	gint partialCount = MT32Emu::DEFAULT_MAX_PARTIALS;
	gint outputSampleFormat = OUTPUT_SAMPLE_FORMAT_SINT16;
	gint bufferFrameCount = DEFAULT_BUFFER_SIZE;
	gint renderMinFrames = 0;
	gint renderMaxFrames = -1;
	gchar **rawStreams = NULL;
	gchar *deprecatedSysexFile = NULL;
	options->inputFilenames = NULL;
	options->outputFilename = NULL;
	options->force = false;
	options->quiet = false;

	options->romDir = NULL;
	options->machineID = NULL;

	options->dacInputMode = DAC_INPUT_MODES[0];
	options->analogOutputMode = ANALOG_OUTPUT_MODES[0];
	options->rendererType = RENDERER_TYPES[0];
	options->srcQuality = SRC_QUALITIES[2];
	options->sampleRate = 0;
	options->rawChannelCount = 0;
	options->outputSampleFormat = OUTPUT_SAMPLE_FORMAT_SINT16;

	options->recordMaxStartSilentFrames = 0;
	options->recordMaxEndSilentFrames = 0;
	options->recordMaxLA32EndSilentFrames = 0;
	options->waitForLA32 = true;
	options->waitForReverb = true;
	options->sendAllNotesOff = true;
	options->niceAmpRamp = true;
	options->nicePanning = false;
	options->nicePartialMixing = false;
	// FIXME: Perhaps there's a nicer way to represent long argument descriptions...
	GOptionEntry entries[] = {
		{"output", 'o', 0, G_OPTION_ARG_FILENAME, &options->outputFilename, "Output file (default: last source file name with \".wav\" appended)", "<filename>"},
		{"force", 'f', 0, G_OPTION_ARG_NONE, &options->force, "Overwrite the output file if it already exists", NULL},
		{"quiet", 'q', 0, G_OPTION_ARG_NONE, &options->quiet, "Be quiet", NULL},

		{"rom-dir", 'm', 0, G_OPTION_ARG_STRING, &options->romDir, "Directory in which ROMs are stored", "<directory>"},
		{"machine-id", 'i', 0, G_OPTION_ARG_STRING, &options->machineID, "ID of machine configuration to search ROMs for (default: any)\n"
		 "                 any:        try cm32l first, then mt32\n"
		 "                 mt32:       any complete set of MT-32 ROMs, the highest control ROM version found wins\n"
		 "                 cm32l:      any complete set of CM-32L / LAPC-I compatible ROMs, the highest control ROM version found wins\n"
		 "                 mt32_1_04:  MT-32 with control ROM version 1.04\n"
		 "                 mt32_1_05:  MT-32 with control ROM version 1.05\n"
		 "                 mt32_1_06:  MT-32 with control ROM version 1.06\n"
		 "                 mt32_1_07:  MT-32 with control ROM version 1.07\n"
		 "                 mt32_bluer: MT-32 Blue Ridge mod version X.XX\n"
		 "                 mt32_2_03:  MT-32 with control ROM version 2.03\n"
		 "                 mt32_2_04:  MT-32 with control ROM version 2.04\n"
		 "                 mt32_2_06:  MT-32 with control ROM version 2.06\n"
		 "                 mt32_2_07:  MT-32 with control ROM version 2.07\n"
		 "                 cm32l_1_00: CM-32L / LAPC-I with control ROM version 1.00\n"
		 "                 cm32l_1_02: CM-32L / LAPC-I with control ROM version 1.02\n"
		 "                 cm32ln_1_00: CM-32LN / CM-500 / LAPC-N with control ROM version 1.00", "<machine_id>"},
		// buffer-size determines the maximum number of frames to be rendered by the emulator in one pass.
		// This can have a big impact on performance (Generally more at a time=better).
		{"buffer-size", 'b', 0, G_OPTION_ARG_INT, &bufferFrameCount, "Buffer size in frames (minimum: 1)", "<frame_count>"},  // FIXME: Show default
		{"sample-rate", 'p', 0, G_OPTION_ARG_INT, &options->sampleRate, "Sample rate in Hz (minimum: 1, default: auto)\n"
		 "                Ignored if -w is used (in which case auto is always used)\n", "<sample_rate>"},

		{"src-quality", 'q', 0, G_OPTION_ARG_INT, &srcQualityIx, "Sample rate conversion quality (default: 2)\n"
		 "                 0: FASTEST\n"
		 "                 1: FAST\n"
		 "                 2: GOOD\n"
		 "                 3: BEST", "<src_quality>"},

		{"max-partials", 'x', 0, G_OPTION_ARG_INT, &partialCount, "The maximum number of partials playing simultaneously.\n"
		 "                (minimum: 8, default: 32)\n", "<max-partials>"},

		{"analog-output-mode", 'a', 0, G_OPTION_ARG_INT, &analogOutputModeIx, "Analogue low-pass filter emulation mode (default: 0)\n"
		 "                Ignored if -w is used (in which case 0/DISABLED is always used)\n"
		 "                 0: DISABLED\n"
		 "                 1: COARSE\n"
		 "                 2: ACCURATE\n"
		 "                 3: OVERSAMPLED", "<analog_output_mode>"},

		{"renderer-type", 'r', 0, G_OPTION_ARG_INT, &rendererTypeIx, "Type of samples to use in renderer and wave generator (default: 0)\n"
		 "                 0: Integer 16-bit\n"
		 "                 1: Float 32-bit\n", "<renderer_type>"},

		{"output-sample-format", 0, 0, G_OPTION_ARG_INT, &outputSampleFormat, "Format of output samples (default: 0)\n"
		"                 0: Signed Integer 16-bit\n"
		"                 1: IEEE 754 Float 32-bit\n", "<output_sample_format>"},

		{"dac-input-mode", 'd', 0, G_OPTION_ARG_INT, &dacInputModeIx, "LA-32 to DAC input mode (default: 0)\n"
		 "                Ignored if -w is used (in which case 1/PURE is always used)\n"
		 "                 0: NICE\n"
		 "                 1: PURE\n"
		 "                 2: GENERATION1\n"
		 "                 3: GENERATION2", "<dac_input_mode>"},
		{"raw-stream", 'w', 0, G_OPTION_ARG_STRING_ARRAY, &rawStreams, "Write a raw file with signed 16-bit big-endian samples instead of a WAVE file, and include the specified channel.\n"
		 "                This option can be specified multiple times (up to eight), in which case streams will be written to the file multiplexed sample-by-sample in the order given.\n"
		 "                Available stream IDs:\n"
		 "                -1: Dummy stream filled with 0\n"
		 "                 0: [LA32] Left non-reverb\n"
		 "                 1: [LA32] Right non-reverb\n"
		 "                 2: [LA32] Left reverb dry\n"
		 "                 3: [LA32] Right reverb dry\n"
		 "                 4: [Reverb] Left reverb wet\n"
		 "                 5: [Reverb] Right reverb wet", "<stream_id>"},

		{"render-min", 0, 0, G_OPTION_ARG_INT, &renderMinFrames, "Render at least this many frames (default: 0) (NYI)", "<frame_count>"},
		{"render-max", 'e', 0, G_OPTION_ARG_INT, &renderMaxFrames, "Render at most this many frames (default: -1)", "<frame_count>|-1 (unlimited)"},
		{"record-max-start-silence", 0, 0, G_OPTION_ARG_INT, &options->recordMaxStartSilentFrames, "Record at most this many silent frames at the start of each SMF file (default: 0)", "<frame_count>|-1 (unlimited)"},
		{"record-max-end-silence", 0, 0, G_OPTION_ARG_INT, &options->recordMaxEndSilentFrames, "Record at most this many silent frames at the end of each SMF file (default: 0)", "<frame_count>|-1 (unlimited)"},
		{"record-max-la32-end-silence", 0, 0, G_OPTION_ARG_INT, &options->recordMaxLA32EndSilentFrames, "Record at most this many silent frames produced by the emulated LA32 before it becomes inactive (default: 0)", "<frame_count>|-1 (unlimited)"},
		{"no-wait-for-la32", 't', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options->waitForLA32, "Don't wait for all partials to become inactive after each SMF file has ended. Implies no-wait-for-reverb.", NULL},
		{"no-wait-for-reverb", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options->waitForReverb, "Don't wait for reverb to finish after each SMF file has ended.", NULL},
		{"no-send-all-notes-off", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options->sendAllNotesOff, "Don't release the hold pedal and perform all-notes-off on all parts in the emulator at the end of each SMF file.\n"
		 "                WARNING: Sound can theoretically continue forever if not limited by other options.", NULL},
		{"no-nice-amp-ramp", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &options->niceAmpRamp, "Emulate amplitude ramp accurately.\n"
		 "                Quick changes of volume or expression on a MIDI channel may result in amp jumps which are avoided by default.", NULL},
		{"nice-panning", 0, 0, G_OPTION_ARG_NONE, &options->nicePanning, "Enlarges the pan setting accuracy to 4 bits providing for smoother panning.\n"
		 "                By default, we accurately emulate the LA32 chip that only supports the pan setting accuracy of 3 bits.", NULL},
		{"nice-partial-mixing", 0, 0, G_OPTION_ARG_NONE, &options->nicePartialMixing, "Disables occasional counter-phase mixing of partials that the LA32 chip exhibits.\n"
		 "                Timbres with closely sounding partials may sound quite differently, or even cancel out if mixed counter-phase.\n"
		 "                Enabling this option makes the behaviour more predictable.", NULL},

		{"s", 's', 0, G_OPTION_ARG_FILENAME, &deprecatedSysexFile, "[DEPRECATED] Play this SMF or sysex file before any other. DEPRECATED: Instead just specify the file first in the file list.", "<midi_file>"},
		{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &options->inputFilenames, NULL, "<midi_file> [midi_file...]"},
		{NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL}
	};
	GOptionContext *context = g_option_context_new("- record MIDI played through the MT-32 emulator to a file");
	g_option_context_add_main_entries(context, entries, NULL);
	GError *error = NULL;
	bool parseSuccess = g_option_context_parse(context, &argc, &argv, &error) != 0;
	if (!parseSuccess) {
		fprintf(stderr, "Option parsing failed: %s\n", error->message);
	}
	if (analogOutputModeIx < 0 || analogOutputModeIx > 3) {
		fprintf(stderr, "analog-output-mode must be between 0 and 3\n");
		parseSuccess = false;
	}
	if (rendererTypeIx < 0 || rendererTypeIx > 1) {
		fprintf(stderr, "renderer-type must be either 0 or 1\n");
		parseSuccess = false;
	}
	if (outputSampleFormat < OUTPUT_SAMPLE_FORMAT_SINT16 || outputSampleFormat > OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
		fprintf(stderr, "output-sample-format must be either 0 or 1\n");
		parseSuccess = false;
	}
	if (srcQualityIx < 0 || srcQualityIx > 3) {
		fprintf(stderr, "src-quality must be between 0 and 3\n");
		parseSuccess = false;
	}
	if (dacInputModeIx < 0 || dacInputModeIx > 3) {
		fprintf(stderr, "dac-input-mode must be between 0 and 3\n");
		parseSuccess = false;
	}
	if (bufferFrameCount < 1) {
		fprintf(stderr, "buffer-size must be greater than 0\n");
		parseSuccess = false;
	} else {
		options->bufferFrameCount = bufferFrameCount;
	}
	options->partialCount = partialCount < 8 ? 8 : partialCount;
	options->renderMaxFrames = renderMaxFrames < 0 ? INT_MAX : renderMaxFrames;
	options->renderMinFrames = renderMinFrames < 0 ? 0 : renderMinFrames;
	if (options->renderMinFrames > options->renderMaxFrames) {
		options->renderMinFrames = options->renderMaxFrames;
	}
	if (options->recordMaxStartSilentFrames < 0) {
		options->recordMaxStartSilentFrames = INT_MAX;
	}
	if (options->recordMaxEndSilentFrames < 0) {
		options->recordMaxEndSilentFrames = INT_MAX;
	}
	if (options->recordMaxLA32EndSilentFrames < 0) {
		options->recordMaxLA32EndSilentFrames = INT_MAX;
	}
	if (rawStreams != NULL && g_strv_length(rawStreams) > 0) {
		gchar **rawStream = rawStreams;
		while(*rawStream != NULL) {
			if (options->rawChannelCount == 8) {
				fprintf(stderr, "Too many raw-stream options - maximum 8\n");
				parseSuccess = false;
				break;
			}
			options->rawChannelMap[options->rawChannelCount] = atoi(*rawStream);
			if (options->rawChannelMap[options->rawChannelCount] < -1 || options->rawChannelMap[options->rawChannelCount] > 5) {
				fprintf(stderr, "Invalid option raw-stream option %s - must be a number between -1 and 5 (inclusive)\n", *rawStream);
				parseSuccess = false;
				break;
			}
			options->rawChannelCount++;
			rawStream++;
		}
	}
	if (deprecatedSysexFile != NULL) {
		guint oldLength = options->inputFilenames == NULL ? 0 : g_strv_length(options->inputFilenames);
		gchar **newInputFilenames = g_new(gchar *, oldLength + 2);
		newInputFilenames[0] = deprecatedSysexFile;
		newInputFilenames[oldLength + 1] = NULL;
		for (guint i = 0; i < oldLength; i++) {
			newInputFilenames[i + 1] = options->inputFilenames[i];
		}
		g_free(options->inputFilenames);
		options->inputFilenames = newInputFilenames;
	}
	if (options->inputFilenames == NULL || g_strv_length(options->inputFilenames) == 0) {
		fprintf(stderr, "No input files specified\n");
		parseSuccess = false;
	}
	options->analogOutputMode = ANALOG_OUTPUT_MODES[analogOutputModeIx];
	options->rendererType = RENDERER_TYPES[rendererTypeIx];
	options->outputSampleFormat = static_cast<OUTPUT_SAMPLE_FORMAT>(outputSampleFormat);
	options->srcQuality = SRC_QUALITIES[srcQualityIx];
	g_strfreev(rawStreams);
	if (options->rawChannelCount > 0) {
		options->dacInputMode = MT32Emu::DACInputMode_PURE;
		options->analogOutputMode = MT32Emu::AnalogOutputMode_DIGITAL_ONLY;
		options->sampleRate = 0;
	} else {
		options->dacInputMode = DAC_INPUT_MODES[dacInputModeIx];
	}
	if (!parseSuccess) {
		gchar *help = g_option_context_get_help(context, TRUE, NULL);
		fputs(help, stderr);
		freeOptions(options);
	}
	g_option_context_free(context);
	return parseSuccess;
}

static long secondsToSamples(double seconds, int sampleRate) {
	return long(seconds * sampleRate);
}

static bool writeWAVEHeader(FILE *outputFile, int sampleRate, OUTPUT_SAMPLE_FORMAT outputSampleFormat) {
	const int channelCount = 2;
	const int bitDepth = outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32 ? 32 : 16;
	const int frameSize = channelCount * (bitDepth / 8);
	const int byteRate = sampleRate * frameSize;

	// All values are little-endian
	unsigned char waveHeader[] = {
		'R','I','F','F',
		0,0,0,0, // Length to be filled in later
		'W','A','V','E',

		// "fmt " chunk
		'f','m','t',' ',
		0x10, 0x00, 0x00, 0x00, // 0x00000010 - 16 byte chunk
		0x01, 0x00, // 0x0001 - PCM/Uncompressed; 0x0003 - WAVE_FORMAT_IEEE_FLOAT
		0x02, 0x00, // 0x0002 - 2 channels
		0x00, 0x7D, 0x00, 0x00, // 0x00007D00 - 32kHz, overwritten by real sample rate below
		0x00, 0xF4, 0x01, 0x00, // 0x0001F400 - 128000 bytes/sec, overwritten with real value below
		0x04, 0x00, // 0x0004 - 4 byte alignment
		0x10, 0x00, // 0x0010 - 16 bits/sample

		// "data" chunk
		'd','a','t','a',
		0x00, 0x00, 0x00, 0x00 // Chunk length, to be filled in later
	};
	waveHeader[HEADEROFFS_FORMAT_TAG] = outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32 ? 3 : 1;
	waveHeader[HEADEROFFS_SAMPLERATE] = sampleRate & 0xFF;
	waveHeader[HEADEROFFS_SAMPLERATE + 1] = (sampleRate >> 8) & 0xFF;
	waveHeader[HEADEROFFS_SAMPLERATE + 2] = (sampleRate >> 16) & 0xFF;
	waveHeader[HEADEROFFS_SAMPLERATE + 3] = (sampleRate >> 24) & 0xFF;
	waveHeader[HEADEROFFS_BYTERATE] = byteRate & 0xFF;
	waveHeader[HEADEROFFS_BYTERATE + 1] = (byteRate >> 8) & 0xFF;
	waveHeader[HEADEROFFS_BYTERATE + 2] = (byteRate >> 16) & 0xFF;
	waveHeader[HEADEROFFS_BYTERATE + 3] = (byteRate >> 24) & 0xFF;
	waveHeader[HEADEROFFS_BLOCK_ALIGN] = frameSize & 0xFF;
	waveHeader[HEADEROFFS_BLOCK_ALIGN + 1] = (frameSize >> 8) & 0xFF;
	waveHeader[HEADEROFFS_BIT_DEPTH] = bitDepth & 0xFF;
	waveHeader[HEADEROFFS_BIT_DEPTH + 1] = (bitDepth >> 8) & 0xFF;
	return fwrite(waveHeader, 1, sizeof(waveHeader), outputFile) == sizeof(waveHeader);
}

static bool fillWAVESizes(FILE *outputFile, int numFrames, OUTPUT_SAMPLE_FORMAT outputSampleFormat) {
	// FIXME: Check return codes, etc.
	const int frameSize = outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32 ? 8 : 4;
	int dataSize = numFrames * frameSize;
	int riffSize = dataSize + 28;
	if (fseek(outputFile, HEADEROFFS_RIFFLEN, SEEK_SET))
		return false;
	if (fputc(riffSize & 0xFF, outputFile) == EOF)
		return false;
	if (fputc((riffSize >> 8) & 0xFF, outputFile) == EOF)
		return false;
	if (fputc((riffSize >> 16) & 0xFF, outputFile) == EOF)
		return false;
	if (fputc((riffSize >> 24) & 0xFF, outputFile) == EOF)
		return false;
	if (fseek(outputFile, HEADEROFFS_DATALEN, SEEK_SET))
		return false;
	if (fputc(dataSize & 0xFF, outputFile) == EOF)
		return false;
	if (fputc((dataSize >> 8) & 0xFF, outputFile) == EOF)
		return false;
	if (fputc((dataSize >> 16) & 0xFF, outputFile) == EOF)
		return false;
	if (fputc((dataSize >> 24) & 0xFF, outputFile) == EOF)
		return false;
	return true;
}

static bool loadFile(MT32Emu::Bit8u *&fileBuffer, gsize &fileBufferLength, const gchar *filename, const gchar *displayFilename) {
	GError *err = NULL;
	g_file_get_contents(filename, (gchar **)&fileBuffer, &fileBufferLength, &err);
	if (err != NULL) {
		fprintf(stderr, "Error reading file '%s': %s\n", displayFilename, err->message);
		g_error_free(err);
		return false;
	}
	return true;
}

static bool playSysexFileBuffer(MT32Emu::Service &service, const gchar *displayFilename, MT32Emu::Bit8u *fileBuffer, gsize fileBufferLength) {
	long start = -1;
	for (gsize i = 0; i < fileBufferLength; i++) {
		if (fileBuffer[i] == 0xF0) {
			if (start != -1) {
				fprintf(stderr, "Started a new sysex message before the last finished - sysex file '%s' may be in an unsupported format.\n", displayFilename);
			}
			start = long(i);
		}
		else if (fileBuffer[i] == 0xF7) {
			if (start == -1) {
				fprintf(stderr, "Ended a sysex message without a start byte - sysex file '%s' may be in an unsupported format.\n", displayFilename);
			} else {
				service.playSysexNow((MT32Emu::Bit8u*)(fileBuffer + start), MT32Emu::Bit32u(i - start + 1));
			}
			start = -1;
		}
	}
	return true;
}

enum Occasion {
	NOISE_DETECTED,
	MIDI_ENDED,
	LA32_INACTIVE
};

static void flushSilence(Occasion occasion, const Options &options, State &state) {
	unsigned int channelCount = options.rawChannelCount > 0 ? options.rawChannelCount : 2;
	int writtenFrames = state.unwrittenSilentFrames;
	switch(occasion) {
	case NOISE_DETECTED:
		if (!state.firstNoiseEncountered) {
			state.firstNoiseEncountered = true;
			writtenFrames = MIN(writtenFrames, options.recordMaxStartSilentFrames);
		}
		state.unwrittenSilentFrames = 0;
		break;
	case MIDI_ENDED:
		writtenFrames = MIN(writtenFrames, options.recordMaxEndSilentFrames);
		state.unwrittenSilentFrames -= writtenFrames;
		break;
	case LA32_INACTIVE:
		writtenFrames = MIN(writtenFrames, options.recordMaxLA32EndSilentFrames);
		state.unwrittenSilentFrames -= writtenFrames;
		break;
	}
	const int sampleSize = options.outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32 ? 4 : 2;
	for (unsigned long i = 0; i < writtenFrames * sampleSize * channelCount; i++) {
		fputc(0, state.outputFile);
	}
	state.writtenFrames += writtenFrames;
}

static inline void renderStereo(MT32Emu::Service &service, void *stereoSampleBuffer, const unsigned int frameCount, const OUTPUT_SAMPLE_FORMAT outputSampleFormat) {
	if (outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
		service.renderFloat(static_cast<float *>(stereoSampleBuffer), frameCount);
	} else {
		service.renderBit16s(static_cast<MT32Emu::Bit16s *>(stereoSampleBuffer), frameCount);
	}
}

static inline void renderRaw(MT32Emu::Service &service, void *rawSampleBuffer[], const unsigned int frameCount, const OUTPUT_SAMPLE_FORMAT outputSampleFormat) {
	if (outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
		mt32emu_dac_output_float_streams streams = {
			static_cast<float *>(rawSampleBuffer[0]),
			static_cast<float *>(rawSampleBuffer[1]),
			static_cast<float *>(rawSampleBuffer[2]),
			static_cast<float *>(rawSampleBuffer[3]),
			static_cast<float *>(rawSampleBuffer[4]),
			static_cast<float *>(rawSampleBuffer[5])
		};
		service.renderFloatStreams(&streams, frameCount);
	} else {
		mt32emu_dac_output_bit16s_streams streams = {
			static_cast<MT32Emu::Bit16s *>(rawSampleBuffer[0]),
			static_cast<MT32Emu::Bit16s *>(rawSampleBuffer[1]),
			static_cast<MT32Emu::Bit16s *>(rawSampleBuffer[2]),
			static_cast<MT32Emu::Bit16s *>(rawSampleBuffer[3]),
			static_cast<MT32Emu::Bit16s *>(rawSampleBuffer[4]),
			static_cast<MT32Emu::Bit16s *>(rawSampleBuffer[5])
		};
		service.renderBit16sStreams(&streams, frameCount);
	}
}

static inline bool isSilence(void * const sampleBuffer, const int sampleIx, const OUTPUT_SAMPLE_FORMAT outputSampleFormat) {
	if (outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
		return static_cast<float *>(sampleBuffer)[sampleIx] == 0;
	} else {
		return static_cast<MT32Emu::Bit16s *>(sampleBuffer)[sampleIx] == 0;
	}
}

static inline MT32Emu::Bit32u makeIeeeFloat(float sample) {
	MT32Emu::Bit32u floatBits = 0;
	// In this context, all the denormals, INFs and NaNs are treated as silence.
	if (sample == sample && sample != 0 && fabs(sample) <= FLT_MAX) {
		int exp;
		float m = frexp(sample, &exp);
		// By the standard, the exp bias is 127, but since the mantissa is in the range [0.5..1], the correct exp bias is one less.
		exp += 127 - 1;
		float absm = fabs(m);
		if ((0.5 <= absm) && (0 < exp) && (exp < 255)) {
			int sgn = m < 0 ? 1 : 0;
			int intm = int(absm * (1 << 24)) & ((1 << 23) - 1);
			floatBits = (sgn << 31) | (exp << 23) | (intm);
		}
	}
	return floatBits;
}

static inline void putSampleLE(void * const sampleBuffer, const int sampleIx, FILE *outputFile, const OUTPUT_SAMPLE_FORMAT outputSampleFormat) {
	if (outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
		MT32Emu::Bit32u sample = makeIeeeFloat(static_cast<float *>(sampleBuffer)[sampleIx]);
		fputc(sample & 0xFF, outputFile);
		fputc((sample >> 8) & 0xFF, outputFile);
		fputc((sample >> 16) & 0xFF, outputFile);
		fputc((sample >> 24) & 0xFF, outputFile);
	} else {
		MT32Emu::Bit16s sample = static_cast<MT32Emu::Bit16s *>(sampleBuffer)[sampleIx];
		fputc(sample & 0xFF, outputFile);
		fputc((sample >> 8) & 0xFF, outputFile);
	}
}

static inline void putSampleBE(void * const sampleBuffer, const int sampleIx, FILE *outputFile, const OUTPUT_SAMPLE_FORMAT outputSampleFormat) {
	if (outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
		MT32Emu::Bit32u sample = makeIeeeFloat(static_cast<float *>(sampleBuffer)[sampleIx]);
		fputc((sample >> 24) & 0xFF, outputFile);
		fputc((sample >> 16) & 0xFF, outputFile);
		fputc((sample >> 8) & 0xFF, outputFile);
		fputc(sample & 0xFF, outputFile);
	} else {
		MT32Emu::Bit16s sample = static_cast<MT32Emu::Bit16s *>(sampleBuffer)[sampleIx];
		fputc((sample >> 8) & 0xFF, outputFile);
		fputc(sample & 0xFF, outputFile);
	}
}

static void renderStereo(unsigned int frameCount, const Options &options, State &state) {
	state.renderedFrames += frameCount;
	while (frameCount > 0) {
		unsigned int renderedFramesThisPass = MIN(frameCount, options.bufferFrameCount);
		renderStereo(state.service, state.stereoSampleBuffer, renderedFramesThisPass, options.outputSampleFormat);
		for (unsigned int i = 0; i < renderedFramesThisPass; i++) {
			unsigned int leftIx = i * 2;
			unsigned int rightIx = leftIx + 1;
			bool silent = isSilence(state.stereoSampleBuffer, leftIx, options.outputSampleFormat)
				&& isSilence(state.stereoSampleBuffer, rightIx, options.outputSampleFormat);
			if (silent) {
				state.unwrittenSilentFrames++;
				continue;
			}
			flushSilence(NOISE_DETECTED, options, state);
			putSampleLE(state.stereoSampleBuffer, leftIx, state.outputFile, options.outputSampleFormat);
			putSampleLE(state.stereoSampleBuffer, rightIx, state.outputFile, options.outputSampleFormat);
			state.writtenFrames++;
		}
		frameCount -= renderedFramesThisPass;
	}
}

static void renderRaw(unsigned int frameCount, const Options &options, State &state) {
	state.renderedFrames += frameCount;
	while (frameCount > 0) {
		unsigned int renderedFramesThisPass = MIN(frameCount, options.bufferFrameCount);
		renderRaw(state.service, state.rawSampleBuffer, renderedFramesThisPass, options.outputSampleFormat);
		for (unsigned int i = 0; i < renderedFramesThisPass; i++) {
			bool allSilent = true;
			for (int chanMapIx = 0; chanMapIx < options.rawChannelCount; chanMapIx++) {
				if (options.rawChannelMap[chanMapIx] >= 0 && !isSilence(state.rawSampleBuffer[options.rawChannelMap[chanMapIx]], i, options.outputSampleFormat)) {
					allSilent = false;
					break;
				}
			}
			if (allSilent) {
				state.unwrittenSilentFrames++;
				continue;
			}
			flushSilence(NOISE_DETECTED, options, state);
			for (int chanMapIx = 0; chanMapIx < options.rawChannelCount; chanMapIx++) {
				if (options.rawChannelMap[chanMapIx] < 0) {
					const int sampleSize = options.outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32 ? 4 : 2;
					for (int sampleIx = 0; sampleIx < sampleSize; sampleIx++) {
						fputc(0, state.outputFile);
					}
				} else {
					putSampleBE(state.rawSampleBuffer[options.rawChannelMap[chanMapIx]], i, state.outputFile, options.outputSampleFormat);
				}
			}
			state.writtenFrames++;
		}
		frameCount -= renderedFramesThisPass;
	}
}

static void render(unsigned int frameCount, const Options &options, State &state) {
	if (options.rawChannelCount > 0) {
		renderRaw(frameCount, options, state);
	} else {
		renderStereo(frameCount, options, state);
	}
}

static void playSMF(smf_t *smf, const Options &options, State &state) {
	int unterminatedSysexLen = 0;
	unsigned char *unterminatedSysex = NULL;
	unsigned long renderedFrames = 0;
	smf_rewind(smf);
	for (;;) {
		smf_event_t *event = smf_get_next_event(smf);
		unsigned long eventFrameIx;

		if (event == NULL) {
			break;
		}

		assert(event->track->track_number >= 0);

		eventFrameIx = secondsToSamples(event->time_seconds, options.sampleRate);
		unsigned int renderLength = (eventFrameIx > renderedFrames) ? eventFrameIx - renderedFrames : 1;
		if (state.renderedFrames + renderLength > options.renderMaxFrames) {
			renderLength = options.renderMaxFrames - state.renderedFrames;
		}
		render(renderLength, options, state);
		renderedFrames += renderLength;
		if (state.renderedFrames == options.renderMaxFrames) {
			break;
		}

		if (smf_event_is_metadata(event)) {
			char *decoded = smf_event_decode(event);
			if (decoded && !options.quiet) {
				fprintf(stdout, "Metadata: %s\n", decoded);
			}
		} else if (smf_event_is_sysex(event) || smf_event_is_sysex_continuation(event))  {
			bool unterminated = smf_event_is_unterminated_sysex(event) != 0;
			bool addUnterminated = unterminated;
			bool continuation = smf_event_is_sysex_continuation(event) != 0;
			unsigned char *buf;
			int len;
			if (continuation) {
				if (unterminatedSysex != NULL) {
					addUnterminated = true;
				} else {
					fprintf(stderr, "Sysex continuation received without preceding unterminated sysex - hoping for the best\n");
				}
				buf = event->midi_buffer + 1;
				len = event->midi_buffer_length - 1;
			} else {
				if (unterminatedSysex != NULL) {
					fprintf(stderr, "New sysex received with an unterminated sysex pending - ignoring unterminated\n");
					delete[] unterminatedSysex;
					unterminatedSysex = NULL;
					unterminatedSysexLen = 0;
				}
				buf = event->midi_buffer;
				len = event->midi_buffer_length;
			}
			if (addUnterminated) {
				unsigned char *newUnterminatedSysex = new unsigned char[unterminatedSysexLen + len];
				if(unterminatedSysex != NULL) {
					memcpy(newUnterminatedSysex, unterminatedSysex, unterminatedSysexLen);
					delete[] unterminatedSysex;
				}
				memcpy(newUnterminatedSysex + unterminatedSysexLen, buf, len);
				unterminatedSysex = newUnterminatedSysex;
				unterminatedSysexLen += len;
				buf = unterminatedSysex;
				len = unterminatedSysexLen;
			}
			if (!unterminated) {
				state.service.playSysex(buf, len);
				if (addUnterminated) {
					delete[] unterminatedSysex;
					unterminatedSysex = NULL;
					unterminatedSysexLen = 0;
				}
			}
		} else {
			if (event->midi_buffer_length > 3) {
				fprintf(stderr, "Got message with unusual length: %d\n", event->midi_buffer_length);
				for (int i = 0; i < event->midi_buffer_length; i++) {
					fprintf(stderr, " %02x", event->midi_buffer[i]);
				}
				fprintf(stderr, "\n");
			} else {
				MT32Emu::Bit32u msg = 0;
				for (int i = 0; i < event->midi_buffer_length; i++) {
					msg |= (event->midi_buffer[i] << (8 * i));
				}
				state.service.playMsg(msg);
			}
		}
	}
	flushSilence(MIDI_ENDED, options, state);
	if (options.sendAllNotesOff) {
		for (unsigned char channel = 0; channel < 16; channel++) {
			state.service.playMsg(0x0040B0 | channel); // Release sustain pedal
			state.service.playMsg(0x007BB0 | channel); // All notes off
		}
	}
	if (state.lastInputFile && options.renderMinFrames > state.renderedFrames) {
		render(options.renderMinFrames - state.renderedFrames, options, state);
	}
	if (options.waitForLA32) {
		while (state.renderedFrames < options.renderMaxFrames && state.service.hasActivePartials()) {
			// FIXME: Rendering one sample at a time is very inefficient, but it's important for
			// some tests to be able to see the precise frame when partials become inactive.
			// Perhaps we should add a renderWhilePartialsActive() to Synth
			// or add a getter for "the minimum number of frames remaining with active partials if
			// no further MIDI is sent". Which would make quite a function name.
			render(1, options, state);
		}
		flushSilence(LA32_INACTIVE, options, state);
		if (options.waitForReverb) {
			unsigned int reverbEndFrames = MIN(MAX_REVERB_END_FRAMES, options.bufferFrameCount);
			while (state.renderedFrames < options.renderMaxFrames && state.service.isActive()) {
				// Render a healthy number of frames while waiting for reverb to become inactive.
				// Note that once we've detected inactivity, silent samples will not be written.
				unsigned int renderLength = reverbEndFrames;
				if (state.renderedFrames + renderLength > options.renderMaxFrames) {
					renderLength = options.renderMaxFrames - state.renderedFrames;
				}
				render(renderLength, options, state);
			}
		}
	}
	if (!state.service.isActive()) {
		state.unwrittenSilentFrames = 0;
	}
	delete[] unterminatedSysex;
}

static bool playFile(const gchar *inputFilename, const gchar *displayInputFilename, const Options &options, State &state) {
	MT32Emu::Bit8u *fileBuffer = NULL;
	gsize fileBufferLength = 0;
	if (!loadFile(fileBuffer, fileBufferLength, inputFilename, displayInputFilename)) {
		return false;
	}
	if (fileBuffer[0] == 0xF0) {
		return playSysexFileBuffer(state.service, displayInputFilename, fileBuffer, fileBufferLength);
	}
	smf_t *smf = smf_load_from_memory(fileBuffer, int(fileBufferLength));
	if (smf != NULL) {
		if (!options.quiet) {
			char *decoded = smf_decode(smf);
			fprintf(stdout, "%s.\n", decoded);
			free(decoded);
		}
		assert(smf->number_of_tracks >= 1);
		playSMF(smf, options, state);
		smf_delete(smf);
		return true;
	}
	fprintf(stderr, "Error parsing SMF file '%s'.\n", displayInputFilename);
	return false;
}

static size_t matchMachineIDs(const char **&matchedMachineIDs, MT32Emu::Service &service, const char * const machineID) {
	bool anyMachine = NULL == machineID || strcmp(machineID, "any") == 0;

	if (!(anyMachine || strcmp(machineID, "mt32") == 0 || strcmp(machineID, "cm32l") == 0)) {
		matchedMachineIDs = NULL;
		return 1;
	}

	const size_t knownMachineCount = service.getMachineIDs(NULL, 0);
	const char **knownMachineIDs = new const char *[knownMachineCount];
	service.getMachineIDs(knownMachineIDs, knownMachineCount);
	if (anyMachine) {
		matchedMachineIDs = knownMachineIDs;
		return knownMachineCount;
	}
	matchedMachineIDs = new const char *[knownMachineCount];
	guint matchedMachineCount = 0;
	for (guint i = 0; i < knownMachineCount; i++) {
		if (strncmp(machineID, knownMachineIDs[i], 4) == 0) {
			matchedMachineIDs[matchedMachineCount++] = knownMachineIDs[i];
		}
	}
	delete[] knownMachineIDs;
	return matchedMachineCount;
}

static bool loadMachineROMs(MT32Emu::Service &service, const char *romDirName, GDir *romDir, const char *machineID, bool logErrors) {
	bool controlROMFound = false;
	bool pcmROMFound = false;
	for (;;) {
		const char *fileName = g_dir_read_name(romDir);
		if (NULL == fileName) break;
		char *pathName = g_build_filename(romDirName, fileName, NULL);
		char *pathNameUtf8 = g_filename_to_utf8(pathName, strlen(pathName), NULL, NULL, NULL);
		char *pathNameLocale = g_locale_from_utf8(pathNameUtf8, strlen(pathNameUtf8), NULL, NULL, NULL);
		mt32emu_return_code rc = service.addMachineROMFile(machineID, pathNameLocale);
		g_free(pathNameLocale);
		g_free(pathNameUtf8);
		g_free(pathName);
		if (MT32EMU_RC_MACHINE_NOT_IDENTIFIED == rc) {
			if (logErrors) fprintf(stderr, "Unrecognised machine configuration.\n");
			return false;
		}
		controlROMFound = controlROMFound || MT32EMU_RC_ADDED_CONTROL_ROM == rc;
		pcmROMFound = pcmROMFound || MT32EMU_RC_ADDED_PCM_ROM == rc;
		if (controlROMFound && pcmROMFound) {
			printf("Using ROMs for machine %s.\n", machineID);
			return true;
		}
	}
	if (logErrors) fprintf(stderr, "ROMs not found for machine configuration.\n");
	return false;
}

static void identifyControlROMs(MT32Emu::Service &service, const char *romDirName, GDir *romDir, GHashTable *seenControlROMIDs) {
	mt32emu_rom_info rom_info;
	for (;;) {
		const char *fileName = g_dir_read_name(romDir);
		if (NULL == fileName) break;
		char *pathName = g_build_filename(romDirName, fileName, NULL);
		char *pathNameUtf8 = g_filename_to_utf8(pathName, strlen(pathName), NULL, NULL, NULL);
		char *pathNameLocale = g_locale_from_utf8(pathNameUtf8, strlen(pathNameUtf8), NULL, NULL, NULL);
		if (MT32EMU_RC_OK == service.identifyROMFile(&rom_info, pathNameLocale, NULL) && rom_info.control_rom_id != NULL) {
			g_hash_table_add(seenControlROMIDs, gpointer(rom_info.control_rom_id));
		}
		g_free(pathNameLocale);
		g_free(pathNameUtf8);
		g_free(pathName);
	}
}

static bool loadROMs(MT32Emu::Service &service, const char *romDirName, GDir *romDir, const char * const *machineIDs, const size_t machineIDCount) {
	GHashTable *seenControlROMIDs = g_hash_table_new(NULL, NULL);
	identifyControlROMs(service, romDirName, romDir, seenControlROMIDs);
	bool romsLoaded = false;
	for (size_t machineIndex = machineIDCount; !romsLoaded && machineIndex-- > 0;) {
		const char *machineID = machineIDs[machineIndex];
		size_t machineROMCount = service.getROMIDs(NULL, 0, machineID);
		const char **machineROMIDs = new const char *[machineROMCount];
		service.getROMIDs(machineROMIDs, machineROMCount, machineID);
		for (guint machineROMIndex = 0; !romsLoaded && machineROMIndex < machineROMCount; machineROMIndex++) {
			if (g_hash_table_contains(seenControlROMIDs, machineROMIDs[machineROMIndex])) {
				g_dir_rewind(romDir);
				if (loadMachineROMs(service, romDirName, romDir, machineID, false)) {
					romsLoaded = true;
					break;
				}
			}
		}
		delete[] machineROMIDs;
	}
	g_hash_table_destroy(seenControlROMIDs);
	return romsLoaded;
}

static bool loadROMs(MT32Emu::Service &service, const Options &options) {
	const char *romDirNameUtf8 = options.romDir;
	if (romDirNameUtf8 == NULL) romDirNameUtf8 = ".";
	char *romDirName = g_filename_from_utf8(romDirNameUtf8, strlen(romDirNameUtf8), NULL, NULL, NULL);
	GDir *romDir = g_dir_open(romDirName, 0, NULL);
	if (NULL == romDir) {
		fprintf(stderr, "Error reading contents of ROM dir.\n");
		g_free(romDirName);
		return false;
	}

	bool res;
	const char **machineIDs;
	const size_t machineIDCount = matchMachineIDs(machineIDs, service, options.machineID);
	if (NULL == machineIDs) {
		res = loadMachineROMs(service, romDirName, romDir, options.machineID, true);
	} else {
		res = loadROMs(service, romDirName, romDir, machineIDs, machineIDCount);
		if (!res) fprintf(stderr, "ROMs not found for machine configuration.\n");
		delete[] machineIDs;
	}

	g_dir_close(romDir);
	g_free(romDirName);
	return res;
}

int main(int argc, char *argv[]) {
	Options options;
	MT32Emu::Service service;
	setlocale(LC_ALL, "");
#ifdef BUILD_MT32EMU_VERSION
	const char *mt32emuVersion = BUILD_MT32EMU_VERSION;
#else
	const char *mt32emuVersion = service.getLibraryVersionString();
#endif
	printf("Munt MT32Emu MIDI to Wave Conversion Utility. Version %s\n", MT32EMU_SMF2WAV_VERSION);
	printf("  Copyright (C) 2009, 2011 Jerome Fisher <re_munt@kingguppy.com>\n");
	printf("  Copyright (C) 2012-2022 Jerome Fisher, Sergey V. Mikayev\n");
	printf("Using Munt MT32Emu Library Version %s, libsmf Version %s (with modifications)\n", mt32emuVersion, smf_get_version());
	if (!parseOptions(argc, argv, &options)) {
		return -1;
	}
	gchar *outputFilename;
	if (options.outputFilename != NULL) {
		outputFilename = options.outputFilename;
	} else {
		gchar *lastInputFilename = options.inputFilenames[g_strv_length(options.inputFilenames) - 1];
		size_t allocLen = strlen(lastInputFilename) + 5;
		outputFilename = new gchar[allocLen];
		if(outputFilename == NULL) {
			fprintf(stderr, "Error allocating %lu bytes for destination filename.\n", (unsigned long)allocLen);
			return -1;
		}
		if (options.rawChannelCount > 0) {
			sprintf(outputFilename, "%s.raw", lastInputFilename);
		} else {
			sprintf(outputFilename, "%s.wav", lastInputFilename);
		}
	}

	service.createContext();
	if (!loadROMs(service, options)) {
		service.freeContext();

		if (options.outputFilename == NULL && outputFilename != NULL) {
			delete[] outputFilename;
		}
		freeOptions(&options);
		return 1;
	}
	service.setStereoOutputSampleRate(options.sampleRate);
	service.setSamplerateConversionQuality(options.srcQuality);
	service.setPartialCount(options.partialCount);
	service.setAnalogOutputMode(options.analogOutputMode);
	service.selectRendererType(options.rendererType);
	if (service.openSynth() == MT32EMU_RC_OK) {
		service.setDACInputMode(options.dacInputMode);
		if (!options.niceAmpRamp) {
			service.setNiceAmpRampEnabled(false);
		}
		if (options.nicePanning) {
			service.setNicePanningEnabled(true);
		}
		if (options.nicePartialMixing) {
			service.setNicePartialMixingEnabled(true);
		}
		options.sampleRate = service.getActualStereoOutputSamplerate();
		printf("Using output sample rate %d Hz\n", options.sampleRate);

		char *outputFilenameUtf8 = g_filename_to_utf8(outputFilename, strlen(outputFilename), NULL, NULL, NULL);
		char *outputFilenameLocale = g_locale_from_utf8(outputFilenameUtf8, strlen(outputFilenameUtf8), NULL, NULL, NULL);

		FILE *outputFile;
		bool outputFileExists = false;
		if (!options.force) {
			// FIXME: Lame way of avoiding overwriting an existing file
			// (since it could theoretically be created between us testing and
			// opening for writing)
			if (g_file_test(outputFilename, G_FILE_TEST_EXISTS)) {
				outputFileExists = true;
			}
		}
		if (outputFileExists) {
			fprintf(stderr, "Destination file '%s' exists.\n", outputFilenameLocale);
			outputFile = NULL;
		} else {
#ifdef _MSC_VER
			fopen_s(&outputFile, outputFilenameLocale, "wb");
#else
			outputFile = fopen(outputFilenameLocale, "wb");
#endif
		}

		clock_t startTime = clock();

		if (outputFile != NULL) {
			if (options.rawChannelCount > 0 || writeWAVEHeader(outputFile, options.sampleRate, options.outputSampleFormat)) {
				State state = {NULL, {NULL, NULL, NULL, NULL, NULL, NULL}, service, outputFile, false, false, 0, 0, 0};
				state.outputFile = outputFile;
				if (options.rawChannelCount > 0) {
					for (int i = 0; i < 6; i++) {
						if (options.outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
							state.rawSampleBuffer[i] = new float[options.bufferFrameCount];
						} else {
							state.rawSampleBuffer[i] = new MT32Emu::Bit16s[options.bufferFrameCount];
						}
					}
				} else {
					if (options.outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
						state.stereoSampleBuffer = new float[options.bufferFrameCount * 2];
					} else {
						state.stereoSampleBuffer = new MT32Emu::Bit16s[options.bufferFrameCount * 2];
					}
				}
				gchar **inputFilename = options.inputFilenames;
				while (*inputFilename != NULL) {
					char *inputFilenameUtf8 = g_filename_to_utf8(*inputFilename, strlen(*inputFilename), NULL, NULL, NULL);
					char *inputFilenameLocale = g_locale_from_utf8(inputFilenameUtf8, strlen(inputFilenameUtf8), NULL, NULL, NULL);
					state.lastInputFile = *(inputFilename + 1) == NULL; // FIXME: This should actually be true if all subsequent files are sysex
					playFile(*inputFilename, inputFilenameLocale, options, state);
					inputFilename++;
					g_free(inputFilenameLocale);
					g_free(inputFilenameUtf8);
				}
				if (options.outputSampleFormat == OUTPUT_SAMPLE_FORMAT_IEEE_FLOAT32) {
					delete[] static_cast<float *>(state.stereoSampleBuffer);
					for (int i = 0; i < 6; i++) {
						delete[] static_cast<float *>(state.rawSampleBuffer[i]);
					}
				} else {
					delete[] static_cast<MT32Emu::Bit16s *>(state.stereoSampleBuffer);
					for (int i = 0; i < 6; i++) {
						delete[] static_cast<MT32Emu::Bit16s *>(state.rawSampleBuffer[i]);
					}
				}
				if (options.rawChannelCount == 0 && !fillWAVESizes(outputFile, state.writtenFrames, options.outputSampleFormat)) {
					fprintf(stderr, "Error writing final sizes to WAVE header\n");
				}
			} else {
				fprintf(stderr, "Error writing WAVE header to '%s'\n", outputFilenameLocale);
			}
			fclose(outputFile);
			printf("Elapsed time: %f sec\n", float(clock() - startTime) / CLOCKS_PER_SEC);
		} else {
			fprintf(stderr, "Error opening file '%s' for writing.\n", outputFilenameLocale);
		}
		g_free(outputFilenameLocale);
		g_free(outputFilenameUtf8);
	} else {
		fprintf(stderr, "Error opening MT32Emu synthesizer.\n");
	}
	service.freeContext();

	if(options.outputFilename == NULL && outputFilename != NULL) {
		delete[] outputFilename;
	}
	freeOptions(&options);
	return 0;
}
