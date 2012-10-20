/* Copyright (C) 2011, 2012 Jerome Fisher, Sergey V. Mikayev
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

#include <process.h>

#include "WinMMAudioDriver.h"
#include "../QSynth.h"
#include "../Master.h"

using namespace MT32Emu;

// Looks resonable
static const DWORD DEFAULT_CHUNK_MS = 10;
// SergM: 100 ms output latency is safe on most systems.
// can be reduced to 35 ms (works on my system)
// 30 ms is the absolute minimum, unavoidable KSMixer latency
static const DWORD DEFAULT_AUDIO_LATENCY = 100;
// 30 ms is the size of KSMixer buffer. Writing to the positions closer to the playCursor is unsafe.
static const DWORD SAFE_MS = 30;
static const DWORD SAFE_FRAMES = 32 * SAFE_MS;
// Stereo, 16-bit
static const DWORD FRAME_SIZE = 4;
// Latency for MIDI processing. 15 ms is the offset of interprocess timeGetTime() difference.
static const DWORD DEFAULT_MIDI_LATENCY = 15;
static const MasterClockNanos MIN_XRUN_WARNING_NANOS = 3 * MasterClock::NANOS_PER_SECOND;

WinMMAudioStream::WinMMAudioStream(const WinMMAudioDevice *device, QSynth *useSynth,
	unsigned int useSampleRate) : synth(useSynth), sampleRate(useSampleRate),
	hWaveOut(NULL), waveHdr(NULL), hEvent(NULL), stopProcessing(false)
{
	unsigned int milliLatency;

	device->driver->getAudioSettings(&chunkSize, &bufferSize, &milliLatency, &useRingBuffer);
	chunkSize *= sampleRate / 1000 /* ms per sec*/;
	bufferSize *= sampleRate / 1000 /* ms per sec*/;
	if (useRingBuffer) {
		numberOfChunks = 1;
		qDebug() << "WinMMAudioDriver: Using looped ring buffer, buffer size:" << bufferSize << "frames.";
	} else {
		// Number of chunks should be ceil(bufferSize / chunkSize)
		numberOfChunks = (bufferSize + chunkSize - 1) / chunkSize;
		// Refine bufferSize as chunkSize * number of chunks, no less then the specified value
		bufferSize = numberOfChunks * chunkSize;
		qDebug() << "WinMMAudioDriver: Using" << numberOfChunks << "chunks, chunk size:" << chunkSize << "frames, buffer size:" << bufferSize << "frames.";
	}
	buffer = new Bit16s[2 * bufferSize];
	midiLatency = milliLatency * MasterClock::NANOS_PER_MILLISECOND;
}

WinMMAudioStream::~WinMMAudioStream() {
	if (hWaveOut != NULL) {
		close();
	}
	delete[] buffer;
}

