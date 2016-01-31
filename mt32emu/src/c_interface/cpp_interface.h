/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2016 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#ifndef MT32EMU_CPP_INTERFACE_H
#define MT32EMU_CPP_INTERFACE_H

#include <cstdarg>

#include "../globals.h"
#include "c_types.h"

#include "../Types.h"
#include "../Enumerations.h"

namespace MT32Emu {

static const mt32emu_report_handler_i NULL_REPORT_HANDLER = { NULL };

/**
 * The class below correspond to the mt32emu_service_i interface defined in c_types.h and provided for convenience when using C++.
 * The approach used makes no assumption of any internal class data memory layout, since the C++ standard does not
 * provide any detail in this area and leaves it up to the implementation. Therefore, this way portability is guaranteed,
 * despite the implementation may be a little inefficient.
 * See c_types.h and c_interface.h for description of the corresponding interface methods.
 */
class Service {
public:
	static void setInterface(mt32emu_service_i interface) { i = interface; }

	static mt32emu_service_version getVersionID() { return i.v0->getVersionID(i); }
	static mt32emu_report_handler_version getSupportedReportHandlerVersionID() { return i.v0->getSupportedReportHandlerVersionID(); }
	static mt32emu_midi_receiver_version getSupportedMIDIReceiverVersionID() { return i.v0->getSupportedMIDIReceiverVersionID(); }

	static Bit32u getLibraryVersionInt() { return i.v0->getLibraryVersionInt(); }
	static const char *getLibraryVersionString() { return i.v0->getLibraryVersionString(); }

	static Bit32u getStereoOutputSamplerate(const AnalogOutputMode analog_output_mode) { return i.v0->getStereoOutputSamplerate(static_cast<mt32emu_analog_output_mode>(analog_output_mode)); }

	explicit Service(mt32emu_context context = NULL) : c(context) {}
	~Service() { if (c != NULL) i.v0->freeContext(c); }

	mt32emu_context getContext() { return c; }
	void createContext(mt32emu_report_handler_i report_handler = NULL_REPORT_HANDLER, void *instanceData = NULL) { if (c != NULL) i.v0->freeContext(c); c = i.v0->createContext(report_handler, instanceData); }
	void freeContext() { if (c != NULL) { i.v0->freeContext(c); c = NULL; } }
	mt32emu_return_code addROMData(const Bit8u *data, size_t data_size, const mt32emu_sha1_digest *sha1_digest = NULL) { return i.v0->addROMData(c, data, data_size, sha1_digest); }
	mt32emu_return_code addROMFile(const char *filename) { return i.v0->addROMFile(c, filename); }
	void getROMInfo(mt32emu_rom_info *rom_info) { i.v0->getROMInfo(c, rom_info); }
	void setPartialCount(const Bit32u partial_count) { i.v0->setPartialCount(c, partial_count); }
	void setAnalogOutputMode(const AnalogOutputMode analog_output_mode) { i.v0->setAnalogOutputMode(c, static_cast<mt32emu_analog_output_mode>(analog_output_mode)); }
	mt32emu_return_code openSynth() { return i.v0->openSynth(c); }
	void closeSynth() { i.v0->closeSynth(c); }
	mt32emu_boolean isOpen() { return i.v0->isOpen(c); }
	Bit32u getActualStereoOutputSamplerate() { return i.v0->getActualStereoOutputSamplerate(c); }
	void flushMIDIQueue() { i.v0->flushMIDIQueue(c); }
	Bit32u setMIDIEventQueueSize(const Bit32u queue_size) { return i.v0->setMIDIEventQueueSize(c, queue_size); }
	void setMIDIReceiver(mt32emu_midi_receiver_i midi_receiver, void *instanceData) { i.v0->setMIDIReceiver(c, midi_receiver, instanceData); }

	void parseStream(const Bit8u *stream, Bit32u length) { i.v0->parseStream(c, stream, length); }
	void parseStream_At(const Bit8u *stream, Bit32u length, Bit32u timestamp) { i.v0->parseStream_At(c, stream, length, timestamp); }
	void playShortMessage(Bit32u message) { i.v0->playShortMessage(c, message); }
	void playShortMessageAt(Bit32u message, Bit32u timestamp) { i.v0->playShortMessageAt(c, message, timestamp); }
	mt32emu_return_code playMsg(Bit32u msg) { return i.v0->playMsg(c, msg); }
	mt32emu_return_code playSysex(const Bit8u *sysex, Bit32u len) { return i.v0->playSysex(c, sysex, len); }
	mt32emu_return_code playMsgAt(Bit32u msg, Bit32u timestamp) { return i.v0->playMsgAt(c, msg, timestamp); }
	mt32emu_return_code playSysexAt(const Bit8u *sysex, Bit32u len, Bit32u timestamp) { return i.v0->playSysexAt(c, sysex, len, timestamp); }

