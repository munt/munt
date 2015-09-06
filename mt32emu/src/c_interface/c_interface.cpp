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

#include "../mt32emu.h"
#include "c_interface.h"

using namespace MT32Emu;

namespace MT32Emu {

class ReportHandlerAdapter;
class MidiStreamParserAdapter;

}

// Exported C-visible stuff

extern "C" {

struct mt32emu_data {
	ReportHandlerAdapter *reportHandler;
	Synth *synth;
	const ROMImage *controlROMImage;
	const ROMImage *pcmROMImage;
	MidiStreamParserAdapter *midiParser;
};

} // extern "C"

// Internal C++ utility stuff

namespace MT32Emu {

class ReportHandlerAdapter : public ReportHandler {
public:
	ReportHandlerAdapter(const mt32emu_report_handler_o *use_report_handler) : delegate(use_report_handler) {}

	static void printDebug(mt32emu_const_context context, const char *fmt, ...) {
		if (context->reportHandler != NULL) {
			va_list ap;
			va_start(ap, fmt);
			context->reportHandler->printDebug(fmt, ap);
			va_end(ap);
		}
	}

	static void onMIDIQueueOverflow(mt32emu_const_context context) {
		if (context->reportHandler != NULL) {
			const mt32emu_report_handler_o *delegate = context->reportHandler->delegate;
			if (delegate->i->v0.onMIDIQueueOverflow != NULL) {
				delegate->i->v0.onMIDIQueueOverflow(delegate);
			}
		}
	}

	static void onMIDISystemRealtime(mt32emu_const_context context, Bit8u systemRealtime) {
		if (context->reportHandler != NULL) {
			const mt32emu_report_handler_o *delegate = context->reportHandler->delegate;
			if (delegate->i->v0.onMIDISystemRealtime != NULL) {
				delegate->i->v0.onMIDISystemRealtime(delegate, systemRealtime);
			}
		}
	}

private:
	const mt32emu_report_handler_o * const delegate;

	void printDebug(const char *fmt, va_list list) {
		if (delegate->i->v0.printDebug == NULL) {
			ReportHandler::printDebug(fmt, list);
		} else {
			delegate->i->v0.printDebug(delegate, fmt, list);
		}
	}

	void onErrorControlROM() {
		if (delegate->i->v0.onErrorControlROM == NULL) {
			ReportHandler::onErrorControlROM();
		} else {
			delegate->i->v0.onErrorControlROM(delegate);
		}
	}

	void onErrorPCMROM() {
		if (delegate->i->v0.onErrorPCMROM == NULL) {
			ReportHandler::onErrorPCMROM();
		} else {
			delegate->i->v0.onErrorPCMROM(delegate);
		}
	}

	void showLCDMessage(const char *message) {
		if (delegate->i->v0.showLCDMessage == NULL) {
			ReportHandler::showLCDMessage(message);
		} else {
			delegate->i->v0.showLCDMessage(delegate, message);
		}
	}

	void onMIDIMessagePlayed() {
		if (delegate->i->v0.onMIDIMessagePlayed == NULL) {
			ReportHandler::onMIDIMessagePlayed();
		} else {
			delegate->i->v0.onMIDIMessagePlayed(delegate);
		}
	}

	void onDeviceReset() {
		if (delegate->i->v0.onDeviceReset == NULL) {
			ReportHandler::onDeviceReset();
		} else {
			delegate->i->v0.onDeviceReset(delegate);
		}
	}

	void onDeviceReconfig() {
		if (delegate->i->v0.onDeviceReconfig == NULL) {
			ReportHandler::onDeviceReconfig();
		} else {
			delegate->i->v0.onDeviceReconfig(delegate);
		}
	}

	void onNewReverbMode(Bit8u mode) {
		if (delegate->i->v0.onNewReverbMode == NULL) {
			ReportHandler::onNewReverbMode(mode);
		} else {
			delegate->i->v0.onNewReverbMode(delegate, mode);
		}
	}

	void onNewReverbTime(Bit8u time) {
		if (delegate->i->v0.onNewReverbTime == NULL) {
			ReportHandler::onNewReverbTime(time);
		} else {
			delegate->i->v0.onNewReverbTime(delegate, time);
		}
	}

	void onNewReverbLevel(Bit8u level) {
		if (delegate->i->v0.onNewReverbLevel == NULL) {
			ReportHandler::onNewReverbLevel(level);
		} else {
			delegate->i->v0.onNewReverbLevel(delegate, level);
		}
	}

