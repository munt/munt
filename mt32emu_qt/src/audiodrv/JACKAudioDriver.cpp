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

#include <QMessageBox>

#include "JACKAudioDriver.h"

#include "../Master.h"
#include "../QSynth.h"
#include "../JACKClient.h"
#include "../QRingBuffer.h"

static const uint CHANNEL_COUNT = 2;
static const uint MINIMUM_JACK_BUFFER_COUNT = 2;
static const uint FRAME_BYTE_SIZE = sizeof(float[CHANNEL_COUNT]);

class JACKAudioProcessor : QThread {
	SynthRoute &synthRoute;
	Utility::QRingBuffer *buffer;
	volatile bool stopProcessing;

	// Used to block this thread until there is some available space in the buffer.
	// Each time the JACK thread retrieves some data from the buffer, it releases one semaphore resource.
	// When the buffer appears full, this thread drains all the resources available to the semaphore
	// yet acquire one more to ensure blocking.
	QSemaphore bufferDataRetrievals;

	// Used to pause the JACK buffer size callback thread while this thread is reallocating the buffer.
	QSemaphore bufferSizeUpdateLatch;
	// Indicates whether an update is pending, if non-zero.
	volatile quint32 pendingUpdateBufferSize;

	void run() {
		forever {
			// Catch the available resources early to avoid blocking should the JACK thread
			// free some buffer space in the meantime.
			int currentRetrievals = bufferDataRetrievals.available();
			if (stopProcessing) return;
			if (pendingUpdateBufferSize > 0) {
				// Getting here means an update is pending, reallocate the buffer.
				reallocateBuffer(pendingUpdateBufferSize);
				pendingUpdateBufferSize = 0;
				// Now, release the waiting JACK thread.
				bufferSizeUpdateLatch.release();
			}
			quint32 bytesAvailable;
			bool freeSpaceContiguous;
			float *writePointer = static_cast<float *>(buffer->writePointer(bytesAvailable, freeSpaceContiguous));
			quint32 framesToRender = quint32(bytesAvailable / FRAME_BYTE_SIZE);
			if (framesToRender == 0) {
				bufferDataRetrievals.acquire(currentRetrievals + 1);
			} else {
				synthRoute.render(writePointer, framesToRender);
				buffer->advanceWritePointer(framesToRender * FRAME_BYTE_SIZE);
			}
		}
	}

public:
	JACKAudioProcessor(SynthRoute &useSynthRoute) :
		synthRoute(useSynthRoute),
		buffer(),
		stopProcessing(),
		pendingUpdateBufferSize()
	{}

	~JACKAudioProcessor() {
		delete buffer;
	}

	void start() {
		QThread::start(QThread::TimeCriticalPriority);
	}

	void stop() {
		stopProcessing = true;
		bufferDataRetrievals.release();
		wait();
	}

	float *getAvailableChunk(quint32 &chunkSizeFrames) const {
		quint32 bytesAvailable;
		float *readPointer = static_cast<float *>(buffer->readPointer(bytesAvailable));
		quint32 framesAvailable = quint32(bytesAvailable / FRAME_BYTE_SIZE);
		chunkSizeFrames = qMin(chunkSizeFrames, framesAvailable);
		return readPointer;
	}

	void markChunkProcessed(quint32 chunkSizeFrames) {
		buffer->advanceReadPointer(chunkSizeFrames * FRAME_BYTE_SIZE);
		// The release operation may be lock-free that depends on the implementation,
		// but this seems to imply the least possible locking in the worst case
		// for the thread synchronisation to work correctly.
		bufferDataRetrievals.release();
	}

	void setBufferSize(quint32 bufferSizeFrames) {
		// First, notify the processor thread that an update is pending.
		pendingUpdateBufferSize = bufferSizeFrames;
		// Ensure that the processor thread awakes if awaiting for free space.
		bufferDataRetrievals.release();
		// Now, await for the processor thread to complete the buffer reallocation.
		bufferSizeUpdateLatch.acquire();
	}

