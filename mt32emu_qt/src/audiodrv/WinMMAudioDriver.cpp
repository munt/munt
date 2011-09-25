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

#include "WinMMAudioDriver.h"
#include "../QSynth.h"
#include "../Master.h"
#include <process.h>

using namespace MT32Emu;

#define	MIN_RENDER_MS 10	// rendering time

// SergM: 100 ms output latency is safe on most systems.
// can be reduced to 35 ms (works on my system)
// 30 ms is the absolute minimum, unavoidable KSMixer latency
// FIXME: should be a variable
static const DWORD FRAMES_IN_BUFFER = 32 * 100;
// 30 ms is the size of KSMixer buffer. Writing to the positions closer to the playCursor is unsafe.
static const DWORD SAFE_FRAMES = 32 * 30;
// Stereo, 16-bit
static const DWORD FRAME_SIZE = 4;
// Latency for MIDI processing. 15 ms is the offset of interprocess timeGetTime() difference.
static const MasterClockNanos latency = 15 * MasterClock::NANOS_PER_MILLISECOND;

WinMMAudioStream::WinMMAudioStream(QSynth *useSynth, unsigned int useSampleRate) :
	synth(useSynth), sampleRate(useSampleRate), hWaveOut(NULL), pendingClose(false) {
		buffer = new Bit16s[2 * FRAMES_IN_BUFFER];
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
	MasterClockNanos nanosNow, firstSampleNanos = MasterClock::getClockNanos() - latency;
	MMTIME mmTime;
	WinMMAudioStream *stream = (WinMMAudioStream *)userData;
	DWORD minSamplesToRender = MIN_RENDER_MS * stream->sampleRate * MasterClock::NANOS_PER_MILLISECOND / MasterClock::NANOS_PER_SECOND;
	double samplePeriod = (double)MasterClock::NANOS_PER_SECOND / MasterClock::NANOS_PER_MILLISECOND / stream->sampleRate;
	DWORD prevPlayPos = 0;
	DWORD getPosWraps = 0;
	while (!stream->pendingClose) {
		mmTime.wType = TIME_SAMPLES;
		if (waveOutGetPosition(stream->hWaveOut, &mmTime, sizeof (MMTIME)) != MMSYSERR_NOERROR) {
			qDebug() << "waveOutGetPosition failed, thread stopped";
			stream->pendingClose = true;
			stream->synth->close();
			return;
		}

		// Deal with waveOutGetPosition() wraparound. For 16-bit stereo output, it equals 2^27,
		// presumably caused by the internal 32-bit counter of bits played.
		// The output of that nasty waveOutGetPosition() isn't monotonically increasing
		// even during 2^27 samples playback, so we have to ensure the difference is big enough...
		int delta = mmTime.u.sample - prevPlayPos;
		if (delta < -(1 << 26)) {
			qDebug() << "GetPos() wrap: " << delta << "\n";
			++getPosWraps;
		}
		prevPlayPos = mmTime.u.sample;
		playCursor = (mmTime.u.sample + (quint64)getPosWraps * (1 << 27)) % FRAMES_IN_BUFFER;

		nanosNow = MasterClock::getClockNanos() - latency;
		if (playCursor < renderPos) {
			// Buffer wrap, render 'till the end of buffer
			frameCount = FRAMES_IN_BUFFER - renderPos;

			// Estimate the buffer wrap time
			nanosNow -= MasterClock::NANOS_PER_SECOND * playCursor / stream->sampleRate;
		} else {
			frameCount = playCursor - renderPos;
			if (frameCount < minSamplesToRender) {
				Sleep(1 + DWORD((minSamplesToRender - frameCount) * samplePeriod));
				continue;
			}
		}
		unsigned int rendered = stream->synth->render(stream->buffer + (renderPos << 1), frameCount,
			firstSampleNanos, stream->sampleRate);
/*
		Detection of writing to unsafe positions
		30 ms is the size of KSMixer buffer. KSMixer pulls audio data at regular intervals (10 ms).
		Thus, writing to the positions closer 30 ms before the playCursor is unsafe.
		Unfortunately, there is no way to predict the safe writeCursor as with DSound.
		Therefore, this allows detecting _possible_ underruns. This doesn't mean the underrun really happened.
*/
		if (((renderPos + FRAMES_IN_BUFFER - playCursor) % FRAMES_IN_BUFFER) < SAFE_FRAMES) {
			qDebug() << "Rendering to unsafe positions! Probable underrun! Buffer size should be higher!";
		}

		// Underrun (buffer wrap) detection
		int framesReallyPlayed = int(double(nanosNow - firstSampleNanos) / MasterClock::NANOS_PER_SECOND * stream->sampleRate);
		if (framesReallyPlayed > (int)FRAMES_IN_BUFFER) {
			qDebug() << "Underrun (buffer wrap) detected! Buffer size should be higher!";
		}
		firstSampleNanos = nanosNow;
		if (rendered < frameCount) { // SergM: never found this true
			char *out = (char *)stream->buffer + FRAME_SIZE * renderPos;
			// Fill this with 0 due to the real synth fault
			memset(out + rendered * FRAME_SIZE, 0, (frameCount - rendered) * FRAME_SIZE);
		}
		renderPos += frameCount;
		if (renderPos >= FRAMES_IN_BUFFER) {
			renderPos -= FRAMES_IN_BUFFER;
		}
	}
	stream->pendingClose = false;
	return;
}

