#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <QtGlobal>
#include <QList>
#include <QString>

class AudioDriver {
public:
	virtual ~AudioDriver() {};

	virtual bool start(int deviceIndex) = 0;
	virtual void close() = 0;
	virtual QList<QString> getDeviceNames();

	/**
	 * Returns the nanosecond currently playing in the audioOutput timebase, plus the latency.
	 * This value must:
	 * - Increase at a rate consistent with realtime on subsequent calls.
	 * - Be as near as possible to the earliest time for which an event can safely be scheduled to be certain that it will be played on time.
	 * - Use the same timebase as given to QSynth::render().
	 */
	// Will only be called after a successful start(). Hopefully.
	virtual qint64 getPlayedAudioNanosPlusLatency() = 0;
};

#endif
