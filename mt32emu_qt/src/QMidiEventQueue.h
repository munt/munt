#ifndef QMIDI_EVENTQUEUE_H
#define QMIDI_EVENTQUEUE_H

#include <QMutex>
#include "QMidiEvent.h"

class QMidiEventQueue {
private:
	static const unsigned int maxPos = 1024;
	QMutex mutex;
	unsigned int startPos;
	unsigned int endPos;
	QMidiEvent ringBuffer[maxPos];
public:
	QMidiEventQueue();

	bool pushEvent(SynthTimestamp timestamp, MT32Emu::Bit32u msg, MT32Emu::Bit8u *sysexData, unsigned int sysexLen);
	const QMidiEvent *popEvent();
	const QMidiEvent *peekEvent();
	const QMidiEvent *peekEventAt(unsigned int pos);
};

#endif
