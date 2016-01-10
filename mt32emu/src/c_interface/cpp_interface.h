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

#ifndef MT32EMU_CPP_INTERFACE_H
#define MT32EMU_CPP_INTERFACE_H

#include <cstdarg>

#include "../globals.h"
#include "c_types.h"
#include "c_interface.h"

#include "../Types.h"
#include "../Enumerations.h"

namespace MT32Emu {

/**
 * The class below correspond to the mt32emu_synth_i interface defined in c_types.h and provided for convenience when using C++.
 * The approach used makes no assumption of any internal class data memory layout, since the C++ standard does not
 * provide any detail in this area and leaves it up to the implementation. Therefore, this way portability is guaranteed,
 * despite the implementation may be a little inefficient.
 * See c_types.h and c_interface.h for description of the corresponding interface methods.
 */
class Service {
public:
	explicit Service(mt32emu_context c) : c(c) {}
	Service(mt32emu_context (*factory)(const mt32emu_report_handler_i *report_handler), const mt32emu_report_handler_i *report_handler) { c = factory(report_handler); }
	~Service() { if (c.d != NULL) c.i->v0->freeContext(c); }

	void freeContext() { if (c.d != NULL) { c.i->v0->freeContext(c); c.d = NULL; } }
	Bit32u getLibraryVersionInt() { return c.i->v0->getLibraryVersionInt(); }
	const char *getLibraryVersionString() { return c.i->v0->getLibraryVersionString(); }
	mt32emu_return_code addROMData(const Bit8u *data, size_t data_size, const mt32emu_sha1_digest *sha1_digest) { return c.i->v0->addROMData(c, data, data_size, sha1_digest); }
	mt32emu_return_code addROMFile(const char *filename) { return c.i->v0->addROMFile(c, filename); }
	void getROMInfo(mt32emu_rom_info *rom_info) { c.i->v0->getROMInfo(c.d, rom_info); }
	mt32emu_return_code openSynth(const Bit32u *partial_count, const mt32emu_analog_output_mode *analog_output_mode) { return c.i->v0->openSynth(c.d, partial_count, analog_output_mode); }
	void closeSynth() { c.i->v0->closeSynth(c.d); }
	mt32emu_boolean isOpen() { return c.i->v0->isOpen(c.d); }
	Bit32u getStereoOutputSamplerate(const mt32emu_analog_output_mode analog_output_mode) { return c.i->v0->getStereoOutputSamplerate(analog_output_mode); }
	Bit32u getActualStereoOutputSamplerate() { return c.i->v0->getActualStereoOutputSamplerate(c.d); }
	void flushMIDIQueue() { c.i->v0->flushMIDIQueue(c.d); }
	Bit32u setMIDIEventQueueSize(const Bit32u queue_size) { return c.i->v0->setMIDIEventQueueSize(c.d, queue_size); }
	mt32emu_midi_receiver_version setMIDIReceiver(const mt32emu_midi_receiver_i *midi_receiver) { return c.i->v0->setMIDIReceiver(c.d, midi_receiver); }

	void parseStream(const Bit8u *stream, Bit32u length) { c.i->v0->parseStream(c.d, stream, length); }
	void parseStream_At(const Bit8u *stream, Bit32u length, Bit32u timestamp) { c.i->v0->parseStream_At(c.d, stream, length, timestamp); }
	void playShortMessage(Bit32u message) { c.i->v0->playShortMessage(c.d, message); }
	void playShortMessageAt(Bit32u message, Bit32u timestamp) { c.i->v0->playShortMessageAt(c.d, message, timestamp); }
	mt32emu_return_code playMsg(Bit32u msg) { return c.i->v0->playMsg(c.d, msg); }
	mt32emu_return_code playSysex(const Bit8u *sysex, Bit32u len) { return c.i->v0->playSysex(c.d, sysex, len); }
	mt32emu_return_code playMsgAt(Bit32u msg, Bit32u timestamp) { return c.i->v0->playMsgAt(c.d, msg, timestamp); }
	mt32emu_return_code playSysexAt(const Bit8u *sysex, Bit32u len, Bit32u timestamp) { return c.i->v0->playSysexAt(c.d, sysex, len, timestamp); }