	void onPolyStateChanged(int partNum) {
		if (delegate->i->v0.onPolyStateChanged == NULL) {
			ReportHandler::onPolyStateChanged(partNum);
		} else {
			delegate->i->v0.onPolyStateChanged(delegate, partNum);
		}
	}

	void onProgramChanged(int partNum, const char *soundGroupName, const char *patchName) {
		if (delegate->i->v0.onProgramChanged == NULL) {
			ReportHandler::onProgramChanged(partNum, soundGroupName, patchName);
		} else {
			delegate->i->v0.onProgramChanged(delegate, partNum, soundGroupName, patchName);
		}
	}
};

class MidiStreamParserAdapter : public MidiStreamParser {
public:
	MidiStreamParserAdapter(mt32emu_const_context useContext) : context(useContext), timestampSet(false) {}

	void setTimestamp(const Bit32u useTimestamp) {
		timestampSet = true;
		timestamp = useTimestamp;
	}

	void resetTimestamp() {
		timestampSet = false;
	}

private:
	const mt32emu_const_context context;
	bool timestampSet;
	Bit32u timestamp;

	void handleShortMessage(const Bit32u message) {
		bool res;
		if (timestampSet) {
			res = context->synth->playMsg(message, timestamp);
		} else {
			res = context->synth->playMsg(message);
		}
		if (!res) ReportHandlerAdapter::onMIDIQueueOverflow(context);
	}

	void handleSysex(const Bit8u *stream, const Bit32u length) {
		bool res;
		if (timestampSet) {
			res = context->synth->playSysex(stream, length, timestamp);
		} else {
			res = context->synth->playSysex(stream, length);
		}
		if (!res) ReportHandlerAdapter::onMIDIQueueOverflow(context);
	}

	void handleSystemRealtimeMessage(const Bit8u realtime) {
		ReportHandlerAdapter::onMIDISystemRealtime(context, realtime);
	}

	void printDebug(const char *debugMessage) {
		ReportHandlerAdapter::printDebug(context, debugMessage);
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
		if (context->controlROMImage != NULL) {
			delete context->controlROMImage->getFile();
			ROMImage::freeROMImage(context->controlROMImage);
		}
		context->controlROMImage = image;
		return MT32EMU_RC_ADDED_CONTROL_ROM;
	} else if (info->type == ROMInfo::PCM) {
		if (context->pcmROMImage != NULL) {
			delete context->pcmROMImage->getFile();
			ROMImage::freeROMImage(context->pcmROMImage);
		}
		context->pcmROMImage = image;
		return MT32EMU_RC_ADDED_PCM_ROM;
	}
	ROMImage::freeROMImage(image);
	return MT32EMU_RC_OK; // No support for reverb ROM yet.
}

} // namespace MT32Emu

// C-visible implementation

