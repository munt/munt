#ifndef MIDI_EVENTQUEUE_H
#define MIDI_EVENTQUEUE_H

#include <QMutex>
#include "MidiEvent.h"

class MidiEventQueue {
private:
	static const unsigned int maxPos = 1024;
	QMutex mutex;
	unsigned int startPos;
	unsigned int endPos;
	MidiEvent ringBuffer[maxPos];
public:
	MidiEventQueue();

	bool pushEvent(SynthTimestamp timestamp, MT32Emu::Bit32u msg, MT32Emu::Bit8u *sysexData, unsigned int sysexLen);
	const MidiEvent *popEvent();
	const MidiEvent *peekEvent();
	const MidiEvent *peekEventAt(unsigned int pos);
};

#endif
