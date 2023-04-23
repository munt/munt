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

#include "JACKClient.h"
#include "Master.h"
#include "MidiSession.h"
#include "mididrv/JACKMidiDriver.h"
#include "audiodrv/JACKAudioDriver.h"

int JACKClient::onJACKProcess(jack_nframes_t nframes, void *instance) {
	JACKClient *jackClient = static_cast<JACKClient *>(instance);
	jackClient->process(nframes);
	return 0;
}

int JACKClient::onBufferSizeChange(jack_nframes_t newFrameSize, void *instance) {
	JACKClient *jackClient = static_cast<JACKClient *>(instance);
	if (jackClient->audioStream != NULL && jackClient->bufferSize != newFrameSize) {
		jackClient->bufferSize = newFrameSize;
		jackClient->audioStream->onJACKBufferSizeChange(newFrameSize);
	} else {
		qDebug() << "JACKClient: Got buffer size change:" << newFrameSize;
	}
	return 0;
}

int JACKClient::onSampleRateChange(jack_nframes_t newSystemSampleRate, void *) {
	qDebug() << "JACKClient: Got sample rate change:" << newSystemSampleRate;
	return 0;
}

void JACKClient::onJACKShutdown(void *instance) {
	JACKClient *jackClient = static_cast<JACKClient *>(instance);
	jackClient->state = JACKClientState_CLOSING;
	if (jackClient->midiSession != NULL) {
		// This eventually deletes jackClient
		Master::getInstance()->deleteJACKMidiPort(jackClient->midiSession);
	} else if (jackClient->audioStream != NULL) {
		jackClient->audioStream->onJACKShutdown();
		jackClient->close();
	}
}

JACKClient::JACKClient() :
	state(JACKClientState_CLOSED),
	client(),
	midiSession(),
	audioStream(),
	midiInPort(),
	leftAudioOutPort(),
	rightAudioOutPort()
{}

JACKClient::~JACKClient() {
	close();
}

JACKClientState JACKClient::open(MidiSession *useMidiSession, JACKAudioStream *useAudioStream) {
	if (state != JACKClientState_CLOSED) return state;

	client = jack_client_open("mt32emu-qt", JackNullOption, NULL);
	if (client == NULL) return state;

	if (useAudioStream != NULL && !useAudioStream->checkSampleRate(getSampleRate())) {
		jack_client_close(client);
		client = NULL;
		return state;
	}

	if (useMidiSession != NULL) {
		midiInPort = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
		if (midiInPort == NULL) {
			jack_client_close(client);
			client = NULL;
			return state;
		}
		midiSession = useMidiSession;
		char *actualClientName = jack_get_client_name(client);
		midiSession->getSynthRoute()->setMidiSessionName(midiSession, midiSession->getName() + " for " + actualClientName);
	}
	if (useAudioStream != NULL) {
		leftAudioOutPort = jack_port_register(client, "left_audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		rightAudioOutPort = jack_port_register(client, "right_audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		if (leftAudioOutPort == NULL || rightAudioOutPort == NULL) {
			jack_client_close(client);
			client = NULL;
			return state;
		}
		audioStream = useAudioStream;
	}

	state = JACKClientState_OPENING;

	bufferSize = jack_get_buffer_size(client);

	jack_on_shutdown(client, onJACKShutdown, this);
	jack_set_buffer_size_callback(client, onBufferSizeChange, this);
	jack_set_sample_rate_callback(client, onSampleRateChange, this);
	jack_set_process_callback(client, onJACKProcess, this);

	return state;
}

JACKClientState JACKClient::close() {
	if (state == JACKClientState_CLOSED) return state;
	if (state == JACKClientState_OPENING || state == JACKClientState_OPEN) {
		jack_client_close(client);
	}
	client = NULL;
	state = JACKClientState_CLOSED;
	return state;
}

JACKClientState JACKClient::start() {
	if (state != JACKClientState_OPENING) return state;
	if (jack_activate(client) != 0) return close();
	state = JACKClientState_OPEN;
	return state;
}

void JACKClient::connectToPhysicalPorts() {
	const char **ports = jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL || ports[0] == NULL || ports[1] == NULL) {
		qDebug() << "JACKClient: No physical playback ports detected, autoconnection failed";
	} else if (0 != jack_connect(client, jack_port_name(leftAudioOutPort), ports[0])) {
		qDebug() << "JACKClient: Autoconnection to port" << ports[0] << "failed";
	} else if (0 != jack_connect(client, jack_port_name(rightAudioOutPort), ports[1])) {
		qDebug() << "JACKClient: Autoconnection to port" << ports[1] << "failed";
	}
	jack_free(ports);
}

bool JACKClient::isRealtimeProcessing() const {
	return jack_is_realtime(client) != 0;
}

quint32 JACKClient::getSampleRate() const {
	return jack_get_sample_rate(client);
}

quint32 JACKClient::getFramesSinceCycleStart() const {
	return jack_frames_since_cycle_start(client);
}

void JACKClient::process(jack_nframes_t nframes) {
	if (midiSession != NULL) {
		quint32 cycleStartFrameTime = audioStream != NULL ? 0 : jack_last_frame_time(client);
		jack_time_t jackTimeNow = audioStream != NULL ? 0 : jack_get_time();
		MasterClockNanos nanosNow = audioStream != NULL ? 0 : MasterClock::getClockNanos();
		void *midiInBuffer = jack_port_get_buffer(midiInPort, nframes);
		uint eventCount = uint(jack_midi_get_event_count(midiInBuffer));
		for (uint eventIx = 0; eventIx < eventCount; eventIx++) {
			jack_midi_event_t eventData;
			bool eventRetrieved = jack_midi_event_get(&eventData, midiInBuffer, eventIx) == 0;
			if (!eventRetrieved) break;
			bool eventConsumed;
			if (audioStream != NULL) {
				quint64 eventTimestamp = audioStream->computeMIDITimestamp(eventData.time);
				eventConsumed = JACKMidiDriver::playMIDIMessage(midiSession, eventTimestamp, eventData.size, eventData.buffer);
			} else {
				quint32 eventFrameTime = cycleStartFrameTime + eventData.time;
				quint64 eventJackTime = jack_frames_to_time(client, eventFrameTime);
				MasterClockNanos eventNanoTime = JACKMidiDriver::jackFrameTimeToMasterClockNanos(nanosNow, eventJackTime, jackTimeNow);
				eventConsumed = JACKMidiDriver::pushMIDIMessage(midiSession, eventNanoTime, eventData.size, eventData.buffer);
			}
			if (!eventConsumed) break;
		}
	}
	if (audioStream != NULL) {
		jack_default_audio_sample_t *leftOutBuffer = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(leftAudioOutPort, nframes));
		jack_default_audio_sample_t *rightOutBuffer = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(rightAudioOutPort, nframes));
		audioStream->renderStreams(nframes, leftOutBuffer, rightOutBuffer);
	}
}
