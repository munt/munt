#ifndef DEMO_PLAYER_H
#define DEMO_PLAYER_H

#include <mt32emu/mt32emu.h>
#include <QtCore>

#include "Master.h"
#include "QMidiEvent.h"
#include "mididrv/SMFDriver.h"

class DemoPlayer : public QObject, public MidiStreamSource {
	Q_OBJECT

public:
	static const MT32Emu::ROMImage *findSuitableROM(Master *master);

	DemoPlayer(Master *master, const MT32Emu::ROMImage *controlROMImage);
	~DemoPlayer();

	const QString getStreamName() const;
	const QMidiEventList &getMIDIEvents() const;
	MasterClockNanos getMidiTick(uint tempo = DEFAULT_TEMPO) const;

	const QStringList getDemoSongs() const;
	void playSong(uint songNumber);
	void chainPlay();
	void randomPlay();
	void stop();

private:
	const MT32Emu::ROMImage * const controlROMImage;
	const uint demoSongCount;
	SMFDriver smfDriver;
	bool playbackDelayed;
	uint currentSongNumber;
	const MT32Emu::Bit8u *songData;
	QString midiStreamName;
	QMidiEventList midiEventList;
	MasterClockNanos midiTick;

	void play(uint songNumber);
	const MT32Emu::Bit8u *getSongData(uint songNumber) const;
	void configureSynth();
	uint makeSongTitleSysex(MT32Emu::Bit8u *sysex);
	uint makeSystemResetSysex(MT32Emu::Bit8u *sysex);
	uint makeSystemSettingsSysex(MT32Emu::Bit8u *sysex);
	uint makeTimbreSysex(MT32Emu::Bit8u *sysex, uint timbreNumber);
	uint makePatchMemorySysex(MT32Emu::Bit8u *sysex, uint patchSysexAddress, uint patchRomAddress);
	uint makeRhythmSetupSysex(MT32Emu::Bit8u *sysex);
	void parseMIDIEvents(const MT32Emu::Bit8u *songDataPtr);
	void addSysex(MT32Emu::Bit8u *sysex, uint length);

private slots:
	void startNextSong(bool enabled);
	void startRandomSong(bool enabled);
};

#endif