extern "C" {

mt32emu_context mt32emu_create_synth(const mt32emu_report_handler_o *report_handler) {
	mt32emu_context data = new mt32emu_data;
	data->reportHandler = (report_handler == NULL) ? NULL : new ReportHandlerAdapter(report_handler);
	data->synth = new Synth(data->reportHandler);
	data->midiParser = new MidiStreamParserAdapter(data);
	data->controlROMImage = NULL;
	data->pcmROMImage = NULL;
	return data;
}

void mt32emu_free_synth(mt32emu_context context) {
	if (context->controlROMImage != NULL) {
		delete context->controlROMImage->getFile();
		ROMImage::freeROMImage(context->controlROMImage);
		context->controlROMImage = NULL;
	}
	if (context->pcmROMImage != NULL) {
		delete context->pcmROMImage->getFile();
		ROMImage::freeROMImage(context->pcmROMImage);
		context->pcmROMImage = NULL;
	}
	delete context->midiParser;
	context->midiParser = NULL;
	delete context->synth;
	context->synth = NULL;
	delete context->reportHandler;
	context->reportHandler = NULL;
	delete context;
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
	if ((context->controlROMImage == NULL) || (context->pcmROMImage == NULL)) {
		return MT32EMU_RC_MISSING_ROMS;
	}
	unsigned int partialCount = (partial_count == NULL) ? DEFAULT_MAX_PARTIALS : *partial_count;
	AnalogOutputMode analogOutputMode = (analog_output_mode == NULL) ? AnalogOutputMode_COARSE : (AnalogOutputMode)*analog_output_mode;
	if (context->synth->open(*context->controlROMImage, *context->pcmROMImage, partialCount, analogOutputMode)) {
		return MT32EMU_RC_OK;
	}
	return MT32EMU_RC_FAILED;
}

void mt32emu_close_synth(mt32emu_const_context context) {
	context->synth->close();
}

mt32emu_boolean mt32emu_is_open(mt32emu_const_context context) {
	return context->synth->isOpen() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

unsigned int mt32emu_get_stereo_output_samplerate(const mt32emu_analog_output_mode analog_output_mode) {
	return Synth::getStereoOutputSampleRate((AnalogOutputMode)analog_output_mode);
}

unsigned int mt32emu_get_actual_stereo_output_samplerate(mt32emu_const_context context) {
	return context->synth->getStereoOutputSampleRate();
}

void mt32emu_flush_midi_queue(mt32emu_const_context context) {
	context->synth->flushMIDIQueue();
}

mt32emu_bit32u mt32emu_set_midi_event_queue_size(mt32emu_const_context context, const mt32emu_bit32u queue_size) {
	return context->synth->setMIDIEventQueueSize(queue_size);
}

void mt32emu_parse_stream(mt32emu_const_context context, const mt32emu_bit8u *stream, mt32emu_bit32u length) {
	context->midiParser->resetTimestamp();
	context->midiParser->parseStream(stream, length);
}

void mt32emu_parse_stream_at(mt32emu_const_context context, const mt32emu_bit8u *stream, mt32emu_bit32u length, mt32emu_bit32u timestamp) {
	context->midiParser->setTimestamp(timestamp);
	context->midiParser->parseStream(stream, length);
}

void mt32emu_play_short_message(mt32emu_const_context context, mt32emu_bit32u message) {
	context->midiParser->resetTimestamp();
	context->midiParser->processShortMessage(message);
}

void mt32emu_play_short_message_at(mt32emu_const_context context, mt32emu_bit32u message, mt32emu_bit32u timestamp) {
	context->midiParser->setTimestamp(timestamp);
	context->midiParser->processShortMessage(message);
}

mt32emu_return_code mt32emu_play_msg(mt32emu_const_context context, mt32emu_bit32u msg) {
	if (!context->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context->synth->playMsg(msg)) return MT32EMU_RC_OK;
	ReportHandlerAdapter::onMIDIQueueOverflow(context);
	return MT32EMU_RC_QUEUE_FULL;
}

mt32emu_return_code mt32emu_play_sysex(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len) {
	if (!context->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context->synth->playSysex(sysex, len)) return MT32EMU_RC_OK;
	ReportHandlerAdapter::onMIDIQueueOverflow(context);
	return MT32EMU_RC_QUEUE_FULL;
}

mt32emu_return_code mt32emu_play_msg_at(mt32emu_const_context context, mt32emu_bit32u msg, mt32emu_bit32u timestamp) {
	if (!context->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context->synth->playMsg(msg, timestamp)) return MT32EMU_RC_OK;
	ReportHandlerAdapter::onMIDIQueueOverflow(context);
	return MT32EMU_RC_QUEUE_FULL;
}

mt32emu_return_code mt32emu_play_sysex_at(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len, mt32emu_bit32u timestamp) {
	if (!context->synth->isOpen()) return MT32EMU_RC_NOT_OPENED;
	if (context->synth->playSysex(sysex, len, timestamp)) return MT32EMU_RC_OK;
	ReportHandlerAdapter::onMIDIQueueOverflow(context);
	return MT32EMU_RC_QUEUE_FULL;
}

void mt32emu_play_msg_now(mt32emu_const_context context, mt32emu_bit32u msg) {
	context->synth->playMsgNow(msg);
}

void mt32emu_play_msg_on_part(mt32emu_const_context context, unsigned char part, unsigned char code, unsigned char note, unsigned char velocity) {
	context->synth->playMsgOnPart(part, code, note, velocity);
}

void mt32emu_play_sysex_now(mt32emu_const_context context, const mt32emu_bit8u *sysex, mt32emu_bit32u len) {
	context->synth->playSysexNow(sysex, len);
}

void mt32emu_write_sysex(mt32emu_const_context context, unsigned char channel, const mt32emu_bit8u *sysex, mt32emu_bit32u len) {
	context->synth->writeSysex(channel, sysex, len);
}

void mt32emu_set_reverb_enabled(mt32emu_const_context context, const mt32emu_boolean reverb_enabled) {
	context->synth->setReverbEnabled(reverb_enabled == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_reverb_enabled(mt32emu_const_context context) {
	return context->synth->isReverbEnabled() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_set_reverb_overridden(mt32emu_const_context context, const mt32emu_boolean reverbOverridden) {
	context->synth->setReverbOverridden(reverbOverridden == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_reverb_overridden(mt32emu_const_context context) {
	return context->synth->isReverbOverridden() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_set_reverb_compatibility_mode(mt32emu_const_context context, const mt32emu_boolean mt32_compatible_mode) {
	context->synth->setReverbCompatibilityMode(mt32_compatible_mode == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_mt32_reverb_compatibility_mode(mt32emu_const_context context) {
	return context->synth->isMT32ReverbCompatibilityMode() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

mt32emu_boolean mt32emu_is_default_reverb_mt32_compatible(mt32emu_const_context context) {
	return context->synth->isDefaultReverbMT32Compatible() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_set_dac_input_mode(mt32emu_const_context context, const mt32emu_dac_input_mode mode) {
	context->synth->setDACInputMode((DACInputMode)mode);
}

mt32emu_dac_input_mode mt32emu_get_dac_input_mode(mt32emu_const_context context) {
	return (mt32emu_dac_input_mode)context->synth->getDACInputMode();
}

void mt32emu_set_midi_delay_mode(mt32emu_const_context context, const mt32emu_midi_delay_mode mode) {
	context->synth->setMIDIDelayMode((MIDIDelayMode)mode);
}

mt32emu_midi_delay_mode mt32emu_get_midi_delay_mode(mt32emu_const_context context) {
	return (mt32emu_midi_delay_mode)context->synth->getMIDIDelayMode();
}

void mt32emu_set_output_gain(mt32emu_const_context context, float gain) {
	context->synth->setOutputGain(gain);
}

float mt32emu_get_output_gain(mt32emu_const_context context) {
	return context->synth->getOutputGain();
}

void mt32emu_set_reverb_output_gain(mt32emu_const_context context, float gain) {
	context->synth->setReverbOutputGain(gain);
}

float mt32emu_get_reverb_output_gain(mt32emu_const_context context) {
	return context->synth->getReverbOutputGain();
}

void mt32emu_set_reversed_stereo_enabled(mt32emu_const_context context, const mt32emu_boolean enabled) {
	context->synth->setReversedStereoEnabled(enabled == MT32EMU_BOOL_TRUE);
}

mt32emu_boolean mt32emu_is_reversed_stereo_enabled(mt32emu_const_context context) {
	return context->synth->isReversedStereoEnabled() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

void mt32emu_render(mt32emu_const_context context, mt32emu_sample *stream, mt32emu_bit32u len) {
	context->synth->render(stream, len);
}

void mt32emu_renderStreams(mt32emu_const_context context, const mt32emu_dac_output_streams *streams, mt32emu_bit32u len) {
	context->synth->renderStreams(streams->nonReverbLeft, streams->nonReverbRight, streams->reverbDryLeft, streams->reverbDryRight,
		streams->reverbWetLeft, streams->reverbWetRight, len);
}

mt32emu_boolean mt32emu_has_active_partials(mt32emu_const_context context) {
	return context->synth->hasActivePartials() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

mt32emu_boolean mt32emu_is_active(mt32emu_const_context context) {
	return context->synth->isActive() ? MT32EMU_BOOL_TRUE : MT32EMU_BOOL_FALSE;
}

unsigned int mt32emu_get_partial_count(mt32emu_const_context context) {
	return context->synth->getPartialCount();
}

mt32emu_bit32u mt32emu_get_part_states(mt32emu_const_context context) {
	return context->synth->getPartStates();
}

void mt32emu_get_partial_states(mt32emu_const_context context, mt32emu_bit8u *partial_states) {
	context->synth->getPartialStates(partial_states);
}

unsigned int mt32emu_get_playing_notes(mt32emu_const_context context, unsigned int part_number, mt32emu_bit8u *keys, mt32emu_bit8u *velocities) {
	return context->synth->getPlayingNotes(part_number, keys, velocities);
}

const char *mt32emu_get_patch_name(mt32emu_const_context context, unsigned int part_number) {
	return context->synth->getPatchName(part_number);
}

void mt32emu_read_memory(mt32emu_const_context context, mt32emu_bit32u addr, mt32emu_bit32u len, mt32emu_bit8u *data) {
	context->synth->readMemory(addr, len, data);
}

mt32emu_report_handler_version mt32emu_get_supported_report_handler_version() {
	return MT32EMU_REPORT_HANDLER_VERSION_CURRENT;
}

} // extern "C"