	void reallocateBuffer(quint32 bufferSizeFrames) {
		delete buffer;
		// QRingBuffer needs a bit of spare space to accommodate the entire requested size.
		// Adding 1 FRAME_BYTE_SIZE does the trick yet ensures proper alignment of pointers.
		buffer = new Utility::QRingBuffer((bufferSizeFrames + 1) * FRAME_BYTE_SIZE);
	}
};

JACKAudioStream::JACKAudioStream(const AudioDriverSettings &useSettings, SynthRoute &useSynthRoute, const quint32 useSampleRate) :
	AudioStream(useSettings, useSynthRoute, useSampleRate),
	jackClient(new JACKClient),
	buffer(),
	processor(),
	configuredAudioLatencyFrames(audioLatencyFrames)
{}

JACKAudioStream::~JACKAudioStream() {
	stop();
	delete jackClient;
	delete[] buffer;
	delete processor;
}

bool JACKAudioStream::start(MidiSession *midiSession) {
	JACKClientState state = jackClient->open(midiSession, this);
	if (JACKClientState_OPEN != state) {
		qDebug() << "JACKAudioDriver: Failed to open JACK client connection";
		return false;
	}
	jackClient->connectToPhysicalPorts();

	const quint32 jackBufferSizeFrames = jackClient->getBufferSize();
	qDebug() << "JACKAudioDriver: JACK reported initial audio buffer size (frames / s):"
		<< jackBufferSizeFrames << "/" << double(jackBufferSizeFrames) / sampleRate;
	if (midiSession == NULL && jackClient->isRealtimeProcessing()) {
		// Use prerendering to prevent the realtime thread from locking, yet to retain complete functionality.
		// Additional latency of at least the JACK buffer length is introduced.
		if (audioLatencyFrames < jackBufferSizeFrames) audioLatencyFrames = jackBufferSizeFrames;
		processor = new JACKAudioProcessor(synthRoute);
		processor->reallocateBuffer(audioLatencyFrames);
		processor->start();
		qDebug() << "JACKAudioDriver: Configured prerendering audio buffer size (frames / s):"
			<< audioLatencyFrames << "/" << double(audioLatencyFrames) / sampleRate;
	} else {
		// Rendering is synchronous, zero additional latency introduced.
		audioLatencyFrames = 0;
		buffer = new float[CHANNEL_COUNT * MT32Emu::MAX_SAMPLES_PER_RUN];
	}

	if (midiSession == NULL) {
		// Setup initial MIDI latency
		if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames + MINIMUM_JACK_BUFFER_COUNT * jackBufferSizeFrames;
		qDebug() << "JACKAudioDriver: Configured MIDI latency (frames / s):" << midiLatencyFrames
			<< "/" << double(midiLatencyFrames) / sampleRate;
	} else {
		// MIDI processing is synchronous, zero latency introduced
		midiLatencyFrames = 0;
		qDebug() << "JACKAudioDriver: Configured synchronous MIDI processing";
		if (jackClient->isRealtimeProcessing()) synthRoute.enableRealtimeMode();
	}

	return true;
}

void JACKAudioStream::stop() {
	qDebug() << "JACKAudioDriver: Stopping JACK client";
	jackClient->close();
	qDebug() << "JACKAudioDriver: JACK client stopped";
	if (processor != NULL) {
		processor->stop();
	}
}

bool JACKAudioStream::checkSampleRate(quint32 jackSampleRate) const {
	if (jackSampleRate == sampleRate) return true;
	qDebug() << "JACKAudioDriver: Sample rate mismatch: configured rate / JACK system rate:" << sampleRate
		<< "/" << jackSampleRate;
	QMessageBox::warning(NULL, "Error", "Sample rate configured for the synth doesn't match the JACK system sample rate");
	return false;
}