	void playMsgNow(Bit32u msg) { c.i->v0->playMsgNow(c.d, msg); }
	void playMsgOnPart(Bit8u part, Bit8u code, Bit8u note, Bit8u velocity) { c.i->v0->playMsgOnPart(c.d, part, code, note, velocity); }
	void playSysexNow(const Bit8u *sysex, Bit32u len) { c.i->v0->playSysexNow(c.d, sysex, len); }
	void writeSysex(Bit8u channel, const Bit8u *sysex, Bit32u len) { c.i->v0->writeSysex(c.d, channel, sysex, len); }

	void setReverbEnabled(const mt32emu_boolean reverb_enabled) { c.i->v0->setReverbEnabled(c.d, reverb_enabled); }
	mt32emu_boolean isReverbEnabled() { return c.i->v0->isReverbEnabled(c.d); }
	void setReverbOverridden(const mt32emu_boolean reverbOverridden) { c.i->v0->setReverbOverridden(c.d, reverbOverridden); }
	mt32emu_boolean isReverbOverridden() { return c.i->v0->isReverbOverridden(c.d); }
	void setReverbCompatibilityMode(const mt32emu_boolean mt32_compatible_mode) { c.i->v0->setReverbCompatibilityMode(c.d, mt32_compatible_mode); }
	mt32emu_boolean isMT32ReverbCompatibilityMode() { return c.i->v0->isMT32ReverbCompatibilityMode(c.d); }
	mt32emu_boolean isDefaultReverbMT32Compatible() { return c.i->v0->isDefaultReverbMT32Compatible(c.d); }

	void setDACInputMode(const mt32emu_dac_input_mode mode) { c.i->v0->setDACInputMode(c.d, mode); }
	mt32emu_dac_input_mode getDACInputMode() { return c.i->v0->getDACInputMode(c.d); }

	void setMIDIDelayMode(const mt32emu_midi_delay_mode mode) { c.i->v0->setMIDIDelayMode(c.d, mode); }
	mt32emu_midi_delay_mode getMIDIDelayMode() { return c.i->v0->getMIDIDelayMode(c.d); }

	void setOutputGain(float gain) { c.i->v0->setOutputGain(c.d, gain); }
	float getOutputGain() { return c.i->v0->getOutputGain(c.d); }
	void setReverbOutputGain(float gain) { c.i->v0->setReverbOutputGain(c.d, gain); }
	float getReverbOutputGain() { return c.i->v0->getReverbOutputGain(c.d); }

	void setReversedStereoEnabled(const mt32emu_boolean enabled) { c.i->v0->setReversedStereoEnabled(c.d, enabled); }
	mt32emu_boolean isReversedStereoEnabled() { return c.i->v0->isReversedStereoEnabled(c.d); }

	void renderBit16s(Bit16s *stream, Bit32u len) { c.i->v0->renderBit16s(c.d, stream, len); }
	void renderFloat(float *stream, Bit32u len) { c.i->v0->renderFloat(c.d, stream, len); }
	void renderBit16sStreams(const mt32emu_dac_output_bit16s_streams *streams, Bit32u len) { c.i->v0->renderBit16sStreams(c.d, streams, len); }
	void renderFloatStreams(const mt32emu_dac_output_float_streams *streams, Bit32u len) { c.i->v0->renderFloatStreams(c.d, streams, len); }

	mt32emu_boolean hasActivePartials() { return c.i->v0->hasActivePartials(c.d); }
	mt32emu_boolean isActive() { return c.i->v0->isActive(c.d); }
	Bit32u getPartialCount() { return c.i->v0->getPartialCount(c.d); }
	Bit32u getPartStates() { return c.i->v0->getPartStates(c.d); }
	void getPartialStates(Bit8u *partial_states) { c.i->v0->getPartialStates(c.d, partial_states); }
	Bit32u getPlayingNotes(Bit8u part_number, Bit8u *keys, Bit8u *velocities) { return c.i->v0->getPlayingNotes(c.d, part_number, keys, velocities); }
	const char *getPatchName(Bit8u part_number) { return c.i->v0->getPatchName(c.d, part_number); }
	void readMemory(Bit32u addr, Bit32u len, Bit8u *data) { c.i->v0->readMemory(c.d, addr, len, data); }
	mt32emu_report_handler_version getSupportedReportHandlerVersionID() { return c.i->v0->getSupportedReportHandlerVersionID(); }

private:
	mt32emu_context c;
};

} // namespace MT32Emu

#endif /* #ifndef MT32EMU_CPP_INTERFACE_H */
