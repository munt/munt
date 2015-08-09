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

	const unsigned int MT32EMU_SAMPLE_RATE = SAMPLE_RATE;
	const unsigned int MT32EMU_DEFAULT_MAX_PARTIALS = DEFAULT_MAX_PARTIALS;
	const unsigned int MT32EMU_MAX_SAMPLES_PER_RUN = MAX_SAMPLES_PER_RUN;

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

	static void printDebug(mt32emu_context context, const char *fmt, ...) {
		va_list ap;
		va_start(ap, fmt);
		context->reportHandler->printDebug(fmt, ap);
		va_end(ap);
	}

private:
	const mt32emu_report_handler_o * const delegate;

	void printDebug(const char *fmt, va_list list) {
		if (delegate->i->printDebug == NULL) {
			ReportHandler::printDebug(fmt, list);
		} else {
			delegate->i->printDebug(delegate, fmt, list);
		}
	}

	void onErrorControlROM() {
		if (delegate->i->onErrorControlROM == NULL) {
			ReportHandler::onErrorControlROM();
		} else {
			delegate->i->onErrorControlROM(delegate);
		}
	}

	void onErrorPCMROM() {
		if (delegate->i->onErrorPCMROM == NULL) {
			ReportHandler::onErrorPCMROM();
		} else {
			delegate->i->onErrorPCMROM(delegate);
		}
	}

	void showLCDMessage(const char *message) {
		if (delegate->i->showLCDMessage == NULL) {
			ReportHandler::showLCDMessage(message);
		} else {
			delegate->i->showLCDMessage(delegate, message);
		}
	}

	void onMIDIMessagePlayed() {
		if (delegate->i->onMIDIMessagePlayed == NULL) {
			ReportHandler::onMIDIMessagePlayed();
		} else {
			delegate->i->onMIDIMessagePlayed(delegate);
		}
	}

	void onDeviceReset() {
		if (delegate->i->onDeviceReset == NULL) {
			ReportHandler::onDeviceReset();
		} else {
			delegate->i->onDeviceReset(delegate);
		}
	}

	void onDeviceReconfig() {
		if (delegate->i->onDeviceReconfig == NULL) {
			ReportHandler::onDeviceReconfig();
		} else {
			delegate->i->onDeviceReconfig(delegate);
		}
	}

	void onNewReverbMode(Bit8u mode) {
		if (delegate->i->onNewReverbMode == NULL) {
			ReportHandler::onNewReverbMode(mode);
		} else {
			delegate->i->onNewReverbMode(delegate, mode);
		}
	}

	void onNewReverbTime(Bit8u time) {
		if (delegate->i->onNewReverbTime == NULL) {
			ReportHandler::onNewReverbTime(time);
		} else {
			delegate->i->onNewReverbTime(delegate, time);
		}
	}

	void onNewReverbLevel(Bit8u level) {
		if (delegate->i->onNewReverbLevel == NULL) {
			ReportHandler::onNewReverbLevel(level);
		} else {
			delegate->i->onNewReverbLevel(delegate, level);
		}
	}

	void onPolyStateChanged(int partNum) {
		if (delegate->i->onPolyStateChanged == NULL) {
			ReportHandler::onPolyStateChanged(partNum);
		} else {
			delegate->i->onPolyStateChanged(delegate, partNum);
		}
	}

	void onProgramChanged(int partNum, const char *soundGroupName, const char *patchName) {
		if (delegate->i->onProgramChanged == NULL) {
			ReportHandler::onProgramChanged(partNum, soundGroupName, patchName);
		} else {
			delegate->i->onProgramChanged(delegate, partNum, soundGroupName, patchName);
		}
	}
};

class MidiStreamParserAdapter : public MidiStreamParser {
public:
	MidiStreamParserAdapter(mt32emu_context useContext) : context(useContext), timestampSet(false) {}

	void setTimestamp(const Bit32u useTimestamp) {
		timestampSet = true;
		timestamp = useTimestamp;
	}

	void resetTimestamp() {
		timestampSet = false;
	}

private:
	const mt32emu_context context;
	bool timestampSet;
	Bit32u timestamp;

	void handleShortMessage(const Bit32u message) {
		if (timestampSet) {
			context->synth->playMsg(message, timestamp);
		} else {
			context->synth->playMsg(message);
		}
	}

	void handleSysex(const Bit8u *stream, const Bit32u length) {
		if (timestampSet) {
			context->synth->playSysex(stream, length, timestamp);
		} else {
			context->synth->playSysex(stream, length);
		}
	}

	void handleSytemRealtimeMessage(const Bit8u realtime) {
		// Not implemented yet
		(void)realtime;
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
		return mt32emu_rc_rom_not_identified;
	}
	if (info->type == ROMInfo::Control) {
		if (context->controlROMImage != NULL) {
			delete context->controlROMImage->getFile();
			ROMImage::freeROMImage(context->controlROMImage);
		}
		context->controlROMImage = image;
		return mt32emu_rc_added_control_rom;
	} else if (info->type == ROMInfo::PCM) {
		if (context->pcmROMImage != NULL) {
			delete context->pcmROMImage->getFile();
			ROMImage::freeROMImage(context->pcmROMImage);
		}
		context->pcmROMImage = image;
		return mt32emu_rc_added_pcm_rom;
	}
	ROMImage::freeROMImage(image);
	return mt32emu_rc_ok; // No support for reverb ROM yet.
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
	mt32emu_return_code rc = mt32emu_rc_ok;
	FileStream *fs = new FileStream;
	if (fs->open(filename)) {
		if (fs->getData() != NULL) {
			rc = addROMFile(context, fs);
			if (rc > 0) return rc;
		} else {
			rc = mt32emu_rc_file_not_loaded;
		}
	} else {
		rc = mt32emu_rc_file_not_found;
	}
	delete fs;
	return rc;
}

} // extern "C"
