/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

#include "QtAudioDriver.h"

#ifdef USE_QT_MULTIMEDIAKIT
#include <QtMultimediaKit/QAudioOutput>
#else
#include <QtMultimedia/QAudioOutput>
#endif

#include <mt32emu/mt32emu.h>

#include "../ClockSync.h"
#include "../Master.h"
#include "../QSynth.h"

using namespace MT32Emu;

class WaveGenerator : public QIODevice {
private:
	ClockSync clockSync;
	QSynth *synth;
	QAudioOutput *audioOutput;
	double sampleRate;
	qint64 samplesCount;
	MasterClockNanos audioLatency;
	MasterClockNanos midiLatency;
	MasterClockNanos lastSampleMasterClockNanos;
	bool advancedTiming;

public:
	WaveGenerator(QSynth *useSynth, QAudioOutput *useAudioOutput, double sampleRate, bool useAdvancedTiming) : synth(useSynth), audioOutput(useAudioOutput), sampleRate(sampleRate), samplesCount(0), audioLatency(0), midiLatency(0), advancedTiming(useAdvancedTiming) {
		open(QIODevice::ReadOnly | QIODevice::Unbuffered);
	}

	void setLatency(MasterClockNanos useMIDILatency) {
		audioLatency = MasterClock::NANOS_PER_SECOND * audioOutput->bufferSize() / (4 * sampleRate);
		MasterClockNanos chunkSize = MasterClock::NANOS_PER_SECOND * audioOutput->periodSize() / (4 * sampleRate);
		qDebug() << "QAudioDriver: Latency set to:" << (double)audioLatency / MasterClock::NANOS_PER_SECOND << "sec." << "Chunk size:" << (double)chunkSize / MasterClock::NANOS_PER_SECOND;
		if (useMIDILatency == 0)
			midiLatency = 2 * chunkSize;
		else
			midiLatency = useMIDILatency;
		qDebug() << "QAudioDriver: MIDI latency set to:" << (double)midiLatency / MasterClock::NANOS_PER_SECOND << "sec";
		lastSampleMasterClockNanos = MasterClock::getClockNanos() - audioLatency - midiLatency;
	}

	qint64 readData(char *data, qint64 len) {
		MasterClockNanos firstSampleMasterClockNanos;
		double realSampleRate;
		unsigned int framesToRender = (unsigned int)(len >> 2);
		if (advancedTiming) {
			MasterClockNanos nanosInBuffer = MasterClock::NANOS_PER_SECOND * (audioOutput->bufferSize() - audioOutput->bytesFree()) / (4 * sampleRate);
			firstSampleMasterClockNanos = MasterClock::getClockNanos() + nanosInBuffer - audioLatency - midiLatency;
			realSampleRate = AudioStream::estimateActualSampleRate(sampleRate, firstSampleMasterClockNanos, lastSampleMasterClockNanos, audioLatency, framesToRender);
		} else {
			MasterClockNanos firstSampleAudioNanos = MasterClock::NANOS_PER_SECOND * samplesCount / sampleRate;
			firstSampleMasterClockNanos = clockSync.sync(firstSampleAudioNanos) - midiLatency;
			realSampleRate = sampleRate / clockSync.getDrift();
			samplesCount += len >> 2;
		}
		return synth->render((Bit16s *)data, framesToRender, firstSampleMasterClockNanos, realSampleRate) << 2;
	}

	qint64 writeData(const char *data, qint64 len) {
		Q_UNUSED(data);
		Q_UNUSED(len);
		return 0;
	}
};

QtAudioStream::QtAudioStream(const AudioDevice *device, QSynth *useSynth, unsigned int useSampleRate) : sampleRate(useSampleRate) {
	QAudioFormat format;
	format.setFrequency(sampleRate);
	format.setChannels(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	// libmt32emu produces samples in native byte order
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	format.setByteOrder(QAudioFormat::LittleEndian);
#else
	format.setByteOrder(QAudioFormat::BigEndian);
#endif
	format.setSampleType(QAudioFormat::SignedInt);

	unsigned int unused;

	device->driver->getAudioSettings(&unused, &audioLatency, &midiLatency, &advancedTiming);

	audioOutput = new QAudioOutput(format);
	waveGenerator = new WaveGenerator(useSynth, audioOutput, sampleRate, advancedTiming);
}

QtAudioStream::~QtAudioStream() {
	delete audioOutput;
	delete waveGenerator;
}

bool QtAudioStream::start() {
	if (audioLatency != 0) audioOutput->setBufferSize(0.004 * sampleRate * audioLatency);
	audioOutput->start(waveGenerator);
	waveGenerator->setLatency(midiLatency * MasterClock::NANOS_PER_MILLISECOND);
	return true;
}

void QtAudioStream::close() {
	audioOutput->stop();
}

QtAudioDefaultDevice::QtAudioDefaultDevice(const QtAudioDriver *driver) : AudioDevice(driver, "default", "Default") {
}

QtAudioStream *QtAudioDefaultDevice::startAudioStream(QSynth *synth, unsigned int sampleRate) const {
	QtAudioStream *stream = new QtAudioStream(this, synth, sampleRate);
	if (stream->start()) {
		return stream;
	}
	delete stream;
	return NULL;
}

QtAudioDriver::QtAudioDriver(Master *useMaster) : AudioDriver("qtaudio", "QtAudio") {
	Q_UNUSED(useMaster);
	loadAudioSettings();
}

QtAudioDriver::~QtAudioDriver() {
}

QList<AudioDevice *> QtAudioDriver::getDeviceList() const {
	QList<AudioDevice *> deviceList;
	deviceList.append(new QtAudioDefaultDevice(this));
	return deviceList;
}

void QtAudioDriver::validateAudioSettings() {
	chunkLen = audioLatency / 5;
	if (midiLatency < chunkLen) midiLatency = chunkLen;
}
