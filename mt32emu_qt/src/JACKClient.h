#ifndef JACK_CLIENT_H
#define JACK_CLIENT_H

#include <QtCore>

#include <jack/jack.h>
#include <jack/midiport.h>

enum JACKClientState {
	JACKClientState_CLOSED,
	JACKClientState_OPENING,
	JACKClientState_OPEN,
	JACKClientState_CLOSING
};

class MidiSession;
class JACKAudioStream;

class JACKClient {
public:
	JACKClient();
	virtual ~JACKClient();

	JACKClientState open(MidiSession *midiSession, JACKAudioStream *audioStream);
	JACKClientState close();
	JACKClientState start();
	void connectToPhysicalPorts();
	bool isRealtimeProcessing() const;
	quint32 getSampleRate() const;
	quint32 getFramesSinceCycleStart() const;
	quint32 getBufferSize() const {
		return bufferSize;
	}

private:
	JACKClientState state;
	jack_client_t *client;
	MidiSession *midiSession;
	JACKAudioStream *audioStream;
	jack_port_t *midiInPort;
	jack_port_t *leftAudioOutPort;
	jack_port_t *rightAudioOutPort;
	quint32 bufferSize;

	static int onJACKProcess(jack_nframes_t nframes, void *instance);
	static int onBufferSizeChange(jack_nframes_t newFrameSize, void *instance);
	static int onSampleRateChange(jack_nframes_t newSystemSampleRate, void *instance);
	static void onJACKShutdown(void *instance);

	void process(jack_nframes_t nframes);
};

#endif
