/* Copyright (C) 2011-2026 Jerome Fisher, Sergey V. Mikayev
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

#include <QtCore>

#include "JACKClient.h"
#include "Master.h"
#include "MidiSession.h"
#include "mididrv/JACKMidiDriver.h"
#include "audiodrv/JACKAudioDriver.h"

#ifdef USE_JACK_DYNAMIC_LOADING

#if defined _WIN32 || defined __CYGWIN__

#include <windows.h>

static LPCSTR JACK_DLL_NAME = "libjack.dll";
static LPCSTR JACK_DLL_x64_NAME = "libjack64.dll";

#else

#include <dlfcn.h>

#ifdef __APPLE__

static const char JACK_SO_NAME[] = "libjack.dylib";
static const char JACK_SO_NAME_MAJOR_VERSION[] = "libjack.0.dylib";

#else

static const char JACK_SO_NAME[] = "libjack.so";
static const char JACK_SO_NAME_MAJOR_VERSION[] = "libjack.so.0";

#endif

#endif

namespace EvenWeakerJACK {

typedef jack_client_t *(*client_open_t)(const char *client_name, jack_options_t options, jack_status_t *status, ...);
typedef int (*client_close_t)(jack_client_t *client);
typedef jack_port_t *(*port_register_t)(jack_client_t *client, const char *port_name, const char *port_type, unsigned long flags, unsigned long buffer_size);
typedef char *(*get_client_name_t)(jack_client_t *client);
typedef jack_nframes_t (*get_buffer_size_t)(jack_client_t *client);
typedef int (*activate_t)(jack_client_t *client);
typedef void (*on_shutdown_t)(jack_client_t *client, JackShutdownCallback shutdown_callback, void *arg);
typedef int (*set_buffer_size_callback_t)(jack_client_t *client, JackBufferSizeCallback bufsize_callback, void *arg);
typedef int (*set_sample_rate_callback_t)(jack_client_t *client, JackSampleRateCallback srate_callback, void *arg);
typedef int (*set_process_callback_t)(jack_client_t *client, JackProcessCallback process_callback, void *arg);
typedef const char **(*get_ports_t)(jack_client_t *client, const char *port_name_pattern, const char *type_name_pattern, unsigned long flags);
typedef int (*connect_t)(jack_client_t *client, const char *source_port, const char *destination_port);
typedef const char *(*port_name_t)(const jack_port_t *port);
typedef void (*free_t)(void *ptr);
typedef int (*is_realtime_t)(jack_client_t *client);
typedef jack_nframes_t (*get_sample_rate_t)(jack_client_t *client);
typedef jack_nframes_t (*frames_since_cycle_start_t)(const jack_client_t *);
typedef jack_nframes_t (*last_frame_time_t)(const jack_client_t *client);
typedef jack_time_t (*get_time_t)();
typedef void *(*port_get_buffer_t)(jack_port_t *port, jack_nframes_t nframes);
typedef uint32_t (*midi_get_event_count_t)(void *port_buffer);
typedef int (*midi_event_get_t)(jack_midi_event_t *event, void *port_buffer, uint32_t event_index);
typedef jack_time_t (*frames_to_time_t)(const jack_client_t *client, jack_nframes_t nframes);

static struct {
	client_open_t client_open;
	client_close_t client_close;
	port_register_t port_register;
	get_client_name_t get_client_name;
	get_buffer_size_t get_buffer_size;
	activate_t activate;
	on_shutdown_t on_shutdown;
	set_buffer_size_callback_t set_buffer_size_callback;
	set_sample_rate_callback_t set_sample_rate_callback;
	set_process_callback_t set_process_callback;
	get_ports_t get_ports;
	connect_t connect;
	port_name_t port_name;
	free_t free;
	is_realtime_t is_realtime;
	get_sample_rate_t get_sample_rate;
	frames_since_cycle_start_t frames_since_cycle_start;
	last_frame_time_t last_frame_time;
	get_time_t get_time;
	port_get_buffer_t port_get_buffer;
	midi_get_event_count_t midi_get_event_count;
	midi_event_get_t midi_event_get;
	frames_to_time_t frames_to_time;
} refs;

} // namespace EvenWeakerJACK

#define ref_jack_client_open EvenWeakerJACK::refs.client_open
#define ref_jack_client_close EvenWeakerJACK::refs.client_close
#define ref_jack_port_register EvenWeakerJACK::refs.port_register
#define ref_jack_get_client_name EvenWeakerJACK::refs.get_client_name
#define ref_jack_get_buffer_size EvenWeakerJACK::refs.get_buffer_size
#define ref_jack_activate EvenWeakerJACK::refs.activate
#define ref_jack_on_shutdown EvenWeakerJACK::refs.on_shutdown
#define ref_jack_set_buffer_size_callback EvenWeakerJACK::refs.set_buffer_size_callback
#define ref_jack_set_sample_rate_callback EvenWeakerJACK::refs.set_sample_rate_callback
#define ref_jack_set_process_callback EvenWeakerJACK::refs.set_process_callback
#define ref_jack_get_ports EvenWeakerJACK::refs.get_ports
#define ref_jack_connect EvenWeakerJACK::refs.connect
#define ref_jack_port_name EvenWeakerJACK::refs.port_name
#define ref_jack_free EvenWeakerJACK::refs.free
#define ref_jack_is_realtime EvenWeakerJACK::refs.is_realtime
#define ref_jack_get_sample_rate EvenWeakerJACK::refs.get_sample_rate
#define ref_jack_frames_since_cycle_start EvenWeakerJACK::refs.frames_since_cycle_start
#define ref_jack_last_frame_time EvenWeakerJACK::refs.last_frame_time
#define ref_jack_get_time EvenWeakerJACK::refs.get_time
#define ref_jack_port_get_buffer EvenWeakerJACK::refs.port_get_buffer
#define ref_jack_midi_get_event_count EvenWeakerJACK::refs.midi_get_event_count
#define ref_jack_midi_event_get EvenWeakerJACK::refs.midi_event_get
#define ref_jack_frames_to_time EvenWeakerJACK::refs.frames_to_time

#else // USE_JACK_DYNAMIC_LOADING

#define ref_jack_client_open jack_client_open
#define ref_jack_client_close jack_client_close
#define ref_jack_port_register jack_port_register
#define ref_jack_get_client_name jack_get_client_name
#define ref_jack_get_buffer_size jack_get_buffer_size
#define ref_jack_activate jack_activate
#define ref_jack_on_shutdown jack_on_shutdown
#define ref_jack_set_buffer_size_callback jack_set_buffer_size_callback
#define ref_jack_set_sample_rate_callback jack_set_sample_rate_callback
#define ref_jack_set_process_callback jack_set_process_callback
#define ref_jack_get_ports jack_get_ports
#define ref_jack_connect jack_connect
#define ref_jack_port_name jack_port_name
#define ref_jack_free jack_free
#define ref_jack_is_realtime jack_is_realtime
#define ref_jack_get_sample_rate jack_get_sample_rate
#define ref_jack_frames_since_cycle_start jack_frames_since_cycle_start
#define ref_jack_last_frame_time jack_last_frame_time
#define ref_jack_get_time jack_get_time
#define ref_jack_port_get_buffer jack_port_get_buffer
#define ref_jack_midi_get_event_count jack_midi_get_event_count
#define ref_jack_midi_event_get jack_midi_event_get
#define ref_jack_frames_to_time jack_frames_to_time

#endif // USE_JACK_DYNAMIC_LOADING

static bool loadJACK() {
#ifdef USE_JACK_DYNAMIC_LOADING
	static bool loaded = false;

	if (loaded) return true;

#if defined _WIN32 || defined __CYGWIN__

	LPCSTR dllName = QSysInfo::WordSize == 64 ? JACK_DLL_x64_NAME : JACK_DLL_NAME;
	qDebug() << "Loading JACK library" << dllName;

	HMODULE hModule = LoadLibraryA(dllName);

	if (NULL == hModule) {
		qDebug() << "JACK library failed to load:" << GetLastError();
		return false;
	}

#define bindJACKFunction(fName) \
	if (success) { \
		function = "jack_" #fName; \
		ref_jack_##fName = (EvenWeakerJACK::fName##_t)(void *)GetProcAddress(hModule, function); \
		success = NULL != ref_jack_##fName; \
	}

#define checkJACKBindError \
	if (!success) { \
		qDebug() << "Failed to get address of JACK library function" << function << "with error" << GetLastError(); \
		FreeLibrary(hModule); \
		return false; \
	}

	LPCSTR function;
	bool success = true;

#else // defined _WIN32 || defined __CYGWIN__

	qDebug() << "Loading JACK library" << JACK_SO_NAME_MAJOR_VERSION;
	void *dlHandle = dlopen(JACK_SO_NAME_MAJOR_VERSION, RTLD_NOW);
	if (NULL == dlHandle) {
		qDebug() << "JACK library failed to load:" << dlerror();
		qDebug() << "Trying versionless JACK library name";
		dlHandle = dlopen(JACK_SO_NAME, RTLD_NOW);
		if (NULL == dlHandle) {
			qDebug() << "JACK library failed to load:" << dlerror();
			return false;
		}
	}

#define bindJACKFunction(fName) \
	if (!error) { \
		function = "jack_" #fName; \
		ref_jack_##fName = (EvenWeakerJACK::fName##_t)dlsym(dlHandle, function); \
		error = dlerror(); \
	}

#define checkJACKBindError \
	if (error) { \
		qDebug() << "Failed to get address of JACK library function" << function << "with error" << error; \
		dlclose(dlHandle); \
		return false; \
	}

	const char *function;
	const char *error = NULL;
	dlerror(); // Clear any stale error.

#endif // defined _WIN32 || defined __CYGWIN__

	bindJACKFunction(client_open);
	bindJACKFunction(client_close);
	bindJACKFunction(port_register);
	bindJACKFunction(get_client_name);
	bindJACKFunction(get_buffer_size);
	bindJACKFunction(activate);
	bindJACKFunction(on_shutdown);
	bindJACKFunction(set_buffer_size_callback);
	bindJACKFunction(set_sample_rate_callback);
	bindJACKFunction(set_process_callback);
	bindJACKFunction(get_ports);
	bindJACKFunction(connect);
	bindJACKFunction(port_name);
	bindJACKFunction(free);
	bindJACKFunction(is_realtime);
	bindJACKFunction(get_sample_rate);
	bindJACKFunction(frames_since_cycle_start);
	bindJACKFunction(last_frame_time);
	bindJACKFunction(get_time);
	bindJACKFunction(port_get_buffer);
	bindJACKFunction(midi_get_event_count);
	bindJACKFunction(midi_event_get);
	bindJACKFunction(frames_to_time);

	checkJACKBindError

	loaded = true;
#endif // USE_JACK_DYNAMIC_LOADING

	return true;
}

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
	state(JACKClientState_UNAVAILABLE),
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
	if (state == JACKClientState_UNAVAILABLE) {
		if (!loadJACK()) return state;
		state = JACKClientState_CLOSED;
	} else if (state != JACKClientState_CLOSED) return state;

	client = ref_jack_client_open("mt32emu-qt", JackNullOption, NULL);
	if (client == NULL) return state;

	if (useAudioStream != NULL && !useAudioStream->checkSampleRate(getSampleRate())) {
		ref_jack_client_close(client);
		client = NULL;
		return state;
	}

	if (useMidiSession != NULL) {
		midiInPort = ref_jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
		if (midiInPort == NULL) {
			ref_jack_client_close(client);
			client = NULL;
			return state;
		}
		midiSession = useMidiSession;
		char *actualClientName = ref_jack_get_client_name(client);
		midiSession->getSynthRoute()->setMidiSessionName(midiSession, midiSession->getName() + " for " + actualClientName);
	}
	if (useAudioStream != NULL) {
		leftAudioOutPort = ref_jack_port_register(client, "left_audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		rightAudioOutPort = ref_jack_port_register(client, "right_audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
		if (leftAudioOutPort == NULL || rightAudioOutPort == NULL) {
			ref_jack_client_close(client);
			client = NULL;
			return state;
		}
		audioStream = useAudioStream;
	}

	state = JACKClientState_OPENING;

	bufferSize = ref_jack_get_buffer_size(client);

	ref_jack_on_shutdown(client, onJACKShutdown, this);
	ref_jack_set_buffer_size_callback(client, onBufferSizeChange, this);
	ref_jack_set_sample_rate_callback(client, onSampleRateChange, this);
	ref_jack_set_process_callback(client, onJACKProcess, this);

	return state;
}

JACKClientState JACKClient::close() {
	if (state == JACKClientState_UNAVAILABLE || state == JACKClientState_CLOSED) return state;
	if (state == JACKClientState_OPENING || state == JACKClientState_OPEN) {
		ref_jack_client_close(client);
	}
	client = NULL;
	state = JACKClientState_CLOSED;
	return state;
}

JACKClientState JACKClient::start() {
	if (state != JACKClientState_OPENING) return state;
	if (ref_jack_activate(client) != 0) return close();
	state = JACKClientState_OPEN;
	return state;
}

void JACKClient::connectToPhysicalPorts() {
	const char **ports = ref_jack_get_ports(client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL || ports[0] == NULL || ports[1] == NULL) {
		qDebug() << "JACKClient: No physical playback ports detected, autoconnection failed";
	} else if (0 != ref_jack_connect(client, ref_jack_port_name(leftAudioOutPort), ports[0])) {
		qDebug() << "JACKClient: Autoconnection to port" << ports[0] << "failed";
	} else if (0 != ref_jack_connect(client, ref_jack_port_name(rightAudioOutPort), ports[1])) {
		qDebug() << "JACKClient: Autoconnection to port" << ports[1] << "failed";
	}
	ref_jack_free(ports);
}

bool JACKClient::isRealtimeProcessing() const {
	return ref_jack_is_realtime(client) != 0;
}

quint32 JACKClient::getSampleRate() const {
	return ref_jack_get_sample_rate(client);
}

quint32 JACKClient::getFramesSinceCycleStart() const {
	return ref_jack_frames_since_cycle_start(client);
}

void JACKClient::process(jack_nframes_t nframes) {
	if (midiSession != NULL) {
		quint32 cycleStartFrameTime = audioStream != NULL ? 0 : ref_jack_last_frame_time(client);
		jack_time_t jackTimeNow = audioStream != NULL ? 0 : ref_jack_get_time();
		MasterClockNanos nanosNow = audioStream != NULL ? 0 : MasterClock::getClockNanos();
		void *midiInBuffer = ref_jack_port_get_buffer(midiInPort, nframes);
		uint eventCount = uint(ref_jack_midi_get_event_count(midiInBuffer));
		for (uint eventIx = 0; eventIx < eventCount; eventIx++) {
			jack_midi_event_t eventData;
			bool eventRetrieved = ref_jack_midi_event_get(&eventData, midiInBuffer, eventIx) == 0;
			if (!eventRetrieved) break;
			bool eventConsumed;
			if (audioStream != NULL) {
				quint64 eventTimestamp = audioStream->computeMIDITimestamp(eventData.time);
				eventConsumed = JACKMidiDriver::playMIDIMessage(midiSession, eventTimestamp, eventData.size, eventData.buffer);
			} else {
				quint32 eventFrameTime = cycleStartFrameTime + eventData.time;
				quint64 eventJackTime = ref_jack_frames_to_time(client, eventFrameTime);
				MasterClockNanos eventNanoTime = JACKMidiDriver::jackFrameTimeToMasterClockNanos(nanosNow, eventJackTime, jackTimeNow);
				eventConsumed = JACKMidiDriver::pushMIDIMessage(midiSession, eventNanoTime, eventData.size, eventData.buffer);
			}
			if (!eventConsumed) break;
		}
	}
	if (audioStream != NULL) {
		jack_default_audio_sample_t *leftOutBuffer = static_cast<jack_default_audio_sample_t *>(ref_jack_port_get_buffer(leftAudioOutPort, nframes));
		jack_default_audio_sample_t *rightOutBuffer = static_cast<jack_default_audio_sample_t *>(ref_jack_port_get_buffer(rightAudioOutPort, nframes));
		audioStream->renderStreams(nframes, leftOutBuffer, rightOutBuffer);
	}
}
