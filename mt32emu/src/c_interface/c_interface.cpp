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

#include "../globals.h"
#include "../Types.h"
#include "../File.h"
#include "../FileStream.h"
#include "../ROMInfo.h"
#include "../Synth.h"
#include "../MidiStreamParser.h"

#include "c_interface.h"

using namespace MT32Emu;

namespace MT32Emu {

class ReportHandlerAdapter;
class MidiStreamParserAdapter;

static mt32emu_interface_version getSynthVersionID(const mt32emu_interface **);
static mt32emu_report_handler_version getSupportedReportHandlerVersionID(mt32emu_const_context context);
static unsigned int getStereoOutputSamplerate(mt32emu_const_context context, const mt32emu_analog_output_mode analog_output_mode);

static const mt32emu_synth_i_v0 SYNTH_VTABLE = {
	getSynthVersionID,
	mt32emu_free_synth,
	mt32emu_add_rom_data,
	mt32emu_add_rom_file,
	mt32emu_open_synth,
	mt32emu_close_synth,
	mt32emu_is_open,
	getStereoOutputSamplerate,
	mt32emu_get_actual_stereo_output_samplerate,
	mt32emu_flush_midi_queue,
	mt32emu_set_midi_event_queue_size,
	mt32emu_set_midi_receiver,
	mt32emu_parse_stream,
	mt32emu_parse_stream_at,
	mt32emu_play_short_message,
	mt32emu_play_short_message_at,
	mt32emu_play_msg,
	mt32emu_play_sysex,
	mt32emu_play_msg_at,
	mt32emu_play_sysex_at,
	mt32emu_play_msg_now,
	mt32emu_play_msg_on_part,
	mt32emu_play_sysex_now,
	mt32emu_write_sysex,
	mt32emu_set_reverb_enabled,
	mt32emu_is_reverb_enabled,
	mt32emu_set_reverb_overridden,
	mt32emu_is_reverb_overridden,
	mt32emu_set_reverb_compatibility_mode,
	mt32emu_is_mt32_reverb_compatibility_mode,
	mt32emu_is_default_reverb_mt32_compatible,
	mt32emu_set_dac_input_mode,
	mt32emu_get_dac_input_mode,
	mt32emu_set_midi_delay_mode,
	mt32emu_get_midi_delay_mode,
	mt32emu_set_output_gain,
	mt32emu_get_output_gain,
	mt32emu_set_reverb_output_gain,
	mt32emu_get_reverb_output_gain,
	mt32emu_set_reversed_stereo_enabled,
	mt32emu_is_reversed_stereo_enabled,
	mt32emu_render_bit16s,
	mt32emu_render_float,
	mt32emu_render_bit16s_streams,
	mt32emu_render_float_streams,
	mt32emu_has_active_partials,
	mt32emu_is_active,
	mt32emu_get_partial_count,
	mt32emu_get_part_states,
	mt32emu_get_partial_states,
	mt32emu_get_playing_notes,
	mt32emu_get_patch_name,
	mt32emu_read_memory,
	getSupportedReportHandlerVersionID
};

} // namespace MT32Emu

struct mt32emu_data {
	mt32emu_synth_i i; // vtable placeholder
	ReportHandlerAdapter *reportHandler;
	Synth *synth;
	const ROMImage *controlROMImage;
	const ROMImage *pcmROMImage;
	MidiStreamParserAdapter *midiParser;
};

// Internal C++ utility stuff

namespace MT32Emu {

class ReportHandlerAdapter : public ReportHandler {
public:
	virtual void printDebug(const char *fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		printDebug(fmt, ap);
		va_end(ap);
	}

	virtual void onMIDIQueueOverflow() {}
	virtual void onMIDISystemRealtime(Bit8u /* systemRealtime */) {}
};

class DelegatingReportHandlerAdapter : public ReportHandlerAdapter {
public:
	DelegatingReportHandlerAdapter(const mt32emu_report_handler_i *use_report_handler) :
		delegate(use_report_handler) {}

protected:
	const mt32emu_report_handler_i * const delegate;

private:
	void printDebug(const char *fmt, va_list list) {
		if (delegate->v0->printDebug == NULL) {
			ReportHandler::printDebug(fmt, list);
		} else {
			delegate->v0->printDebug(delegate, fmt, list);
		}
	}

