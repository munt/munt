/* Copyright (C) 2011-2019 Jerome Fisher, Sergey V. Mikayev
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

#include "JackAudioDriver.h"

#include "../Master.h"
#include "../QSynth.h"

using namespace MT32Emu;

static const unsigned int DEFAULT_CHUNK_MS = 16;
static const unsigned int DEFAULT_AUDIO_LATENCY = 64;
static const unsigned int DEFAULT_MIDI_LATENCY = 32;

inline float int16_to_float(int16_t v) { return v / 32768.0; }

JackAudioStream::JackAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate) :
	AudioStream(useSettings, useSynth, useSampleRate), buffer(NULL), client(NULL)
{
	port[0] = NULL;
	port[1] = NULL;
}

JackAudioStream::~JackAudioStream() {
	close();

	if (buffer != NULL) {
		delete[] buffer;
	}
}

int JackAudioStream::processingThread(jack_nframes_t nframes, void *userData) {
	JackAudioStream &audioStream = *(JackAudioStream *)userData;
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	float *ports[NUM_OF_CHANNELS] = {
		(float *)jack_port_get_buffer(audioStream.port[0], nframes),
		(float *)jack_port_get_buffer(audioStream.port[1], nframes),
	};

	audioStream.updateTimeInfo(nanosNow, 0);
	audioStream.synth.render(audioStream.buffer, nframes);

	for (unsigned int i = 0, j = 0; i < nframes * NUM_OF_CHANNELS; i += NUM_OF_CHANNELS, j++) {
		for (unsigned int channel = 0; channel < NUM_OF_CHANNELS; channel++) {
			ports[channel][j] = int16_to_float(audioStream.buffer[i + channel]);
		}
	}

	audioStream.renderedFramesCount += nframes;

	return 0;
}

int JackAudioStream::allocateBuffer(jack_nframes_t nframes, void *userData) {
	JackAudioStream &audioStream = *(JackAudioStream *)userData;

	qDebug() << "JackAudioDriver: number of frames is" << nframes;

	if (audioStream.buffer != NULL) {
		delete[] audioStream.buffer;
		audioStream.buffer = NULL;
	}

	audioStream.buffer = new Bit16s[nframes * NUM_OF_CHANNELS];

	return 0;
}

bool JackAudioStream::start() {
	int error;
	close();

	qDebug() << "Using JACK default audio device";

	client = jack_client_open("Munt MT-32", JackNoStartServer, NULL);
	if (client == NULL) {
		qDebug() << "jack_client_open failed";
		client = NULL;
		return false;
	}

	port[0] = jack_port_register(client, "left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);
	port[1] = jack_port_register(client, "right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput|JackPortIsTerminal, 0);

	if (!port[0] || !port[1]) {
		qDebug() << "jack_port_register failed";
		jack_client_close(client);
		client = NULL;
		return false;
	}

	error = jack_set_process_callback(client, &processingThread, this);
	if (error != 0) {
		qDebug() << "jack_set_process_callback failed:" << error;
		jack_client_close(client);
		client = NULL;
		return false;
	}

	error = jack_set_buffer_size_callback(client, &allocateBuffer, this);
	if (error != 0) {
		qDebug() << "jack_set_buffer_size_callback failed:" << error;
		jack_client_close(client);
		client = NULL;
		return false;
	}

	jack_nframes_t size = settings.chunkLen * sampleRate / MasterClock::MILLIS_PER_SECOND;
	// Find a power of 2 that is >= size
	Bit32u binarySize = 1;
	// Using simple linear search as this isn't time critical
	while (binarySize < size) binarySize <<= 1;

	error = jack_set_buffer_size(client, binarySize);
	if (error != 0) {
		qDebug() << "jack_set_buffer_size failed:" << error;
		jack_client_close(client);
		client = NULL;
		return false;
	}

	audioLatencyFrames = binarySize;
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames + ((DEFAULT_MIDI_LATENCY * sampleRate) / MasterClock::MILLIS_PER_SECOND);

	error = jack_activate(client);
	if (error != 0) {
		qDebug() << "jack_activate failed:" << error;
		jack_client_close(client);
		client = NULL;
		return false;
	}

	jack_connect(client, jack_port_name(port[0]), "system:playback_1");
	jack_connect(client, jack_port_name(port[1]), "system:playback_2");

	return true;
}

void JackAudioStream::close() {
	if (client != NULL) {
		jack_deactivate(client);
		jack_client_close(client);
		client = NULL;
	}
}

JackAudioDevice::JackAudioDevice(JackAudioDriver &driver) : AudioDevice(driver, "Default") {}

AudioStream *JackAudioDevice::startAudioStream(QSynth &synth, const uint sampleRate) const {
	JackAudioStream *stream = new JackAudioStream(driver.getAudioSettings(), synth, sampleRate);
	if (stream->start()) return stream;
	delete stream;
	return NULL;
}

JackAudioDriver::JackAudioDriver(Master *master) : AudioDriver("jack", "JACK") {
	Q_UNUSED(master);

	loadAudioSettings();
}

JackAudioDriver::~JackAudioDriver() {
}

const QList<const AudioDevice *> JackAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new JackAudioDevice(*this));
	return deviceList;
}

void JackAudioDriver::validateAudioSettings(AudioDriverSettings &settings) const {
	if (settings.audioLatency == 0) {
		settings.audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (settings.chunkLen == 0) {
		settings.chunkLen = DEFAULT_CHUNK_MS;
	}
	if (settings.chunkLen > settings.audioLatency) {
		settings.chunkLen = settings.audioLatency;
	}
	if ((settings.midiLatency != 0) && (settings.midiLatency < settings.chunkLen)) {
		settings.midiLatency = settings.chunkLen;
	}
	settings.advancedTiming = false;
}
