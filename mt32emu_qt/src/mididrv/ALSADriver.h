#ifndef ALSA_MIDI_DRIVER_H
#define ALSA_MIDI_DRIVER_H

#include <alsa/asoundlib.h>

#include "OSSMidiPortDriver.h"

class ALSAMidiDriver : public MidiDriver {
	Q_OBJECT
public:
	ALSAMidiDriver(Master *useMaster);
	~ALSAMidiDriver();
	void start();
	void stop();
	bool canCreatePort();
	bool canDeletePort(MidiSession *midiSession);
	bool canReconnectPort(MidiSession *midiSession);
	PortNamingPolicy getPortNamingPolicy();
	bool createPort(int portIx, const QString &portName, MidiSession *midiSession);
	void deletePort(MidiSession *midiSession);
	void reconnectPort(int newPortIx, const QString &newPortName, MidiSession *midiSession);
	QString getNewPortNameHint(QStringList &knownPortNames);

private:
	snd_seq_t *snd_seq;
	pthread_t processingThreadID;
	volatile bool stopProcessing;
	QList<unsigned int> clients;
	QVarLengthArray<MT32Emu::Bit8u,MT32Emu::SYSEX_BUFFER_SIZE> sysexBuffer;
	OSSMidiPortDriver rawMidiPortDriver;

	static void *processingThread(void *userData);
	int alsa_setup_midi();
	void processSeqEvents();
	bool processSeqEvent(snd_seq_event_t *seq_event, MidiSession *midiSession);
	unsigned int getSourceAddr(snd_seq_event_t *seq_event);
	QString getClientName(unsigned int clientAddr);
	MidiSession *findMidiSessionForClient(unsigned int clientAddr);

signals:
	void mainWindowTitleContributionUpdated(const QString &);
};

#endif