	void onErrorControlROM() {
		if (delegate->v0->onErrorControlROM == NULL) {
			ReportHandler::onErrorControlROM();
		} else {
			delegate->v0->onErrorControlROM(delegate);
		}
	}

	void onErrorPCMROM() {
		if (delegate->v0->onErrorPCMROM == NULL) {
			ReportHandler::onErrorPCMROM();
		} else {
			delegate->v0->onErrorPCMROM(delegate);
		}
	}

	void showLCDMessage(const char *message) {
		if (delegate->v0->showLCDMessage == NULL) {
			ReportHandler::showLCDMessage(message);
		} else {
			delegate->v0->showLCDMessage(delegate, message);
		}
	}

	void onMIDIMessagePlayed() {
		if (delegate->v0->onMIDIMessagePlayed == NULL) {
			ReportHandler::onMIDIMessagePlayed();
		} else {
			delegate->v0->onMIDIMessagePlayed(delegate);
		}
	}

	void onDeviceReset() {
		if (delegate->v0->onDeviceReset == NULL) {
			ReportHandler::onDeviceReset();
		} else {
			delegate->v0->onDeviceReset(delegate);
		}
	}

	void onDeviceReconfig() {
		if (delegate->v0->onDeviceReconfig == NULL) {
			ReportHandler::onDeviceReconfig();
		} else {
			delegate->v0->onDeviceReconfig(delegate);
		}
	}

	void onNewReverbMode(Bit8u mode) {
		if (delegate->v0->onNewReverbMode == NULL) {
			ReportHandler::onNewReverbMode(mode);
		} else {
			delegate->v0->onNewReverbMode(delegate, mode);
		}
	}

	void onNewReverbTime(Bit8u time) {
		if (delegate->v0->onNewReverbTime == NULL) {
			ReportHandler::onNewReverbTime(time);
		} else {
			delegate->v0->onNewReverbTime(delegate, time);
		}
	}

	void onNewReverbLevel(Bit8u level) {
		if (delegate->v0->onNewReverbLevel == NULL) {
			ReportHandler::onNewReverbLevel(level);
		} else {
			delegate->v0->onNewReverbLevel(delegate, level);
		}
	}

	void onPolyStateChanged(int partNum) {
		if (delegate->v0->onPolyStateChanged == NULL) {
			ReportHandler::onPolyStateChanged(partNum);
		} else {
			delegate->v0->onPolyStateChanged(delegate, partNum);
		}
	}

	void onProgramChanged(int partNum, const char *soundGroupName, const char *patchName) {
		if (delegate->v0->onProgramChanged == NULL) {
			ReportHandler::onProgramChanged(partNum, soundGroupName, patchName);
		} else {
			delegate->v0->onProgramChanged(delegate, partNum, soundGroupName, patchName);
		}
	}

	void onMIDIQueueOverflow() {
		if (delegate->v0->onMIDIQueueOverflow != NULL) {
			delegate->v0->onMIDIQueueOverflow(delegate);
		}
	}

	void onMIDISystemRealtime(Bit8u systemRealtime) {
		if (delegate->v0->onMIDISystemRealtime != NULL) {
			delegate->v0->onMIDISystemRealtime(delegate, systemRealtime);
		}
	}
};

class MidiStreamParserAdapter : public MidiStreamParser {
public:
	MidiStreamParserAdapter(const mt32emu_data *useData) : delegate(NULL), d(*useData), timestampSet(false) {}

	void setMIDIReceiver(const mt32emu_midi_receiver_i *newMIDIReceiver) {
		delegate = newMIDIReceiver;
	}

	void setTimestamp(const Bit32u useTimestamp) {
		timestampSet = true;
		timestamp = useTimestamp;
	}