	void playMsgNow(Bit32u msg) { i.v0->playMsgNow(c, msg); }
	void playMsgOnPart(Bit8u part, Bit8u code, Bit8u note, Bit8u velocity) { i.v0->playMsgOnPart(c, part, code, note, velocity); }
	void playSysexNow(const Bit8u *sysex, Bit32u len) { i.v0->playSysexNow(c, sysex, len); }
	void writeSysex(Bit8u channel, const Bit8u *sysex, Bit32u len) { i.v0->writeSysex(c, channel, sysex, len); }

	void setReverbEnabled(const mt32emu_boolean reverb_enabled) { i.v0->setReverbEnabled(c, reverb_enabled); }
	mt32emu_boolean isReverbEnabled() { return i.v0->isReverbEnabled(c); }
	void setReverbOverridden(const mt32emu_boolean reverbOverridden) { i.v0->setReverbOverridden(c, reverbOverridden); }
	mt32emu_boolean isReverbOverridden() { return i.v0->isReverbOverridden(c); }
	void setReverbCompatibilityMode(const mt32emu_boolean mt32_compatible_mode) { i.v0->setReverbCompatibilityMode(c, mt32_compatible_mode); }
	mt32emu_boolean isMT32ReverbCompatibilityMode() { return i.v0->isMT32ReverbCompatibilityMode(c); }
	mt32emu_boolean isDefaultReverbMT32Compatible() { return i.v0->isDefaultReverbMT32Compatible(c); }

	void setDACInputMode(const DACInputMode mode) { i.v0->setDACInputMode(c, static_cast<mt32emu_dac_input_mode>(mode)); }
	DACInputMode getDACInputMode() { return static_cast<DACInputMode>(i.v0->getDACInputMode(c)); }

	void setMIDIDelayMode(const MIDIDelayMode mode) { i.v0->setMIDIDelayMode(c, static_cast<mt32emu_midi_delay_mode>(mode)); }
	MIDIDelayMode getMIDIDelayMode() { return static_cast<MIDIDelayMode>(i.v0->getMIDIDelayMode(c)); }

	void setOutputGain(float gain) { i.v0->setOutputGain(c, gain); }
	float getOutputGain() { return i.v0->getOutputGain(c); }
	void setReverbOutputGain(float gain) { i.v0->setReverbOutputGain(c, gain); }
	float getReverbOutputGain() { return i.v0->getReverbOutputGain(c); }

	void setReversedStereoEnabled(const mt32emu_boolean enabled) { i.v0->setReversedStereoEnabled(c, enabled); }
	mt32emu_boolean isReversedStereoEnabled() { return i.v0->isReversedStereoEnabled(c); }

	void renderBit16s(Bit16s *stream, Bit32u len) { i.v0->renderBit16s(c, stream, len); }
	void renderFloat(float *stream, Bit32u len) { i.v0->renderFloat(c, stream, len); }
	void renderBit16sStreams(const mt32emu_dac_output_bit16s_streams *streams, Bit32u len) { i.v0->renderBit16sStreams(c, streams, len); }
	void renderFloatStreams(const mt32emu_dac_output_float_streams *streams, Bit32u len) { i.v0->renderFloatStreams(c, streams, len); }

	mt32emu_boolean hasActivePartials() { return i.v0->hasActivePartials(c); }
	mt32emu_boolean isActive() { return i.v0->isActive(c); }
	Bit32u getPartialCount() { return i.v0->getPartialCount(c); }
	Bit32u getPartStates() { return i.v0->getPartStates(c); }
	void getPartialStates(Bit8u *partial_states) { i.v0->getPartialStates(c, partial_states); }
	Bit32u getPlayingNotes(Bit8u part_number, Bit8u *keys, Bit8u *velocities) { return i.v0->getPlayingNotes(c, part_number, keys, velocities); }
	const char *getPatchName(Bit8u part_number) { return i.v0->getPatchName(c, part_number); }
	void readMemory(Bit32u addr, Bit32u len, Bit8u *data) { i.v0->readMemory(c, addr, len, data); }

private:
	static mt32emu_service_i i;

	mt32emu_context c;
};

mt32emu_service_i Service::i;

} // namespace MT32Emu

#endif /* #ifndef MT32EMU_CPP_INTERFACE_H */
