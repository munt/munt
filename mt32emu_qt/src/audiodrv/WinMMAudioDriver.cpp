/* Copyright (C) 2011-2017 Jerome Fisher, Sergey V. Mikayev
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

#include "WinMMAudioDriver.h"
#include "../QSynth.h"
#include "../Master.h"

using namespace MT32Emu;

// Looks resonable as KMixer pulls data by 10 ms chunks
static const DWORD DEFAULT_CHUNK_MS = 10;
// SergM: 100 ms output latency is safe on most systems.
static const DWORD DEFAULT_AUDIO_LATENCY = 100;
// Stereo, 16-bit samples
static const DWORD FRAME_SIZE = 4;
// Latency for MIDI processing. 15 ms is the offset of interprocess timeGetTime() difference.
static const DWORD DEFAULT_MIDI_LATENCY = 15;

WinMMAudioStream::WinMMAudioStream(const AudioDriverSettings &useSettings, bool useRingBufferMode, QSynth &useSynth, const uint useSampleRate) :
	AudioStream(useSettings, useSynth, useSampleRate),
	hWaveOut(NULL), waveHdr(NULL), hEvent(NULL), hWaitableTimer(NULL), stopProcessing(false),
	processor(*this), ringBufferMode(useRingBufferMode), prevPlayPosition(0L)
{
	chunkSize = (settings.chunkLen * sampleRate) / MasterClock::MILLIS_PER_SECOND;
	if (ringBufferMode) {
		numberOfChunks = 1;
		qDebug() << "WinMMAudioDriver: Using looped ring buffer, buffer size:" << audioLatencyFrames << "frames, min. rendering interval:" << settings.chunkLen << "ms.";
	} else {
		// Number of chunks should be ceil(bufferSize / chunkSize)
		numberOfChunks = (audioLatencyFrames + chunkSize - 1) / chunkSize;
		// Refine bufferSize as chunkSize * number of chunks, no less then the specified value
		audioLatencyFrames = numberOfChunks * chunkSize;
		qDebug() << "WinMMAudioDriver: Using" << numberOfChunks << "chunks, chunk size:" << chunkSize << "frames, buffer size:" << audioLatencyFrames << "frames.";
	}
	buffer = new Bit16s[2 * audioLatencyFrames];
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames + ((DEFAULT_MIDI_LATENCY * sampleRate) / MasterClock::MILLIS_PER_SECOND);
}

WinMMAudioStream::~WinMMAudioStream() {
	if (hWaveOut != NULL) {
		close();
	}
	delete[] buffer;
}

DWORD WinMMAudioStream::getCurrentPlayPosition() {
	static const uint WRAP_BITS = 27;
	static const quint64 WRAP_MASK = (1 << WRAP_BITS) - 1;
	static const int WRAP_THRESHOLD = 1 << (WRAP_BITS - 1);

	DWORD wrapCount = DWORD(prevPlayPosition >> WRAP_BITS);
	DWORD wrappedPosition = DWORD(prevPlayPosition & WRAP_MASK);

	MMTIME mmTime;
	mmTime.wType = TIME_SAMPLES;

	if (waveOutGetPosition(hWaveOut, &mmTime, sizeof(MMTIME)) != MMSYSERR_NOERROR) {
		qDebug() << "WinMMAudioDriver: waveOutGetPosition failed, thread stopped";
		return (DWORD)-1;
	}
	if (mmTime.wType != TIME_SAMPLES) {
		qDebug() << "WinMMAudioDriver: Failed to get # of samples played";
		return (DWORD)-1;
	}
	mmTime.u.sample &= WRAP_MASK;

	// Deal with waveOutGetPosition() wraparound. For 16-bit stereo output, it equals 2^27,
	// presumably caused by the internal 32-bit counter of bits played.
	// The output of that nasty waveOutGetPosition() isn't monotonically increasing
	// even during 2^27 samples playback, so we have to ensure the difference is big enough...
	int delta = mmTime.u.sample - wrappedPosition;
	if (delta < -WRAP_THRESHOLD) {
		qDebug() << "WinMMAudioDriver: GetPos() wrap:" << delta;
		++wrapCount;
	} else if (delta < 0) {
		// This ensures the return is monotonically increased
		qDebug() << "WinMMAudioDriver: GetPos() went back by:" << delta << "samples";
		return DWORD(prevPlayPosition % audioLatencyFrames);
	}
	prevPlayPosition = mmTime.u.sample + (wrapCount << WRAP_BITS);
	return DWORD(prevPlayPosition % audioLatencyFrames);
}

WinMMAudioProcessor::WinMMAudioProcessor(WinMMAudioStream &stream) : stream(stream) {}

void WinMMAudioProcessor::run() {
	const double samplePeriod = (double)MasterClock::NANOS_PER_SECOND / (double)stream.sampleRate;
	while (!stream.stopProcessing) {
		const DWORD playCursor = stream.getCurrentPlayPosition();
		if (playCursor == (DWORD)-1) {
			stream.stopProcessing = true;
			stream.synth.close();
			return;
		}

		MasterClockNanos nanosNow = MasterClock::getClockNanos();
		DWORD frameCount = 0;
		DWORD renderPos = DWORD(stream.renderedFramesCount % stream.audioLatencyFrames);
		Bit16s *buf = NULL;
		WAVEHDR *waveHdr = NULL;
		if (stream.ringBufferMode) {
			if (playCursor < renderPos) {
				// Buffer wrap, render 'till the end of buffer
				frameCount = stream.audioLatencyFrames - renderPos;
			} else {
				frameCount = playCursor - renderPos;
				if (frameCount < stream.chunkSize) {
					MasterClockNanos nanos = MasterClockNanos((stream.chunkSize - frameCount) * samplePeriod);
					if (NULL != stream.hWaitableTimer) {
						LARGE_INTEGER dueTime;
						dueTime.QuadPart = -qMax(1LL, nanos / 100LL);
						if (SetWaitableTimer(stream.hWaitableTimer, &dueTime, 0, NULL, NULL, 0) != FALSE) {
							if (WaitForSingleObject(stream.hWaitableTimer, INFINITE) == WAIT_OBJECT_0) continue;
						}
						qDebug() << "Waitable timer failed, falling back to Sleep()" << GetLastError();
						CloseHandle(stream.hWaitableTimer);
						stream.hWaitableTimer = NULL;
					}
					MasterClock::sleepForNanos(nanos);
					continue;
				}
			}
			buf = stream.buffer + (renderPos << 1);
		} else {
			bool allBuffersRendered = true;
			for (uint i = 0; i < stream.numberOfChunks; i++) {
				if (stream.waveHdr[i].dwFlags & WHDR_DONE) {
					allBuffersRendered = false;
					waveHdr = &stream.waveHdr[i];
					buf = (Bit16s *)waveHdr->lpData;
					frameCount = stream.chunkSize;
				}
			}
			if (allBuffersRendered) {
				WaitForSingleObject(stream.hEvent, INFINITE);
				continue;
			}
		}
		DWORD framesInAudioBuffer = playCursor < renderPos ? renderPos - playCursor : (renderPos + stream.audioLatencyFrames) - playCursor;
		stream.updateTimeInfo(nanosNow, framesInAudioBuffer);
		stream.synth.render(buf, frameCount);
		stream.renderedFramesCount += frameCount;
		if (!stream.ringBufferMode && waveOutWrite(stream.hWaveOut, waveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutWrite failed, thread stopped";
			stream.stopProcessing = true;
			stream.synth.close();
			return;
		}
	}
	stream.stopProcessing = false;
}

bool WinMMAudioStream::start(int deviceIndex) {
	if (buffer == NULL) return false;

	memset(buffer, 0, FRAME_SIZE * audioLatencyFrames);

	if (hWaveOut != NULL) {
		close();
	}
	if (deviceIndex < 0) {
		deviceIndex = WAVE_MAPPER;
		qDebug() << "WinMMAudioDriver: Using default WaveOut device";
	} else {
		qDebug() << "WinMMAudioDriver: Using WaveOut device:" << deviceIndex;
	}

	DWORD callbackType = CALLBACK_NULL;
	DWORD_PTR callback = (DWORD_PTR)NULL;
	if (ringBufferMode) {
		hWaitableTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	} else {
		hEvent = CreateEvent(NULL, false, true, NULL);
		callback = (DWORD_PTR)hEvent;
		callbackType = CALLBACK_EVENT;
	}

	PCMWAVEFORMAT wFormat = {{WAVE_FORMAT_PCM, /* stereo */ 2, sampleRate,
		sampleRate * FRAME_SIZE, FRAME_SIZE}, /* BitsPerSample */ 16};

	// Open waveout device
	if (waveOutOpen(&hWaveOut, deviceIndex, (LPWAVEFORMATEX)&wFormat, callback,
		(DWORD_PTR)this, callbackType) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutOpen failed";
			return false;
	}

	waveHdr = new WAVEHDR[numberOfChunks];
	LPSTR chunkStart = (LPSTR)buffer;
	DWORD chunkBytes = FRAME_SIZE * chunkSize;
	for (uint i = 0; i < numberOfChunks; i++) {
		if (ringBufferMode) {
			waveHdr[i].dwBufferLength = FRAME_SIZE * audioLatencyFrames;
			waveHdr[i].lpData = chunkStart;
			waveHdr[i].dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
			waveHdr[i].dwLoops = (DWORD)-1L;
		} else {
			waveHdr[i].dwBufferLength = chunkBytes;
			waveHdr[i].lpData = chunkStart;
			waveHdr[i].dwFlags = 0L;
			waveHdr[i].dwLoops = 0L;
			chunkStart += chunkBytes;
		}
		// Prepare headers
		if (waveOutPrepareHeader(hWaveOut, &waveHdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutPrepareHeader failed";
			stopProcessing = true;
			close();
			return false;
		}
		// Start playing
		if (waveOutWrite(hWaveOut, &waveHdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutWrite failed";
			stopProcessing = true;
			close();
			return false;
		}
	}

	processor.start(QThread::TimeCriticalPriority);
	return true;
}

void WinMMAudioStream::close() {
	if (hWaveOut != NULL) {
		if (stopProcessing == false) {
			stopProcessing = true;
			processor.wait();
		}
		stopProcessing = false;
		qDebug() << "WinMMAudioDriver: Processing thread stopped";

		waveOutReset(hWaveOut);
		for (uint i = 0; i < numberOfChunks; i++) {
			waveOutUnprepareHeader(hWaveOut, &waveHdr[i], sizeof(WAVEHDR));
		}
		delete[] waveHdr;
		waveHdr = NULL;
		CloseHandle(hWaitableTimer);
		hWaitableTimer = NULL;
		CloseHandle(hEvent);
		hEvent = NULL;
		waveOutClose(hWaveOut);
		hWaveOut = NULL;
	}
	return;
}

WinMMAudioDevice::WinMMAudioDevice(WinMMAudioDriver &driver, int useDeviceIndex, QString useDeviceName) :
	AudioDevice(driver, useDeviceName), deviceIndex(useDeviceIndex) {
}

AudioStream *WinMMAudioDevice::startAudioStream(QSynth &synth, const uint sampleRate) const {
	WinMMAudioDriver &winDriver = (WinMMAudioDriver &)driver;
	WinMMAudioStream *stream = new WinMMAudioStream(winDriver.getAudioStreamSettings(), winDriver.isRingBufferMode(), synth, sampleRate);
	if (stream->start(deviceIndex)) {
		return stream;
	}
	delete stream;
	return NULL;
}

WinMMAudioDriver::WinMMAudioDriver(Master *master) : AudioDriver("waveout", "WinMMAudio") {
	Q_UNUSED(master);

	loadAudioSettings();
}

const QList<const AudioDevice *> WinMMAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	UINT deviceCount = waveOutGetNumDevs();
	for(UINT deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		WAVEOUTCAPS deviceInfo;
		if (waveOutGetDevCaps(deviceIndex, &deviceInfo, sizeof(deviceInfo)) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutGetDevCaps failed for" << deviceIndex;
			continue;
		}
		deviceList.append(new WinMMAudioDevice(*this, deviceIndex, QString().fromLocal8Bit(deviceInfo.szPname)));
	}
	return deviceList;
}