	void resetTimestamp() {
		timestampSet = false;
	}

protected:
	const mt32emu_midi_receiver_i *delegate;
	const mt32emu_data &d;

private:
	bool timestampSet;
	Bit32u timestamp;

	void handleShortMessage(const Bit32u message) {
		if (delegate != NULL) {
			delegate->v0->handleShortMessage(delegate, message);
			return;
		}

		bool res;
		if (timestampSet) {
			res = d.synth->playMsg(message, timestamp);
		} else {
			res = d.synth->playMsg(message);
		}
		if (!res) d.reportHandler->onMIDIQueueOverflow();
	}

	void handleSysex(const Bit8u *stream, const Bit32u length) {
		if (delegate != NULL) {
			delegate->v0->handleSysex(delegate, stream, length);
			return;
		}

		bool res;
		if (timestampSet) {
			res = d.synth->playSysex(stream, length, timestamp);
		} else {
			res = d.synth->playSysex(stream, length);
		}
		if (!res) d.reportHandler->onMIDIQueueOverflow();
	}

	void handleSystemRealtimeMessage(const Bit8u realtime) {
		if (delegate != NULL) {
			delegate->v0->handleSystemRealtimeMessage(delegate, realtime);
			return;
		}

		d.reportHandler->onMIDISystemRealtime(realtime);
	}

	void printDebug(const char *debugMessage) {
		d.reportHandler->printDebug(debugMessage);
	}
};

static mt32emu_return_code addROMFile(mt32emu_context context, File *file) {
	const ROMImage *image = ROMImage::makeROMImage(file);
	const ROMInfo *info = image->getROMInfo();
	if (info == NULL) {
		ROMImage::freeROMImage(image);
		return MT32EMU_RC_ROM_NOT_IDENTIFIED;
	}
	if (info->type == ROMInfo::Control) {
		if (context.d->controlROMImage != NULL) {
			delete context.d->controlROMImage->getFile();
			ROMImage::freeROMImage(context.d->controlROMImage);
		}
		context.d->controlROMImage = image;
		return MT32EMU_RC_ADDED_CONTROL_ROM;
	} else if (info->type == ROMInfo::PCM) {
		if (context.d->pcmROMImage != NULL) {
			delete context.d->pcmROMImage->getFile();
			ROMImage::freeROMImage(context.d->pcmROMImage);
		}
		context.d->pcmROMImage = image;
		return MT32EMU_RC_ADDED_PCM_ROM;
	}
	ROMImage::freeROMImage(image);
	return MT32EMU_RC_OK; // No support for reverb ROM yet.
}

mt32emu_interface_version getSynthVersionID(const mt32emu_interface **) {
	mt32emu_interface_version v;
	v.s = MT32EMU_SYNTH_VERSION_CURRENT;
	return v;
}

mt32emu_report_handler_version getSupportedReportHandlerVersionID(mt32emu_const_context) {
	return mt32emu_get_supported_report_handler_version();
}

unsigned int getStereoOutputSamplerate(mt32emu_const_context, const mt32emu_analog_output_mode analog_output_mode) {
	return mt32emu_get_stereo_output_samplerate(analog_output_mode);
}

} // namespace MT32Emu

// C-visible implementation