void JACKAudioStream::onJACKBufferSizeChange(const quint32 newBufferSize) {
	bool reallocationNeeded = processor != NULL;
	qDebug() << "JACKAudioDriver: JACK reported new buffer size" << newBufferSize
		<< (reallocationNeeded ? "reallocating buffer..." : "ignored");
	if (!reallocationNeeded) return;
	audioLatencyFrames = qMax(newBufferSize, configuredAudioLatencyFrames);
	processor->setBufferSize(audioLatencyFrames);
	qDebug() << "JACKAudioDriver: Reconfigured prerendering audio buffer size (frames / s):"
		<< audioLatencyFrames << "/" << double(audioLatencyFrames) / sampleRate;
}

void JACKAudioStream::onJACKShutdown() {
	qDebug() << "JACKAudioDriver: JACK server is shutting down, closing synth";
	synthRoute.audioStreamFailed();
}

void JACKAudioStream::renderStreams(const quint32 totalFrameCount, JACKAudioSample *leftOutBuffer, JACKAudioSample *rightOutBuffer) {
	// Only bother with updating TimeInfo when MIDI processing is asynchronous
	if (midiLatencyFrames != 0) {
		quint32 framesInAudioBuffer;
		if (settings.advancedTiming) {
			framesInAudioBuffer = qMin(0U, jackClient->getBufferSize() - jackClient->getFramesSinceCycleStart());
		} else {
			framesInAudioBuffer = 0U;
		}
		updateTimeInfo(MasterClock::getClockNanos(), framesInAudioBuffer);
	}
	for (quint32 framesLeft = totalFrameCount; framesLeft > 0;) {
		float *bufferPtr;
		uint framesToRender;
		if (processor != NULL) {
			framesToRender = framesLeft;
			bufferPtr = processor->getAvailableChunk(framesToRender);
			if (framesToRender == 0) {
				for (JACKAudioSample *leftOutBufferEnd = leftOutBuffer + framesLeft; leftOutBuffer < leftOutBufferEnd;) {
					*(leftOutBuffer++) = 0;
					*(rightOutBuffer++) = 0;
				}
				return;
			}
		} else {
			bufferPtr = buffer;
			framesToRender = qMin(framesLeft, MT32Emu::MAX_SAMPLES_PER_RUN);
			synthRoute.render(buffer, framesToRender);
		}
		for (JACKAudioSample *leftOutBufferEnd = leftOutBuffer + framesToRender; leftOutBuffer < leftOutBufferEnd;) {
			*(leftOutBuffer++) = JACKAudioSample(*(bufferPtr++));
			*(rightOutBuffer++) = JACKAudioSample(*(bufferPtr++));
		}
		if (processor != NULL) processor->markChunkProcessed(framesToRender);
		framesLeft -= framesToRender;
	}
	framesRendered(totalFrameCount);
}

JACKAudioDefaultDevice::JACKAudioDefaultDevice(JACKAudioDriver &useDriver) :
	AudioDevice(useDriver, "Default")
{}

AudioStream *JACKAudioDefaultDevice::startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const {
	return startAudioStream(this, synthRoute, sampleRate, NULL);
}

AudioStream *JACKAudioDefaultDevice::startAudioStream(const AudioDevice *audioDevice, SynthRoute &synthRoute, const uint sampleRate, MidiSession *midiSession) {
	JACKAudioStream *stream = new JACKAudioStream(audioDevice->driver.getAudioSettings(), synthRoute, sampleRate);
	if (stream->start(midiSession)) return stream;
	delete stream;
	return NULL;
}

JACKAudioDriver::JACKAudioDriver(Master *useMaster) : AudioDriver("jackaudio", "JACKAudio") {
	Q_UNUSED(useMaster)

	loadAudioSettings();
}

const QList<const AudioDevice *> JACKAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new JACKAudioDefaultDevice(*this));
	return deviceList;
}

void JACKAudioDriver::validateAudioSettings(AudioDriverSettings &newSettings) const {
	newSettings.chunkLen = 0;
}