void WinMMAudioDriver::validateAudioSettings(AudioDriverSettings &useSettings) const {
	if (useSettings.audioLatency == 0) {
		useSettings.audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (useSettings.chunkLen == 0) {
		useSettings.chunkLen = DEFAULT_CHUNK_MS;
	}
	if (useSettings.audioLatency < useSettings.chunkLen) {
		useSettings.chunkLen = useSettings.audioLatency;
	}
	if ((useSettings.midiLatency != 0) && (useSettings.midiLatency < useSettings.chunkLen)) {
		useSettings.midiLatency = useSettings.chunkLen;
	}
}

void WinMMAudioDriver::loadAudioSettings() {
	AudioDriver::loadAudioSettings();
	QSettings *qSettings = Master::getInstance()->getSettings();
	streamSettings = settings;
	streamSettings.advancedTiming = true;
	settings.advancedTiming = ringBufferMode = qSettings->value("Audio/" + id + "/UseRingBuffer").toBool();
}

void WinMMAudioDriver::setAudioSettings(AudioDriverSettings &useSettings) {
	AudioDriver::setAudioSettings(useSettings);
	ringBufferMode = settings.advancedTiming;
	streamSettings = settings;
	streamSettings.advancedTiming = true;
	QSettings *qSettings = Master::getInstance()->getSettings();
	qSettings->setValue("Audio/" + id + "/UseRingBuffer", ringBufferMode);
	qSettings->remove("Audio/" + id + "/AdvancedTiming");
}

bool WinMMAudioDriver::isRingBufferMode() {
	return ringBufferMode;
}

const AudioDriverSettings &WinMMAudioDriver::getAudioStreamSettings() {
	return streamSettings;
}