extern "C" {

mt32emu_context mt32emu_create_synth(const mt32emu_report_handler_i *report_handler) {
	mt32emu_data *data = new mt32emu_data;
	data->i.v0 = &SYNTH_VTABLE;
	data->reportHandler = (report_handler != NULL) ? new DelegatingReportHandlerAdapter(report_handler) : new ReportHandlerAdapter;
	data->synth = new Synth(data->reportHandler);
	data->midiParser = new MidiStreamParserAdapter(data);
	data->controlROMImage = NULL;
	data->pcmROMImage = NULL;
	mt32emu_context context;
	context.d = data;
	return context;
}

void mt32emu_free_synth(mt32emu_context context) {
	mt32emu_data *data = context.d;
	if (data->controlROMImage != NULL) {
		delete data->controlROMImage->getFile();
		ROMImage::freeROMImage(data->controlROMImage);
		data->controlROMImage = NULL;
	}
	if (data->pcmROMImage != NULL) {
		delete data->pcmROMImage->getFile();
		ROMImage::freeROMImage(data->pcmROMImage);
		data->pcmROMImage = NULL;
	}
	delete data->midiParser;
	data->midiParser = NULL;
	delete data->synth;
	data->synth = NULL;
	delete data->reportHandler;
	data->reportHandler = NULL;
	delete data;
}

mt32emu_return_code mt32emu_add_rom_data(mt32emu_context context, const mt32emu_bit8u *data, size_t data_size, const mt32emu_sha1_digest *sha1_digest) {
	return addROMFile(context, new ArrayFile(data, data_size, *sha1_digest));
}

mt32emu_return_code mt32emu_add_rom_file(mt32emu_context context, const char *filename) {
	mt32emu_return_code rc = MT32EMU_RC_OK;
	FileStream *fs = new FileStream;
	if (fs->open(filename)) {
		if (fs->getData() != NULL) {
			rc = addROMFile(context, fs);
			if (rc > 0) return rc;
		} else {
			rc = MT32EMU_RC_FILE_NOT_LOADED;
		}
	} else {
		rc = MT32EMU_RC_FILE_NOT_FOUND;
	}
	delete fs;
	return rc;
}

mt32emu_return_code mt32emu_open_synth(mt32emu_const_context context, const unsigned int *partial_count, const mt32emu_analog_output_mode *analog_output_mode) {
	if ((context.c->controlROMImage == NULL) || (context.c->pcmROMImage == NULL)) {
		return MT32EMU_RC_MISSING_ROMS;
	}
	unsigned int partialCount = (partial_count == NULL) ? DEFAULT_MAX_PARTIALS : *partial_count;
	AnalogOutputMode analogOutputMode = (analog_output_mode == NULL) ? AnalogOutputMode_COARSE : (AnalogOutputMode)*analog_output_mode;
	if (context.c->synth->open(*context.c->controlROMImage, *context.c->pcmROMImage, partialCount, analogOutputMode)) {
		return MT32EMU_RC_OK;
	}
	return MT32EMU_RC_FAILED;
}

void mt32emu_close_synth(mt32emu_const_context context) {
	context.c->synth->close();
}

mt32emu_boolean mt32emu_is_open(mt32emu_const_context context) {
	return context.c->synth->isOpen() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

unsigned int mt32emu_get_stereo_output_samplerate(const mt32emu_analog_output_mode analog_output_mode) {
	return Synth::getStereoOutputSampleRate((AnalogOutputMode)analog_output_mode);
}

unsigned int mt32emu_get_actual_stereo_output_samplerate(mt32emu_const_context context) {
	return context.c->synth->getStereoOutputSampleRate();
}

void mt32emu_flush_midi_queue(mt32emu_const_context context) {
	context.c->synth->flushMIDIQueue();
}

mt32emu_bit32u mt32emu_set_midi_event_queue_size(mt32emu_const_context context, const mt32emu_bit32u queue_size) {
	return context.c->synth->setMIDIEventQueueSize(queue_size);
}

mt32emu_midi_receiver_version mt32emu_set_midi_receiver(mt32emu_const_context context, const mt32emu_midi_receiver_i *midi_receiver) {
	context.c->midiParser->setMIDIReceiver(midi_receiver);
	return MT32EMU_MIDI_RECEIVER_VERSION_CURRENT;
}

void mt32emu_parse_stream(mt32emu_const_context context, const mt32emu_bit8u *stream, mt32emu_bit32u length) {
	context.c->midiParser->resetTimestamp();
	context.c->midiParser->parseStream(stream, length);
}

void mt32emu_parse_stream_at(mt32emu_const_context context, const mt32emu_bit8u *stream, mt32emu_bit32u length, mt32emu_bit32u timestamp) {
	context.c->midiParser->setTimestamp(timestamp);
	context.c->midiParser->parseStream(stream, length);
}

void mt32emu_play_short_message(mt32emu_const_context context, mt32emu_bit32u message) {
	context.c->midiParser->resetTimestamp();
	context.c->midiParser->processShortMessage(message);
}

void mt32emu_play_short_message_at(mt32emu_const_context context, mt32emu_bit32u message, mt32emu_bit32u timestamp) {
	context.c->midiParser->setTimestamp(timestamp);
	context.c->midiParser->processShortMessage(message);
}

mt32emu_return_code mt32emu_play_msg(mt32emu_const_context context, mt32emu_bit32u msg) {
	if (!context.c->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context.c->synth->playMsg(msg)) return MT32EMU_RC_OK;
	context.c->reportHandler->onMIDIQueueOverflow();
	return MT32EMU_RC_QUEUE_FULL;
}

mt32emu_return_code mt32emu_play_sysex(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len) {
	if (!context.c->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context.c->synth->playSysex(sysex, len)) return MT32EMU_RC_OK;
	context.c->reportHandler->onMIDIQueueOverflow();
	return MT32EMU_RC_QUEUE_FULL;
}

mt32emu_return_code mt32emu_play_msg_at(mt32emu_const_context context, mt32emu_bit32u msg, mt32emu_bit32u timestamp) {
	if (!context.c->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context.c->synth->playMsg(msg, timestamp)) return MT32EMU_RC_OK;
	context.c->reportHandler->onMIDIQueueOverflow();
	return MT32EMU_RC_QUEUE_FULL;
}

mt32emu_return_code mt32emu_play_sysex_at(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len, mt32emu_bit32u timestamp) {
	if (!context.c->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context.c->synth->playSysex(sysex, len, timestamp)) return MT32EMU_RC_OK;
	context.c->reportHandler->onMIDIQueueOverflow();
	return MT32EMU_RC_QUEUE_FULL;
}

void mt32emu_play_msg_now(mt32emu_const_context context, mt32emu_bit32u msg) {
	context.c->synth->playMsgNow(msg);
}

void mt32emu_play_msg_on_part(mt32emu_const_context context, unsigned char part, unsigned char code, unsigned char note, unsigned char velocity) {
	context.c->synth->playMsgOnPart(part, code, note, velocity);
}

void mt32emu_play_sysex_now(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len) {
	context.c->synth->playSysexNow(sysex, len);
}

void mt32emu_write_sysex(mt32emu_const_context context, unsigned char channel, const mt32emu_bit8u *sysex, mt32emu_bit32u len) {
	context.c->synth->writeSysex(channel, sysex, len);
}

void mt32emu_set_reverb_enabled(mt32emu_const_context context, const mt32emu_boolean reverb_enabled) {
	context.c->synth->setReverbEnabled(reverb_enabled == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_reverb_enabled(mt32emu_const_context context) {
	return context.c->synth->isReverbEnabled() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_set_reverb_overridden(mt32emu_const_context context, const mt32emu_boolean reverbOverridden) {
	context.c->synth->setReverbOverridden(reverbOverridden == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_reverb_overridden(mt32emu_const_context context) {
	return context.c->synth->isReverbOverridden() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_set_reverb_compatibility_mode(mt32emu_const_context context, const mt32emu_boolean mt32_compatible_mode) {
	context.c->synth->setReverbCompatibilityMode(mt32_compatible_mode == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_mt32_reverb_compatibility_mode(mt32emu_const_context context) {
	return context.c->synth->isMT32ReverbCompatibilityMode() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

mt32emu_boolean mt32emu_is_default_reverb_mt32_compatible(mt32emu_const_context context) {
	return context.c->synth->isDefaultReverbMT32Compatible() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_set_dac_input_mode(mt32emu_const_context context, const mt32emu_dac_input_mode mode) {
	context.c->synth->setDACInputMode((DACInputMode)mode);
}

mt32emu_dac_input_mode mt32emu_get_dac_input_mode(mt32emu_const_context context) {
	return (mt32emu_dac_input_mode)context.c->synth->getDACInputMode();
}

void mt32emu_set_midi_delay_mode(mt32emu_const_context context, const mt32emu_midi_delay_mode mode) {
	context.c->synth->setMIDIDelayMode((MIDIDelayMode)mode);
}

mt32emu_midi_delay_mode mt32emu_get_midi_delay_mode(mt32emu_const_context context) {
	return (mt32emu_midi_delay_mode)context.c->synth->getMIDIDelayMode();
}

void mt32emu_set_output_gain(mt32emu_const_context context, float gain) {
	context.c->synth->setOutputGain(gain);
}

float mt32emu_get_output_gain(mt32emu_const_context context) {
	return context.c->synth->getOutputGain();
}

void mt32emu_set_reverb_output_gain(mt32emu_const_context context, float gain) {
	context.c->synth->setReverbOutputGain(gain);
}

float mt32emu_get_reverb_output_gain(mt32emu_const_context context) {
	return context.c->synth->getReverbOutputGain();
}

void mt32emu_set_reversed_stereo_enabled(mt32emu_const_context context, const mt32emu_boolean enabled) {
	context.c->synth->setReversedStereoEnabled(enabled == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_reversed_stereo_enabled(mt32emu_const_context context) {
	return context.c->synth->isReversedStereoEnabled() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_render_bit16s(mt32emu_const_context context, mt32emu_bit16s *stream, mt32emu_bit32u len) {
	context.c->synth->render(stream, len);
}

void mt32emu_render_float(mt32emu_const_context context, float *stream, mt32emu_bit32u len) {
	context.c->synth->render(stream, len);
}

void mt32emu_render_bit16s_streams(mt32emu_const_context context, const mt32emu_dac_output_bit16s_streams *streams, mt32emu_bit32u len) {
	context.c->synth->renderStreams(streams->nonReverbLeft, streams->nonReverbRight, streams->reverbDryLeft, streams->reverbDryRight,
		streams->reverbWetLeft, streams->reverbWetRight, len);
}

void mt32emu_render_float_streams(mt32emu_const_context context, const mt32emu_dac_output_float_streams *streams, mt32emu_bit32u len) {
	context.c->synth->renderStreams(streams->nonReverbLeft, streams->nonReverbRight, streams->reverbDryLeft, streams->reverbDryRight,
		streams->reverbWetLeft, streams->reverbWetRight, len);
}

mt32emu_boolean mt32emu_has_active_partials(mt32emu_const_context context) {
	return context.c->synth->hasActivePartials() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

mt32emu_boolean mt32emu_is_active(mt32emu_const_context context) {
	return context.c->synth->isActive() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

unsigned int mt32emu_get_partial_count(mt32emu_const_context context) {
	return context.c->synth->getPartialCount();
}

mt32emu_bit32u mt32emu_get_part_states(mt32emu_const_context context) {
	return context.c->synth->getPartStates();
}

void mt32emu_get_partial_states(mt32emu_const_context context, mt32emu_bit8u *partial_states) {
	context.c->synth->getPartialStates(partial_states);
}

unsigned int mt32emu_get_playing_notes(mt32emu_const_context context, unsigned int part_number, mt32emu_bit8u *keys, mt32emu_bit8u *velocities) {
	return context.c->synth->getPlayingNotes(part_number, keys, velocities);
}

const char *mt32emu_get_patch_name(mt32emu_const_context context, unsigned int part_number) {
	return context.c->synth->getPatchName(part_number);
}

void mt32emu_read_memory(mt32emu_const_context context, mt32emu_bit32u addr, mt32emu_bit32u len, mt32emu_bit8u *data) {
	context.c->synth->readMemory(addr, len, data);
}

mt32emu_report_handler_version mt32emu_get_supported_report_handler_version() {
	return MT32EMU_REPORT_HANDLER_VERSION_CURRENT;
}

} // extern "C"