void WinMMAudioStream::processingThread(void *userData) {
	DWORD renderPos = 0;
	DWORD playCursor, frameCount;
	MMTIME mmTime;
	WinMMAudioStream &stream = *(WinMMAudioStream *)userData;
	MasterClockNanos nanosNow;
	MasterClockNanos firstSampleNanos = MasterClock::getClockNanos() - stream.midiLatency;
	MasterClockNanos lastSampleNanos = firstSampleNanos;
	MasterClockNanos audioLatency = stream.bufferSize * MasterClock::NANOS_PER_SECOND / stream.sampleRate;
	MasterClockNanos lastXRunWarningNanos = firstSampleNanos - MIN_XRUN_WARNING_NANOS;
	double samplePeriod = (double)MasterClock::NANOS_PER_SECOND / MasterClock::NANOS_PER_MILLISECOND / stream.sampleRate;
	DWORD prevPlayPos = 0;
	DWORD getPosWraps = 0;
	while (!stream.stopProcessing) {
		mmTime.wType = TIME_SAMPLES;
		if (waveOutGetPosition(stream.hWaveOut, &mmTime, sizeof (MMTIME)) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutGetPosition failed, thread stopped";
			stream.stopProcessing = true;
			stream.synth->close();
			return;
		}

		// Deal with waveOutGetPosition() wraparound. For 16-bit stereo output, it equals 2^27,
		// presumably caused by the internal 32-bit counter of bits played.
		// The output of that nasty waveOutGetPosition() isn't monotonically increasing
		// even during 2^27 samples playback, so we have to ensure the difference is big enough...
		int delta = mmTime.u.sample - prevPlayPos;
		if (delta < -(1 << 26)) {
			qDebug() << "WinMMAudioDriver: GetPos() wrap: " << delta << "\n";
			++getPosWraps;
		}
		prevPlayPos = mmTime.u.sample;
		playCursor = (mmTime.u.sample + (quint64)getPosWraps * (1 << 27)) % stream.bufferSize;

		nanosNow = MasterClock::getClockNanos() - stream.midiLatency;
		Bit16s *buf = NULL;
		WAVEHDR *waveHdr = NULL;
		if (stream.useRingBuffer) {
			if (playCursor < renderPos) {
				// Buffer wrap, render 'till the end of buffer
				frameCount = stream.bufferSize - renderPos;

				// Estimate the buffer wrap time
				nanosNow -= MasterClock::NANOS_PER_SECOND * playCursor / stream.sampleRate;
			} else {
				frameCount = playCursor - renderPos;
				if (frameCount < stream.chunkSize) {
					Sleep(1 + DWORD((stream.chunkSize - frameCount) * samplePeriod));
					continue;
				}
			}
			buf = stream.buffer + (renderPos << 1);
		} else {
			bool allBuffersRendered = true;
			for (unsigned int i = 0; i < stream.numberOfChunks; i++) {
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
		double actualSampleRate = estimateActualSampleRate(stream.sampleRate, firstSampleNanos, lastSampleNanos, audioLatency, frameCount);
		unsigned int rendered = stream.synth->render(buf, frameCount, firstSampleNanos, actualSampleRate);
		if (!stream.useRingBuffer && waveOutWrite(stream.hWaveOut, waveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutWrite failed, thread stopped";
			stream.stopProcessing = true;
			stream.synth->close();
			return;
		}

		if (nanosNow - lastXRunWarningNanos > MIN_XRUN_WARNING_NANOS) {
			/*
			 *	Detection of writing to unsafe positions
			 *	30 ms is the size of KSMixer buffer. KSMixer pulls audio data at regular intervals (10 ms).
			 *	Thus, writing to the positions closer 30 ms before the playCursor is unsafe.
			 *	Unfortunately, there is no way to predict the safe writeCursor as with DSound.
			 *	Therefore, this allows detecting _possible_ underruns. This doesn't mean the underrun really happened.
			 */
			if (((renderPos + stream.bufferSize - playCursor) % stream.bufferSize) < SAFE_FRAMES) {
				qDebug() << "WinMMAudioDriver: Rendering to unsafe positions! Probable underrun! Buffer size should be higher!";
				lastXRunWarningNanos = nanosNow;
			}

			// Underrun (buffer wrap) detection
			int framesReallyPlayed = int(double(nanosNow - firstSampleNanos) / MasterClock::NANOS_PER_SECOND * stream.sampleRate);
			if (framesReallyPlayed > (int)stream.bufferSize) {
				qDebug() << "WinMMAudioDriver: Underrun (buffer wrap) detected! Buffer size should be higher!";
				lastXRunWarningNanos = nanosNow;
			}
		}

		firstSampleNanos = nanosNow;
		if (rendered < frameCount) { // SergM: never found this true
			char *out = (char *)stream.buffer + FRAME_SIZE * renderPos;
			// Fill this with 0 due to the real synth fault
			memset(out + rendered * FRAME_SIZE, 0, (frameCount - rendered) * FRAME_SIZE);
		}
		renderPos += frameCount;
		if (renderPos >= stream.bufferSize) {
			renderPos -= stream.bufferSize;
		}
	}
	stream.stopProcessing = false;
	return;
}

bool WinMMAudioStream::start(int deviceIndex) {
	if (buffer == NULL) {
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * bufferSize);
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
	DWORD_PTR callback = NULL;
	if (!useRingBuffer) {
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
	for (unsigned int i = 0; i < numberOfChunks; i++) {
		if (useRingBuffer) {
			waveHdr[i].dwBufferLength = FRAME_SIZE * bufferSize;
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
	_beginthread(processingThread, 1024*1024, this);
	return true;
}

void WinMMAudioStream::close() {
	if (hWaveOut != NULL) {
		if (hEvent != NULL) SetEvent(hEvent);

		// pendingClose == true means the thread has already exited upon a failure
		if (stopProcessing == false) {
			stopProcessing = true;
			while (stopProcessing) {
				Sleep(10);
			}
		} else {
			stopProcessing = false;
		}
		waveOutReset(hWaveOut);
		for (unsigned int i = 0; i < numberOfChunks; i++) {
			waveOutUnprepareHeader(hWaveOut, &waveHdr[i], sizeof(WAVEHDR));
		}
		delete waveHdr;
		waveHdr = NULL;
		CloseHandle(hEvent);
		hEvent = NULL;
		waveOutClose(hWaveOut);
		hWaveOut = NULL;
	}
	return;
}

WinMMAudioDevice::WinMMAudioDevice(const WinMMAudioDriver * const driver, int useDeviceIndex, QString useDeviceName) :
	AudioDevice(driver, QString::number(useDeviceIndex), useDeviceName),
	deviceIndex(useDeviceIndex) {
}

WinMMAudioStream *WinMMAudioDevice::startAudioStream(QSynth *synth, unsigned int sampleRate) const {
	WinMMAudioStream *stream = new WinMMAudioStream(this, synth, sampleRate);
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

WinMMAudioDriver::~WinMMAudioDriver() {
}

QList<AudioDevice *> WinMMAudioDriver::getDeviceList() const {
	QList<AudioDevice *> deviceList;
	UINT deviceCount = waveOutGetNumDevs();
	for(UINT deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		WAVEOUTCAPS deviceInfo;
		if (waveOutGetDevCaps(deviceIndex, &deviceInfo, sizeof(deviceInfo)) != MMSYSERR_NOERROR) {
			qDebug() << "WinMMAudioDriver: waveOutGetDevCaps failed for" << deviceIndex;
			continue;
		}
		deviceList.append(new WinMMAudioDevice(this, deviceIndex, QString().fromLocal8Bit(deviceInfo.szPname)));
	}
	return deviceList;
}

void WinMMAudioDriver::validateAudioSettings() {
	if (midiLatency == 0) {
		midiLatency = DEFAULT_MIDI_LATENCY;
	}
	if (audioLatency == 0) {
		audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (chunkLen == 0) {
		chunkLen = DEFAULT_CHUNK_MS;
	}
	if (audioLatency <= SAFE_MS) {
		audioLatency = SAFE_MS + 1;
	}
	if (chunkLen > (audioLatency - SAFE_MS)) {
		chunkLen = audioLatency - SAFE_MS;
	}
}
