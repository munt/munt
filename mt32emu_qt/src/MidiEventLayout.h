/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
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

#ifndef MIDI_EVENT_LAYOUT_H
#define MIDI_EVENT_LAYOUT_H

#include "QtGlobal"

// Utility classes that facilitate laying out a stream of timestamped MIDI messages in a byte buffer.
namespace MidiEventLayout {

enum MidiEventType {
	MidiEventType_SHORT_MESSAGE,
	MidiEventType_SYSEX_MESSAGE,
	MidiEventType_PAD
};

struct MidiEventHeader {
	MidiEventType eventType;
};

template <class T>
struct ShortMessageEntry : MidiEventHeader {
	quint32 shortMessageData;
	T timestamp;
};

template <class T>
struct SysexMessageHeader : MidiEventHeader {
	quint32 sysexDataLength;
	T timestamp;
	// Raw SysEx data of sysexDataLength bytes follows.
};

// The allocated SysEx data length may be rounded, so that events are laid out aligned.
static inline quint32 alignSysexDataLength(quint32 dataLength, quint32 alignment) {
	dataLength += alignment - 1;
	return dataLength - (dataLength % alignment);
}

} // namespace MidiEventLayout

#endif // MIDI_EVENT_LAYOUT_H