bool WinMMAudioStream::start(int deviceIndex) {
	if (buffer == NULL) {
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * FRAMES_IN_BUFFER);
	if (hWaveOut != NULL) {
		close();
	}
	if (deviceIndex < 0) {
		deviceIndex = WAVE_MAPPER;
		qDebug() << "Using default WaveOut device";
	} else {
		qDebug() << "Using WaveOut device:" << deviceIndex;
	}

	PCMWAVEFORMAT wFormat = {{WAVE_FORMAT_PCM, /* stereo */ 2, sampleRate,
		sampleRate * FRAME_SIZE, FRAME_SIZE}, /* BitsPerSample */ 16};

	//	Open waveout device
	if (waveOutOpen(&hWaveOut, deviceIndex, (LPWAVEFORMATEX)&wFormat, NULL,
		NULL, CALLBACK_NULL) != MMSYSERR_NOERROR) {
			qDebug() << "waveOutOpen failed";
			return false;
	}

	//	Prepare header
	WaveHdr.dwBufferLength = FRAME_SIZE * FRAMES_IN_BUFFER;
	WaveHdr.lpData = (LPSTR)buffer;
	WaveHdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
	WaveHdr.dwLoops = -1;
	if (waveOutPrepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		qDebug() << "waveOutPrepareHeader failed";
		waveOutClose(hWaveOut);
		return false;
	}

	//	Start playing
	if (waveOutWrite(hWaveOut, &WaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		qDebug() << "waveOutWrite failed";
		waveOutUnprepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
		waveOutClose(hWaveOut);
		return false;
	}
	_beginthread(processingThread, 1024*1024, this);
	return true;
}

void WinMMAudioStream::close() {
	if (hWaveOut != NULL) {

		// pendingClose == true means the thread has already exited upon a failure
		if (pendingClose == false) {
			pendingClose = true;
			while (pendingClose) {
				Sleep(10);
			}
		} else {
			pendingClose = false;
		}
		waveOutReset(hWaveOut);
		waveOutUnprepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
		waveOutClose(hWaveOut);
		hWaveOut = NULL;
	}
	return;
}

WinMMAudioDevice::WinMMAudioDevice(WinMMAudioDriver *driver, int useDeviceIndex,
																	 QString useDeviceName) :
	AudioDevice(driver, QString::number(useDeviceIndex), useDeviceName),
	deviceIndex(useDeviceIndex) {
}

WinMMAudioStream *WinMMAudioDevice::startAudioStream(QSynth *synth,
																										 unsigned int sampleRate) {
	WinMMAudioStream *stream = new WinMMAudioStream(synth, sampleRate);
	if (stream->start(deviceIndex)) {
		return stream;
	}
	delete stream;
	return NULL;
}

WinMMAudioDriver::WinMMAudioDriver(Master *master) : AudioDriver("waveout", "WinMMAudio") {
}

WinMMAudioDriver::~WinMMAudioDriver() {
}

QList<AudioDevice *> WinMMAudioDriver::getDeviceList() {
	QList<AudioDevice *> deviceList;
	UINT deviceCount = waveOutGetNumDevs();
	for(UINT deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		WAVEOUTCAPS deviceInfo;
		if (waveOutGetDevCaps(deviceIndex, &deviceInfo, sizeof(deviceInfo)) != MMSYSERR_NOERROR) {
			qDebug() << "waveOutGetDevCaps failed for" << deviceIndex;
			continue;
		}
		deviceList.append(new WinMMAudioDevice(this, deviceIndex, deviceInfo.szPname));
	}
	return deviceList;
}
